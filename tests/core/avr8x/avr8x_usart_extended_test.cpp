#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/uart8x.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

static const auto& D = devices::atmega4809.uarts8x[0];

struct UartFixture {
    PinMux pin_mux{8};
    MemoryBus bus{devices::atmega4809};
    Uart8x uart{D, pin_mux};

    UartFixture() {
        uart.set_memory_bus(&bus);
        uart.reset();
    }
};

TEST_CASE("Uart8x — reset defaults") {
    UartFixture f;

    CHECK(f.uart.read(D.ctrla_address)     == 0x00U);
    CHECK(f.uart.read(D.ctrlb_address)     == 0x00U);
    CHECK(f.uart.read(D.ctrlc_address)     == 0x03U); // 8N1
    CHECK(f.uart.read(D.ctrld_address)     == 0x00U);
    CHECK(f.uart.read(D.status_address)    == 0x20U); // DREIF
    CHECK(f.uart.read(D.dbgctrl_address)   == 0x00U);
    CHECK(f.uart.read(D.evctrl_address)    == 0x00U);
}

TEST_CASE("Uart8x — register round-trips") {
    UartFixture f;

    f.uart.write(D.ctrla_address, 0xE1U); // RXCIE|TXCIE|DREIE|LBME
    CHECK(f.uart.read(D.ctrla_address) == 0xE1U);

    f.uart.write(D.ctrlb_address, 0xCFU); // RXEN|TXEN|LBME|RXMODE|MPCM
    CHECK(f.uart.read(D.ctrlb_address) == 0xCFU);

    f.uart.write(D.ctrlc_address, 0x3FU); // CMODE|PMODE|SBMODE|CHSIZE
    CHECK(f.uart.read(D.ctrlc_address) == 0x3FU);

    f.uart.write(D.ctrld_address, 0xFFU);
    CHECK(f.uart.read(D.ctrld_address) == 0xFFU);

    f.uart.write(D.baud_address, 0x0BU);
    f.uart.write(static_cast<u16>(D.baud_address + 1), 0x1AU);
    CHECK(f.uart.read(D.baud_address) == 0x0BU);
    CHECK(f.uart.read(static_cast<u16>(D.baud_address + 1)) == 0x1AU);
}

TEST_CASE("Uart8x — DBGCTRL register") {
    UartFixture f;

    CHECK(f.uart.read(D.dbgctrl_address) == 0x00U);
    f.uart.write(D.dbgctrl_address, 0x01U); // DBGRUN
    CHECK(f.uart.read(D.dbgctrl_address) == 0x01U);
}

TEST_CASE("Uart8x — EVCTRL register") {
    UartFixture f;

    CHECK(f.uart.read(D.evctrl_address) == 0x00U);
    f.uart.write(D.evctrl_address, 0x87U);
    CHECK(f.uart.read(D.evctrl_address) == 0x87U);
}

TEST_CASE("Uart8x — TXCIE completes transaction and pending interrupt") {
    UartFixture f;

    f.uart.write(D.ctrla_address, 0x40U); // TXCIE=1
    f.uart.write(D.ctrlb_address, 0x40U); // TXEN
    f.uart.write(D.baud_address, 100); f.uart.write(static_cast<u16>(D.baud_address + 1), 0);

    InterruptRequest req{};
    CHECK_FALSE(f.uart.pending_interrupt_request(req));

    f.uart.write(D.txdata_address, 0x55);
    f.uart.tick(500);

    CHECK((f.uart.read(D.status_address) & 0x40) != 0); // TXCIF
    CHECK(f.uart.pending_interrupt_request(req));
    CHECK(req.vector_index == D.tx_vector_index);
    CHECK(f.uart.consume_interrupt_request(req));
    CHECK_FALSE(f.uart.pending_interrupt_request(req));
}

TEST_CASE("Uart8x — DREIE generates pending interrupt") {
    UartFixture f;

    // DREIF is set after reset → DREIE enables the interrupt
    f.uart.write(D.ctrla_address, 0x20U); // DREIE=1
    f.uart.write(D.ctrlb_address, 0x40U); // TXEN
    f.uart.write(D.baud_address, 0x0BU); f.uart.write(static_cast<u16>(D.baud_address + 1), 0x1AU);

    InterruptRequest req{};
    CHECK(f.uart.pending_interrupt_request(req));
    CHECK(req.vector_index == D.dre_vector_index);

    // Consume acknowledges the interrupt but DREIF remains set (status flag)
    // So pending remains true until DREIF is cleared by writing data
    CHECK(f.uart.consume_interrupt_request(req));

    // DREIF still set → still pending
    CHECK(f.uart.pending_interrupt_request(req));

    // Writing TX data clears DREIF
    f.uart.write(D.txdata_address, 0xAA);
    CHECK_FALSE(f.uart.pending_interrupt_request(req));
}

TEST_CASE("Uart8x — DREIE interrupt suppressed when DREIF cleared") {
    UartFixture f;

    f.uart.write(D.ctrla_address, 0x20U); // DREIE=1
    f.uart.write(D.ctrlb_address, 0x40U); // TXEN
    f.uart.write(D.baud_address, 0x0BU); f.uart.write(static_cast<u16>(D.baud_address + 1), 0x1AU);

    // Write data → DREIF clears → pending should clear
    f.uart.write(D.txdata_address, 0x01);
    InterruptRequest req{};
    CHECK_FALSE(f.uart.pending_interrupt_request(req));

    // After tx starts, DREIF re-sets → pending returns
    f.uart.tick(1);
    CHECK(f.uart.pending_interrupt_request(req));
}

