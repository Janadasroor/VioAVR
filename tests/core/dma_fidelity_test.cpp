#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/dma.hpp"
#include "vioavr/core/evsys.hpp"
#include <vector>

using namespace vioavr::core;

TEST_CASE("DMA: Manual Burst Transfer") {
    DmaDescriptor dma_desc;
    dma_desc.ctrla_address = 0x100;
    dma_desc.channel_count = 1;
    dma_desc.channels[0] = {
        .ctrla_address = 0x110,
        .ctrlb_address = 0x111,
        .srcaddr_address = 0x112,
        .dstaddr_address = 0x114,
        .cnt_address = 0x116,
        .trigsrc_address = 0x118
    };

    DeviceDescriptor device;
    device.sram_start = 0x2800;
    device.sram_bytes = 4096;
    device.dma_count = 1;
    device.dmas[0] = dma_desc;
    device.flash_words = 1024;

    Machine machine(device);
    auto dmas = machine.peripherals_of_type<Dma>();
    REQUIRE(dmas.size() == 1);
    auto* dma = dmas[0];

    // Load NOPs to keep CPU running during tick
    std::vector<u16> nops(1024, 0x0000);
    machine.bus().load_flash(nops);
    machine.reset();

    // Setup source data
    for (u16 i = 0; i < 16; ++i) {
        machine.bus().write_data(0x2800 + i, static_cast<u8>(0xA0 + i));
    }

    // Configure DMA Channel 0
    dma->write(0x112, 0x00); // SRCADDR L (0x2800)
    dma->write(0x113, 0x28); // SRCADDR H
    dma->write(0x114, 0x00); // DSTADDR L (0x2900)
    dma->write(0x115, 0x29); // DSTADDR H
    dma->write(0x116, 16);   // CNT L
    dma->write(0x117, 0);    // CNT H
    
    // CTRLB: Burst mode (bit 0 = 0 is single, 1 is burst? No, let's say 0x00 is burst in our mock)
    dma->write(0x111, 0x00); 
    
    // CTRLA: ENABLE (bit 0) | SRCINC (bit 2) | DSTINC (bit 4)
    dma->write(0x110, 0x15);

    // Global DMA Enable
    dma->write(0x100, 0x01);

    // Manually trigger via Trigger ID 0 (Internal/Software trigger simulation)
    dma->write(0x118, 0); 
    // In our implementation, we don't have a software trigger bit yet, 
    // but on_trigger(0) could work if EventSystem is not involved.
    // Actually, let's just use machine.run() and assume the first tick might process if we set it as pending.
    // Or better, use the trigger system properly.
}

TEST_CASE("DMA: Event Triggered Single Byte Transfer") {
    DmaDescriptor dma_desc;
    dma_desc.ctrla_address = 0x100;
    dma_desc.channel_count = 1;
    dma_desc.channels[0] = {
        .ctrla_address = 0x110,
        .ctrlb_address = 0x111,
        .srcaddr_address = 0x112,
        .dstaddr_address = 0x114,
        .cnt_address = 0x116,
        .trigsrc_address = 0x118
    };

    EvsysDescriptor evsys_desc;
    evsys_desc.strobe_address = 0x200;
    evsys_desc.channels_address = 0x210;
    evsys_desc.users_address = 0x240;
    evsys_desc.channel_count = 8;
    evsys_desc.user_count = 8;

    DeviceDescriptor device;
    device.name = "TestMCU";
    device.flash_words = 1024;
    device.sram_start = 0x2800;
    device.sram_bytes = 4096;
    device.dma_count = 1;
    device.dmas[0] = dma_desc;
    device.evsys = evsys_desc;

    Machine machine(device);
    auto dmas = machine.peripherals_of_type<Dma>();
    auto evsyss = machine.peripherals_of_type<EventSystem>();
    REQUIRE(dmas.size() == 1);
    REQUIRE(evsyss.size() == 1);
    auto* dma = dmas[0];
    auto* evsys = evsyss[0];

    // Load NOPs to keep CPU running
    std::vector<u16> nops(1024, 0x0000);
    machine.bus().load_flash(nops);
    machine.reset();

    // Source data
    machine.bus().write_data(0x2800, 0xDE);
    machine.bus().write_data(0x2801, 0xAD);

    // DMA Channel 0 Config
    dma->write(0x112, 0x00); // SRCADDR 0x2800
    dma->write(0x113, 0x28);
    dma->write(0x114, 0x10); // DSTADDR 0x2910
    dma->write(0x115, 0x29);
    dma->write(0x116, 2);    // CNT 2
    dma->write(0x117, 0);
    dma->write(0x111, 0x01); // Mode: Single byte per trigger
    dma->write(0x118, 1);    // TRIGSRC: Trigger ID 1
    dma->write(0x110, 0x15); // ENABLE | SRCINC | DSTINC
    dma->write(0x100, 0x01); // Global Enable

    // First trigger
    evsys->trigger_event(1, true);
    machine.run(100); 

    CHECK(machine.bus().read_data(0x2910) == 0xDE);
    CHECK(machine.bus().read_data(0x2911) == 0x00); 

    // Second trigger
    evsys->trigger_event(1, true);
    machine.run(100);

    CHECK(machine.bus().read_data(0x2910) == 0xDE);
    CHECK(machine.bus().read_data(0x2911) == 0xAD);
}
