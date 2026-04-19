#include "vioavr/core/timer8.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/dac.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

namespace {
constexpr u8 kWgm00 = 0x01U;
constexpr u8 kWgm01 = 0x02U;
constexpr u8 kWgm02 = 0x08U;
constexpr u8 kCsMask = 0x07U;
constexpr u8 kAssrTcr2bub = 0x01U;
constexpr u8 kAssrTcr2aub = 0x02U;
constexpr u8 kAssrOcr2bub = 0x04U;
constexpr u8 kAssrOcr2aub = 0x08U;
constexpr u8 kAssrTcn2ub = 0x10U;
constexpr u8 kAssrAs2 = 0x20U;
constexpr u8 kAssrExclk = 0x40U;
}

Timer8::Timer8(std::string_view name, const Timer8Descriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc_.tcnt_address, desc_.ocra_address, desc_.ocrb_address,
        desc_.tifr_address, desc_.timsk_address, desc_.tccra_address, desc_.tccrb_address,
        desc_.assr_address
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

ClockDomain Timer8::clock_domain() const noexcept
{
    // Timer2 is usually the only 8-bit timer with AS2 bit in ASSR
    return has_async_status_register() ? ClockDomain::async_timer : ClockDomain::io;
}

void Timer8::reset() noexcept
{
    tcnt_ = 0U;
    ocra_ = 0U;
    ocrb_ = 0U;
    ocra_buffer_ = 0U;
    ocrb_buffer_ = 0U;
    tccra_ = 0U;
    tccrb_ = 0U;
    assr_ = 0U;
    timsk_ = 0U;
    tifr_ = 0U;
    mode_ = Mode::normal;
    counting_up_ = true;
    cycle_accumulator_ = 0U;
    last_clk_pin_state_ = 0U;
}

void Timer8::tick(const u64 elapsed_cycles) noexcept
{
    if (power_reduction_enabled()) {
        return;
    }

    if (elapsed_cycles > 0U) {
        retire_async_busy(elapsed_cycles);
    }

    if (async_mode_enabled()) {
        return;
    }

    const u8 cs = tccrb_ & desc_.cs_mask;
    if (cs == 0) return;


    if (cs <= 5) {
        static const u16 prescalers[] = {0, 1, 8, 64, 256, 1024};
        const u16 divisor = prescalers[cs];
        for (u64 cycle = 0; cycle < elapsed_cycles; ++cycle) {
            if (++cycle_accumulator_ >= divisor) {
                cycle_accumulator_ = 0;
                perform_tick();
            }
        }
    } else {
        // External clock via T pin
        for (u64 cycle = 0; cycle < elapsed_cycles; ++cycle) {
            if (bus_ && desc_.t_pin_address != 0U) {
                // Pin sampling happens on every cycle
                const u8 current_state = (bus_->read_data(desc_.t_pin_address) >> desc_.t_pin_bit) & 0x01U;
                const bool edge = (cs == 6) ? (last_clk_pin_state_ == 1 && current_state == 0) : (last_clk_pin_state_ == 0 && current_state == 1);
                if (edge) {
                    perform_tick();
                }
                last_clk_pin_state_ = current_state;
            }
        }
    }
}

void Timer8::tick_async(const u64 elapsed_ticks) noexcept
{
    if (elapsed_ticks > 0U) {
        // In async mode, busy bits retire based on TOSC ticks too 
        // but we'll stick to CPU cycles for consistency in synchronization.
        retire_async_busy(elapsed_ticks * 512); // Heuristic
    }
    if (!async_mode_enabled()) {
        return;
    }

    const u8 cs = tccrb_ & kCsMask;
    if (cs == 0) {
        return;
    }

    if (cs <= 5) {
        static const u16 prescalers[] = {0, 1, 8, 64, 256, 1024};
        const u16 divisor = prescalers[cs];
        cycle_accumulator_ += elapsed_ticks;
        const u64 ticks = cycle_accumulator_ / divisor;
        cycle_accumulator_ %= divisor;
        perform_ticks(ticks);
    }
}

u8 Timer8::read(const u16 address) noexcept
{
    if (address == desc_.tcnt_address) return tcnt_;
    if (address == desc_.ocra_address) return ocra_buffer_;
    if (address == desc_.ocrb_address) return ocrb_buffer_;
    if (address == desc_.tccra_address) return tccra_;
    if (address == desc_.tccrb_address) return tccrb_;
    if (address == desc_.assr_address) return assr_;
    if (address == desc_.timsk_address) return timsk_;
    if (address == desc_.tifr_address) return tifr_;
    return 0U;
}

void Timer8::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.tcnt_address) {
        tcnt_ = value;
        mark_async_busy(address);
    }
    else if (address == desc_.ocra_address) {
        ocra_buffer_ = value;
        bool is_pwm = (mode_ != Mode::normal && mode_ != Mode::ctc_ocra);
        const bool is_stopped = (tccrb_ & 0x07U) == 0U;
        if (!is_pwm || is_stopped) {
            ocra_ = value;
        }
        mark_async_busy(address);
    }
    else if (address == desc_.ocrb_address) {
        ocrb_buffer_ = value;
        bool is_pwm = (mode_ != Mode::normal && mode_ != Mode::ctc_ocra);
        const bool is_stopped = (tccrb_ & 0x07U) == 0U;
        if (!is_pwm || is_stopped) {
            ocrb_ = value;
        }
        mark_async_busy(address);
    }
    else if (address == desc_.tccra_address) {
        tccra_ = value;
        update_mode();
        mark_async_busy(address);
    }
    else if (address == desc_.tccrb_address) {
        tccrb_ = value;
        update_mode();
        // Force Output Compare: only in non-PWM modes
        if (mode_ == Mode::normal || mode_ == Mode::ctc_ocra) {
            if (value & desc_.foca_mask) handle_compare_match_a();
            if (value & desc_.focb_mask) handle_compare_match_b();
        }
        mark_async_busy(address);
    }
    else if (address == desc_.assr_address) {
        assr_ = static_cast<u8>((assr_ & (desc_.as2_mask | desc_.exclk_mask | desc_.tcn2ub_mask | desc_.ocr2aub_mask | desc_.ocr2bub_mask | desc_.tcr2aub_mask | desc_.tcr2bub_mask)) | (value & (desc_.as2_mask | desc_.exclk_mask)));
    }
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

