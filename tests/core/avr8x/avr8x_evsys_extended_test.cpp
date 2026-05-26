#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/evsys.hpp"
#include <vector>

using namespace vioavr::core;

static constexpr u16 STROBE = 0x180;
static constexpr u16 CH0    = 0x190;
static constexpr u16 CH1    = 0x191;
static constexpr u16 CH2    = 0x192;
static constexpr u16 CH3    = 0x193;
static constexpr u16 CH4    = 0x194;
static constexpr u16 CH5    = 0x195;
static constexpr u16 USER0  = 0x1A0;
static constexpr u16 USER1  = 0x1A1;
static constexpr u16 USER2  = 0x1A2;
static constexpr u16 USER3  = 0x1A3;
static constexpr u16 USER4  = 0x1A4;
static constexpr u16 USER5  = 0x1A5;
static constexpr u16 USER6  = 0x1A6;
static constexpr u16 USER7  = 0x1A7;

static EventSystem& get_evsys(Machine& m) {
    return *m.peripherals_of_type<EventSystem>()[0];
}

TEST_CASE("EVSYS — Reset defaults") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();

    CHECK(bus.read_data(STROBE) == 0x00);
    CHECK(bus.read_data(CH0) == 0x00);
    CHECK(bus.read_data(CH1) == 0x00);
    CHECK(bus.read_data(CH2) == 0x00);
    CHECK(bus.read_data(CH3) == 0x00);
    CHECK(bus.read_data(CH4) == 0x00);
    CHECK(bus.read_data(CH5) == 0x00);
    CHECK(bus.read_data(USER0) == 0x00);
    CHECK(bus.read_data(USER1) == 0x00);
    CHECK(bus.read_data(USER2) == 0x00);
    CHECK(bus.read_data(USER3) == 0x00);
    CHECK(bus.read_data(USER4) == 0x00);
    CHECK(bus.read_data(USER5) == 0x00);
    CHECK(bus.read_data(USER6) == 0x00);
    CHECK(bus.read_data(USER7) == 0x00);
}

TEST_CASE("EVSYS — Register round-trips CH0-CH5") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();

    bus.write_data(CH0, 0x12);
    CHECK(bus.read_data(CH0) == 0x12);

    bus.write_data(CH1, 0x34);
    CHECK(bus.read_data(CH1) == 0x34);

    bus.write_data(CH2, 0x56);
    CHECK(bus.read_data(CH2) == 0x56);

    bus.write_data(CH3, 0x78);
    CHECK(bus.read_data(CH3) == 0x78);

    bus.write_data(CH4, 0x9A);
    CHECK(bus.read_data(CH4) == 0x9A);

    bus.write_data(CH5, 0xBC);
    CHECK(bus.read_data(CH5) == 0xBC);
}

TEST_CASE("EVSYS — Channel routing write preserves source value") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();
    auto& evsys = get_evsys(machine);

    bus.write_data(CH0, 0x05);
    CHECK(bus.read_data(CH0) == 0x05);

    evsys.trigger_event(0x05);
    CHECK(evsys.get_channel_level(0));
}

TEST_CASE("EVSYS — Multiple channel writes preserve values") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();

    bus.write_data(CH0, 0x12);
    bus.write_data(CH1, 0x34);
    bus.write_data(CH2, 0x56);

    CHECK(bus.read_data(CH0) == 0x12);
    CHECK(bus.read_data(CH1) == 0x34);
    CHECK(bus.read_data(CH2) == 0x56);
}

TEST_CASE("EVSYS — User routing write and readback USER0-USER7") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();

    bus.write_data(USER0, 0x01);
    bus.write_data(USER1, 0x02);
    bus.write_data(USER2, 0x03);
    bus.write_data(USER3, 0x04);
    bus.write_data(USER4, 0x05);
    bus.write_data(USER5, 0x06);
    bus.write_data(USER6, 0x07);
    bus.write_data(USER7, 0x08);

    CHECK(bus.read_data(USER0) == 0x01);
    CHECK(bus.read_data(USER1) == 0x02);
    CHECK(bus.read_data(USER2) == 0x03);
    CHECK(bus.read_data(USER3) == 0x04);
    CHECK(bus.read_data(USER4) == 0x05);
    CHECK(bus.read_data(USER5) == 0x06);
    CHECK(bus.read_data(USER6) == 0x07);
    CHECK(bus.read_data(USER7) == 0x08);
}

