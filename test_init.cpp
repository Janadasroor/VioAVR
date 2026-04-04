#include <iostream>
#include <string_view>
#include <array>
#include <cstdint>
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
struct AddressRange { u16 start, end; };
struct AdcDescriptor { u16 adcl; };
struct PortDescriptor { std::string_view name; };
struct DeviceDescriptor {
    std::string_view name {};
    u32 flash_words {};
    u16 sram_bytes {};
    u16 eeprom_bytes {};
    u8 interrupt_vector_count {};
    u8 interrupt_vector_size {2U};
    AddressRange register_file_range {0x0000, 0x001F};
    AddressRange io_range {0x0020, 0x005F};
    AddressRange extended_io_range {0x0060, 0x00FF};
    u16 spl_address {0x005DU};
    u16 sph_address {0x005EU};
    u16 sreg_address {0x005FU};
    u64 cpu_frequency_hz {16'000'000U};
    AdcDescriptor adc {};
    std::array<PortDescriptor, 8> ports {};
};

int main() {
    DeviceDescriptor d {
        .name = "ATmega8",
        .flash_words = 4096U, .sram_bytes = 1024U, .eeprom_bytes = 512U,
        .interrupt_vector_count = 19U, .interrupt_vector_size = 2U,
        .adc = { .adcl = 0x24U },
        .ports = {{ { "PORTB" } }}
    };
    std::cout << "name: " << d.name << "\n";
    std::cout << "flash: " << d.flash_words << "\n";
    std::cout << "size: " << (int)d.interrupt_vector_size << "\n";
    std::cout << "reg_start: " << d.register_file_range.start << "\n";
    std::cout << "io_start: " << d.io_range.start << "\n";
    std::cout << "hz: " << d.cpu_frequency_hz << "\n";
}
