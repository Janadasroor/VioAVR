#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/bridge_shm_server.hpp"
#include "vioavr/core/bridge_shm_client.hpp"
#include "vioavr/core/device_catalog.hpp"
#include <thread>
#include <chrono>

using namespace vioavr::core;

TEST_CASE("ATtiny85 Device Descriptor Verification") {
    auto* device = DeviceCatalog::find("ATtiny85");
    REQUIRE(device != nullptr);
    CHECK(device->name == "ATtiny85");
    CHECK(device->flash_words == 4096U);
    CHECK(device->sram_bytes == 512U);
    CHECK(device->sram_start == 0x60U);
    CHECK(device->port_count == 1U);
    CHECK(device->ports[0].name == "PORTB");
    CHECK(device->ports[0].pin_address == 0x36U);
    CHECK(device->ports[0].ddr_address == 0x37U);
    CHECK(device->ports[0].port_address == 0x38U);
}

TEST_CASE("ATtiny85 SHM Bridge Connection and Functional Testing") {
    auto* device = DeviceCatalog::find("ATtiny85");
    REQUIRE(device != nullptr);

    // 1. Start Server in a separate thread
    BridgeShmServer server(*device, "attiny_test_instance");
    std::thread server_thread([&]() {
        server.run_loop();
    });

    // Wait for server to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 2. Connect Client
    BridgeShmClient client("attiny_test_instance");
    REQUIRE(client.connect() == true);

    // 3. Set Input and Request Step
    client.set_digital_input(0, true);
    client.set_frequency(16000000.0);
    
    // Request cycles
    client.step_cycles(100);

    // 4. Verify CPU state was updated
    auto state = client.get_cpu_state();
    CHECK(state.pc == 0);

    // 5. Test Analog to Digital Threshold Mapping (Schmitt Trigger)
    client.set_thresholds(1, 4.0f, 1.0f); // Pin 1: VIH=4V, VIL=1V
    
    client.set_analog_input(1, 4.5f); // High
    client.step_cycles(1);
    CHECK(client.test_get_digital_input(1) == true);
    
    client.set_analog_input(1, 0.5f); // Low
    client.step_cycles(1);
    CHECK(client.test_get_digital_input(1) == false);

    // 6. Shutdown
    client.set_status(BridgeStatus::Quit);

    if (server_thread.joinable()) {
        server_thread.join();
    }
}
