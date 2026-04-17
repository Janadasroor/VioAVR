#include "vioavr/core/timer16.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/adc.hpp"
#include <algorithm>
#include <vector>
#include <optional>

namespace vioavr::core {

namespace {
// TIFR bits
constexpr u8 kIcf1  = 0x20U;
constexpr u8 kOcf1b = 0x04U;
constexpr u8 kOcf1a = 0x02U;
constexpr u8 kTov1  = 0x01U;

// TIMSK bits
constexpr u8 kIcie1  = 0x20U;
constexpr u8 kOcie1b = 0x04U;
constexpr u8 kOcie1a = 0x02U;
constexpr u8 kToie1  = 0x01U;

// TCCRA bits
constexpr u8 kWgm11 = 0x02U;
constexpr u8 kWgm10 = 0x01U;

// TCCRB bits
constexpr u8 kIcnc1 = 0x80U;
constexpr u8 kIces1 = 0x40U;
constexpr u8 kWgm13 = 0x10U;
constexpr u8 kWgm12 = 0x08U;
constexpr u8 kCsMask = 0x07U;
}

Timer16::Timer16(std::string_view name, const Timer16Descriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs;
    auto add_16bit = [&](u16 addr) {
        if (addr != 0) {
            addrs.push_back(addr);
            addrs.push_back(addr + 1);
        }
    };
    auto add_8bit = [&](u16 addr) {
        if (addr != 0) addrs.push_back(addr);
    };

    add_16bit(desc_.tcnt_address);
    add_16bit(desc_.ocra_address);
    add_16bit(desc_.ocrb_address);
    add_16bit(desc_.ocrc_address);
    add_16bit(desc_.icr_address);
    add_8bit(desc_.tifr_address);
    add_8bit(desc_.timsk_address);
    add_8bit(desc_.tccra_address);
    add_8bit(desc_.tccrb_address);
    add_8bit(desc_.tccrc_address);

    std::sort(addrs.begin(), addrs.end());
    addrs.erase(std::unique(addrs.begin(), addrs.end()), addrs.end());
    
    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < addrs.size(); ++i) {
        if (addrs[i] == 0) continue;
        if (ri > 0 && addrs[i] == ranges_[ri-1].end + 1) {
             ranges_[ri-1].end = addrs[i];
        } else if (ri < ranges_.size()) {
             ranges_[ri++] = {addrs[i], addrs[i]};
        }
    }
}

