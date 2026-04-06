#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/uart0.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/tracing.hpp"

#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>
#include <string>

namespace {

class StandardTraceHook final : public vioavr::core::ITraceHook {
public:
    void on_instruction(vioavr::core::u32 address, vioavr::core::u16 opcode, std::string_view mnemonic) override
    {
        std::cout << "[0x" << std::hex << std::right << std::setw(4) << std::setfill('0') << address << "] "
                  << std::left << std::setw(8) << std::setfill(' ') << mnemonic 
                  << " (0x" << std::hex << std::right << std::setw(4) << std::setfill('0') << opcode << ")" << std::endl;
    }

    void on_register_write(vioavr::core::u8 index, vioavr::core::u8 value) override
    {
        std::cout << "  R" << std::dec << static_cast<int>(index) << " = 0x" 
                  << std::hex << std::right << std::setw(2) << std::setfill('0') << static_cast<int>(value) << std::endl;
    }

    void on_sreg_write(vioavr::core::u8 value) override
    {
        std::cout << "  SREG = 0x" << std::hex << std::right << std::setw(2) << std::setfill('0') << static_cast<int>(value) << std::endl;
    }

    void on_memory_read(vioavr::core::u16 address, vioavr::core::u8 value) override
    {
        (void)address; (void)value;
    }

    void on_memory_write(vioavr::core::u16 address, vioavr::core::u8 value) override
    {
        std::cout << "  MEM[0x" << std::hex << std::setw(4) << std::setfill('0') << address << "] = 0x"
                  << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value) << std::endl;
    }

    void on_interrupt(vioavr::core::u8 vector) override
    {
        std::cout << "  INTERRUPT vector " << std::dec << static_cast<int>(vector) << std::endl;
    }
};

} // namespace