TEST_CASE("Uart8x — RXCIE generates pending interrupt") {
    UartFixture f;

    f.uart.write(D.ctrla_address, 0x80U); // RXCIE=1
    f.uart.write(D.ctrlb_address, 0x80U); // RXEN
    f.uart.write(D.baud_address, 100); f.uart.write(static_cast<u16>(D.baud_address + 1), 0);

    InterruptRequest req{};
    CHECK_FALSE(f.uart.pending_interrupt_request(req));

    // inject_received_byte + tick to complete reception
    f.uart.inject_received_byte(0x7B);
    f.uart.tick(500);

    CHECK((f.uart.read(D.status_address) & 0x80) != 0); // RXCIF
    CHECK(f.uart.pending_interrupt_request(req));
    CHECK(req.vector_index == D.rx_vector_index);
    CHECK(f.uart.consume_interrupt_request(req));

    // Consume doesn't clear RXCIF (status flag remains until data read)
    CHECK(f.uart.pending_interrupt_request(req));

    // Read byte clears RXCIF
    CHECK(f.uart.read(D.rxdata_address) == 0x7B);
    CHECK_FALSE(f.uart.pending_interrupt_request(req));
    CHECK((f.uart.read(D.status_address) & 0x80) == 0); // RXCIF cleared
}

TEST_CASE("Uart8x — STATUS write-1-to-clear TXCIF") {
    UartFixture f;

    f.uart.write(D.ctrlb_address, 0x40U); // TXEN
    f.uart.write(D.baud_address, 100); f.uart.write(static_cast<u16>(D.baud_address + 1), 0);

    // TX to set TXCIF
    f.uart.write(D.txdata_address, 0x01);
    f.uart.tick(500);
    CHECK((f.uart.read(D.status_address) & 0x40) != 0); // TXCIF

    // Clear by writing 1
    f.uart.write(D.status_address, 0x40U);
    CHECK((f.uart.read(D.status_address) & 0x40) == 0);
}

TEST_CASE("Uart8x — RXDATAH read after RXDATA latches high byte") {
    UartFixture f;

    f.uart.write(D.ctrlb_address, 0x80U); // RXEN
    f.uart.write(D.baud_address, 100); f.uart.write(static_cast<u16>(D.baud_address + 1), 0);

    // Inject a byte with bit9 set
    f.uart.inject_received_byte(0x5A, true);
    f.uart.tick(500);

    // Verify byte was received
    CHECK((f.uart.read(D.status_address) & 0x80) != 0); // RXCIF

    // Read RXDATA first to latch RXDATAH
    CHECK(f.uart.read(D.rxdata_address) == 0x5A);
    u16 rxdatah_addr = static_cast<u16>(D.rxdata_address + 1);
    u8 datah = f.uart.read(rxdatah_addr);
    CHECK((datah & 0x01) != 0); // DATA8 = bit 9
    CHECK((datah & 0x80) == 0); // RXCIF should be clear (FIFO empty after read)
}

TEST_CASE("Uart8x — LBME loopback pushes TX to RX FIFO") {
    UartFixture f;

    // Enable loopback: both TXEN and LBME
    f.uart.write(D.ctrla_address, 0x01U); // LBME=1
    f.uart.write(D.ctrlb_address, 0x40U); // TXEN
    f.uart.write(D.baud_address, 100); f.uart.write(static_cast<u16>(D.baud_address + 1), 0);

    InterruptRequest req{};
    CHECK_FALSE(f.uart.pending_interrupt_request(req));

    // TX a byte
    f.uart.write(D.txdata_address, 0x7B);
    f.uart.tick(500);

    // Loopback should put it in RX FIFO
    CHECK((f.uart.read(D.status_address) & 0x80) != 0); // RXCIF
    CHECK(f.uart.read(D.rxdata_address) == 0x7B);
}

TEST_CASE("Uart8x — consume_transmitted_byte returns TX'd data") {
    UartFixture f;

    f.uart.write(D.ctrlb_address, 0x40U); // TXEN
    f.uart.write(D.baud_address, 100); f.uart.write(static_cast<u16>(D.baud_address + 1), 0);

    f.uart.write(D.txdata_address, 0x7B);
    f.uart.tick(500);

    u16 consumed = 0;
    CHECK(f.uart.consume_transmitted_byte(consumed));
    CHECK((consumed & 0xFF) == 0x7B);
}

TEST_CASE("Uart8x — TXDATAH write via txdata_address+1") {
    UartFixture f;

    u16 txdatah_addr = static_cast<u16>(D.txdata_address + 1);
    f.uart.write(txdatah_addr, 0xFFU);
    // Readback of TXDATAH not implemented (returns 0), but the write
    // is used by the transmitter for 9-bit data
    // Verifying 9-bit TX works
    f.uart.write(D.ctrlb_address, 0x40U); // TXEN
    f.uart.write(D.baud_address, 100); f.uart.write(static_cast<u16>(D.baud_address + 1), 0);

    f.uart.write(txdatah_addr, 0x01U); // bit 9 = 1
    f.uart.write(D.txdata_address, 0x7B);
    f.uart.tick(500); // 11 bits × 25 = 275 cycles

    u16 consumed = 0;
    CHECK(f.uart.consume_transmitted_byte(consumed));
    CHECK((consumed & 0x1FF) == (0x17B)); // bit 9 + data
}

TEST_CASE("Uart8x — unmapped addresses return 0") {
    UartFixture f;

    CHECK(f.uart.read(0x0000) == 0x00U);
    CHECK(f.uart.read(0x0010) == 0x00U);
    CHECK(f.uart.read(0xFFFFU) == 0x00U);
}