std::string_view Timer16::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> Timer16::mapped_ranges() const noexcept
{
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

ClockDomain Timer16::clock_domain() const noexcept
{
    return ClockDomain::io;
}

void Timer16::reset() noexcept
{
    tcnt_ = 0U;
    ocra_ = 0U;
    ocrb_ = 0U;
    ocrc_ = 0U;
    ocra_buffer_ = 0U;
    ocrb_buffer_ = 0U;
    ocrc_buffer_ = 0U;
    icr_ = 0U;
    tccra_ = 0U;
    tccrb_ = 0U;
    tccrc_ = 0U;
    timsk_ = 0U;
    tifr_ = 0U;
    temp_ = 0U;
    mode_ = Mode::normal;
    counting_up_ = true;
    last_icp_state_ = 0U;
    last_t1_state_ = 0U;
    noise_canceler_register_ = 0U;
    noise_canceler_counter_ = 0U;
    cycle_accumulator_ = 0U;
}

void Timer16::tick(const u64 elapsed_cycles) noexcept
{
    if (power_reduction_enabled()) return;

    const u8 cs = tccrb_ & desc_.cs_mask;
    if (cs == 0) return; // Stopped

    for (u64 cycle = 0; cycle < elapsed_cycles; ++cycle) {
        bool should_tick = false;
        
        // 1. Handle Clocking
        if (cs <= 5) {
            static const u16 prescalers[] = {0, 1, 8, 64, 256, 1024};
            const u16 divisor = prescalers[cs];
            if (++cycle_accumulator_ >= divisor) {
                cycle_accumulator_ = 0;
                should_tick = true;
            }
        } else {
            // External Clocking (CS=110 falling, 111 rising)
            if (bus_ && desc_.t_pin_address != 0U) {
                const u8 current_pin_state = (bus_->read_data(desc_.t_pin_address) >> desc_.t_pin_bit) & 0x01U;
                const bool rising_edge = (cs == 7);
                const bool edge = rising_edge ? (last_t1_state_ == 0 && current_pin_state == 1)
                                              : (last_t1_state_ == 1 && current_pin_state == 0);
                if (edge) should_tick = true;
                last_t1_state_ = current_pin_state;
            }
        }

        // 2. Handle Input Capture
        bool event_triggered = false;
        u8 current_raw_state = 0xFFU;

        if (bus_ && desc_.icp_pin_address != 0U) {
            current_raw_state = (bus_->read_data(desc_.icp_pin_address) >> desc_.icp_pin_bit) & 0x01U;
        } else if (pin_icp_) {
            current_raw_state = (pin_icp_->port->read(pin_icp_->port->pin_address()) >> pin_icp_->bit) & 0x01U;
        }

        if (current_raw_state != 0xFFU) {
            // Initialization: use first sample as baseline
            if (noise_canceler_counter_ == 0) {
                noise_canceler_register_ = current_raw_state;
                noise_canceler_counter_ = 1;
                last_icp_state_ = current_raw_state;
            }

            if (tccrb_ & desc_.icnc_mask) {
                if (current_raw_state == noise_canceler_register_) {
                    if (noise_canceler_counter_ < 4) {
                        if (++noise_canceler_counter_ == 4) {
                            const bool rising_edge = (tccrb_ & desc_.ices_mask) != 0;
                            if (rising_edge && last_icp_state_ == 0 && current_raw_state == 1) event_triggered = true;
                            else if (!rising_edge && last_icp_state_ == 1 && current_raw_state == 0) event_triggered = true;
                            last_icp_state_ = current_raw_state;
                        }
                    }
                } else {
                    noise_canceler_register_ = current_raw_state;
                    noise_canceler_counter_ = 1;
                }
            } else {
                const bool rising_edge = (tccrb_ & desc_.ices_mask) != 0;
                if (rising_edge && last_icp_state_ == 0 && current_raw_state == 1) event_triggered = true;
                else if (!rising_edge && last_icp_state_ == 1 && current_raw_state == 0) event_triggered = true;
                last_icp_state_ = current_raw_state;
            }
        }

        // 3. Execution order: Compare Match -> TCNT Increment -> Input Capture
        if (should_tick) {
            handle_matches();
            perform_tick();
        }

        if (event_triggered) {
            handle_input_capture();
        }
    }
}

u8 Timer16::read(const u16 address) noexcept
{
    // 16-bit Read Protocol: Read Low byte first, High byte is latched into TEMP
    if (address == desc_.tcnt_address) {
        temp_ = static_cast<u8>(tcnt_ >> 8U);
        return static_cast<u8>(tcnt_ & 0xFFU);
    }
    if (address == desc_.tcnt_address + 1) return temp_;
    
    if (address == desc_.ocra_address) {
        temp_ = static_cast<u8>(ocra_buffer_ >> 8U);
        return static_cast<u8>(ocra_buffer_ & 0xFFU);
    }
    if (address == desc_.ocra_address + 1) return temp_;

    if (address == desc_.ocrb_address) {
        temp_ = static_cast<u8>(ocrb_buffer_ >> 8U);
        return static_cast<u8>(ocrb_buffer_ & 0xFFU);
    }
    if (address == desc_.ocrb_address + 1) return temp_;

    if (address == desc_.ocrc_address && desc_.ocrc_address != 0U) {
        temp_ = static_cast<u8>(ocrc_buffer_ >> 8U);
        return static_cast<u8>(ocrc_buffer_ & 0xFFU);
    }
    if (address == desc_.ocrc_address + 1 && desc_.ocrc_address != 0U) return temp_;

    if (address == desc_.icr_address) {
        temp_ = static_cast<u8>(icr_ >> 8U);
        return static_cast<u8>(icr_ & 0xFFU);
    }
    if (address == desc_.icr_address + 1) return temp_;

    if (address == desc_.tccra_address) return tccra_;
    if (address == desc_.tccrb_address) return tccrb_;
    if (address == desc_.tccrc_address) return tccrc_;
    if (address == desc_.timsk_address) return timsk_;
    if (address == desc_.tifr_address) return tifr_;

    return 0U;
}

void Timer16::write(const u16 address, const u8 value) noexcept
{
    const bool is_stopped = (tccrb_ & 0x07U) == 0U;
    const bool is_pwm = (mode_ != Mode::normal && mode_ != Mode::ctc_ocr && mode_ != Mode::ctc_icr);

    // 16-bit Write Protocol: Write High byte first (to TEMP), then Low byte (updates register)
    if (address == desc_.tcnt_address + 1) {
        temp_ = value;
    } else if (address == desc_.tcnt_address) {
        tcnt_ = static_cast<u16>((static_cast<u16>(temp_) << 8U) | value);
    } else if (address == desc_.ocra_address + 1) {
        temp_ = value;
    } else if (address == desc_.ocra_address) {
        const u16 val = static_cast<u16>((static_cast<u16>(temp_) << 8U) | value);
        ocra_buffer_ = val;
        if (!is_pwm || is_stopped) ocra_ = val;
    } else if (address == desc_.ocrb_address + 1) {
        temp_ = value;
    } else if (address == desc_.ocrb_address) {
        const u16 val = static_cast<u16>((static_cast<u16>(temp_) << 8U) | value);
        ocrb_buffer_ = val;
        if (!is_pwm || is_stopped) ocrb_ = val;
    } else if (address == desc_.ocrc_address + 1 && desc_.ocrc_address != 0U) {
        temp_ = value;
    } else if (address == desc_.ocrc_address && desc_.ocrc_address != 0U) {
        const u16 val = static_cast<u16>((static_cast<u16>(temp_) << 8U) | value);
        ocrc_buffer_ = val;
        if (!is_pwm || is_stopped) ocrc_ = val;
    } else if (address == desc_.icr_address + 1) {
        temp_ = value;
    } else if (address == desc_.icr_address) {
        icr_ = static_cast<u16>((static_cast<u16>(temp_) << 8U) | value);
    } else if (address == desc_.tccra_address) {
        tccra_ = value;
        update_mode();
    } else if (address == desc_.tccrb_address) {
        tccrb_ = value;
        update_mode();
    } else if (address == desc_.tccrc_address && desc_.tccrc_address != 0U) {
        tccrc_ = value;
        // Force Output Compare: Only active in non-PWM modes
        if (mode_ == Mode::normal || mode_ == Mode::ctc_ocr || mode_ == Mode::ctc_icr) {
            if (value & desc_.foca_mask) handle_compare_match_a();
            if (value & desc_.focb_mask) handle_compare_match_b();
            if (value & desc_.focc_mask && desc_.focc_mask != 0U) handle_compare_match_c();
        }
    } else if (address == desc_.timsk_address) {
        timsk_ = value;
    } else if (address == desc_.tifr_address) {
        tifr_ &= static_cast<u8>(~value); // Write 1 to clear
    }
}

bool Timer16::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if ((tifr_ & desc_.compare_a_enable_mask) && (timsk_ & desc_.compare_a_enable_mask)) {
        request = {desc_.compare_a_vector_index, 0U};
        return true;
    }
    if ((tifr_ & desc_.compare_b_enable_mask) && (timsk_ & desc_.compare_b_enable_mask)) {
        request = {desc_.compare_b_vector_index, 0U};
        return true;
    }
    if ((tifr_ & desc_.compare_c_enable_mask) && (timsk_ & desc_.compare_c_enable_mask)) {
        request = {desc_.compare_c_vector_index, 0U};
        return true;
    }
    if ((tifr_ & desc_.capture_enable_mask) && (timsk_ & desc_.capture_enable_mask)) {
        request = {desc_.capture_vector_index, 0U};
        return true;
    }
    if ((tifr_ & desc_.overflow_enable_mask) && (timsk_ & desc_.overflow_enable_mask)) {
        request = {desc_.overflow_vector_index, 0U};
        return true;
    }
    return false;
}

