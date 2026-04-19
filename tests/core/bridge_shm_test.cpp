#include "vioavr/core/bridge_shm_server.hpp"
#include "vioavr/core/bridge_shm_client.hpp"
#include "vioavr/core/device_catalog.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>

using namespace vioavr::core;

int main() {
    std::cout << "Starting SHM Bridge Functional Test..." << std::endl;

    auto device = DeviceCatalog::find("ATmega328P");
    if (!device) {
        std::cerr << "Device not found" << std::endl;
        return 1;
    }

    // 1. Start Server in a separate thread
    BridgeShmServer server(*device, "test_instance");
    std::thread server_thread([&]() {
        server.run_loop();
    });

    // Wait for server to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 2. Connect Client
    BridgeShmClient client("test_instance");
    if (!client.connect()) {
        std::cerr << "Client failed to connect" << std::endl;
        return 1;
    }
    std::cout << "1. Client connected to bridge." << std::endl;

    // 3. Simple Test: Set Input and Request Step
    client.set_digital_input(0, true);
    client.set_frequency(16000000.0);
    
    // Request 1000 cycles
    std::cout << "2. Requesting 1000 cycles..." << std::endl;
    client.step_cycles(1000);

    // 4. Verify CPU state was updated
    auto state = client.get_cpu_state();
    std::cout << "3. CPU State - PC: " << state.pc << ", SP: " << state.sp << std::endl;

    // 5. Test Analog to Digital Mapping (Schmitt Trigger)
    client.set_thresholds(1, 4.0f, 1.0f); // Pin 1: VIH=4V, VIL=1V
    client.set_analog_input(1, 4.5f); // High
    client.step_cycles(1);
    assert(client.test_get_digital_input(1) == true);
    
    client.set_analog_input(1, 0.5f); // Low
    client.step_cycles(1);
    assert(client.test_get_digital_input(1) == false);
    std::cout << "4. Threshold logic verified." << std::endl;

    // 6. Shutdown
    std::cout << "5. Shutting down..." << std::endl;
    client.set_status(BridgeStatus::Quit);

    if (server_thread.joinable()) server_thread.join();

    std::cout << "SHM BRIDGE TEST PASSED!" << std::endl;
    return 0;
}
 
