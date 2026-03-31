#include "vioavr/core/timer8.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/adc.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

namespace {
constexpr u8 kWgm00 = 0x01U;
constexpr u8 kWgm01 = 0x02U;
constexpr u8 kWgm02 = 0x08U;
constexpr u8 kCsMask = 0x07U;
}

Timer8::Timer8(std::string_view name, const DeviceDescriptor& device) noexcept
    : name_(name), desc_(device.timer0)
{
    std::vector<u16> addrs = {
        desc_.tcnt_address, desc_.ocra_address, desc_.ocrb_address,
        desc_.tifr_address, desc_.timsk_address, desc_.tccra_address, desc_.tccrb_address
    };
    std::sort(addrs.begin(), addrs.end());
    
    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < addrs.size(); ++i) {
        if (addrs[i] == 0) continue;
        if (ri > 0 && addrs[i] == ranges_[ri-1].end + 1) {
             ranges_[ri-1].end = addrs[i];
        } else {
             ranges_[ri++] = {addrs[i], addrs[i]};
        }
    }
}

Timer8::Timer8(std::string_view name, const Timer8Descriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc_.tcnt_address, desc_.ocra_address, desc_.ocrb_address,
        desc_.tifr_address, desc_.timsk_address, desc_.tccra_address, desc_.tccrb_address
    };
    std::sort(addrs.begin(), addrs.end());
    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < addrs.size(); ++i) {
        if (addrs[i] == 0) continue;
        if (ri > 0 && addrs[i] == ranges_[ri-1].end + 1) {
             ranges_[ri-1].end = addrs[i];
        } else {
             ranges_[ri++] = {addrs[i], addrs[i]};
        }
    }
}

std::string_view Timer8::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> Timer8::mapped_ranges() const noexcept
{
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void Timer8::reset() noexcept
{
    tcnt_ = 0U;
    ocra_ = 0U;
    ocrb_ = 0U;
    tccra_ = 0U;
    tccrb_ = 0U;
    timsk_ = 0U;
    tifr_ = 0U;
    mode_ = Mode::normal;
    counting_up_ = true;
    last_clk_pin_state_ = 0U;
}

void Timer8::tick(const u64 elapsed_cycles) noexcept
{
    const u8 cs = tccrb_ & kCsMask;
    if (cs == 0) return;

    if (cs <= 5) {
        static const u16 prescalers[] = {0, 1, 8, 64, 256, 1024};
        const u16 divisor = prescalers[cs];
        for (u64 i = 0; i < (elapsed_cycles / divisor); ++i) {
            perform_tick();
        }
    } else {
        // External clock via T0 pin
        if (bus_) {
            const u8 current_state = (bus_->read_data(desc_.t0_pin_address) >> desc_.t0_pin_bit) & 0x01U;
            const bool edge = (cs == 6) ? (last_clk_pin_state_ == 1 && current_state == 0) : (last_clk_pin_state_ == 0 && current_state == 1);
            if (edge) {
                perform_tick();
            }
            last_clk_pin_state_ = current_state;
        }
    }
}

u8 Timer8::read(const u16 address) noexcept
{
    if (address == desc_.tcnt_address) return tcnt_;
    if (address == desc_.ocra_address) return ocra_;
    if (address == desc_.ocrb_address) return ocrb_;
    if (address == desc_.tccra_address) return tccra_;
    if (address == desc_.tccrb_address) return tccrb_;
    if (address == desc_.timsk_address) return timsk_;
    if (address == desc_.tifr_address) return tifr_;
    return 0U;
}

void Timer8::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.tcnt_address) tcnt_ = value;
    else if (address == desc_.ocra_address) ocra_ = value;
    else if (address == desc_.ocrb_address) ocrb_ = value;
    else if (address == desc_.tccra_address) { tccra_ = value; update_mode(); }
    else if (address == desc_.tccrb_address) { tccrb_ = value; update_mode(); }
    else if (address == desc_.timsk_address) timsk_ = value;
    else if (address == desc_.tifr_address) tifr_ &= static_cast<u8>(~value);
}

bool Timer8::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if ((tifr_ & desc_.compare_a_enable_mask) && (timsk_ & desc_.compare_a_enable_mask)) {
        request = {desc_.compare_a_vector_index, 0U};
        return true;
    }
    if ((tifr_ & desc_.compare_b_enable_mask) && (timsk_ & desc_.compare_b_enable_mask)) {
        request = {desc_.compare_b_vector_index, 0U};
        return true;
    }
    if ((tifr_ & desc_.overflow_enable_mask) && (timsk_ & desc_.overflow_enable_mask)) {
        request = {desc_.overflow_vector_index, 0U};
        return true;
    }
    return false;
}