bool Timer16::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (pending_interrupt_request(request)) {
        if (request.vector_index == desc_.compare_a_vector_index) tifr_ &= ~desc_.compare_a_enable_mask;
        else if (request.vector_index == desc_.compare_b_vector_index) tifr_ &= ~desc_.compare_b_enable_mask;
        else if (request.vector_index == desc_.compare_c_vector_index && desc_.compare_c_enable_mask != 0U) tifr_ &= ~desc_.compare_c_enable_mask;
        else if (request.vector_index == desc_.capture_vector_index) tifr_ &= ~desc_.capture_enable_mask;
        else tifr_ &= ~desc_.overflow_enable_mask;
        return true;
    }
    return false;
}

void Timer16::connect_input_capture(GpioPort& port, const u8 bit) noexcept { pin_icp_ = {&port, bit}; }
void Timer16::connect_compare_output_a(GpioPort& port, const u8 bit) noexcept { pin_a_ = {&port, bit}; }
void Timer16::connect_compare_output_b(GpioPort& port, const u8 bit) noexcept { pin_b_ = {&port, bit}; }
void Timer16::connect_compare_output_c(GpioPort& port, const u8 bit) noexcept { pin_c_ = {&port, bit}; }

void Timer16::update_mode() noexcept
{
    // wgm10_mask is usually 0x03 in TCCRA. wgm12_mask is usually 0x18 in TCCRB.
    const u8 wgm_low = (tccra_ & desc_.wgm10_mask);
    const u8 wgm_high = static_cast<u8>(((tccrb_ & desc_.wgm12_mask) >> 3U) << 2U);
    const u8 wgm = wgm_high | wgm_low;
    
    static const Mode modes[] = {
        Mode::normal, Mode::pc_pwm_8bit, Mode::pc_pwm_9bit, Mode::pc_pwm_10bit,
        Mode::ctc_ocr, Mode::fast_pwm_8bit, Mode::fast_pwm_9bit, Mode::fast_pwm_10bit,
        Mode::pfc_pwm_icr, Mode::pfc_pwm_ocr, Mode::pc_pwm_icr, Mode::pc_pwm_ocr,
        Mode::ctc_icr, Mode::normal, Mode::fast_pwm_icr, Mode::fast_pwm_ocr
    };
    mode_ = modes[wgm & 0x0FU];
}

