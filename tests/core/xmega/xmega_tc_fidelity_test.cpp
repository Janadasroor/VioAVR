#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/tc.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/device.hpp"

using namespace vioavr::core;

static DeviceDescriptor minimal_device() noexcept {
    DeviceDescriptor d{};
    d.name = "test";
    d.flash_words = 16384;
    d.sram_bytes = 2048;
    d.eeprom_bytes = 512;
    d.interrupt_vector_count = 64;
    d.interrupt_vector_size = 4;
    return d;
}

static TcDescriptor make_test_tc0() noexcept {
    TcDescriptor d{};
    d.ctrla_address = 0x0800;
    d.ctrlb_address = 0x0801;
    d.ctrlc_address = 0x0802;
    d.ctrld_address = 0x0803;
    d.ctrle_address = 0x0804;
    d.intctrla_address = 0x0806;
    d.intctrlb_address = 0x0807;
    d.ctrlfclr_address = 0x0808;
    d.ctrlfset_address = 0x0809;
    d.ctrlgclr_address = 0x080A;
    d.ctrlgset_address = 0x080B;
    d.intflags_address = 0x080C;
    d.temp_address = 0x080F;
    d.cnt_address = 0x0820;
    d.period_address = 0x0826;
    d.cca_address = 0x0828;
    d.ccb_address = 0x082A;
    d.ccc_address = 0x082C;
    d.ccd_address = 0x082E;
    d.perbuf_address = 0x0836;
    d.ccabuf_address = 0x0838;
    d.ccbbuf_address = 0x083A;
    d.cccbuf_address = 0x083C;
    d.ccdbuf_address = 0x083E;
    d.ovf_vector_index = 20;
    d.err_vector_index = 21;
    d.cca_vector_index = 22;
    d.ccb_vector_index = 23;
    d.ccc_vector_index = 24;
    d.ccd_vector_index = 25;
    return d;
}

TEST_CASE("XMEGA TC Fidelity: Normal Mode Counting") {
    auto d = make_test_tc0();
    Tc tc("TCC0", d);
    tc.reset();

    tc.write(d.ctrla_address, 0x01);
    tc.tick(1);
    CHECK(tc.read(d.cnt_address) == 1);

    tc.tick(4);
    CHECK(tc.read(d.cnt_address) == 5);
}

TEST_CASE("XMEGA TC Fidelity: Period Match and Wrap") {
    auto d = make_test_tc0();
    Tc tc("TCC0", d);
    tc.reset();

    tc.write(d.ctrla_address, 0x01);
    tc.write(d.period_address, 10);
    tc.write(d.period_address + 1, 0);

    tc.tick(9);
    CHECK(tc.read(d.cnt_address) == 9);

    tc.tick(1);
    CHECK(tc.read(d.cnt_address) == 0);

    u8 intflags = tc.read(d.intflags_address);
    CHECK((intflags & 0x01) != 0);
}

TEST_CASE("XMEGA TC Fidelity: Compare Match") {
    auto d = make_test_tc0();
    auto dev = minimal_device();
    MemoryBus bus{dev};
    Tc tc("TCC0", d);
    tc.set_memory_bus(&bus);
    tc.reset();

    tc.write(d.ctrla_address, 0x01);
    tc.write(d.period_address, 20);
    tc.write(d.period_address + 1, 0);
    tc.write(d.cca_address, 7);
    tc.write(d.cca_address + 1, 0);
    tc.write(d.intctrlb_address, 0x01);

    tc.tick(7);
    CHECK(tc.read(d.cnt_address) == 7);

    u8 intflags = tc.read(d.intflags_address);
    CHECK((intflags & 0x10) != 0);

    InterruptRequest req;
    CHECK(tc.pending_interrupt_request(req));
    CHECK(req.vector_index == d.cca_vector_index);

    tc.write(d.intflags_address, 0x10);
    CHECK((tc.read(d.intflags_address) & 0x10) == 0);
}

TEST_CASE("XMEGA TC Fidelity: Double Buffering") {
    auto d = make_test_tc0();
    Tc tc("TCC0", d);
    tc.reset();

    tc.write(d.ctrla_address, 0x01);
    tc.write(d.period_address, 10);
    tc.write(d.period_address + 1, 0);

    tc.tick(5);
    CHECK(tc.read(d.cnt_address) == 5);

    tc.write(d.perbuf_address, 20);
    tc.write(d.perbuf_address + 1, 0);

    CHECK(tc.read(d.period_address) == 10);

    tc.tick(4);
    CHECK(tc.read(d.cnt_address) == 9);

    tc.tick(1);
    CHECK(tc.read(d.cnt_address) == 0);
    CHECK(tc.read(d.period_address) == 20);
}

