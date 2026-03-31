#include "vioavr/core/register_file.hpp"
#include "vioavr/core/avr_cpu.hpp"

namespace vioavr::core {

RegisterFile::RegisterFile(AvrCpu& cpu) noexcept 
    : cpu_(cpu), 
      ranges_({AddressRange{0x0000, 0x001F}}) 
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
    // Reset handled by AvrCpu
}

void RegisterFile::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
}

u8 RegisterFile::read(const u16 address) noexcept
{
    if (address < kRegisterFileSize) {
        return cpu_.registers()[address];
    }
    return 0U;
}

void RegisterFile::write(const u16 address, const u8 value) noexcept
{
    if (address < kRegisterFileSize) {
        cpu_.write_register(static_cast<u8>(address), value);
    }
}

}  // namespace vioavr::core