bool Timer16::power_reduction_enabled() const noexcept
{
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) {
        return false;
    }

    // PRR bit 1 = Disabled
    return (bus_->read_data(desc_.pr_address) & desc_.pr_bit) != 0;
}

void Timer16::perform_tick() noexcept
{
    const u16 top = get_top();
    const bool is_phase_correct = (mode_ == Mode::pc_pwm_8bit || mode_ == Mode::pc_pwm_9bit ||
                                   mode_ == Mode::pc_pwm_10bit || mode_ == Mode::pfc_pwm_icr ||
                                   mode_ == Mode::pfc_pwm_ocr || mode_ == Mode::pc_pwm_icr ||
                                   mode_ == Mode::pc_pwm_ocr);
    const bool is_fast_pwm = (mode_ == Mode::fast_pwm_8bit || mode_ == Mode::fast_pwm_9bit ||
                              mode_ == Mode::fast_pwm_10bit || mode_ == Mode::fast_pwm_icr ||
                              mode_ == Mode::fast_pwm_ocr);

    const bool is_pc = (mode_ == Mode::pc_pwm_8bit || mode_ == Mode::pc_pwm_9bit ||
                        mode_ == Mode::pc_pwm_10bit || mode_ == Mode::pc_pwm_icr ||
                        mode_ == Mode::pc_pwm_ocr);
    const bool is_pfc = (mode_ == Mode::pfc_pwm_icr || mode_ == Mode::pfc_pwm_ocr);

    if (is_phase_correct) {
        if (counting_up_) {
            if (tcnt_ == top) {
                if (is_pc) {
                    ocra_ = ocra_buffer_;
                    ocrb_ = ocrb_buffer_;
                    ocrc_ = ocrc_buffer_;
                }
                counting_up_ = false;
                tcnt_--;
            } else {
                tcnt_++;
            }
        } else {
            if (tcnt_ == 0) {
                if (is_pfc) {
                    ocra_ = ocra_buffer_;
                    ocrb_ = ocrb_buffer_;
                    ocrc_ = ocrc_buffer_;
                }
                counting_up_ = true;
                handle_overflow();
                tcnt_++;
            } else {
                tcnt_--;
            }
        }
    } else {
        if (tcnt_ == top) {
            tcnt_ = 0;
            if (is_fast_pwm) {
                ocra_ = ocra_buffer_;
                ocrb_ = ocrb_buffer_;
                ocrc_ = ocrc_buffer_;
            }
            if (mode_ == Mode::normal || is_fast_pwm) {
                handle_overflow();
            }
        } else {
            tcnt_++;
        }
    }
}

