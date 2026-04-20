#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

TEST_CASE("UART0 Interrupt Flag and Priority Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328p};
    PinMux mux{1}; Uart uart0 {"USART0", atmega328p.uarts[0], mux};
    bus.attach_peripheral(uart0);
    bus.reset();

    const auto ucsra = atmega328p.uarts[0].ucsra_address;
    const auto ucsrb = atmega328p.uarts[0].ucsrb_address;
    const auto udr = atmega328p.uarts[0].udr_address;
    const auto rx_vec = atmega328p.uarts[0].rx_vector_index;
    const auto udre_vec = atmega328p.uarts[0].udre_vector_index;
    const auto tx_vec = atmega328p.uarts[0].tx_vector_index;

    SUBCASE("Interrupt Priority: RX > UDRE > TX") {
        bus.write_data(ucsrb, 0xF8U); // Enable RXCIE, TXCIE, UDRIE, RXEN, TXEN

        InterruptRequest request {};
        
        // UDRE should be pending immediately since UDR is empty
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == udre_vec);

        // Inject byte to trigger RXC
        uart0.inject_received_byte(0x33U);
        
        // RXC should now override UDRE (higher priority, lower vector index)
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == rx_vec);

        // Consume RXC
        CHECK(bus.consume_interrupt_request(request));
        CHECK(request.vector_index == rx_vec);
        // Hardware accuracy: RXC is cleared by reading UDR
        bus.read_data(udr);
        CHECK((bus.read_data(ucsra) & 0x80U) == 0U);

        // Now UDRE should be pending again
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == udre_vec);

        // Start transmission to clear UDRE
        bus.write_data(udr, 0x7EU);
        CHECK((bus.read_data(ucsra) & 0x20U) == 0U); // UDRE cleared
        
        // Advance to complete transmission
        bus.tick_peripherals(161U); 
        
        // Now UDRE should be set again, and TXC should also be set.
        // UDRE has higher priority than TXC.
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == udre_vec);

        // Consume UDRE
        CHECK(bus.consume_interrupt_request(request));
        CHECK(request.vector_index == udre_vec);
        // Hardware accuracy: UDRE is cleared by writing UDR or disabling UDRIE
        bus.write_data(ucsrb, 0xD8U); // Disable UDRIE (clear bit 5), keep RXEN/TXEN/RXCIE/TXCIE
        CHECK((bus.read_data(ucsra) & 0x20U) != 0U); // UDRE flag itself stays set (it's empty)
        // Now it shouldn't be pending anymore because UDRIE is 0
        {
            InterruptRequest dummy;
            // But wait, TXC might be pending!
        }

        // Now TXC should be pending
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == tx_vec);

        // Consume TXC
        CHECK(bus.consume_interrupt_request(request));
        CHECK(request.vector_index == tx_vec);
        CHECK((bus.read_data(ucsra) & 0x40U) == 0U);
    }
}
