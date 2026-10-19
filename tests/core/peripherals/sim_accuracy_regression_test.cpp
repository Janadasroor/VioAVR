#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/clkctrl.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/slpctrl.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/spi.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/tcd.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/twi.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/uart8x.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/devices/avr128db32.hpp"
#include "vioavr/core/eusart.hpp"

using namespace vioavr::core;

namespace {

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

// LDD Rd, Y+q  (10q0 qq0d dddd 1qqq)
constexpr u16 encode_ldd_y(const u8 destination, const u8 q) {
    return static_cast<u16>(0x8008U | (static_cast<u16>(destination) << 4U)
        | ((static_cast<u16>(q & 0x20U)) << 8U)
        | ((static_cast<u16>(q & 0x18U)) << 7U) | (q & 0x07U));
}

} // namespace

TEST_CASE("Regression: LDD Y+q low-byte carry (Y=0x..FF + q wraps low byte)")
{
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    const auto sram_base = atmega328p.sram_range().begin;

    // Y = sram_base + 0xFF so that low byte 0xFF + q=1 carries into high byte.
    const u16 y_addr = static_cast<u16>(sram_base + 0xFFU);
    const u16 target = static_cast<u16>(y_addr + 1U);

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(28U, static_cast<u8>(y_addr & 0xFFU)),
            encode_ldi(29U, static_cast<u8>((y_addr >> 8U) & 0xFFU)),
            encode_ldd_y(16U, 1U),
            0x0000U
        },
        .entry_word = 0U
    });
    cpu.reset();
    // Pre-fill AFTER reset: bus_->reset() clears SRAM.
    bus.write_data(target, 0x5AU);
    bus.write_data(sram_base, 0x00U); // buggy address would read this instead
    cpu.step(); // LDI r28
    CHECK(cpu.program_counter() == 1U);
    cpu.step(); // LDI r29
    CHECK(cpu.program_counter() == 2U);
    cpu.step(); // LDD r16, Y+1
    CHECK(cpu.snapshot().gpr[16] == 0x5AU);
}

TEST_CASE("Regression: Timer2 async prescaler table (CS=3 is /32, not /64)")
{
    auto device = DeviceCatalog::find("ATmega328P");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Timer2 (async-capable): TCCRB=0xB1, TCNT=0xB2. Timer0: TCCRB=0x45, TCNT=0x46.
    bus.write_data(0xB1U, 0x03U); // Timer2 CS=3 -> /32
    bus.write_data(0x45U, 0x03U); // Timer0 CS=3 -> /64
    bus.tick_peripherals(64U);
    CHECK(bus.read_data(0xB2U) == 2U); // Timer2: 64/32
    CHECK(bus.read_data(0x46U) == 1U); // Timer0: 64/64
}

TEST_CASE("Regression: Timer8 CTC sets TOV only when TOP==MAX")
{
    auto device = DeviceCatalog::find("ATmega328P");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // TCCRA=0x44 (WGM01=CTC), OCRA=0x47, TCCRB=0x45, TIFR=0x35.
    bus.write_data(0x44U, 0x02U);
    bus.write_data(0x47U, 0xFFU); // TOP == MAX: both OCF and TOV
    bus.write_data(0x45U, 0x01U); // run, /1
    bus.tick_peripherals(256U);
    CHECK((bus.read_data(0x35U) & 0x03U) == 0x03U);

    bus.write_data(0x35U, 0x07U); // clear flags
    bus.write_data(0x46U, 0x00U); // restart counter
    bus.write_data(0x47U, 0x10U); // TOP < MAX: OCF only, never TOV
    bus.tick_peripherals(256U);
    CHECK((bus.read_data(0x35U) & 0x02U) == 0x02U);
    CHECK((bus.read_data(0x35U) & 0x01U) == 0x00U);
}

TEST_CASE("Regression: ADC REFS=1.1V, bandgap and GND channels")
{
    auto device = DeviceCatalog::find("ATmega328P");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // ADMUX=0x7C, ADCSRA=0x7A, ADCL=0x78, ADCH=0x79.
    bus.write_data(0x7CU, 0xCEU); // REFS=11 (1.1V), MUX=bandgap
    bus.write_data(0x7AU, 0xC0U); // ADEN + ADSC
    bus.tick_peripherals(4000U);
    CHECK((bus.read_data(0x7AU) & 0x10U) != 0U); // ADIF
    const u16 bg = static_cast<u16>(bus.read_data(0x78U) | (static_cast<u16>(bus.read_data(0x79U)) << 8U));
    CHECK(bg == 1023U); // 1.1V / 1.1V full scale

    bus.write_data(0x7CU, 0xCFU); // REFS=11, MUX=GND
    bus.write_data(0x7AU, 0xD0U); // ADEN + ADSC + clear ADIF
    bus.tick_peripherals(4000U);
    const u16 gnd = static_cast<u16>(bus.read_data(0x78U) | (static_cast<u16>(bus.read_data(0x79U)) << 8U));
    CHECK(gnd == 0U);
}

TEST_CASE("Regression: UART MPCM drops 9-bit data frames")
{
    auto device = DeviceCatalog::find("ATmega328P");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto list = machine.peripherals_of_type<Uart>();
    REQUIRE(!list.empty());
    Uart* uart = list[0];

    bus.write_data(0xC1U, 0x14U); // RXEN + 9-bit (UCSZ02)
    bus.write_data(0xC0U, 0x01U); // MPCM
    CHECK(uart->inject_received_byte(0x0055U) == false); // data frame ignored
    CHECK((bus.read_data(0xC0U) & 0x80U) == 0U);
    CHECK(uart->inject_received_byte(0x0155U) == true); // address frame accepted
    bus.tick_peripherals(300U);
    CHECK((bus.read_data(0xC0U) & 0x80U) != 0U); // RXC
    CHECK(bus.read_data(0xC6U) == 0x55U); // UDR, clears RXC
    CHECK((bus.read_data(0xC0U) & 0x80U) == 0U);
}

TEST_CASE("Regression: XMEGA USART BSCALE shapes baud rate")
{
    Uart8xDescriptor desc{};
    desc.rxdata_address = 0x80U;
    desc.txdata_address = 0x80U; // == rxdata, below ctrla -> xmega layout
    desc.ctrla_address = 0x90U;
    desc.ctrlb_address = 0x91U;
    desc.ctrlc_address = 0x92U;
    desc.status_address = 0x93U;
    desc.baud_address = 0x94U;
    PinMux pin_mux(1U);
    Uart8x uart(desc, pin_mux);
    uart.reset();

    // 8N1, BSEL=0, BSCALE=0 -> 16 cycles/bit, 10 bits = 160 cycles.
    uart.write(0x91U, 0x40U); // TXEN
    uart.write(0x80U, 0xA5U);
    uart.tick(100U);
    CHECK((uart.read(0x93U) & 0x40U) == 0U); // TXCIF clear
    uart.tick(100U);
    CHECK((uart.read(0x93U) & 0x40U) != 0U); // TXCIF set

    // BSCALE=+1 doubles bit time -> 320 cycles.
    uart.reset();
    uart.write(0x94U, 0x00U); // BSEL low
    uart.write(0x95U, 0x10U); // BSCALE=+1
    uart.write(0x91U, 0x40U); // TXEN
    uart.write(0x80U, 0xA5U);
    uart.tick(200U);
    CHECK((uart.read(0x93U) & 0x40U) == 0U);
    uart.tick(200U);
    CHECK((uart.read(0x93U) & 0x40U) != 0U);
}

TEST_CASE("Regression: TCD reserved CNTPRES clamps instead of OOB read")
{
    using namespace vioavr::core::devices;
    Tcd tcd(avr128db32.timers_tcd[0]);
    tcd.reset();

    // CMPASET=5, OneRamp default; ENABLE + reserved CNTPRES=0x03.
    tcd.write(0x0BA8U, 5U);
    tcd.write(0x0BA9U, 0U);
    tcd.write(0x0B80U, 0x19U);
    tcd.tick(320U); // 320 / 32 = 10 counts, passes CMPASET=5
    CHECK((tcd.read(0x0B8DU) & 0x04U) != 0U); // TRIGA

    // Explicit DIV32 behaves identically.
    Tcd ref(avr128db32.timers_tcd[0]);
    ref.reset();
    ref.write(0x0BA8U, 5U);
    ref.write(0x0BA9U, 0U);
    ref.write(0x0B80U, 0x11U); // ENABLE + DIV32
    ref.tick(320U);
    CHECK((ref.read(0x0B8DU) & 0x04U) != 0U);
}