void Timer16::handle_matches() noexcept
{
    const bool is_phase_correct = (mode_ == Mode::pc_pwm_8bit || mode_ == Mode::pc_pwm_9bit ||
                                   mode_ == Mode::pc_pwm_10bit || mode_ == Mode::pfc_pwm_icr ||
                                   mode_ == Mode::pfc_pwm_ocr || mode_ == Mode::pc_pwm_icr ||
                                   mode_ == Mode::pc_pwm_ocr);
    const bool is_fast_pwm = (mode_ == Mode::fast_pwm_8bit || mode_ == Mode::fast_pwm_9bit ||
                              mode_ == Mode::fast_pwm_10bit || mode_ == Mode::fast_pwm_icr ||
                              mode_ == Mode::fast_pwm_ocr);

    if (is_phase_correct) {
        // Phase-correct PWM: update output pins based on current TCNT and counting direction
        if (tcnt_ == ocra_) {
            PinAction action = get_pin_action_a();
            if (action == PinAction::clear) {
                action = counting_up_ ? PinAction::clear : PinAction::set;
            } else if (action == PinAction::set) {
                action = counting_up_ ? PinAction::set : PinAction::clear;
            }
            apply_pin_action(pin_a_, action);
        }
        if (tcnt_ == ocrb_) {
            PinAction action = get_pin_action_b();
            if (action == PinAction::clear) {
                action = counting_up_ ? PinAction::clear : PinAction::set;
            } else if (action == PinAction::set) {
                action = counting_up_ ? PinAction::set : PinAction::clear;
            }
            apply_pin_action(pin_b_, action);
        }
        if (tcnt_ == ocrc_ && desc_.ocrc_address != 0U) {
            PinAction action = get_pin_action_c();
            if (action == PinAction::clear) {
                action = counting_up_ ? PinAction::clear : PinAction::set;
            } else if (action == PinAction::set) {
                action = counting_up_ ? PinAction::set : PinAction::clear;
            }
            apply_pin_action(pin_c_, action);
        }
        
        // Special case: set pins at BOTTOM if we are about to transition there
        if (!counting_up_ && tcnt_ == 0) {
            if (get_pin_action_a() == PinAction::clear) apply_pin_action(pin_a_, PinAction::set);
            if (get_pin_action_b() == PinAction::clear) apply_pin_action(pin_b_, PinAction::set);
            if (get_pin_action_c() == PinAction::clear && desc_.ocrc_address != 0U) apply_pin_action(pin_c_, PinAction::set);
        }
    } else {
        // Normal / CTC / Fast PWM matches
        if (tcnt_ == ocra_) handle_compare_match_a();
        if (tcnt_ == ocrb_) handle_compare_match_b();
        if (tcnt_ == ocrc_ && desc_.ocrc_address != 0U) handle_compare_match_c();
        
        // Fast PWM: Set pins at BOTTOM (transition from TOP to 0)
        if (is_fast_pwm && tcnt_ == get_top()) {
            if (get_pin_action_a() == PinAction::clear) apply_pin_action(pin_a_, PinAction::set);
            if (get_pin_action_b() == PinAction::clear) apply_pin_action(pin_b_, PinAction::set);
            if (get_pin_action_c() == PinAction::clear && desc_.ocrc_address != 0U) apply_pin_action(pin_c_, PinAction::set);
        }
    }
}