TEST_CASE("XMEGA TC Fidelity: Single-Slope PWM WO Output") {
    auto d = make_test_tc0();
    Tc tc("TCC0", d);
    tc.reset();

    // Enable, single-slope PWM (wg_mode=1), non-inverting
    tc.write(d.ctrla_address, 0x01);
    tc.write(d.ctrlb_address, 0x01); // wg_mode=1, no WO output enable
    tc.write(d.period_address, 10);
    tc.write(d.period_address + 1, 0);
    tc.write(d.cca_address, 5);
    tc.write(d.cca_address + 1, 0);

    // With wg_mode==1 but WOEN clear, outputs are not enabled
    // Enable WO0 (bit 4 in CTRLB)
    tc.write(d.ctrlb_address, 0x11); // wg_mode=1 | WO0EN

    // After reset, no ticks yet: wo should be false (BOTTOM state)
    CHECK(tc.get_wo_level(0) == false);

    // Tick to cnt=5 (compare match should set wo)
    tc.tick(5);
    CHECK(tc.get_wo_level(0) == true);

    // Tick to cnt=10 (TOP, wo should clear)
    tc.tick(5);
    CHECK(tc.get_wo_level(0) == false);
    CHECK((tc.read(d.intflags_address) & 0x01) != 0); // OVF

    // Count to 5 again (wo should set at CMP)
    tc.tick(5);
    CHECK(tc.get_wo_level(0) == true);

    // Inverting mode: set CMPPOL bit
    tc.write(d.ctrlc_address, 0x10);
    tc.reset();
    tc.write(d.ctrla_address, 0x01);
    tc.write(d.ctrlb_address, 0x11);
    tc.write(d.period_address, 10);
    tc.write(d.period_address + 1, 0);
    tc.write(d.cca_address, 5);
    tc.write(d.cca_address + 1, 0);
    tc.write(d.ctrlc_address, 0x10); // CCA polarity = inverting

    // After reset: wo should follow CMPPOL = HIGH (inverting starts HIGH)
    CHECK(tc.get_wo_level(0) == true);

    tc.tick(5);
    // At CMP match: inverting clears the output
    CHECK(tc.get_wo_level(0) == false);

    tc.tick(5);
    // At TOP: inverting sets the output back to HIGH
    CHECK(tc.get_wo_level(0) == true);
}

TEST_CASE("XMEGA TC Fidelity: CMP==PER Boundary Match") {
    auto d = make_test_tc0();
    auto dev = minimal_device();
    MemoryBus bus{dev};
    Tc tc("TCC0", d);
    tc.set_memory_bus(&bus);
    tc.reset();

    tc.write(d.ctrla_address, 0x01);
    tc.write(d.period_address, 5);
    tc.write(d.period_address + 1, 0);
    tc.write(d.cca_address, 5);
    tc.write(d.cca_address + 1, 0);
    tc.write(d.intctrlb_address, 0x01);

    // Count to 4 (one before PER)
    tc.tick(4);
    CHECK(tc.read(d.cnt_address) == 4);
    CHECK((tc.read(d.intflags_address) & 0x10) == 0);
    CHECK((tc.read(d.intflags_address) & 0x01) == 0);

    // 5th tick: cnt 4→5→0, CMP match fires (cnt_==cca_) before wrap
    tc.tick(1);
    CHECK(tc.read(d.cnt_address) == 0); // wrapped

    u8 intflags = tc.read(d.intflags_address);
    CHECK((intflags & 0x10) != 0); // CMP match fired
    CHECK((intflags & 0x01) != 0); // OVF also fired in same tick
}

TEST_CASE("XMEGA TC Fidelity: Dual Slope Mode") {
    auto d = make_test_tc0();
    Tc tc("TCC0", d);
    tc.reset();

    tc.write(d.ctrla_address, 0x01);
    tc.write(d.ctrlb_address, 0x04);

    tc.tick(1);
    CHECK(tc.read(d.cnt_address) == 1);

    tc.tick(1);
    CHECK(tc.read(d.cnt_address) == 2);

    tc.write(d.period_address, 10);
    tc.write(d.period_address + 1, 0);

    tc.tick(1);
    CHECK(tc.read(d.cnt_address) == 3);

    tc.tick(7);
    CHECK(tc.read(d.cnt_address) == 10);

    tc.tick(1);
    CHECK(tc.read(d.cnt_address) <= 10);

    tc.tick(5);
    CHECK(tc.read(d.cnt_address) == 4);

    tc.tick(4);
    u8 tcnt_val = tc.read(d.cnt_address);
    CHECK_MESSAGE(tcnt_val == 0, "tcnt should reach 0 after counting down");

    u8 intflags = tc.read(d.intflags_address);
    CHECK((intflags & 0x01) != 0);
}
