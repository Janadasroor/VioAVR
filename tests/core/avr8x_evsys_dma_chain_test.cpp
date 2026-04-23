#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/ccl.hpp"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/dma.hpp"

using namespace vioavr::core;

TEST_CASE("Complex Chain: TCA0 -> EVSYS -> CCL -> EVSYS -> DMA") {
    auto desc = devices::atmega4809;
    
    // Setup DMA Descriptor (Mocking what we'd find in a chip with DMA)
    // Using 0x300 block to avoid conflicts with ATmega4809 peripherals
    DmaDescriptor dma_desc{};
    dma_desc.ctrla_address = 0x300;
    dma_desc.status_address = 0x301;
    dma_desc.channel_count = 1;
    dma_desc.channels[0].ctrla_address = 0x310;
    dma_desc.channels[0].srcaddr_address = 0x312;
    dma_desc.channels[0].dstaddr_address = 0x314;
    dma_desc.channels[0].cnt_address = 0x316;
    dma_desc.channels[0].trigsrc_address = 0x318;
    // ATmega4809 EVSYS users are at 0x1A0-0x1B7. We mock DMA0 at 0x1AA.
    dma_desc.channels[0].user_event_address = 0x1AA; 
    
    Machine machine(desc);
    
    // Add DMA manually
    auto dma_ptr = std::make_unique<Dma>(dma_desc);
    Dma* dma = dma_ptr.get();
    
    // Configure and attach
    dma->set_memory_bus(&machine.bus());
    dma->set_event_system(machine.event_system());
    machine.bus().attach_peripheral(*dma);
    machine.add_peripheral(std::move(dma_ptr));
    
    // Load a dummy program so CPU stays in RUNNING state
    u16 loop[] = { 0xCFFF }; // RJMP -1
    machine.bus().load_flash(loop);
    machine.reset();

    auto& bus = machine.bus();

    // 1. Setup Memory
    bus.write_data(0x3000, 0xAA);
    bus.write_data(0x3010, 0x00);

    // 2. Configure TCA0 (Generator 128)
    bus.write_data(0xA09, 0x01); // EVCTRL: OVF=1 (Enable event generation)
    bus.write_data(0xA26, 10);   // PER (low) = 10
    bus.write_data(0xA27, 0x00); // PER (high)
    bus.write_data(0xA20, 0x00); // CNT (low)
    bus.write_data(0xA21, 0x00); // CNT (high)
    bus.write_data(0xA00, 0x01); // CTRLA: ENABLE=1, CLKSEL=DIV1 (Start Timer)
    
    // 3. Configure EVSYS Channel 0: SourceId = 128 (TCA0_OVF)
    bus.write_data(0x190, 128); // CHANNEL0: 128 (0x80)

    // 4. Configure CCL LUT0: Input 0 = EVSYS CH0
    bus.write_data(0x1C0, 0x01); // CCL.CTRLA: ENABLE=1
    bus.write_data(0x1C9, 0x03); // LUT0CTRLB: INSEL0 = 3 (EVENTA)
    bus.write_data(0x1C8, 0x01); // LUT0CTRLA: ENABLE=1
    bus.write_data(0x1CB, 0xAA); // TRUTH0: Pass-through Input 0
    // USERCCLLUT0A is at 0x1A0. Connect LUT0A to CHANNEL0 (index 1)
    bus.write_data(0x1A0, 0x01); 

    // 5. Configure EVSYS Channel 1: SourceId = 16 (CCL_LUT0)
    bus.write_data(0x191, 16); // CHANNEL1: 16 (0x10)

    // 6. Configure DMA Channel 0: Trigger = EVSYS CH1 via User Slot
    // USERDMA0 (mocked at 0x1AA). Connect to CHANNEL1 (index 2)
    bus.write_data(0x1AA, 0x02); 
    
    // Configure DMA registers via bus
    bus.write_data(0x300, 0x01); // MCTRLA: ENABLE=1 (GLOBAL DMA ENABLE)
    bus.write_data(0x310, 0x01); // CH0.CTRLA: ENABLE=1 (CHANNEL ENABLE)
    bus.write_data(0x312, 0x00); // SRCADDR (low)
    bus.write_data(0x313, 0x30); // SRCADDR (high) -> 0x3000
    bus.write_data(0x314, 0x10); // DSTADDR (low)
    bus.write_data(0x315, 0x30); // DSTADDR (high) -> 0x3010
    bus.write_data(0x316, 0x01); // CNT (low)
    bus.write_data(0x317, 0x00); // CNT (high) -> 1 byte
    
    // 7. Tick and Verify
    for (int i = 0; i < 100; ++i) {
        machine.step();
        if (bus.read_data(0x3010) == 0xAA) break;
    }
    
    // 8. Assertions
    CHECK(bus.read_data(0x3010) == 0xAA);
}