TEST_CASE("Regression: TCD TwoRamp/FourRamp/DualSlope cycle correctly")
{
    using namespace vioavr::core::devices;

    // TwoRamp: ramp0 ends at CMPACLR=10, ramp1 ends at CMPBCLR=20.
    // Full cycle = 10 + 20 = 30 ticks -> OVF.
    {
        Tcd tcd(avr128db32.timers_tcd[0]);
        tcd.reset();
        tcd.write(0x0BAAU, 10U); // CMPACLR
        tcd.write(0x0BAEU, 20U); // CMPBCLR
        tcd.write(0x0BA8U, 4U);  // CMPASET -> TRIGA in ramp 0
        tcd.write(0x0B81U, 0x01U); // WGMODE=TwoRamp
        tcd.write(0x0B80U, 0x01U); // ENABLE, prescalers /1
        tcd.tick(29U);
        CHECK((tcd.read(0x0B8DU) & 0x01U) == 0U); // no OVF yet
        CHECK((tcd.read(0x0B8DU) & 0x04U) != 0U); // TRIGA fired
        tcd.tick(1U);
        CHECK((tcd.read(0x0B8DU) & 0x01U) != 0U); // OVF at tick 30
    }

    // FourRamp: limits 5/6/7/8 -> cycle = 26 ticks -> OVF; TRIGA at 5, TRIGB at 5+6+7=18.
    {
        Tcd tcd(avr128db32.timers_tcd[0]);
        tcd.reset();
        tcd.write(0x0BA8U, 5U);  // CMPASET (ramp 0)
        tcd.write(0x0BAAU, 6U);  // CMPACLR (ramp 1)
        tcd.write(0x0BACU, 7U);  // CMPBSET (ramp 2)
        tcd.write(0x0BAEU, 8U);  // CMPBCLR (ramp 3)
        tcd.write(0x0B81U, 0x02U); // WGMODE=FourRamp
        tcd.write(0x0B80U, 0x01U); // ENABLE
        tcd.tick(25U);
        CHECK((tcd.read(0x0B8DU) & 0x01U) == 0U);
        CHECK((tcd.read(0x0B8DU) & 0x04U) != 0U); // TRIGA (ramp 0 end)
        CHECK((tcd.read(0x0B8DU) & 0x08U) != 0U); // TRIGB (ramp 2 end)
        tcd.tick(1U);
        CHECK((tcd.read(0x0B8DU) & 0x01U) != 0U); // OVF at tick 26
    }

    // DualSlope: CMPBCLR=10 -> down 10, up 10, OVF at top.
    {
        Tcd tcd(avr128db32.timers_tcd[0]);
        tcd.reset();
        tcd.write(0x0BAEU, 10U); // CMPBCLR
        tcd.write(0x0BA8U, 4U);  // CMPASET -> TRIGA on the way down
        tcd.write(0x0B81U, 0x03U); // WGMODE=DualSlope
        tcd.write(0x0B80U, 0x01U); // ENABLE (starts at top, down)
        tcd.tick(6U); // 10 -> 4: down-match CMPASET
        CHECK((tcd.read(0x0B8DU) & 0x04U) != 0U); // TRIGA
        CHECK((tcd.read(0x0B8DU) & 0x01U) == 0U); // no OVF yet
        tcd.tick(14U); // to 0, up to 10: cycle end
        CHECK((tcd.read(0x0B8DU) & 0x01U) != 0U); // OVF
    }
}

TEST_CASE("Regression: SPI SS abort clears MSTR and raises SPIF")
{
    auto device = DeviceCatalog::find("ATmega328P");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto list = machine.peripherals_of_type<Spi>();
    REQUIRE(!list.empty());
    Spi* spi = list[0];

    bus.write_data(0x4CU, 0x5CU); // SPE + MSTR + mode 3
    CHECK(spi->spi_mode() == 3U);
    spi->inject_miso_byte(0xA5U);
    bus.write_data(0x4EU, 0x3CU); // start transfer
    CHECK(spi->busy() == true);
    spi->on_ss_asserted();
    CHECK((bus.read_data(0x4CU) & 0x10U) == 0U); // MSTR cleared
    CHECK((bus.read_data(0x4DU) & 0x80U) != 0U); // SPIF set
    CHECK(spi->busy() == false);
}

TEST_CASE("Regression: TWI NACK without TWEA, arbitration/bus-error codes")
{
    using namespace vioavr::core::devices;
    Twi twi("TWI", atmega328p.twis[0]);
    twi.reset();
    twi.write(0xBAU, 0xA0U); // TWAR = 0x50
    twi.write(0xBCU, 0x04U); // TWEN, no TWEA
    twi.on_twi_match(0xA0U);
    CHECK(twi.twi_match_acked() == false);
    twi.write(0xBCU, 0x44U); // TWEN + TWEA
    twi.on_twi_match(0xA0U);
    CHECK(twi.twi_match_acked() == true);

    twi.force_arbitration_lost();
    CHECK(twi.read(0xB9U) == 0x38U);
    twi.force_bus_error();
    CHECK(twi.read(0xB9U) == 0x00U);
}

