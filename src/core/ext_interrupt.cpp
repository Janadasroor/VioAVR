#include <string>
#include <algorithm>
#include <vector>
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/logger.hpp"

namespace vioavr::core {

namespace {
constexpr u8 kInt0Mask = 0x01U;
}

ExtInterrupt::ExtInterrupt(std::string_view name,
                           const ExtInterruptDescriptor& descriptor,
                           PinMux& pin_mux,
                           u8 source_id) noexcept
    : name_(name),
      desc_(descriptor),
      pin_mux_(&pin_mux),
      source_id_(source_id)
{
    std::vector<u16> addrs = {desc_.eicra_address, desc_.eimsk_address, desc_.eifr_address};
    std::sort(addrs.begin(), addrs.end());
    
    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < addrs.size(); ++i) {
        if (addrs[i] == 0) continue;
        if (ri > 0 && addrs[i] == ranges_[ri-1].end + 1) {
             ranges_[ri-1].end = addrs[i];
        } else {
             ranges_[ri++] = AddressRange{addrs[i], addrs[i]};
        }
    }
}

std::string_view ExtInterrupt::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> ExtInterrupt::mapped_ranges() const noexcept
{
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void ExtInterrupt::reset() noexcept
{
    eicra_ = 0U;
    eimsk_ = 0U;
    eifr_ = 0U;
    int0_level_ = true;
    int1_level_ = true;
    int0_pending_ = false;
    int1_pending_ = false;
}

void ExtInterrupt::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
    refresh_bound_input();
    if ((int0_sense_mode() == 0x00U) && !int0_level_) {
        raise_int0();
    }
}

u8 ExtInterrupt::read(const u16 address) noexcept
{
    refresh_bound_input();
    if (address == desc_.eicra_address) {
        return eicra_;
    }
    if (address == desc_.eimsk_address) {
        return eimsk_;
    }
    if (address == desc_.eifr_address) {
        return eifr_;
    }
    return 0U;
}

void ExtInterrupt::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.eicra_address) {
        eicra_ = value;
        return;
    }
    if (address == desc_.eimsk_address) {
        eimsk_ = value;
        return;
    }
    if (address == desc_.eifr_address) {
        eifr_ = static_cast<u8>(eifr_ & static_cast<u8>(~value));
        if ((value & kInt0Mask) != 0U) {
            int0_pending_ = false;
        }
    }
}

bool ExtInterrupt::on_external_pin_change(u8 bit_index, PinLevel level) noexcept
{
    Logger::debug("ExtInterrupt: external pin change bit=" + std::to_string(bit_index));
    // ATmega328P INT0 is on PD2
    if (bit_index == 2) {
        set_int0_level(level == PinLevel::high);
        return true;
    }
    return false;
}

bool ExtInterrupt::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    const_cast<ExtInterrupt*>(this)->refresh_bound_input();
    if (int0_pending_ && (eimsk_ & 0x01U) != 0U) {
        request = {.vector_index = desc_.vector_indices[0], .source_id = source_id_};
        return true;
    }
    if (int1_pending_ && (eimsk_ & 0x02U) != 0U) {
        request = {.vector_index = desc_.vector_indices[1], .source_id = source_id_};
        return true;
    }
    return false;
}

bool ExtInterrupt::consume_interrupt_request(InterruptRequest& request) noexcept
{
    refresh_bound_input();
    if (!pending_interrupt_request(request)) {
        return false;
    }

    if (request.vector_index == desc_.vector_indices[0]) {
        int0_pending_ = false;
        eifr_ &= static_cast<u8>(~0x01U);
        return true;
    }
    if (request.vector_index == desc_.vector_indices[1]) {
        int1_pending_ = false;
        eifr_ &= static_cast<u8>(~0x02U);
        return true;
    }
    return false;
}

void ExtInterrupt::bind_int0_signal(const AnalogSignalBank& signal_bank,
                                    const u8 channel,
                                    const DigitalThresholdConfig threshold) noexcept
{
    signal_bank_ = &signal_bank;
    signal_channel_ = channel;
    threshold_config_ = threshold;
    refresh_bound_input();
}

void ExtInterrupt::connect_adc_auto_trigger(Adc& adc) noexcept
{
    auto_trigger_adc_ = &adc;
}

void ExtInterrupt::set_int0_level(const bool high) noexcept
{
    const bool previous = int0_level_;
    int0_level_ = high;

    if (previous != high) {
        // Use a simpler log call to avoid string builder issues if it persists
        Logger::debug("INT0 level change detected");
    }

    switch (int0_sense_mode()) {
    case 0x00U:
        if (!high) {
            raise_int0();
        }
        break;
    case 0x01U:
        if (previous != high) {
            raise_int0();
        }
        break;
    case 0x02U:
        if (previous && !high) {
            raise_int0();
        }
        break;
    case 0x03U:
        if (!previous && high) {
            raise_int0();
        }
        break;
    default:
        break;
    }
}

void ExtInterrupt::set_int0_voltage(const double normalized_voltage) noexcept
{
    set_int0_level(apply_schmitt_threshold(int0_level_, normalized_voltage));
}

void ExtInterrupt::refresh_bound_input() noexcept
{
    if (signal_bank_ == nullptr) {
        return;
    }

    set_int0_level(apply_schmitt_threshold(int0_level_, signal_bank_->voltage(signal_channel_), threshold_config_));
}

u8 ExtInterrupt::int0_sense_mode() const noexcept
{
    return static_cast<u8>(eicra_ & 0x03U);
}

void ExtInterrupt::raise_int0() noexcept
{
    eifr_ |= kInt0Mask;
    int0_pending_ = true;
    if (auto_trigger_adc_ != nullptr) {
        auto_trigger_adc_->notify_auto_trigger(Adc::AutoTriggerSource::external_interrupt_0);
    }
}

}  // namespace vioavr::core
