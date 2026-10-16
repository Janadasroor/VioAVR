#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/eeprom.hpp"
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

TEST_CASE("Regression: TCB async RTC ratio and sync counting")
{
    using namespace vioavr::core::devices;
    // Sync: CLK_PER counts every cycle.
    Tcb sync("TCB0", avr128db32.timers_tcb[0]);
    sync.reset();
    sync.write(0xB0CU, 0xFFU);
    sync.write(0xB0DU, 0xFFU); // CCMP=0xFFFF
    sync.write(0xB00U, 0x01U); // enable, CLK_PER
    sync.tick(10U);
    CHECK(sync.read(0xB0AU) == 10U);

    // Async RTC @16MHz: 16000000/32768 = 488 cycles per tick.
    Tcb async("TCB0", avr128db32.timers_tcb[0]);
    async.reset();
    async.write(0xB0CU, 0xFFU);
    async.write(0xB0DU, 0xFFU);
    async.write(0xB01U, 0x40U); // ASYNC
    async.write(0xB00U, 0x01U); // enable
    async.tick(487U);
    CHECK(async.read(0xB0AU) == 0U);
    async.tick(1U);
    CHECK(async.read(0xB0AU) == 1U);
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