TEST_CASE("Regression: TCB ASYNC is output-only, CLKSEL width and reserved values")
{
    // AVR DA datasheet 22.5.2 / EB datasheet: ASYNC only selects
    // asynchronous output updates in Single-Shot mode - it never changes the
    // counter clock. CLKSEL alone does (4809: CLKSEL[1:0], 3 = reserved).
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();
    std::vector<u16> program(2000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    const u16 kCtrla = 0x0A80U;
    const u16 kCtrlb = 0x0A81U;
    const u16 kCcmpL = 0x0A8CU;
    const u16 kCcmpH = 0x0A8DU;
    const u16 kCnt = 0x0A8AU;

    bus.write_data(kCcmpL, 0xFFU);
    bus.write_data(kCcmpH, 0xFFU); // CCMP = 0xFFFF (never reaches TOP)
    bus.write_data(kCnt, 0x00U);   // phases below check a fresh counter

    // ASYNC=1 with CLK_PER: counting is unaffected (1 tick/cycle).
    bus.write_data(kCtrlb, 0x40U);
    bus.write_data(kCtrla, (0x00U << 1) | 0x01U);
    bus.tick_peripherals(10U);
    CHECK(bus.read_data(kCnt) == 10U);

    // Reserved CLKSEL (3 on a 2-bit field) freezes the counter.
    bus.write_data(kCtrla, 0x00U); // disable
    bus.write_data(kCnt, 0x00U);
    bus.write_data(kCtrla, (0x03U << 1) | 0x01U);
    bus.tick_peripherals(100U);
    CHECK(bus.read_data(kCnt) == 0U);

    // CLKSEL = CLKTCA with TCA0 stopped freezes the counter too.
    bus.write_data(kCtrla, 0x00U);
    bus.write_data(kCnt, 0x00U);
    bus.write_data(kCtrla, (0x02U << 1) | 0x01U);
    bus.tick_peripherals(50U);
    CHECK(bus.read_data(kCnt) == 0U);

    // Running TCA0 (DIV1) supplies CLK_TCA at the CLK_PER rate.
    bus.write_data(kCtrla, 0x00U);
    bus.write_data(kCnt, 0x00U);
    bus.write_data(0x0A26U, 0x09U); // PER = 9
    bus.write_data(0x0A00U, 0x01U); // TCA0 ENABLE
    bus.write_data(kCtrla, (0x02U << 1) | 0x01U);
    bus.tick_peripherals(10U);
    CHECK(bus.read_data(kCnt) == 10U);
}

TEST_CASE("Regression: AVR-Dx TCB CLKSEL is 3 bits wide")
{
    // AVR128DB32 (Dx): CLKSEL[2:0]. Value 4 is reserved; value 7 is the
    // event clock. On a 2-bit device both would read as DIV1/reserved.
    auto device = DeviceCatalog::find("AVR128DB32");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();
    std::vector<u16> program(2000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    const u16 kCtrla = 0x0B00U;
    const u16 kCcmpL = 0x0B0CU;
    const u16 kCcmpH = 0x0B0DU;
    const u16 kCnt = 0x0B0AU;
    bus.write_data(kCcmpL, 0xFFU);
    bus.write_data(kCcmpH, 0xFFU);

    // Reserved CLKSEL=4: with a 3-bit field it stays reserved (frozen).
    bus.write_data(kCtrla, (0x04U << 1) | 0x01U);
    bus.tick_peripherals(50U);
    CHECK(bus.read_data(kCnt) == 0U);

    // EVENT clock (7) with no event activity: frozen.
    bus.write_data(kCtrla, (0x07U << 1) | 0x01U);
    bus.tick_peripherals(50U);
    CHECK(bus.read_data(kCnt) == 0U);

    // DIV1 still counts every cycle.
    bus.write_data(kCtrla, (0x00U << 1) | 0x01U);
    bus.tick_peripherals(10U);
    CHECK(bus.read_data(kCnt) == 10U);
}

TEST_CASE("Regression: TCA split mode has no HCMP flags; FRQ drives WO0 only")
{
    using namespace vioavr::core::devices;
    const auto& D = atmega4809.timers_tca[0];

    // Split mode: HCMPn matches only drive WO[n+3]; INTFLAGS is
    // LCMP2/LCMP1/LCMP0/HUNF/LUNF and bit 7 is reserved.
    {
        Tca tca("TCA0", D);
        tca.reset();
        tca.write(D.ctrla_address, 0x01U); // ENABLE, DIV1
        tca.write(D.ctrld_address, 0x01U); // SPLITM
        tca.write(D.period_address, 0x20U);     // PERL
        tca.write(D.period_address + 1U, 0x20U); // PERH
        tca.write(D.cmp0_address, 0x05U);     // CMP0L
        tca.write(D.cmp0_address + 1U, 0x05U); // HCMP0 = 5

        // Run until the high counter matches HCMP0.
        tca.tick(4U);
        CHECK((tca.read(D.intflags_address) & 0x80U) == 0U); // no HCMP flag
        CHECK((tca.read(D.intflags_address) & 0x02U) == 0U); // no premature HUNF
    }

    // FRQ mode: waveform exists on WO0 only (TOP = CMP0).
    {
        Tca tca("TCA0", D);
        tca.reset();
        tca.write(D.ctrla_address, 0x01U);   // ENABLE
        tca.write(D.ctrlb_address, 0x01U);    // WGMODE=FRQ
        tca.write(D.cmp0_address, 4U);
        tca.write(D.cmp0_address + 1U, 0U);
        tca.write(D.period_address, 10U);    // ignored as TOP in FRQ
        tca.write(D.period_address + 1U, 0U);

        CHECK(tca.get_wo_level(0) == false);
        CHECK(tca.get_wo_level(1) == false);
        CHECK(tca.get_wo_level(2) == false);
        tca.tick(5U); // CMP0 match toggles WO0
        CHECK(tca.get_wo_level(0) == true);
        CHECK(tca.get_wo_level(1) == false); // no FRQ waveform on WO1
        CHECK(tca.get_wo_level(2) == false); // no FRQ waveform on WO2
    }
}

TEST_CASE("Regression: AVR-Ex EEPER erases only buffered bytes of one page")
{
    // Ex datasheet: "only the bytes written in the page buffer will be
    // erased. To erase a specific byte, write to its corresponding address."
    auto device = DeviceCatalog::find("AVR16EB32");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 kMapped = device->mapped_eeprom.data_start; // 0x1400
    const u16 kCtrla = device->nvm_ctrls[0].ctrla_address;
    REQUIRE(bus.nvm_ctrl() != nullptr);
    CHECK(bus.nvm_ctrl()->family() == NvmFamily::Ex);

    auto commit = [&](u8 lo, u8 l1, u8 l2, u8 l3) {
        bus.write_data(kMapped + 0U, lo);
        bus.write_data(kMapped + 1U, l1);
        bus.write_data(kMapped + 2U, l2);
        bus.write_data(kMapped + 3U, l3);
        bus.write_data(kCtrla, 0x14U); // EEPW
        bus.tick_peripherals(65000U);
    };

    commit(0x11U, 0x22U, 0x33U, 0x44U);
    CHECK(bus.read_data(kMapped + 0U) == 0x11U);
    CHECK(bus.read_data(kMapped + 3U) == 0x44U);

    // Load only byte 1 in the page buffer, then EEPER.
    bus.write_data(kMapped + 1U, 0xFFU);
    bus.write_data(kCtrla, 0x17U); // EEPER
    bus.tick_peripherals(65000U);

    CHECK(bus.read_data(kMapped + 1U) == 0xFFU); // erased
    CHECK(bus.read_data(kMapped + 0U) == 0x11U); // untouched
    CHECK(bus.read_data(kMapped + 2U) == 0x33U); // untouched
    CHECK(bus.read_data(kMapped + 3U) == 0x44U); // untouched

    // EECHER still wipes the whole array.
    bus.write_data(kCtrla, 0x30U); // EECHER
    bus.tick_peripherals(1600000U);
    CHECK(bus.read_data(kMapped + 0U) == 0xFFU);
    CHECK(bus.read_data(kMapped + 3U) == 0xFFU);
}

TEST_CASE("Regression: AVR-Dx EEBER/EEMBERn erase only the addressed bytes")
{
    // Dx datasheet Table 11-3: EEPROM erase granularity is byte-based.
    auto device = DeviceCatalog::find("AVR128DB32");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 kMapped = device->mapped_eeprom.data_start;
    const u16 kCtrla = device->nvm_ctrls[0].ctrla_address;
    REQUIRE(bus.nvm_ctrl() != nullptr);
    CHECK(bus.nvm_ctrl()->family() == NvmFamily::Dx);

    bus.write_data(kMapped + 0U, 0x11U);
    bus.write_data(kCtrla, 0x12U); bus.tick_peripherals(65000U); // EEWR: 1 byte
    bus.write_data(kMapped + 1U, 0x22U);
    bus.write_data(kCtrla, 0x12U); bus.tick_peripherals(65000U);
    bus.write_data(kMapped + 2U, 0x33U);
    bus.write_data(kCtrla, 0x12U); bus.tick_peripherals(65000U);
    bus.write_data(kMapped + 3U, 0x44U);
    bus.write_data(kCtrla, 0x12U); bus.tick_peripherals(65000U);
    CHECK(bus.read_data(kMapped + 0U) == 0x11U);
    CHECK(bus.read_data(kMapped + 2U) == 0x33U);

    // EEBER at ADDR = mapped+2 erases that byte only.
    bus.write_data(kMapped + 2U, 0x33U); // refresh buffer + set ADDR
    bus.write_data(kCtrla, 0x18U);       // EEBER
    bus.tick_peripherals(65000U);
    CHECK(bus.read_data(kMapped + 2U) == 0xFFU);
    CHECK(bus.read_data(kMapped + 0U) == 0x11U);
    CHECK(bus.read_data(kMapped + 3U) == 0x44U);

    // EEMBER4 at ADDR = mapped+0 erases bytes 0..3.
    bus.write_data(kMapped + 0U, 0x11U);
    bus.write_data(kCtrla, 0x1AU); // EEMBER4
    bus.tick_peripherals(65000U);
    CHECK(bus.read_data(kMapped + 0U) == 0xFFU);
    CHECK(bus.read_data(kMapped + 3U) == 0xFFU);
}

TEST_CASE("Regression: EUSART framing error leaves its configuration intact")
{
    // AT90PWM316 EUSART (ATDF + avr-libc/avrasm agree): EUCSRA = UTxS/URxS
    // character size, EUCSRB = control, EUCSRC = STP[1:0]/F1617/FEM. There is
    // no framing-error bit, so a bad stop bit must not corrupt F1617.
    auto device = DeviceCatalog::find("AT90PWM316");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* eusart = machine.peripherals_of_type<Eusart>()[0];
    REQUIRE(eusart != nullptr);

    // EUCSRA resets to 8-bit transmit and 8-bit receive character size.
    CHECK(bus.read_data(0xC8U) == 0x33U);

    const u16 kEucsrb = 0xC9U;
    const u16 kEucsrc = 0xCAU;
    const u16 kEudr = 0xCEU;     // high byte of the character
    const u16 kUdr = 0xC6U;      // low byte; reading it clears RXC
    const u16 kUcsra = 0xC0U;    // shared status register
    const u16 kRxd = 0x29U; // PD1

    bus.write_data(kEucsrb, 0x10U); // EUSART enable
    bus.write_data(0xCCU, 0x00U);   // MUBRR = 0 (16 cycles per bit)
    bus.write_data(kEucsrc, 0x04U); // F1617: status of a previous 17-bit frame

    auto drive = [&](bool level) {
        bus.pin_mux()->update_pin_by_address(kRxd, 1U, PinOwner::external, true, level);
    };
    bus.pin_mux()->claim_pin_by_address(kRxd, 1U, PinOwner::external);
    drive(true);
    bus.tick_peripherals(2);

    // Frame: start(0) + 0x5A LSB-first + stop. Levels are held for one bit
    // time and ticked in half-bit steps so the mid-bit samples land inside
    // each bit.
    auto send_frame = [&](bool stop_bit) {
        const u64 bit = 16; // MUBRR = 0 -> 16 cycles per bit
        auto hold = [&](u64 cycles) {
            while (cycles > 0) {
                const u64 step = cycles > (bit / 2) ? (bit / 2) : cycles;
                bus.tick_peripherals(step);
                cycles -= step;
            }
        };
        drive(false); hold(bit); // start
        const u8 data = 0x5AU;
        for (int i = 0; i < 8; ++i) {
            drive((data >> i) & 1U);
            hold(bit);
        }
        drive(stop_bit);
        hold(bit + (bit / 2));
    };

    send_frame(true);
    // RXC is reported in the shared UCSRA, and the character is read as
    // EUDR (high) + UDR (low) per datasheet 18.6.2.
    CHECK((bus.read_data(kUcsra) & 0x80U) != 0U);
    CHECK(bus.read_data(kEudr) == 0x00U); // high byte
    CHECK(bus.read_data(kUdr) == 0x5AU);  // low byte, clears RXC
    CHECK((bus.read_data(kUcsra) & 0x80U) == 0U);
    CHECK(eusart->frame_error_count() == 0);

    // Same byte with a LOW stop bit: framing error.
    send_frame(false);
    CHECK(eusart->frame_error_count() == 1);

    // The byte is still delivered; FEM is the Manchester-only flag, so a
    // level-encoded framing error must not set it.
    CHECK((bus.read_data(kEucsrc) & 0x08U) == 0U);
    CHECK(bus.read_data(kUdr) == 0x5AU);
    // Reading the buffer clears the EUCSRC status flags (F1617 included).
    CHECK((bus.read_data(kEucsrc) & 0x0CU) == 0U);
    CHECK((bus.read_data(kEucsrb) & 0x10U) != 0U); // still enabled
}

TEST_CASE("Regression: EUSART Manchester framing error sets FEM with RXC")
{
    // Datasheet 18.5.4/18.6.5: on a Manchester framing error the hardware
    // sets FEM and RXC together; FEM stays valid until the buffer is read.
    auto device = DeviceCatalog::find("AT90PWM316");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 kEucsrb = 0xC9U;
    const u16 kEucsrc = 0xCAU;
    const u16 kUdr = 0xC6U;
    const u16 kEucsra = 0xC8U;
    const u16 kRxd = 0x29U; // PD1

    bus.write_data(kEucsrb, 0x12U); // EUSART enable + Manchester
    bus.write_data(0xCCU, 0x00U);   // MUBRR = 0 -> 8 cycles per half bit
    bus.write_data(kEucsra, 0x33U); // 8-bit TX/RX character size

    bus.pin_mux()->claim_pin_by_address(kRxd, 1U, PinOwner::external);
    auto drive = [&](bool level) {
        bus.pin_mux()->update_pin_by_address(kRxd, 1U, PinOwner::external, true, level);
    };
    drive(true);
    bus.tick_peripherals(4);

    // Manchester: '1' = LOW then HIGH, '0' = HIGH then LOW (datasheet 18.4).
    // MUBRR = 0 gives 8 cycles per half bit, and the decoder samples at
    // 4 cycles, so levels are ticked in 2-cycle steps.
    auto hold = [&](u64 cycles) {
        while (cycles > 0) {
            const u64 step = cycles > 2U ? 2U : cycles;
            bus.tick_peripherals(step);
            cycles -= step;
        }
    };
    auto send_bit = [&](bool bit, bool corrupt = false) {
        if (corrupt) {
            drive(true); hold(8); // equal halves: no transition, framing error
            drive(true); hold(8);
            return;
        }
        drive(!bit); hold(8);
        drive(bit);  hold(8);
    };

    send_bit(true); // start bit
    const u8 data = 0x3CU;
    for (int i = 0; i < 8; ++i) {
        send_bit((data >> i) & 1U, i == 3); // 4th symbol is corrupt
    }
    bus.tick_peripherals(24);

    CHECK((bus.read_data(kEucsrc) & 0x08U) != 0U); // FEM set with RXC
    // Reading the shared data register clears it.
    (void)bus.read_data(kUdr);
    CHECK((bus.read_data(kEucsrc) & 0x08U) == 0U);
}

TEST_CASE("Regression: EUSART shares UCSRA status and UDR with the USART")
{
    // Datasheet 18.6.2/18.6.4: the EUSART and the USART are one peripheral.
    // RXC lives in UCSRA (0xC0) and the character is read as EUDR (high)
    // plus UDR (low) - the example in 18.5.3 polls `sbis UCSRA, RXC`.
    auto device = DeviceCatalog::find("AT90PWM316");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 kUcsra = 0xC0U;
    const u16 kUcsrb = 0xC1U;
    const u16 kUdr = 0xC6U;
    const u16 kEucsrb = 0xC9U;
    const u16 kEudr = 0xCEU;

    bus.write_data(kEucsrb, 0x10U); // EUSART enable

    // 16-bit receive character size
    bus.write_data(0xC8U, 0x3BU); // UTxS = 3 (8-bit TX), URxS = 0xB (16-bit RX)
    bus.write_data(0xCCU, 0x00U);   // MUBRR = 0

    machine.peripherals_of_type<Eusart>()[0]->inject_received_byte(0x1234U);

    // RXC is visible in the shared UCSRA, not in EUCSRA (character size).
    CHECK((bus.read_data(kUcsra) & 0x80U) != 0U);
    CHECK(bus.read_data(0xC8U) == 0x3BU); // EUCSRA untouched by the status

    CHECK(bus.read_data(kEudr) == 0x12U); // high byte
    CHECK(bus.read_data(kUdr) == 0x34U);  // low byte clears RXC
    CHECK((bus.read_data(kUcsra) & 0x80U) == 0U);

    // Transmit: EUDR high byte, UDR low byte triggers the frame.
    bus.write_data(kEudr, 0x00U);
    bus.write_data(kUdr, 0x5AU);
    bus.tick_peripherals(200U);
    CHECK((bus.read_data(kUcsra) & 0x40U) != 0U); // TXC in the shared UCSRA

    u32 tx_word{};
    auto* eusart = machine.peripherals_of_type<Eusart>()[0];
    // The engine consumed the frame from its queue.
    CHECK(eusart->consume_transmitted_byte(tx_word) == false);

    // The classic USART registers stay writable alongside.
    bus.write_data(kUcsrb, 0x98U); // RXEN | TXEN | RXCIE
    CHECK((bus.read_data(kUcsrb) & 0x90U) == 0x90U);
}

TEST_CASE("Regression: TCA CTRLE command encoding and split-mode commands")
{
    // Datasheet 21.5.5/21.7.5 and the ATDF agree: CMD is bits 3:2 in both
    // modes. Normal mode adds DIR (bit 0) and LUPD (bit 1); split mode
    // replaces them with CMDEN (bits 1:0), where 3 = BOTH.
    using namespace vioavr::core::devices;
    const auto& D = atmega4809.timers_tca[0];

    // Normal mode: UPDATE applies the buffers, DIR/LUPD live in bits 0:1.
    {
        Tca tca("TCA0", D);
        tca.reset();
        tca.write(D.ctrla_address, 0x01U);
        // 16-bit registers take the low byte first, then the high byte.
        tca.write(D.period_address, 10U);
        tca.write(D.period_address + 1U, 0U);
        tca.write(D.perbuf_address, 30U);
        tca.write(D.perbuf_address + 1U, 0U);
        CHECK(tca.read(D.period_address) == 10U);
        tca.write(D.ctrleset_address, 0x04U); // CMD = UPDATE
        CHECK(tca.read(D.period_address) == 30U);

        tca.write(D.ctrleset_address, 0x01U); // DIR
        CHECK((tca.read(D.ctrleclr_address) & 0x01U) != 0U);
        tca.write(D.ctrleclr_address, 0x01U);
        CHECK((tca.read(D.ctrleclr_address) & 0x01U) == 0U);
        tca.write(D.ctrleset_address, 0x02U); // LUPD
        CHECK((tca.read(D.ctrleclr_address) & 0x02U) != 0U);
        CHECK((tca.read(D.ctrleclr_address) & 0x0CU) == 0U); // CMD reads 0
    }

    // Split mode: CMDEN = BOTH restarts both counters; CMD reads as 0.
    {
        Tca tca("TCA0", D);
        tca.reset();
        tca.write(D.ctrla_address, 0x01U);
        tca.write(D.ctrld_address, 0x01U); // SPLITM
        tca.write(D.period_address, 20U);     // LPER
        tca.write(D.period_address + 1U, 20U); // HPER
        tca.tick(5U);
        CHECK(tca.read(D.tcnt_address) == 5U);
        CHECK(tca.read(D.tcnt_address + 1U) == 5U);

        tca.write(D.ctrleset_address, 0x08U | 0x03U); // CMD=RESTART, CMDEN=BOTH
        CHECK(tca.read(D.tcnt_address) == 0U);
        CHECK(tca.read(D.tcnt_address + 1U) == 0U);
        CHECK(tca.read(D.ctrleclr_address) == 0U); // CMD/CMDEN are write-only

        // CMDEN = 0 means no timer/counter is selected: nothing happens.
        tca.tick(3U);
        CHECK(tca.read(D.tcnt_address) == 3U);
        tca.write(D.ctrleset_address, 0x08U); // RESTART without CMDEN
        CHECK(tca.read(D.tcnt_address) == 3U);
    }
}

TEST_CASE("Regression: classic UART receives frames from the RXD pin")
{
    // ATmega328P USART0: UBRR=0 gives 16 cycles per bit; RXD is PD0. A
    // Machine is needed so the port pins are registered with the PinMux.
    auto device = DeviceCatalog::find("ATmega328P");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();
    const auto D = device->uarts[0];
    const u16 kRxd = 0x29U; // PD0

    bus.pin_mux()->claim_pin_by_address(kRxd, 0U, PinOwner::external);
    auto drive = [&](bool level) {
        bus.pin_mux()->update_pin_by_address(kRxd, 0U, PinOwner::external, true, level);
    };
    // Hold each level for one bit time, ticking in half-bit steps so the
    // mid-bit samples land inside the bit.
    auto send_frame = [&](u8 value, bool stop_bit) {
        const u64 bit = 16;
        auto hold = [&](u64 cycles) {
            while (cycles > 0) {
                const u64 step = cycles > (bit / 2) ? (bit / 2) : cycles;
                bus.tick_peripherals(step);
                cycles -= step;
            }
        };
        drive(true); hold(4);
        drive(false); hold(bit);        // start
        for (int i = 0; i < 8; ++i) {   // LSB first
            drive((value >> i) & 1U);
            hold(bit);
        }
        drive(stop_bit); hold(bit + (bit / 2));
    };

    bus.write_data(D.ubrrl_address, 0x00U);
    bus.write_data(D.ubrrh_address, 0x00U);
    bus.write_data(D.ucsrc_address, 0x06U); // 8N1
    bus.write_data(D.ucsrb_address, D.rxen_mask);

    // A good frame.
    send_frame(0x5AU, true);
    CHECK((bus.read_data(D.ucsra_address) & 0x80U) != 0U); // RXC
    CHECK((bus.read_data(D.ucsra_address) & 0x1CU) == 0U); // no FE/DOR/UPE
    CHECK(bus.read_data(D.udr_address) == 0x5AU);
    CHECK((bus.read_data(D.ucsra_address) & 0x80U) == 0U); // cleared by UDR read

    // Bad stop bit sets FE with RXC.
    send_frame(0xA3U, false);
    CHECK((bus.read_data(D.ucsra_address) & 0x90U) == 0x90U); // RXC | FE
    CHECK(bus.read_data(D.udr_address) == 0xA3U);
    CHECK((bus.read_data(D.ucsra_address) & 0x90U) == 0U);

    // A second frame while RXC is still set loses data: DOR.
    send_frame(0x11U, true);
    CHECK((bus.read_data(D.ucsra_address) & 0x80U) != 0U);
    send_frame(0x22U, true);
    CHECK((bus.read_data(D.ucsra_address) & 0x88U) == 0x88U); // RXC | DOR
    CHECK(bus.read_data(D.udr_address) == 0x11U); // first byte kept
    CHECK((bus.read_data(D.ucsra_address) & 0x8CU) == 0U);

    // Even parity: a wrong parity bit sets UPE.
    bus.write_data(D.ucsrc_address, 0x26U); // 8-bit, even parity
    const u64 bit = 16;
    auto hold = [&](u64 cycles) {
        while (cycles > 0) {
            const u64 step = cycles > (bit / 2) ? (bit / 2) : cycles;
            bus.tick_peripherals(step);
            cycles -= step;
        }
    };
    drive(true); hold(4);
    drive(false); hold(bit);
    const bool parity_bit = false; // 0x01 has one set bit: 0 makes the count odd
    for (int i = 0; i < 8; ++i) { drive((0x01U >> i) & 1U); hold(bit); }
    drive(parity_bit); hold(bit);
    drive(true); hold(bit + (bit / 2));
    CHECK((bus.read_data(D.ucsra_address) & 0x84U) == 0x84U); // RXC | UPE
    CHECK(bus.read_data(D.udr_address) == 0x01U);

    // Disabling RXEN stops reception.
    bus.write_data(D.ucsrb_address, 0x00U);
    send_frame(0x77U, true);
    CHECK((bus.read_data(D.ucsra_address) & 0x80U) == 0U);
}

TEST_CASE("Regression: RTC prescaler reaches DIV32768, PIT saturates safely")
{
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // RTC CTRLA=0x140 (ENABLE bit0, PRESCALER bits 6:3), CNT=0x148, PER=0x14A.
    bus.write_data(0x14AU, 0xFFU);
    bus.write_data(0x14BU, 0xFFU); // PER=0xFFFF: no wrap interference
    bus.write_data(0x140U, 0x01U | (15U << 3U)); // ENABLE + DIV32768
    bus.tick_peripherals(32767U);
    u16 cnt = static_cast<u16>(bus.read_data(0x148U) | (bus.read_data(0x149U) << 8U));
    CHECK(cnt == 0U);
    bus.tick_peripherals(1U);
    cnt = static_cast<u16>(bus.read_data(0x148U) | (bus.read_data(0x149U) << 8U));
    CHECK(cnt == 1U); // 32768 cycles per tick, not clamped to 1024

    // PIT PERIOD saturates instead of truncating to 0 (old 1U<<16 bug).
    bus.write_data(0x150U, (15U << 3U) | 0x01U); // PERIOD=15, ENABLE
    bus.tick_peripherals(100000U);
    CHECK((bus.read_data(0x153U) & 0x01U) != 0U); // fires, no hang/crash
}

TEST_CASE("Regression: TWI slave sees STOP as 0xA0")
{
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    Twi master("TWI0", atmega328p.twis[0]);
    Twi slave("TWI1", atmega328p.twis[0]);
    // Same register addresses, so only attach master; drive slave directly.
    bus.attach_peripheral(master);
    slave.reset();
    slave.write(0xBAU, 0xA0U); // TWAR = 0x50
    slave.write(0xBCU, 0x44U); // TWEN + TWEA
    slave.on_twi_match(0xA0U);
    REQUIRE(slave.twi_match_acked() == true);
    slave.on_twi_stop();
    CHECK(slave.read(0xB9U) == 0xA0U);
    InterruptRequest req;
    CHECK(slave.pending_interrupt_request(req) == false); // TWIE not set
}

TEST_CASE("Regression: NVMCTRL Dx commands use Dx busy model")
{
    CHECK(nvm_family_for_name("AVR128DB32") == NvmFamily::Dx);
    CHECK(nvm_family_for_name("AVR16EB32") == NvmFamily::Ex);
    CHECK(nvm_family_for_name("AVR32EA48") == NvmFamily::Ex);
    CHECK(nvm_family_for_name("ATmega4809") == NvmFamily::MegaTiny);
    CHECK(nvm_family_for_name("ATtiny402") == NvmFamily::MegaTiny);

    auto device = DeviceCatalog::find("AVR128DB32");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // CTRLA=0x1000, STATUS=0x1002 (bit0 FLBUSY, bit1 EEBUSY).
    // FLPER (0x08): flash busy ~4ms, not EE busy.
    bus.write_data(0x1000U, 0x08U);
    CHECK((bus.read_data(0x1002U) & 0x01U) != 0U);
    CHECK((bus.read_data(0x1002U) & 0x02U) == 0U);
    bus.tick_peripherals(1000U);
    CHECK((bus.read_data(0x1002U) & 0x01U) != 0U);
    bus.tick_peripherals(64000U);
    CHECK((bus.read_data(0x1002U) & 0x03U) == 0U);

    // EEWR (0x12): EE busy only.
    bus.write_data(0x1000U, 0x12U);
    CHECK((bus.read_data(0x1002U) & 0x02U) != 0U);
    CHECK((bus.read_data(0x1002U) & 0x01U) == 0U);
    bus.tick_peripherals(65000U);
    CHECK((bus.read_data(0x1002U) & 0x03U) == 0U);

    // NOOP (0x01, was misclassified as 4ms PAGEWRITE): clears immediately.
    bus.write_data(0x1000U, 0x01U);
    bus.tick_peripherals(10U);
    CHECK((bus.read_data(0x1002U) & 0x03U) == 0U);
}

TEST_CASE("Regression: EEPROM mapped write is async, no long stall")
{
    // NOTE: on AVR-Dx the bus routes mapped EEPROM through the NVMCTRL page
    // buffer first; the Eeprom peripheral's own mapped path is exercised here
    // with a classic-memory-map bus (ATmega328P has no mapped_eeprom window).
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};

    EepromDescriptor desc{};
    desc.size = 0x100U;
    desc.mapped_data = {0x1400U, 0x100U};
    auto eeprom_ptr = std::make_unique<Eeprom>("EEPROM", desc);
    Eeprom* eeprom = eeprom_ptr.get();
    bus.attach_peripheral(*eeprom_ptr.release());
    eeprom->reset();

    bus.write_data(0x1400U, 0x5AU);
    CHECK(bus.read_data(0x1400U) == 0xFFU); // not yet committed
    bus.write_data(0x1400U, 0xA5U); // ignored while busy
    bus.tick_peripherals(1000U);
    CHECK(bus.read_data(0x1400U) == 0xFFU); // still busy
    bus.tick_peripherals(60000U);
    CHECK(bus.read_data(0x1400U) == 0x5AU); // first value committed
}

TEST_CASE("Regression: SPI pin shaping follows CPOL/CPHA/DORD")
{
    // ATmega328P SPI on PORTB: SS=PB2, MOSI=PB3, MISO=PB4, SCK=PB5.
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    PinMux mux(atmega328p.port_count);
    bus.set_pin_mux(&mux);
    auto gpio_ptr = std::make_unique<GpioPort>(atmega328p.ports[0], mux); // PORTB
    bus.attach_peripheral(*gpio_ptr.release());
    auto spi_ptr = std::make_unique<Spi>("SPI0", atmega328p.spis[0], &mux);
    Spi* spi = spi_ptr.get();
    bus.attach_peripheral(*spi_ptr.release());

    // MISO test driver: PB4 output, held high.
    bus.write_data(0x24U, 0x10U); // DDRB bit4 out
    bus.write_data(0x25U, 0x10U); // PORTB bit4 high

    // Mode 0 (CPOL=0, CPHA=0), master, fosc/4: 2 cycles per half-bit.
    bus.write_data(0x4CU, 0x50U); // SPE|MSTR
    bus.write_data(0x4EU, 0x80U); // MSB first, bit7 = 1
    CHECK(spi->busy() == true);
    bus.tick_peripherals(1U); // no edge yet: SS fallen, MOSI set up, SCK idle
    CHECK(mux.get_state_by_address(0x25U, 2U).owner == PinOwner::spi);
    CHECK(mux.get_state_by_address(0x25U, 2U).drive_level == false); // SS low
    CHECK(mux.get_state_by_address(0x25U, 3U).owner == PinOwner::spi);
    CHECK(mux.get_state_by_address(0x25U, 3U).drive_level == true); // bit7
    CHECK(mux.get_state_by_address(0x25U, 5U).drive_level == false); // SCK idle
    bus.tick_peripherals(1U); // leading edge of bit 0: SCK rises
    CHECK(mux.get_state_by_address(0x25U, 5U).drive_level == true);
    bus.tick_peripherals(30U); // 32 cycles total: transfer done
    CHECK(spi->busy() == false);
    CHECK((bus.read_data(0x4DU) & 0x80U) != 0U); // SPIF
    CHECK(bus.read_data(0x4EU) == 0xFFU); // MISO held high: all ones
    // Pins released back to GPIO, SCK resting at idle level.
    CHECK(mux.get_state_by_address(0x25U, 5U).owner == PinOwner::gpio);
    CHECK(mux.get_state_by_address(0x25U, 5U).drive_level == false);
    CHECK(mux.get_state_by_address(0x25U, 2U).owner == PinOwner::gpio);

    // Mode 3 (CPOL=1, CPHA=1): SCK idles high, MOSI set up on leading edge.
    bus.write_data(0x25U, 0x00U); // MISO driver low
    bus.write_data(0x4CU, 0x5CU); // SPE|MSTR|CPOL|CPHA
    bus.write_data(0x4EU, 0x80U);
    bus.tick_peripherals(1U);
    CHECK(mux.get_state_by_address(0x25U, 5U).drive_level == true); // idle high
    bus.tick_peripherals(1U); // leading edge: SCK falls, MOSI bit7 driven
    CHECK(mux.get_state_by_address(0x25U, 5U).drive_level == false);
    CHECK(mux.get_state_by_address(0x25U, 3U).drive_level == true);
    bus.tick_peripherals(30U);
    CHECK((bus.read_data(0x4DU) & 0x80U) != 0U); // SPIF
    CHECK(bus.read_data(0x4EU) == 0x00U); // MISO held low: all zeros

    // DORD=1 (LSB first): first MOSI bit out is bit0.
    bus.write_data(0x4CU, 0x70U); // SPE|MSTR|DORD
    bus.write_data(0x4EU, 0x01U); // bit0 = 1 goes out first
    bus.tick_peripherals(1U);
    CHECK(mux.get_state_by_address(0x25U, 3U).owner == PinOwner::spi);
    CHECK(mux.get_state_by_address(0x25U, 3U).drive_level == true);
    bus.tick_peripherals(31U);
    CHECK((bus.read_data(0x4DU) & 0x80U) != 0U); // SPIF
}

namespace {

// XCH Z,Rd = 0x9204|(d<<4); LAS = 0x9205|..; LAC = 0x9206|..; LAT = 0x9207|..
constexpr u16 encode_rmw_z(const u8 base_low, const u8 reg) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(reg) << 4U) | base_low);
}

