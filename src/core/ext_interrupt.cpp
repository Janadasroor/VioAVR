#include <string>
#include <algorithm>
#include <vector>
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/logger.hpp"

namespace vioavr::core {

namespace {
constexpr u8 kIntMask(u8 index) noexcept { return static_cast<u8>(1U << index); }

// Sense mode in EICRA: INT0-3 at bits [1:0], [3:2], [5:4], [7:6]
// Sense mode in EICRB: INT4-7 at bits [1:0], [3:2], [5:4], [7:6]
[[nodiscard]] u8 extract_sense_mode(u8 reg, u8 index) noexcept {
    return static_cast<u8>((reg >> (index * 2U)) & 0x03U);
}
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
    std::vector<u16> addrs;
    if (desc_.eicra_address) addrs.push_back(desc_.eicra_address);
    if (desc_.eicrb_address) addrs.push_back(desc_.eicrb_address);
    if (desc_.eimsk_address) addrs.push_back(desc_.eimsk_address);
    if (desc_.eifr_address) addrs.push_back(desc_.eifr_address);
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

ClockDomain ExtInterrupt::clock_domain() const noexcept
{
    return ClockDomain::none;
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
    eicrb_ = 0U;
    eimsk_ = 0U;
    eifr_ = 0U;
    for (auto& lvl : int_levels_) lvl = true;
    for (auto& pnd : int_pending_) pnd = false;
}

void ExtInterrupt::tick(const u64 elapsed_cycles) noexcept
{
    if (elapsed_cycles > 0U) refresh_bound_input();
    for (u8 i = 0; i < kIntCount; ++i) {
        if (sense_mode(i) == 0x00U && !int_levels_[i]) {
            raise_interrupt(i);
        }
    }
}

u8 ExtInterrupt::read(const u16 address) noexcept
{
    refresh_bound_input();
    if (address == desc_.eicra_address) return eicra_;
    if (address == desc_.eicrb_address) return eicrb_;
    if (address == desc_.eimsk_address) return eimsk_;
    if (address == desc_.eifr_address) return eifr_;
    return 0U;
}

void ExtInterrupt::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.eicra_address) {
        eicra_ = value;
        update_interrupt_pending();
        return;
    }
    if (address == desc_.eicrb_address) {
        eicrb_ = value;
        update_interrupt_pending();
        return;
    }
    if (address == desc_.eimsk_address) {
        eimsk_ = value;
        update_interrupt_pending();
        return;
    }
    if (address == desc_.eifr_address) {
        eifr_ = static_cast<u8>(eifr_ & static_cast<u8>(~value));
        for (u8 i = 0; i < kIntCount; ++i) {
            if ((value & kIntMask(i)) != 0U) {
                int_pending_[i] = false;
            }
        }
        update_interrupt_pending();
    }
}

bool ExtInterrupt::on_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept
{
    Logger::debug("ExtInterrupt: external pin change addr=0x" + std::to_string(pin_address) + " bit=" + std::to_string(bit_index));
    bool level_bool = (level == PinLevel::high);
    bool has_mapping = false;
    for (u8 i = 0; i < kIntCount; ++i) {
        if (desc_.pin_addresses[i] != 0U) {
            has_mapping = true;
            if (desc_.pin_addresses[i] == pin_address && desc_.pin_bit_indices[i] == bit_index) {
                handle_level_change(i, level_bool);
                return true;
            }
        }
    }
    if (!has_mapping) {
        if (bit_index == 2) { handle_level_change(0, level_bool); return true; }
        if (bit_index == 3) { handle_level_change(1, level_bool); return true; }
    }
    return false;
}

bool ExtInterrupt::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    const_cast<ExtInterrupt*>(this)->refresh_bound_input();
    for (u8 i = 0; i < kIntCount; ++i) {
        if (int_pending_[i] && (eimsk_ & kIntMask(i)) != 0U) {
            request = {.vector_index = desc_.vector_indices[i], .source_id = source_id_};
            return true;
        }
    }
    return false;
}

bool ExtInterrupt::consume_interrupt_request(InterruptRequest& request) noexcept
{
    refresh_bound_input();
    if (!pending_interrupt_request(request)) {
        return false;
    }

    for (u8 i = 0; i < kIntCount; ++i) {
        if (request.vector_index == desc_.vector_indices[i]) {
            int_pending_[i] = false;
            eifr_ &= static_cast<u8>(~kIntMask(i));
            break;
        }
    }

    update_interrupt_pending();
    return true;
}

void ExtInterrupt::bind_int0_signal(const AnalogSignalBank& signal_bank,
                                    const u8 channel,
                                    const DigitalThresholdConfig threshold) noexcept
{
    signal_bank_ = &signal_bank;
    signal_channel_ = channel;
    threshold_config_ = threshold;
    if (bus_ != nullptr) {
        bus_->register_ticking_peripheral(*this);
    }
    refresh_bound_input();
}

void ExtInterrupt::connect_adc_auto_trigger(Adc& adc) noexcept
{
    auto_trigger_adc_ = &adc;
}

void ExtInterrupt::set_int0_level(const bool high) noexcept
{
    handle_level_change(0, high);
}

void ExtInterrupt::set_int1_level(const bool high) noexcept
{
    handle_level_change(1, high);
}

void ExtInterrupt::set_int0_voltage(const double normalized_voltage) noexcept
{
    if (bus_ != nullptr) {
        bus_->register_ticking_peripheral(*this);
    }
    set_int0_level(apply_schmitt_threshold(int_levels_[0], normalized_voltage));
}

void ExtInterrupt::refresh_bound_input() noexcept
{
    if (signal_bank_ == nullptr) return;
    set_int0_level(apply_schmitt_threshold(int_levels_[0], signal_bank_->voltage(signal_channel_), threshold_config_));
}

u8 ExtInterrupt::sense_mode(const u8 index) const noexcept
{
    if (index < 4) return extract_sense_mode(eicra_, index);
    return extract_sense_mode(eicrb_, static_cast<u8>(index - 4));
}

void ExtInterrupt::handle_level_change(const u8 index, const bool high) noexcept
{
    const bool previous = int_levels_[index];
    int_levels_[index] = high;

    if (previous != high) {
        Logger::debug("INT" + std::to_string(index) + " level change detected");
    }

    switch (sense_mode(index)) {
    case 0x00U:
        if (!high) raise_interrupt(index);
        break;
    case 0x01U:
        if (previous != high) raise_interrupt(index);
        break;
    case 0x02U:
        if (previous && !high) raise_interrupt(index);
        break;
    case 0x03U:
        if (!previous && high) raise_interrupt(index);
        break;
    default:
        break;
    }
}

void ExtInterrupt::raise_interrupt(const u8 index) noexcept
{
    eifr_ |= kIntMask(index);
    int_pending_[index] = true;
    update_interrupt_pending();
    if (index == 0 && auto_trigger_adc_ != nullptr) {
        auto_trigger_adc_->notify_auto_trigger(Adc::AutoTriggerSource::external_interrupt_0);
    }
}

void ExtInterrupt::update_interrupt_pending() noexcept {
    InterruptRequest req;
    set_interrupt_pending(pending_interrupt_request(req));
}

} // namespace vioavr::core