bool Timer8::power_reduction_enabled() const noexcept
{
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) {
        return false;
    }

    // PRR bit 1 = Disabled/Powered Down
    return (bus_->read_data(desc_.pr_address) & (1U << desc_.pr_bit)) != 0;
}

void Timer8::update_mode() noexcept
{
    // Extract WGM bits. wgm0_mask is usually in TCCRA, wgm2_mask in TCCRB bit 3.
    const u8 wgm01 = (tccra_ & desc_.wgm0_mask);
    const u8 wgm2 = (tccrb_ & desc_.wgm2_mask) ? 4U : 0U;
    const u8 wgm = wgm2 | wgm01;

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
    const bool is_fast_pwm = (mode_ == Mode::fast_pwm_ff || mode_ == Mode::fast_pwm_ocra);

    // 2. Perform the counter increment/wrap logic
    if (is_phase_correct) {
        if (counting_up_) {
            if (tcnt_ == top) {
                counting_up_ = false;
                if (tcnt_ > 0) tcnt_--;
            } else {
                tcnt_++;
                if (tcnt_ == top) {
                    counting_up_ = false;
                    ocra_ = ocra_buffer_;
                    ocrb_ = ocrb_buffer_;
                }
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
    } else if (is_fast_pwm) {
        if (tcnt_ == top) {
            handle_overflow();
            ocra_ = ocra_buffer_;
            ocrb_ = ocrb_buffer_;
            tcnt_ = 0;
        } else {
            tcnt_++;
        }
    } else { // Normal or CTC
        if (tcnt_ == top) {
            if (mode_ != Mode::ctc_ocra) handle_overflow();
            tcnt_ = 0;
        } else {
            tcnt_++;
        }
    }

    // 3. Sample matches AFTER the update 
    if (is_phase_correct) {
        if (tcnt_ == ocra_) handle_compare_match_a();
        if (tcnt_ == ocrb_) handle_compare_match_b();
    } else {
        if (tcnt_ == ocra_) handle_compare_match_a();
        if (tcnt_ == ocrb_) handle_compare_match_b();
        
        // Fast PWM: Set pins at BOTTOM (tcnt_ == 0 and we just wrapped)
        // Wait, the previous logic checked if tcnt_ == top before increment. 
        // If it wrapped this tick, tcnt_ is now 0.
        if (is_fast_pwm && tcnt_ == 0 && top != 0) { // wrapped
            if (get_pin_action_a() == PinAction::clear) apply_pin_action(pin_a_, PinAction::set);
            if (get_pin_action_b() == PinAction::clear) apply_pin_action(pin_b_, PinAction::set);
        } else if (is_fast_pwm && top == 0) {
            // Special case TOP=0
            if (get_pin_action_a() == PinAction::clear) apply_pin_action(pin_a_, PinAction::set);
            if (get_pin_action_b() == PinAction::clear) apply_pin_action(pin_b_, PinAction::set);
        }
    }
}

void Timer8::perform_ticks(const u64 ticks) noexcept
{
    for (u64 i = 0; i < ticks; ++i) {
        perform_tick();
    }
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

void Timer8::connect_dac_auto_trigger(Dac& dac) noexcept
{
    dac_trigger_ = &dac;
}

void Timer8::handle_compare_match_a() noexcept
{
    tifr_ |= desc_.compare_a_enable_mask;
    
    PinAction action = get_pin_action_a();
    if (mode_ == Mode::pc_pwm_ff || mode_ == Mode::pc_pwm_ocra) {
        if (action == PinAction::clear) { // Non-inverting
            action = counting_up_ ? PinAction::clear : PinAction::set;
        } else if (action == PinAction::set) { // Inverting
            action = counting_up_ ? PinAction::set : PinAction::clear;
        }
    }
    
    apply_pin_action(pin_a_, action);

    if (adc_compare_trigger_) {
        adc_compare_trigger_->notify_auto_trigger(desc_.compare_a_trigger_source);
    }
    if (dac_trigger_) {
        dac_trigger_->notify_auto_trigger(desc_.compare_a_trigger_source);
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
        adc_overflow_trigger_->notify_auto_trigger(desc_.overflow_trigger_source);
    }
    if (dac_trigger_) {
        dac_trigger_->notify_auto_trigger(desc_.overflow_trigger_source);
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

bool Timer8::has_async_status_register() const noexcept
{
    return desc_.assr_address != 0U;
}

bool Timer8::async_mode_enabled() const noexcept
{
    return has_async_status_register() && (assr_ & desc_.as2_mask) != 0U;
}

void Timer8::mark_async_busy(const u16 address) noexcept
{
    if (!async_mode_enabled()) {
        return;
    }

    if (address == desc_.tcnt_address) assr_ |= desc_.tcn2ub_mask;
    else if (address == desc_.ocra_address) assr_ |= desc_.ocr2aub_mask;
    else if (address == desc_.ocrb_address) assr_ |= desc_.ocr2bub_mask;
    else if (address == desc_.tccra_address) assr_ |= desc_.tcr2aub_mask;
    else if (address == desc_.tccrb_address) assr_ |= desc_.tcr2bub_mask;
    
    // Roughly 1 TOSC cycle @ 32kHz is ~488 cycles @ 16MHz.
    async_busy_countdown_ = 512;
}

void Timer8::retire_async_busy(const u64 cycles) noexcept
{
    if (!has_async_status_register()) {
        return;
    }
    
    // Only bother if any busy bits are set
    const u8 busy_mask = static_cast<u8>(desc_.tcn2ub_mask | desc_.ocr2aub_mask | desc_.ocr2bub_mask | desc_.tcr2aub_mask | desc_.tcr2bub_mask);
    if ((assr_ & busy_mask) == 0) {
        return;
    }

    if (async_busy_countdown_ > cycles) {
        async_busy_countdown_ -= static_cast<u16>(cycles);
    } else {
        async_busy_countdown_ = 0;
        assr_ &= static_cast<u8>(desc_.as2_mask | desc_.exclk_mask);
    }
}

}  // namespace vioavr::core