// MOV Rd,Rr = 0010 11rd dddd rrrr
constexpr u16 encode_mov(const u8 dest, const u8 src) {
    return static_cast<u16>(0x2C00U | (static_cast<u16>(dest & 0x1FU) << 4U)
        | ((static_cast<u16>(src & 0x10U)) << 5U) | (src & 0x0FU));
}

// DES K = 1001 0100 KKKK 1011
constexpr u16 encode_des(const u8 round) {
    return static_cast<u16>(0x940BU | (static_cast<u16>(round & 0x0FU) << 4U));
}

// ST Z+,Rr = 1001 001r rrrr 0001
constexpr u16 encode_st_z_postinc(const u8 reg) {
    return static_cast<u16>(0x9201U | (static_cast<u16>(reg) << 4U));
}

// OUT A,Rr: 1011 1AAr rrrr AAAA (A = 6-bit I/O offset)
constexpr u16 encode_out(const u8 io_offset, const u8 reg) {
    return static_cast<u16>(0xB800U | (static_cast<u16>(reg) << 4U)
        | ((static_cast<u16>(io_offset & 0x30U)) << 5U) | (io_offset & 0x0FU));
}

// STS k,Rr opcode word (address word follows): 1001 001r rrrr 0000
constexpr u16 encode_sts(const u8 reg) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(reg) << 4U));
}

} // namespace