TEST_CASE("EVSYS — STROBE register round-trip") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();

    bus.write_data(STROBE, 0xA5);
    CHECK(bus.read_data(STROBE) == 0xA5);

    bus.write_data(STROBE, 0x5A);
    CHECK(bus.read_data(STROBE) == 0x5A);
}

TEST_CASE("EVSYS — trigger_event invokes registered user callback") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();
    auto& evsys = get_evsys(machine);

    bus.write_data(CH0, 0x07);
    bus.write_data(USER0, 0x01);

    int count = 0;
    evsys.register_user_callback(0, [&](bool) { count++; });

    evsys.trigger_event(0x07);
    CHECK(count == 1);
}

TEST_CASE("EVSYS — User callback receives level=true then level=false on trigger_event") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();
    auto& evsys = get_evsys(machine);

    bus.write_data(CH0, 0x07);
    bus.write_data(USER0, 0x01);

    bool first_level = false;
    int call_count = 0;
    evsys.register_user_callback(0, [&](bool level) {
        if (call_count == 0) first_level = level;
        call_count++;
    });

    evsys.trigger_event(0x07);
    CHECK(first_level == true);
}

TEST_CASE("EVSYS — Multiple events to different users") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();
    auto& evsys = get_evsys(machine);

    bus.write_data(CH0, 0x07);
    bus.write_data(CH1, 0x08);
    bus.write_data(USER0, 0x01);
    bus.write_data(USER1, 0x02);

    int count0 = 0;
    int count1 = 0;
    evsys.register_user_callback(0, [&](bool) { count0++; });
    evsys.register_user_callback(1, [&](bool) { count1++; });

    evsys.trigger_event(0x07);
    CHECK(count0 == 1);
    CHECK(count1 == 0);

    evsys.trigger_event(0x08);
    CHECK(count0 == 1);
    CHECK(count1 == 1);
}

TEST_CASE("EVSYS — Same event to multiple users") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();
    auto& evsys = get_evsys(machine);

    bus.write_data(CH0, 0x07);
    bus.write_data(USER0, 0x01);
    bus.write_data(USER1, 0x01);

    int count0 = 0;
    int count1 = 0;
    evsys.register_user_callback(0, [&](bool) { count0++; });
    evsys.register_user_callback(1, [&](bool) { count1++; });

    evsys.trigger_event(0x07);
    CHECK(count0 == 1);
    CHECK(count1 == 1);
}

TEST_CASE("EVSYS — STROBE write triggers connected user callbacks") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();
    auto& evsys = get_evsys(machine);

    bus.write_data(CH0, 0x01);
    bus.write_data(USER0, 0x01);

    int call_count = 0;
    evsys.register_user_callback(0, [&](bool) { call_count++; });

    bus.write_data(STROBE, 0x01);
    CHECK(call_count == 2);
}

TEST_CASE("EVSYS — Clear channel disables it") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();
    auto& evsys = get_evsys(machine);

    bus.write_data(CH0, 0x07);
    bus.write_data(USER0, 0x01);

    int count = 0;
    evsys.register_user_callback(0, [&](bool) { count++; });

    bus.write_data(CH0, 0x00);

    evsys.trigger_event(0x07);
    CHECK(count == 0);
}

TEST_CASE("EVSYS — Unmapped address returns 0") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();

    CHECK(bus.read_data(0x1C0) == 0x00);
    CHECK(bus.read_data(0x1FF) == 0x00);
    CHECK(bus.read_data(0x000) == 0x00);
    CHECK(bus.read_data(0x17F) == 0x00);
}

TEST_CASE("EVSYS — trigger_event with generator_id 0 does nothing") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();
    auto& evsys = get_evsys(machine);

    bus.write_data(CH0, 0x07);
    bus.write_data(USER0, 0x01);

    int count = 0;
    evsys.register_user_callback(0, [&](bool) { count++; });

    evsys.trigger_event(0x00);
    CHECK(count == 0);
}

TEST_CASE("EVSYS — Multiple event triggers in sequence") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    std::vector<u16> loop(1000, 0xCFFF);
    machine.bus().load_flash(loop);
    machine.reset();
    auto& bus = machine.bus();
    auto& evsys = get_evsys(machine);

    bus.write_data(CH0, 0x07);
    bus.write_data(USER0, 0x01);

    int count = 0;
    evsys.register_user_callback(0, [&](bool) { count++; });

    evsys.trigger_event(0x07);
    evsys.trigger_event(0x07);
    evsys.trigger_event(0x07);
    CHECK(count == 3);
}