int main(int argc, char** argv)
{
    using namespace std::literals;
    using namespace vioavr::core;

    const auto* device = DeviceCatalog::find("ATmega328P");
    if (!device) {
        std::cerr << "Default device not found" << std::endl;
        return 1;
    }

    MemoryBus bus {*device};
    AvrCpu cpu {bus};
    
    // Pin Mux
    PinMux pin_mux {3};

    // Peripherals
    Eeprom eeprom {"EEPROM", bus.device()};
    Timer8 timer0 {"TIMER0", bus.device().timer0};
    Timer8 timer2 {"TIMER2", bus.device().timer2};
    Timer16 timer1 {"TIMER1", bus.device().timer1};
    Adc adc {"ADC", bus.device().adc, pin_mux, 0, 13};
    adc.set_bus(bus);
    Uart0 uart0 {"UART0", bus.device()};

    timer0.set_bus(bus);
    timer2.set_bus(bus);

    // Attach all to bus
    bus.attach_peripheral(eeprom);
    bus.attach_peripheral(timer0);
    bus.attach_peripheral(timer2);
    bus.attach_peripheral(timer1);
    bus.attach_peripheral(adc);
    bus.attach_peripheral(uart0);

    // GPIO Ports
    std::vector<std::unique_ptr<GpioPort>> gpio_ports;
    for (size_t i = 0; i < device->ports.size(); ++i) {
        const auto& p_desc = device->ports[i];
        if (p_desc.name.empty()) continue;
        
        auto port = std::make_unique<GpioPort>(p_desc.name, p_desc.pin_address, p_desc.ddr_address, p_desc.port_address);
        
        // Register port with pin mux using PORT address
        pin_mux.register_port(port->port_address(), static_cast<u8>(i));
        
        bus.attach_peripheral(*port);
        gpio_ports.push_back(std::move(port));
    }

    PinChangeInterruptSharedState pcint_shared {};
    auto get_port = [&](std::string_view name) -> GpioPort* {
        for (auto& p : gpio_ports) if (p->name() == name) return p.get();
        return nullptr;
    };

    auto* portb = get_port("PORTB");
    auto* portc = get_port("PORTC");
    auto* portd = get_port("PORTD");

    if (portb) {
        auto pci0 = std::make_unique<PinChangeInterrupt>("PCINT0", device->pin_change_interrupt_0, *portb, pcint_shared, true);
        bus.attach_peripheral(*pci0.release());
    }
    if (portc) {
        auto pci1 = std::make_unique<PinChangeInterrupt>("PCINT1", device->pin_change_interrupt_1, *portc, pcint_shared, false);
        bus.attach_peripheral(*pci1.release());
    }
    if (portd) {
        auto pci2 = std::make_unique<PinChangeInterrupt>("PCINT2", device->pin_change_interrupt_2, *portd, pcint_shared, false);
        bus.attach_peripheral(*pci2.release());
    }

    auto bind_timer8_outputs = [&](Timer8& timer, const Timer8Descriptor& desc) {
        auto resolve_port = [&](const u16 pin_address) -> GpioPort* {
            for (auto& p : gpio_ports) if (p->port_address() == pin_address || p->pin_address() == pin_address) return p.get();
            return nullptr;
        };

        if (auto* port = resolve_port(desc.ocra_pin_address); port != nullptr) {
            timer.connect_compare_output_a(*port, desc.ocra_pin_bit);
        }
        if (auto* port = resolve_port(desc.ocrb_pin_address); port != nullptr) {
            timer.connect_compare_output_b(*port, desc.ocrb_pin_bit);
        }
    };

    bind_timer8_outputs(timer0, device->timer0);
    bind_timer8_outputs(timer2, device->timer2);

    bool benchmark = false;
    bool trace = false;
    u64 max_cycles_arg = 0;
    std::string_view hex_path;
    std::string eeprom_path;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg {argv[i]};
        if (arg == "--benchmark") {
            benchmark = true;
        } else if (arg == "--trace") {
            trace = true;
        } else if (arg == "--max-cycles" && i + 1 < argc) {
            max_cycles_arg = std::stoull(std::string(argv[++i]));
        } else if (arg == "--eeprom-file" && i + 1 < argc) {
            eeprom_path = argv[++i];
        } else if (hex_path.empty() && arg.find("--") != 0) {
            hex_path = arg;
        }
    }

    if (!eeprom_path.empty()) {
        if (eeprom.load_from_file(eeprom_path)) {
            std::cout << "Loaded EEPROM from " << eeprom_path << std::endl;
        }
    }

    StandardTraceHook trace_hook;
    if (trace) {
        cpu.set_trace_hook(&trace_hook);
        bus.set_trace_hook(&trace_hook);
    }

    if (!hex_path.empty()) {
        try {
            const auto image = HexImageLoader::load_file(hex_path, bus.device());
            bus.load_image(image);
            cpu.reset();
        } catch (const std::exception& e) {
            std::cerr << "Error loading HEX: " << e.what() << std::endl;
            return 2;
        }
    } else if (benchmark) {
        // Load a simple tight loop for benchmarking if no file provided
        std::vector<u16> code(1024, 0x0000); // NOPs
        code.back() = 0xC000 | (static_cast<u16>(-1024) & 0x0FFF); // rjmp back to start
        bus.load_flash(code);
        cpu.reset();
    }

    if (benchmark) {
        const u64 kBenchmarkCycles = 100'000'000;
        std::cout << "Starting benchmark (" << kBenchmarkCycles << " cycles)..." << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        cpu.run(kBenchmarkCycles);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double seconds = duration.count() / 1'000'000.0;
        double mips = (cpu.cycles() / seconds) / 1'000'000.0;
        
        std::cout << "Benchmark finished." << std::endl;
        std::cout << "Time: " << seconds << "s" << std::endl;
        std::cout << "Throughput: " << mips << " MHz (Cycles per second)" << std::endl;
        return 0;
    }

    // Run until halted or a reasonably large number of cycles (for safety)
    u64 kMaxCycles = trace ? 1000 : 1'000'000;
    if (max_cycles_arg > 0) {
        kMaxCycles = max_cycles_arg;
    }

    while (cpu.state() == CpuState::running && cpu.cycles() < kMaxCycles) {
        cpu.step();
    }

    if (!eeprom_path.empty()) {
        if (eeprom.save_to_file(eeprom_path)) {
            std::cout << "Saved EEPROM to " << eeprom_path << std::endl;
        }
    }

    return 0;
}