TEST_CASE("Regression: XCH/LAS/LAC/LAT exchange with DS(Z), Rd gets old value")
{
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    const auto sram_base = atmega328p.sram_range().begin;

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(30U, static_cast<u8>(sram_base & 0xFFU)),
            encode_ldi(31U, static_cast<u8>((sram_base >> 8U) & 0xFFU)),
            encode_ldi(16U, 0x0FU), encode_rmw_z(0x04U, 16U), // XCH
            encode_ldi(17U, 0x0FU), encode_rmw_z(0x05U, 17U), // LAS
            encode_ldi(18U, 0x0FU), encode_rmw_z(0x06U, 18U), // LAC
            encode_ldi(19U, 0xFFU), encode_rmw_z(0x07U, 19U), // LAT
            0x0000U
        },
        .entry_word = 0U
    });
    cpu.reset();
    bus.write_data(sram_base, 0xF0U); // pre-fill AFTER reset
    cpu.step(); // LDI R30
    cpu.step(); // LDI R31

    cpu.step(); // LDI R16
    cpu.step(); // XCH Z,R16: R16 <- 0xF0, mem <- 0x0F
    CHECK(cpu.snapshot().gpr[16] == 0xF0U);
    CHECK(bus.read_data(sram_base) == 0x0FU);

    bus.write_data(sram_base, 0xF0U);
    cpu.step(); // LDI R17
    cpu.step(); // LAS Z,R17: R17 <- 0xF0, mem <- 0x0F|0xF0 = 0xFF
    CHECK(cpu.snapshot().gpr[17] == 0xF0U);
    CHECK(bus.read_data(sram_base) == 0xFFU);

    bus.write_data(sram_base, 0xFFU);
    cpu.step(); // LDI R18
    cpu.step(); // LAC Z,R18: R18 <- 0xFF, mem <- ~0x0F&0xFF = 0xF0
    CHECK(cpu.snapshot().gpr[18] == 0xFFU);
    CHECK(bus.read_data(sram_base) == 0xF0U);

    bus.write_data(sram_base, 0xF0U);
    cpu.step(); // LDI R19
    cpu.step(); // LAT Z,R19: R19 <- 0xF0, mem <- 0xFF^0xF0 = 0x0F
    CHECK(cpu.snapshot().gpr[19] == 0xF0U);
    CHECK(bus.read_data(sram_base) == 0x0FU);

    // Z unchanged by all four.
    CHECK(cpu.snapshot().gpr[30] == static_cast<u8>(sram_base & 0xFFU));
    CHECK(cpu.snapshot().gpr[31] == static_cast<u8>((sram_base >> 8U) & 0xFFU));
}