u16 Timer16::get_top() const noexcept
{
    switch (mode_) {
    case Mode::normal: return 0xFFFFU;
    case Mode::pc_pwm_8bit:
    case Mode::fast_pwm_8bit: return 0x00FFU;
    case Mode::pc_pwm_9bit:
    case Mode::fast_pwm_9bit: return 0x01FFU;
    case Mode::pc_pwm_10bit:
    case Mode::fast_pwm_10bit: return 0x03FFU;
    case Mode::ctc_ocr:
    case Mode::fast_pwm_ocr:
    case Mode::pfc_pwm_ocr:
    case Mode::pc_pwm_ocr: return ocra_;
    case Mode::ctc_icr:
    case Mode::fast_pwm_icr:
    case Mode::pfc_pwm_icr:
    case Mode::pc_pwm_icr: return icr_;
    default: return 0xFFFFU;
    }
}

void Timer16::handle_compare_match_a() noexcept 
{ 
    tifr_ |= desc_.compare_a_enable_mask; 
    apply_pin_action(pin_a_, get_pin_action_a());
}

void Timer16::handle_compare_match_b() noexcept 
{ 
    tifr_ |= desc_.compare_b_enable_mask; 
    apply_pin_action(pin_b_, get_pin_action_b());
    if (adc_) adc_->notify_auto_trigger(desc_.compare_b_trigger_source);
}

void Timer16::handle_compare_match_c() noexcept 
{ 
    if (desc_.compare_c_enable_mask != 0U) {
        tifr_ |= desc_.compare_c_enable_mask; 
        apply_pin_action(pin_c_, get_pin_action_c());
    }
}

void Timer16::handle_overflow() noexcept { 
    tifr_ |= desc_.overflow_enable_mask; 
    if (adc_) adc_->notify_auto_trigger(desc_.overflow_trigger_source);
}

void Timer16::handle_input_capture() noexcept
{
    icr_ = tcnt_;
    tifr_ |= desc_.capture_enable_mask;
    if (adc_) adc_->notify_auto_trigger(desc_.capture_trigger_source);
}

void Timer16::connect_adc_auto_trigger(Adc& adc) noexcept {
    adc_ = &adc;
}

void Timer16::apply_pin_action(std::optional<BoundPin> pin, PinAction action) noexcept
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

Timer16::PinAction Timer16::get_pin_action_a() const noexcept
{
    return static_cast<PinAction>((tccra_ >> 6U) & 0x03U);
}

Timer16::PinAction Timer16::get_pin_action_b() const noexcept
{
    return static_cast<PinAction>((tccra_ >> 4U) & 0x03U);
}

Timer16::PinAction Timer16::get_pin_action_c() const noexcept
{
    return static_cast<PinAction>((tccra_ >> 2U) & 0x03U);
}

void Timer16::notify_input_capture(bool high) noexcept {
    const bool rising_edge = (tccrb_ & desc_.ices_mask) != 0;
    if (high == rising_edge) {
        handle_input_capture();
    }
}

}  // namespace vioavr::core