bool Timer8::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (pending_interrupt_request(request)) {
        if (request.vector_index == desc_.compare_a_vector_index) tifr_ &= ~desc_.compare_a_enable_mask;
        else if (request.vector_index == desc_.compare_b_vector_index) tifr_ &= ~desc_.compare_b_enable_mask;
        else tifr_ &= ~desc_.overflow_enable_mask;
        return true;
    }
    return false;
}

void Timer8::connect_compare_output_a(GpioPort& port, const u8 bit) noexcept { pin_a_ = {&port, bit}; }
void Timer8::connect_compare_output_b(GpioPort& port, const u8 bit) noexcept { pin_b_ = {&port, bit}; }

void Timer8::connect_adc_auto_trigger(Adc& adc) noexcept
{
    adc_compare_trigger_ = &adc;
}

void Timer8::connect_adc_overflow_auto_trigger(Adc& adc) noexcept
{
    adc_overflow_trigger_ = &adc;
}

void Timer8::update_mode() noexcept
{
    const u8 wgm = static_cast<u8>(((tccrb_ & kWgm02) >> 1U) | (tccra_ & (kWgm01 | kWgm00)));
    static const Mode modes[] = {
        Mode::normal, Mode::pc_pwm_ff, Mode::ctc_ocra, Mode::fast_pwm_ff,
        Mode::normal, Mode::pc_pwm_ocra, Mode::normal, Mode::fast_pwm_ocra
    };
    mode_ = modes[wgm & 0x07U];
}

void Timer8::perform_tick() noexcept
{
    const u8 top = get_top();
    const bool is_phase_correct = (mode_ == Mode::pc_pwm_ff || mode_ == Mode::pc_pwm_ocra);

    if (is_phase_correct) {
        if (counting_up_) {
            if (tcnt_ >= top) {
                counting_up_ = false;
                tcnt_--;
            } else {
                tcnt_++;
            }
        } else {
            if (tcnt_ == 0) {
                counting_up_ = true;
                handle_overflow();
                tcnt_++;
            } else {
                tcnt_--;
            }
        }
    } else {
        if (tcnt_ >= top) {
            tcnt_ = 0;
            handle_overflow();
        } else {
            tcnt_++;
        }
    }

    if (tcnt_ == ocra_) handle_compare_match_a();
    if (tcnt_ == ocrb_) handle_compare_match_b();
}

u8 Timer8::get_top() const noexcept
{
    switch (mode_) {
    case Mode::ctc_ocra:
    case Mode::pc_pwm_ocra:
    case Mode::fast_pwm_ocra: return ocra_;
    default: return 0xFFU;
    }
}

void Timer8::handle_compare_match_a() noexcept
{
    tifr_ |= desc_.compare_a_enable_mask;
    apply_pin_action(pin_a_, get_pin_action_a());
    if (adc_compare_trigger_) {
        adc_compare_trigger_->notify_auto_trigger(Adc::AutoTriggerSource::timer_compare);
    }
}

void Timer8::handle_compare_match_b() noexcept
{
    tifr_ |= desc_.compare_b_enable_mask;
    apply_pin_action(pin_b_, get_pin_action_b());
}

void Timer8::handle_overflow() noexcept
{
    tifr_ |= desc_.overflow_enable_mask;
    if (adc_overflow_trigger_) {
        adc_overflow_trigger_->notify_auto_trigger(Adc::AutoTriggerSource::timer_overflow);
    }
}

void Timer8::apply_pin_action(std::optional<BoundPin> pin, PinAction action) noexcept
{
    if (!pin) return;
    const u16 addr = pin->port->port_address();
    const u8 current = pin->port->read(addr);
    switch (action) {
    case PinAction::toggle: pin->port->write(addr, current ^ (1U << pin->bit)); break;
    case PinAction::clear: pin->port->write(addr, current & ~(1U << pin->bit)); break;
    case PinAction::set: pin->port->write(addr, current | (1U << pin->bit)); break;
    default: break;
    }
}

Timer8::PinAction Timer8::get_pin_action_a() const noexcept
{
    return static_cast<PinAction>((tccra_ >> 6U) & 0x03U);
}

Timer8::PinAction Timer8::get_pin_action_b() const noexcept
{
    return static_cast<PinAction>((tccra_ >> 4U) & 0x03U);
}

}  // namespace vioavr::core