TEST_CASE("Regression: DES K encrypts FIPS-46 vector, decrypt inverts it")
{
    // FIPS-46: key 0x133457799BBCDFF1, plaintext 0x0123456789ABCDEF,
    // ciphertext 0x85E813540F0AB405. LSB of block/key in R0/R8 LSB.
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    const auto sram_base = atmega328p.sram_range().begin;

    const u8 plaintext[8] = {0xEFU, 0xCDU, 0xABU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U};
    const u8 key[8] = {0xF1U, 0xDFU, 0xBCU, 0x9BU, 0x79U, 0x57U, 0x34U, 0x13U};
    const u8 ciphertext[8] = {0x05U, 0xB4U, 0x0AU, 0x0FU, 0x54U, 0x13U, 0xE8U, 0x85U};

    std::vector<u16> flash;
    flash.push_back(encode_ldi(30U, static_cast<u8>(sram_base & 0xFFU)));
    flash.push_back(encode_ldi(31U, static_cast<u8>((sram_base >> 8U) & 0xFFU)));
    for (u8 i = 0U; i < 8U; ++i) {
        flash.push_back(encode_ldi(16U, plaintext[i]));
        flash.push_back(encode_mov(i, 16U)); // R0-R7
    }
    for (u8 i = 0U; i < 8U; ++i) {
        flash.push_back(encode_ldi(16U, key[i]));
        flash.push_back(encode_mov(static_cast<u8>(8U + i), 16U)); // R8-R15
    }
    for (u8 k = 0U; k < 16U; ++k) {
        flash.push_back(encode_des(k)); // H=0: encrypt rounds 0..15
    }
    for (u8 i = 0U; i < 8U; ++i) {
        flash.push_back(encode_st_z_postinc(i)); // store ciphertext
    }
    flash.push_back(0x9458U); // BSET 5: H=1 for decryption
    for (u8 k = 0U; k < 16U; ++k) {
        flash.push_back(encode_des(k)); // H=1: decrypt rounds 0..15
    }
    for (u8 i = 0U; i < 8U; ++i) {
        flash.push_back(encode_st_z_postinc(i)); // store recovered plaintext
    }
    flash.push_back(0x0000U);

    bus.load_image(HexImage {.flash_words = flash, .entry_word = 0U});
    cpu.reset();
    for (size_t i = 0U; i < flash.size() - 1U; ++i) {
        cpu.step();
    }
    for (u8 i = 0U; i < 8U; ++i) {
        CHECK(bus.read_data(static_cast<u16>(sram_base + i)) == ciphertext[i]);
    }
    for (u8 i = 0U; i < 8U; ++i) {
        CHECK(bus.read_data(static_cast<u16>(sram_base + 8U + i)) == plaintext[i]);
    }
}

