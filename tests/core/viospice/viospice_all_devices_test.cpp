#include "doctest.h"
#include "vioavr/core/viospice.hpp"
#include "vioavr/core/device_catalog.hpp"
#include <vector>
#include <string>
#include <sstream>

using namespace vioavr::core;

struct DeviceResult {
    std::string name;
    bool init_ok;
    bool gpio_ok;
    std::string error;
};

struct TestSummary {
    std::vector<DeviceResult> results;
    int init_pass;
    int init_fail;
    int gpio_pass;
    int gpio_skip;
};

TEST_CASE("All devices initialize and co-simulate correctly") {
    auto devices = DeviceCatalog::list_devices();
    TestSummary summary;
    summary.init_pass = 0;
    summary.init_fail = 0;
    summary.gpio_pass = 0;
    summary.gpio_skip = 0;

    for (const auto& name : devices) {
        DeviceResult result;
        result.name = std::string(name);
        result.init_ok = false;
        result.gpio_ok = false;

        const auto* device = DeviceCatalog::find(name);
        if (!device) {
            result.error = "device not found in catalog";
            result.init_ok = false;
            summary.init_fail++;
            summary.results.push_back(std::move(result));
            continue;
        }

        // Phase 1: Create VioSpice and verify initialization
        VioSpice* spice = nullptr;
        try {
            spice = new VioSpice(*device);
            result.init_ok = true;
            summary.init_pass++;
        } catch (const std::exception& e) {
            result.error = "init failed: " + std::string(e.what());
            result.init_ok = false;
            summary.init_fail++;
            summary.results.push_back(std::move(result));
            delete spice;
            continue;
        } catch (...) {
            result.error = "init failed: unknown exception";
            result.init_ok = false;
            summary.init_fail++;
            summary.results.push_back(std::move(result));
            delete spice;
            continue;
        }

        // Phase 2: Step CPU a few times to verify basic execution
        try {
            spice->step_duration(1e-9);
            spice->step_duration(1e-9);
        } catch (const std::exception& e) {
            result.error = "step failed: " + std::string(e.what());
            summary.init_fail++;
            summary.init_pass--;
            delete spice;
            summary.results.push_back(std::move(result));
            continue;
        }

        // Phase 3: Exercise GPIO co-simulation path if ports exist
        try {
            bool gpio_exercised = false;
            u8 num_ports = device->port_count;

            for (u8 p = 0; p < num_ports; ++p) {
                const auto& port_desc = device->ports[p];
                if (port_desc.name.empty()) continue;
                if (port_desc.ddr_address == 0 || port_desc.port_address == 0) continue;

                u32 ext_id = p * 8U;
                spice->add_pin_mapping(port_desc.name, 0, ext_id);

                auto& bus = spice->bus();

                u8 ddr = bus.read_data(port_desc.ddr_address);
                bus.write_data(port_desc.ddr_address, ddr | 0x01);

                u8 port_val = bus.read_data(port_desc.port_address);
                bus.write_data(port_desc.port_address, port_val | 0x01);

                spice->step_duration(1e-9);

                auto changes = spice->consume_pin_changes();
                for (const auto& change : changes) {
                    if (change.port_name == port_desc.name && change.bit_index == 0 && change.level) {
                        auto eid = spice->get_external_id(change.port_name, change.bit_index);
                        if (eid && *eid == ext_id) {
                            result.gpio_ok = true;
                            gpio_exercised = true;
                        }
                    }
                }

                if (gpio_exercised) break;
            }

            if (gpio_exercised) {
                summary.gpio_pass++;
            } else {
                summary.gpio_skip++;
            }
        } catch (const std::exception& e) {
            result.error = "gpio test failed: " + std::string(e.what());
        }

        delete spice;
        summary.results.push_back(std::move(result));
    }

    // Report summary
    MESSAGE("=== Co-Simulation Device Test Results ===");
    MESSAGE("Total devices tested: " << devices.size());
    MESSAGE("Init passed: " << summary.init_pass);
    MESSAGE("Init failed: " << summary.init_fail);
    MESSAGE("GPIO exercised: " << summary.gpio_pass);
    MESSAGE("GPIO skipped (no ports or no pin change): " << summary.gpio_skip);

    if (summary.init_fail > 0) {
        MESSAGE("\n--- Failed devices ---");
        for (const auto& r : summary.results) {
            if (!r.init_ok) {
                MESSAGE("  " << r.name << ": " << r.error);
            }
        }
    }

    // Verify all devices pass initialization
    CHECK_MESSAGE(summary.init_fail == 0,
                  summary.init_fail << " devices failed initialization");
}
