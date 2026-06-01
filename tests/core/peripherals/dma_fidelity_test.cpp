#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/dma.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/ebi.hpp"
#include <vector>

using namespace vioavr::core;

TEST_CASE("DMA: Single Byte Transfer Timing") {
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

    std::vector<u16> nops(1024, 0x0000);
    machine.bus().load_flash(nops);
    machine.reset();

    machine.bus().write_data(0x2800, 0x42);

    dma->write(0x112, 0x00); dma->write(0x113, 0x28);
    dma->write(0x114, 0x00); dma->write(0x115, 0x29);
    dma->write(0x116, 1);    dma->write(0x117, 0);
    dma->write(0x111, 0x02); // SINGLE (1 beat/trigger)
    dma->write(0x118, 1);
    dma->write(0x110, 0x51); // ENABLE | SRCINC=01 | DSTINC=01
    dma->write(0x100, 0x01);

    machine.bus().tick_peripherals(100, 0xFF);
    CHECK(machine.bus().read_data(0x2900) == 0x00);

    evsys->trigger_event(1, true);
    machine.bus().tick_peripherals(100, 0xFF);
    CHECK(machine.bus().read_data(0x2900) == 0x42);
}

TEST_CASE("DMA: Multi-Byte Burst") {
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

    std::vector<u16> nops(1024, 0x0000);
    machine.bus().load_flash(nops);
    machine.reset();

    for (u16 i = 0; i < 4; ++i)
        machine.bus().write_data(0x2800 + i, static_cast<u8>(0xA0 + i));

    dma->write(0x112, 0x00); dma->write(0x113, 0x28);
    dma->write(0x114, 0x00); dma->write(0x115, 0x29);
    dma->write(0x116, 4);    dma->write(0x117, 0);
    dma->write(0x111, 0x00); // TRIGACT=0: block xfer (entire block per trigger)
    dma->write(0x118, 1);
    dma->write(0x110, 0x51); // ENABLE | SRCINC=01 | DSTINC=01
    dma->write(0x100, 0x01);

    evsys->trigger_event(1, true);
    machine.bus().tick_peripherals(20, 0xFF);

    for (u16 i = 0; i < 4; ++i)
        CHECK(machine.bus().read_data(0x2900 + i) == static_cast<u8>(0xA0 + i));
}

TEST_CASE("DMA: Single Byte Transfer Event Triggered") {
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

    std::vector<u16> nops(1024, 0x0000);
    machine.bus().load_flash(nops);
    machine.reset();

    machine.bus().write_data(0x2800, 0xDE);
    machine.bus().write_data(0x2801, 0xAD);

    dma->write(0x112, 0x00); dma->write(0x113, 0x28);
    dma->write(0x114, 0x10); dma->write(0x115, 0x29);
    dma->write(0x116, 2);    dma->write(0x117, 0);
    dma->write(0x111, 0x02); // TRIGACT=0x02: SINGLE (1 byte per trigger)
    dma->write(0x118, 1);
    dma->write(0x110, 0x51); // ENABLE | SRCINC=01 | DSTINC=01
    dma->write(0x100, 0x01);

    evsys->trigger_event(1, true);
    machine.run(100);

    CHECK(machine.bus().read_data(0x2910) == 0xDE);
    CHECK(machine.bus().read_data(0x2911) == 0x00);

    evsys->trigger_event(1, true);
    machine.run(100);

    CHECK(machine.bus().read_data(0x2910) == 0xDE);
    CHECK(machine.bus().read_data(0x2911) == 0xAD);
}

TEST_CASE("DMA: EBI wait state timing") {
    EbiDescriptor ebi_desc;
    ebi_desc.ctrl_address = 0x300;
    ebi_desc.sdramctrla_address = 0x301;
    ebi_desc.refresh_address = 0x302;
    ebi_desc.initdly_address = 0x303;
    ebi_desc.sdramctrlb_address = 0x304;
    ebi_desc.sdramctrlc_address = 0x305;
    ebi_desc.cs0_ctrla_address = 0x310;
    ebi_desc.cs0_ctrlb_address = 0x311;
    ebi_desc.cs0_baseaddr_address = 0x312;

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
    device.name = "TestMCU_WS";
    device.flash_words = 1024;
    device.sram_start = 0x2800;
    device.sram_bytes = 4096;
    device.dma_count = 1;
    device.dmas[0] = dma_desc;
    device.evsys = evsys_desc;
    device.ebi_count = 1;
    device.ebis[0] = ebi_desc;

    Machine machine(device);
    auto dmas = machine.peripherals_of_type<Dma>();
    auto evsyss = machine.peripherals_of_type<EventSystem>();
    REQUIRE(dmas.size() == 1);
    REQUIRE(evsyss.size() == 1);
    auto* dma = dmas[0];
    auto* evsys = evsyss[0];

    std::vector<u16> nops(1024, 0x0000);
    machine.bus().load_flash(nops);
    machine.reset();

    // Setup EBI: CS0 at 0x8000 with 3 wait states, 64KB SRAM mode
    // EBI CTRL bit 0 = enable, CS0_CTRLA bits 2:0 = 4 (64KB SRAM)
    machine.bus().write_data(0x300, 0x01);       // EBI enable
    machine.bus().write_data(0x310, 0x04);       // CS0_CTRLA: 64KB
    machine.bus().write_data(0x312, 0x00);       // CS0_BASEADDR L
    machine.bus().write_data(0x313, 0x80);       // CS0_BASEADDR H (0x8000)
    machine.bus().write_data(0x311, 0x03);       // CS0_CTRLB: 3 wait states

    // Verify EBI is mapped via bus
    CHECK(machine.bus().get_wait_states(0x8000) == 3);
    CHECK(machine.bus().get_wait_states(0x8FFF) == 3);

    // Source data in internal SRAM
    machine.bus().write_data(0x2800, 0x11);
    machine.bus().write_data(0x2801, 0x22);

    // DMA: src=SRAM(0 ws), dst=EBI(3 ws), burstlen=1, TRIGACT=0 (block)
    dma->write(0x112, 0x00); dma->write(0x113, 0x28);
    dma->write(0x114, 0x00); dma->write(0x115, 0x80);
    dma->write(0x116, 2);    dma->write(0x117, 0);
    dma->write(0x111, 0x00); // TRIGACT=0: block
    dma->write(0x118, 1);
    dma->write(0x110, 0x51); // ENABLE | SRCINC=01 | DSTINC=01
    dma->write(0x100, 0x01);

    // Trigger and give enough cycles for 2 beats × (1+0 + 1+3) = 10 cycles
    evsys->trigger_event(1, true);
    machine.bus().tick_peripherals(10, 0xFF);

    CHECK(machine.bus().read_data(0x8000) == 0x11);
    CHECK(machine.bus().read_data(0x8001) == 0x22);
}