TEST_CASE("Regression: Idle wake costs start-up(0) + 4-cycle ISR entry")
{
    // Timer0 overflows during the SLEEP step itself; wake must then cost
    // exactly 0 (oscillator already running) + 4 (interrupt response).
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    Timer8 timer0 {"TIMER0", atmega328p.timers8[0]};
    bus.attach_peripheral(timer0);
    AvrCpu cpu {bus};

    const u16 timsk0 = atmega328p.timers8[0].timsk_address;
    std::vector<u16> flash = {
        encode_ldi(16U, 0x01U),
        0x9200U | (16U << 4U), timsk0, // STS TIMSK0, TOIE0=1
        encode_ldi(16U, 0x01U),
        encode_out(0x25U, 16U), // OUT TCCR0B (0x45), CS=1 (/1)
        0x9478U, // SEI
        0x9588U, // SLEEP
    };
    flash.resize(24U, 0x0000U); // pad past the Timer0 OVF vector
    bus.load_image(HexImage {.flash_words = flash, .entry_word = 0U});
    cpu.reset();

    bus.write_data(0x53U, 0x01U); // SMCR: SE=1, Idle
    for (int i = 0; i < 5; ++i) cpu.step(); // LDI/STS/LDI/OUT/SEI
    bus.write_data(0x46U, 0xFFU); // TCNT0 one tick before overflow
    const u64 c0 = cpu.cycles();
    cpu.step(); // SLEEP: timer wraps to 0x00, TOV0 sets
    CHECK(cpu.state() == CpuState::sleeping);
    int watchdog = 8;
    while (cpu.state() == CpuState::sleeping && --watchdog > 0) cpu.step();
    CHECK(watchdog > 0);
    CHECK(cpu.state() == CpuState::running);
    // 1 (SLEEP) + 0 (start-up) + 4 (ISR entry).
    CHECK(cpu.cycles() - c0 == 5U);
}

TEST_CASE("Regression: sleep wake latency follows mode and clock fuses")
{
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    cpu.reset();

    bus.write_data(0x53U, 0x01U); // Idle
    CHECK(cpu.get_sleep_wake_latency() == 0U);
    bus.write_data(0x53U, 0x03U); // ADC Noise Reduction
    CHECK(cpu.get_sleep_wake_latency() == 0U);
    bus.write_data(0x53U, 0x05U); // Power-down, internal RC fuses
    CHECK(cpu.get_sleep_wake_latency() == 6U);
    bus.write_data(0x53U, 0x07U); // Power-save, internal RC fuses
    CHECK(cpu.get_sleep_wake_latency() == 6U);
    bus.write_data(0x53U, 0x0DU); // Standby: oscillator running
    CHECK(cpu.get_sleep_wake_latency() == 2U);
    bus.write_data(0x53U, 0x0FU); // Extended Standby: oscillator running
    CHECK(cpu.get_sleep_wake_latency() == 2U);

    // Crystal fuse variants (copied descriptor; MemoryBus copies it).
    auto dev_xtal = atmega328p;
    dev_xtal.fuses[0] = 0xFFU; // CKSEL=1111, SUT=11 -> 16K CK
    MemoryBus bus_xtal {dev_xtal};
    AvrCpu cpu_xtal {bus_xtal};
    cpu_xtal.reset();
    bus_xtal.write_data(0x53U, 0x05U);
    CHECK(cpu_xtal.get_sleep_wake_latency() == 16384U);

    auto dev_1k = atmega328p;
    dev_1k.fuses[0] = 0x2EU; // CKSEL=1110, SUT=10 -> 1K CK
    MemoryBus bus_1k {dev_1k};
    AvrCpu cpu_1k {bus_1k};
    cpu_1k.reset();
    bus_1k.write_data(0x53U, 0x05U);
    CHECK(cpu_1k.get_sleep_wake_latency() == 1024U);

    auto dev_32k = atmega328p;
    dev_32k.fuses[0] = 0x25U; // CKSEL=0101 (32K crystal), SUT=10 -> 32K CK
    MemoryBus bus_32k {dev_32k};
    AvrCpu cpu_32k {bus_32k};
    cpu_32k.reset();
    bus_32k.write_data(0x53U, 0x05U);
    CHECK(cpu_32k.get_sleep_wake_latency() == 32768U);
}

TEST_CASE("Regression: AVRxt wake latency is 6 + oscillator start-up")
{
    // Datasheet §SLPCTRL: six CLK_PER plus OSC start-up when stopped.
    using namespace vioavr::core::devices;
    MemoryBus bus {avr128db32};
    auto clk = std::make_unique<ClkCtrl>(avr128db32.clkctrl, 20'000'000U);
    auto slp = std::make_unique<SlpCtrl>(avr128db32.slpctrl);
    bus.attach_peripheral(*clk);
    bus.attach_peripheral(*slp);
    AvrCpu cpu {bus}; // ctor auto-discovers ClkCtrl/SlpCtrl
    cpu.reset();

    const u16 slpctrla = avr128db32.slpctrl.ctrla_address;
    const u16 mclkctrla = avr128db32.clkctrl.ctrla_address;
    bus.write_data(slpctrla, 0x01U); // SEN + Idle
    CHECK(cpu.get_sleep_wake_latency() == 2U);
    bus.write_data(slpctrla, 0x05U); // SEN + Power-down, OSC20M default
    CHECK(cpu.get_sleep_wake_latency() == 8U);
    bus.write_data(mclkctrla, 0x03U); // EXTCLK: 2-cycle start-up
    CHECK(cpu.get_sleep_wake_latency() == 4U);
    bus.write_data(mclkctrla, 0x02U); // XOSC32K, CSUT reset = 1K
    CHECK(cpu.get_sleep_wake_latency() == 1026U);
}

TEST_CASE("Regression: SPM RWW erase stalls RWW execution until done")
{
    // Firmware lives in RWW and erases an RWW page: the CPU must freeze
    // (pc stuck, cycles advancing) for the 64k-cycle operation, then resume.
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    const u16 spmcsr = atmega328p.spmcsr_address; // 0x57

    bus.load_image(HexImage {.flash_words = {
        encode_ldi(30U, 0x00U),
        encode_ldi(31U, 0x02U), // Z = 0x0200 (word 0x100, RWW)
        encode_ldi(16U, 0x03U), // PGERS|SPMEN
        encode_sts(16U), spmcsr,
        0x95E8U, // SPM (erase)
        0x0000U, // NOP, pc=6: stall point
        encode_ldi(16U, 0x11U), // RWWSRE|SPMEN
        encode_sts(16U), spmcsr,
        0x95E8U, // SPM (RWWSRE)
        0x0000U, 0x0000U,
    }, .entry_word = 0U});
    bus.write_program_word(0x100U, 0x1234U); // pre-fill target page
    cpu.reset();

    for (int i = 0; i < 5; ++i) cpu.step(); // through SPM
    CHECK(cpu.program_counter() == 6U);
    CHECK((bus.read_data(spmcsr) & 0x40U) != 0U); // RWWSB set at op start
    const u64 c0 = cpu.cycles();
    for (int i = 0; i < 20; ++i) cpu.step();
    CHECK(cpu.program_counter() == 6U); // frozen...
    CHECK(cpu.cycles() - c0 == 20U); // ...while cycles advance
    cpu.run(70000U); // drains the busy window, runs the RWWSRE tail
    CHECK(bus.flash_words()[0x100U] == 0xFFFFU); // page erased
    CHECK((bus.read_data(spmcsr) & 0x40U) == 0U); // RWWSRE cleared RWWSB
    CHECK(cpu.program_counter() != 6U);
}

TEST_CASE("Regression: SPM NRWW erase does not stall RWW execution")
{
    // Same firmware shape, but the target page is in NRWW: the RWW section
    // stays readable, so the CPU must keep running while the op is in flight.
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    const u16 spmcsr = atmega328p.spmcsr_address;

    bus.load_image(HexImage {.flash_words = {
        encode_ldi(30U, 0x00U),
        encode_ldi(31U, 0x70U), // Z = 0x7000 (word 0x3800, NRWW)
        encode_ldi(16U, 0x03U), // PGERS|SPMEN
        encode_sts(16U), spmcsr,
        0x95E8U, // SPM (erase)
        0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U,
    }, .entry_word = 0U});
    bus.write_program_word(0x3800U, 0x1234U); // pre-fill target page
    cpu.reset();

    for (int i = 0; i < 5; ++i) cpu.step(); // through SPM
    CHECK(cpu.program_counter() == 6U);
    CHECK((bus.read_data(spmcsr) & 0x01U) != 0U); // SPMEN: op in flight
    for (int i = 0; i < 5; ++i) cpu.step();
    CHECK(cpu.program_counter() == 11U); // kept running, not stuck at 6
    cpu.run(70000U);
    CHECK(bus.flash_words()[0x3800U] == 0xFFFFU); // op really ran
}

TEST_CASE("Regression: NVMCTRL EEPROM write does not stall the CPU")
{
    // Dx EEWR raises EEBUSY for ~4ms but must not freeze execution.
    auto device = DeviceCatalog::find("AVR128DB32");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    machine.bus().load_image(HexImage {
        .flash_words = std::vector<u16>(64U, 0x0000U),
        .entry_word = 0U
    });

    machine.bus().write_data(0x1000U, 0x12U); // EEWR
    for (int i = 0; i < 20; ++i) machine.step();
    CHECK(machine.cpu().program_counter() == 20U); // advanced, not stalled
    CHECK((machine.bus().read_data(0x1002U) & 0x02U) != 0U); // EEBUSY: still in flight
}

