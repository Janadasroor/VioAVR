#include "vioavr/core/register_file.hpp"
#include "vioavr/core/avr_cpu.hpp"

namespace vioavr::core {

RegisterFile::RegisterFile(AvrCpu& cpu, AddressRange range) noexcept 
    : cpu_(cpu), 
      ranges_({range}) 
{}

std::string_view RegisterFile::name() const noexcept
{
    return "Register File";
}

std::span<const AddressRange> RegisterFile::mapped_ranges() const noexcept
{
    return ranges_;
}

void RegisterFile::reset() noexcept
{
}

void RegisterFile::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
}

u8 RegisterFile::read(const u16 address) noexcept
{
    auto range = ranges_[0];
    if (address >= range.begin && address <= range.end) {
        return cpu_.registers()[address - range.begin];
    }
    return 0U;
}

void RegisterFile::write(const u16 address, const u8 value) noexcept
{
    auto range = ranges_[0];
    if (address >= range.begin && address <= range.end) {
        cpu_.write_register(static_cast<u8>(address - range.begin), value);
    }
}

} // namespace vioavr::core
