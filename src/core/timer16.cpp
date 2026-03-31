#include "vioavr/core/timer16.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/gpio_port.hpp"
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

Timer16::Timer16(std::string_view name, const DeviceDescriptor& device) noexcept
    : Timer16(name, device.timer1)
{
}

Timer16::Timer16(std::string_view name, const Timer16Descriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc_.tcnt_address, desc_.tcnt_address + 1,
        desc_.ocra_address, desc_.ocra_address + 1,
        desc_.ocrb_address, desc_.ocrb_address + 1,
        desc_.icr_address, desc_.icr_address + 1,
        desc_.tifr_address, desc_.timsk_address,
        desc_.tccra_address, desc_.tccrb_address, desc_.tccrc_address
    };
    std::sort(addrs.begin(), addrs.end());
    
    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < addrs.size(); ++i) {
        if (addrs[i] == 0) continue;
        // Address 0x01 is not a valid IO address for registers
        if (addrs[i] < 0x20 && addrs[i] != desc_.tifr_address && addrs[i] != desc_.timsk_address) {
            // Some devices have TIFR/TIMSK in low IO, others in high IO.
        }

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

void Timer16::reset() noexcept
{
    tcnt_ = 0U;
    ocra_ = 0U;
    ocrb_ = 0U;
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
}

void Timer16::tick(const u64 elapsed_cycles) noexcept
{
    const u8 cs = tccrb_ & kCsMask;
    if (cs == 0) return; // Stopped

    // External Clocking (CS=110 falling, 111 rising)
    if (cs >= 6) {
        if (bus_) {
            const u8 current_pin_state = (bus_->read_data(desc_.t1_pin_address) >> desc_.t1_pin_bit) & 0x01U;
            const bool rising_edge = (cs == 7);
            const bool edge = rising_edge ? (last_t1_state_ == 0 && current_pin_state == 1) 
                                          : (last_t1_state_ == 1 && current_pin_state == 0);
            
            if (edge) {
                perform_tick();
            }
            last_t1_state_ = current_pin_state;
        }
        return;
    }

    static const u16 prescalers[] = {0, 1, 8, 64, 256, 1024};
    const u16 divisor = prescalers[cs];
    
    // Check for ICP edge if connected
    if (pin_icp_) {
        const u8 current_state = (pin_icp_->port->read(pin_icp_->port->pin_address()) >> pin_icp_->bit) & 0x01U;
        const bool rising_edge = (tccrb_ & kIces1) != 0;
        const bool edge = rising_edge ? (last_icp_state_ == 0 && current_state == 1) : (last_icp_state_ == 1 && current_state == 0);
        if (edge) {
            handle_input_capture();
        }
        last_icp_state_ = current_state;
    }

    for (u64 i = 0; i < (elapsed_cycles / divisor); ++i) {
        perform_tick();
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
        temp_ = static_cast<u8>(ocra_ >> 8U);
        return static_cast<u8>(ocra_ & 0xFFU);
    }
    if (address == desc_.ocra_address + 1) return temp_;

    if (address == desc_.ocrb_address) {
        temp_ = static_cast<u8>(ocrb_ >> 8U);
        return static_cast<u8>(ocrb_ & 0xFFU);
    }
    if (address == desc_.ocrb_address + 1) return temp_;

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
    // 16-bit Write Protocol: Write High byte first (to TEMP), then Low byte (updates register)
    if (address == desc_.tcnt_address + 1) {
        temp_ = value;
    } else if (address == desc_.tcnt_address) {
        tcnt_ = static_cast<u16>((static_cast<u16>(temp_) << 8U) | value);
    } else if (address == desc_.ocra_address + 1) {
        temp_ = value;
    } else if (address == desc_.ocra_address) {
        ocra_ = static_cast<u16>((static_cast<u16>(temp_) << 8U) | value);
    } else if (address == desc_.ocrb_address + 1) {
        temp_ = value;
    } else if (address == desc_.ocrb_address) {
        ocrb_ = static_cast<u16>((static_cast<u16>(temp_) << 8U) | value);
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
    } else if (address == desc_.tccrc_address) {
        tccrc_ = value;
    } else if (address == desc_.timsk_address) {
        timsk_ = value;
    } else if (address == desc_.tifr_address) {
        tifr_ &= static_cast<u8>(~value);
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
        else if (request.vector_index == desc_.capture_vector_index) tifr_ &= ~desc_.capture_enable_mask;
        else tifr_ &= ~desc_.overflow_enable_mask;
        return true;
    }
    return false;
}

void Timer16::connect_input_capture(GpioPort& port, const u8 bit) noexcept { pin_icp_ = {&port, bit}; }
void Timer16::connect_compare_output_a(GpioPort& port, const u8 bit) noexcept { pin_a_ = {&port, bit}; }
void Timer16::connect_compare_output_b(GpioPort& port, const u8 bit) noexcept { pin_b_ = {&port, bit}; }

void Timer16::update_mode() noexcept
{
    const u8 wgm = static_cast<u8>(((tccrb_ & 0x18U) >> 1U) | (tccra_ & 0x03U));
    static const Mode modes[] = {
        Mode::normal, Mode::pc_pwm_8bit, Mode::pc_pwm_9bit, Mode::pc_pwm_10bit,
        Mode::ctc_ocr, Mode::fast_pwm_8bit, Mode::fast_pwm_9bit, Mode::fast_pwm_10bit,
        Mode::pfc_pwm_icr, Mode::pfc_pwm_ocr, Mode::pc_pwm_icr, Mode::pc_pwm_ocr,
        Mode::ctc_icr, Mode::normal, Mode::fast_pwm_icr, Mode::fast_pwm_ocr
    };
    mode_ = modes[wgm & 0x0FU];
}

void Timer16::perform_tick() noexcept
{
    const u16 top = get_top();
    const bool is_phase_correct = (mode_ == Mode::pc_pwm_8bit || mode_ == Mode::pc_pwm_9bit || 
                                   mode_ == Mode::pc_pwm_10bit || mode_ == Mode::pfc_pwm_icr || 
                                   mode_ == Mode::pfc_pwm_ocr || mode_ == Mode::pc_pwm_icr || 
                                   mode_ == Mode::pc_pwm_ocr);

    if (is_phase_correct) {
        if (counting_up_) {
            if (tcnt_ == top) {
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
        if (tcnt_ == top) {
            tcnt_ = 0;
            if (mode_ == Mode::normal || mode_ == Mode::fast_pwm_8bit || 
                mode_ == Mode::fast_pwm_9bit || mode_ == Mode::fast_pwm_10bit ||
                mode_ == Mode::fast_pwm_icr || mode_ == Mode::fast_pwm_ocr) {
                handle_overflow();
            }
        } else {
            tcnt_++;
        }
    }

    if (tcnt_ == ocra_) handle_compare_match_a();
    if (tcnt_ == ocrb_) handle_compare_match_b();
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
}

void Timer16::handle_overflow() noexcept { tifr_ |= desc_.overflow_enable_mask; }

void Timer16::handle_input_capture() noexcept
{
    icr_ = tcnt_;
    tifr_ |= desc_.capture_enable_mask;
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

}  // namespace vioavr::core