TEST_CASE("Regression: TWI slave stretches SCL until TWINT clears")
{
    // ATmega328P TWI on PORTC: SDA=PC4, SCL=PC5 (PORTC data = 0x28).
    // Master + slave as separate instances; slave registers remapped to
    // 0xE0-0xE5 so both can share the bus (same SCL/SDA pins).
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    PinMux mux(atmega328p.port_count);
    bus.set_pin_mux(&mux);
    auto gpio_ptr = std::make_unique<GpioPort>(atmega328p.ports[1], mux); // PORTC
    bus.attach_peripheral(*gpio_ptr.release());

    Twi master {"TWIM", atmega328p.twis[0], &mux};
    bus.attach_peripheral(master);
    auto slave_desc = atmega328p.twis[0];
    slave_desc.twbr_address = 0xE0U; slave_desc.twsr_address = 0xE1U;
    slave_desc.twar_address = 0xE2U; slave_desc.twdr_address = 0xE3U;
    slave_desc.twcr_address = 0xE4U; slave_desc.twamr_address = 0xE5U;
    Twi slave {"TWIS", slave_desc, &mux};
    bus.attach_peripheral(slave);

    constexpr u16 kTwcr = 0xBCU; // master TWCR
    constexpr u16 kTwdr = 0xBBU; // master TWDR
    constexpr u16 kTwsr = 0xB9U; // master TWSR
    constexpr u16 kSlvTwcr = 0xE4U, kSlvTwar = 0xE2U;
    constexpr u8 kTwint = 0x80U, kTwea = 0x40U, kSta = 0x20U, kSto = 0x10U, kTwen = 0x04U;
    auto scl = [&mux]() { return mux.get_state_by_address(0x28U, 5U); };

    CHECK(scl().owner == PinOwner::gpio); // idle: no hold

    // Slave listens on SLA 0x42 with ACK enabled.
    bus.write_data(kSlvTwar, 0x84U);
    bus.write_data(kSlvTwcr, kTwea | kTwen);

    // Master: START, then SLA+W to 0x42.
    bus.write_data(kTwcr, kTwint | kTwea | kSta | kTwen);
    bus.tick_peripherals(200U);
    CHECK((bus.read_data(kTwsr) & 0xF8U) == 0x08U); // START sent
    bus.write_data(kTwdr, 0x84U);
    bus.write_data(kTwcr, kTwint | kTwen);
    bus.tick_peripherals(200U);
    CHECK((bus.read_data(kTwsr) & 0xF8U) == 0x18U); // SLA+W ACKed
    // Addressed slave stretches: SCL held low until it clears TWINT.
    CHECK(scl().owner == PinOwner::twi);
    CHECK(scl().drive_level == false);

    // Master queues a data byte; its bit clock must freeze while stretched.
    bus.write_data(kTwdr, 0x55U);
    bus.write_data(kTwcr, kTwint | kTwen);
    bus.tick_peripherals(200U);
    CHECK((bus.read_data(kTwcr) & kTwint) == 0U); // master step frozen
    CHECK((bus.read_data(kTwsr) & 0xF8U) == 0x18U); // status unchanged
    CHECK(scl().drive_level == false); // still stretched

    // Slave services TWINT: releases SCL; master bit clock resumes.
    bus.write_data(kSlvTwcr, kTwint | kTwea | kTwen);
    CHECK(scl().owner == PinOwner::gpio);
    bus.tick_peripherals(200U);
    CHECK((bus.read_data(kTwcr) & kTwint) != 0U); // master step done
    CHECK((bus.read_data(kTwsr) & 0xF8U) == 0x28U); // data ACKed
    // Slave holds again for the received byte until serviced.
    CHECK(scl().owner == PinOwner::twi);
    bus.write_data(kSlvTwcr, kTwint | kTwea | kTwen);
    CHECK(scl().owner == PinOwner::gpio);

    // Master STOP frees the bus; slave reports 0xA0 without holding.
    bus.write_data(kTwcr, kTwint | kSto | kTwen);
    bus.tick_peripherals(200U);
    CHECK((bus.read_data(0xE1U) & 0xF8U) == 0xA0U); // slave STOP status
    CHECK(scl().owner == PinOwner::gpio);
}

TEST_CASE("Regression: UART RX latches FE and mode-gated UPE")
{
    // ATmega328P USART0: UDR=0xC6, UCSRA=0xC0 (RXC=0x80, FE=0x10, UPE=0x04).
    using namespace vioavr::core::devices;
    MemoryBus bus {atmega328p};
    PinMux mux {3};
    Uart uart {"USART0", atmega328p.uarts[0], mux};
    bus.attach_peripheral(uart);
    const auto desc = atmega328p.uarts[0];

    bus.write_data(desc.ucsrb_address, 0x10U); // RXEN
    bus.write_data(desc.ucsrc_address, 0x06U); // 8N1, parity off

    // Clean frame: no flags.
    CHECK(uart.inject_received_byte(0x55U) == true);
    bus.tick_peripherals(200U); // 8N1 at UBRR=0: 10 bits x 16 cycles
    CHECK((bus.read_data(desc.ucsra_address) & 0x80U) != 0U); // RXC
    CHECK((bus.read_data(desc.ucsra_address) & 0x14U) == 0U); // no FE/UPE
    CHECK(bus.read_data(desc.udr_address) == 0x55U);

    // Bad stop bit: FE latches with RXC, clears on UDR read.
    CHECK(uart.inject_received_byte(0xA3U, true) == true);
    bus.tick_peripherals(200U);
    CHECK((bus.read_data(desc.ucsra_address) & 0x80U) != 0U); // RXC
    CHECK((bus.read_data(desc.ucsra_address) & 0x10U) != 0U); // FE
    CHECK((bus.read_data(desc.ucsra_address) & 0x04U) == 0U); // no UPE (8N1)
    CHECK(bus.read_data(desc.udr_address) == 0xA3U);
    CHECK((bus.read_data(desc.ucsra_address) & 0x94U) == 0U); // RXC/FE cleared

    // Parity error with parity disabled: UPE must NOT latch.
    CHECK(uart.inject_received_byte(0x5AU, false, true) == true);
    bus.tick_peripherals(200U);
    CHECK((bus.read_data(desc.ucsra_address) & 0x80U) != 0U); // RXC
    CHECK((bus.read_data(desc.ucsra_address) & 0x04U) == 0U); // no UPE
    (void)bus.read_data(desc.udr_address);

    // Even parity mode (UPM1:0=10): wrong parity bit latches UPE.
    bus.write_data(desc.ucsrc_address, 0x26U); // 8-bit, even parity
    CHECK(uart.inject_received_byte(0x5AU, false, true) == true);
    bus.tick_peripherals(200U); // 8E1: 11 bits x 16 cycles
    CHECK((bus.read_data(desc.ucsra_address) & 0x80U) != 0U); // RXC
    CHECK((bus.read_data(desc.ucsra_address) & 0x04U) != 0U); // UPE
    CHECK(bus.read_data(desc.udr_address) == 0x5AU);
    CHECK((bus.read_data(desc.ucsra_address) & 0x84U) == 0U); // RXC/UPE cleared
}

TEST_CASE("Regression: USART RXDATAH carries FERR and mode-gated PERR")
{
    // ATmega4809 USART0: RXDATA=0x800, RXDATAH=0x801 (FERR=0x04, PERR=0x02).
    using namespace vioavr::core::devices;
    PinMux mux {8};
    MemoryBus bus {atmega4809};
    Uart8x uart {atmega4809.uarts8x[0], mux};
    bus.attach_peripheral(uart);
    const auto D = atmega4809.uarts8x[0];

    uart.write(D.ctrlb_address, 0x80U); // RXEN
    CHECK(D.ctrlc_address != 0U);

    // Clean frame: no error bits.
    uart.inject_received_byte(0x5AU);
    uart.tick(500U);
    CHECK(uart.read(D.rxdata_address) == 0x5AU);
    CHECK((uart.read(static_cast<u16>(D.rxdata_address + 1U)) & 0x06U) == 0U);

    // Bad stop bit: FERR rides with the byte (8N1 default: PMODE=0).
    uart.inject_received_byte(0xA3U, false, true);
    uart.tick(500U);
    CHECK(uart.read(D.rxdata_address) == 0xA3U);
    CHECK((uart.read(static_cast<u16>(D.rxdata_address + 1U)) & 0x04U) != 0U); // FERR
    CHECK((uart.read(static_cast<u16>(D.rxdata_address + 1U)) & 0x02U) == 0U); // no PERR

    // Parity error with parity disabled: PERR must NOT latch.
    uart.inject_received_byte(0x3CU, false, false, true);
    uart.tick(500U);
    CHECK(uart.read(D.rxdata_address) == 0x3CU);
    CHECK((uart.read(static_cast<u16>(D.rxdata_address + 1U)) & 0x06U) == 0U);

    // Even parity mode (PMODE=10): wrong parity bit latches PERR.
    uart.write(D.ctrlc_address, 0x23U); // 8-bit, even parity
    uart.inject_received_byte(0x3CU, false, false, true);
    uart.tick(500U);
    CHECK(uart.read(D.rxdata_address) == 0x3CU);
    CHECK((uart.read(static_cast<u16>(D.rxdata_address + 1U)) & 0x02U) != 0U); // PERR
}
