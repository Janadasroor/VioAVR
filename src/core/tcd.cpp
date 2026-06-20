#include "vioavr/core/tcd.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/pin_mux.hpp"

namespace vioavr::core {

// Register offsets
enum TcdReg : u8 {
    REG_CTRLA     = 0x00,
    REG_CTRLB     = 0x01,
    REG_CTRLC     = 0x02,
    REG_CTRLD     = 0x03,
    REG_CTRLE     = 0x04,
    REG_EVCTRLA   = 0x08,
    REG_EVCTRLB   = 0x09,
    REG_INTCTRL   = 0x0C,
    REG_INTFLAGS  = 0x0D,
    REG_STATUS    = 0x0E,
    REG_INPUTCTRLA = 0x10,
    REG_INPUTCTRLB = 0x11,
    REG_FAULTCTRL = 0x12,
    REG_DLYCTRL   = 0x14,
    REG_DLYVAL    = 0x15,
    REG_DITCTRL   = 0x18,
    REG_DITVAL    = 0x19,
    REG_DBGCTRL   = 0x1E,
    REG_CAPTUREA  = 0x22,
    REG_CAPTUREB  = 0x24,
    REG_CMPASET   = 0x28,
    REG_CMPACLR   = 0x2A,
    REG_CMPBSET   = 0x2C,
    REG_CMPBCLR   = 0x2E,
};

// CTRLA bits
constexpr u8 CTRLA_ENABLE   = 0x01;
constexpr u8 CTRLA_SYNCPRES = 0x06;
constexpr u8 CTRLA_CNTPRES  = 0x18;
constexpr u8 CTRLA_CLKSEL   = 0x60;

// CTRLE bits (strobe commands)
constexpr u8 CTRLE_SYNCEOC   = 0x01;
constexpr u8 CTRLE_SYNC      = 0x02;
constexpr u8 CTRLE_RESTART   = 0x04;
constexpr u8 CTRLE_SCAPTUREA = 0x08;
constexpr u8 CTRLE_SCAPTUREB = 0x10;
constexpr u8 CTRLE_DISEOC    = 0x80;

// INTCTRL/INTFLAGS
constexpr u8 INT_OVF   = 0x01;
constexpr u8 INT_TRIGA = 0x04;
constexpr u8 INT_TRIGB = 0x08;

// STATUS
constexpr u8 STATUS_ENRDY   = 0x01;
constexpr u8 STATUS_CMDRDY  = 0x02;

// WGMODE values
constexpr u8 WGMODE_ONERAMP  = 0;
constexpr u8 WGMODE_TWORAMP  = 1;
constexpr u8 WGMODE_FOURRAMP = 2;
constexpr u8 WGMODE_DS       = 3;

// Prescaler dividers
static constexpr u16 kSyncPrescalers[] = {1, 2, 4, 8};
static constexpr u16 kCntPrescalers[]  = {1, 4, 32};

// Approximate cycles per counter step (after prescaler)
constexpr u64 kCounterClockCycles = 2;

static u16 tcd_range_min(const TcdDescriptor& desc) noexcept {
    u16 min_addr = desc.base_address;
    auto update = [&](u16 addr) { if (addr && addr < min_addr) min_addr = addr; };
    update(desc.ctrla_address); update(desc.ctrlb_address);
    update(desc.ctrlc_address); update(desc.ctrld_address); update(desc.ctrle_address);
    update(desc.evctrla_address); update(desc.evctrlB_address);
    update(desc.intctrl_address); update(desc.intflags_address); update(desc.status_address);
    update(desc.inputctrla_address); update(desc.inputctrlB_address);
    update(desc.faultctrl_address);
    update(desc.dlyctrl_address); update(desc.dlyval_address);
    update(desc.ditctrl_address); update(desc.ditval_address);
    update(desc.dbgctrl_address);
    update(desc.capturea_address); update(desc.captureb_address);
    update(desc.cmpaset_address); update(desc.cmpacl_address);
    update(desc.cmpbset_address); update(desc.cmpbcl_address);
    return min_addr;
}

static u16 tcd_range_max(const TcdDescriptor& desc) noexcept {
    u16 max_addr = desc.base_address;
    auto update = [&](u16 addr) { if (addr && addr > max_addr) max_addr = addr; };
    update(desc.ctrla_address); update(desc.ctrlb_address);
    update(desc.ctrlc_address); update(desc.ctrld_address); update(desc.ctrle_address);
    update(desc.evctrla_address); update(desc.evctrlB_address);
    update(desc.intctrl_address); update(desc.intflags_address); update(desc.status_address);
    update(desc.inputctrla_address); update(desc.inputctrlB_address);
    update(desc.faultctrl_address);
    update(desc.dlyctrl_address); update(desc.dlyval_address);
    update(desc.ditctrl_address); update(desc.ditval_address);
    update(desc.dbgctrl_address);
    update(desc.capturea_address); update(desc.captureb_address);
    update(desc.cmpaset_address); update(desc.cmpacl_address);
    update(desc.cmpbset_address); update(desc.cmpbcl_address);
    return max_addr;
}

Tcd::Tcd(const TcdDescriptor& desc) noexcept : desc_(desc) {
    if (desc_.base_address != 0) {
        u16 min_addr = tcd_range_min(desc_);
        u16 max_addr = tcd_range_max(desc_);
        if (max_addr <= min_addr) {
            if (desc_.base_address != 0) {
                max_addr = static_cast<u16>(desc_.base_address + 0x2E);
            }
        }
        ranges_[0] = {min_addr, max_addr};
    }
}

std::span<const AddressRange> Tcd::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.base_address != 0 ? 1U : 0U};
}

void Tcd::reset() noexcept {
    ctrla_ = 0; ctrlb_ = 0; ctrlc_ = 0; ctrld_ = 0; ctrle_ = 0;
    evctrla_ = 0; evctrlB_ = 0;
    intctrl_ = 0; intflags_ = 0; status_ = 0;
    inputctrla_ = 0; inputctrlb_ = 0;
    faultctrl_ = 0; dlyctrl_ = 0; dlyval_ = 0;
    ditctrl_ = 0; ditval_ = 0; dbgctrl_ = 0;
    capturea_ = 0; captureb_ = 0;
    cmpaset_ = 0; cmpacl_ = 0; cmpbset_ = 0; cmpbcl_ = 0;
    counter_ = 0; accumulated_cycles_ = 0;
    enabled_ = false; int_pending_ = false;
}

u8 Tcd::reg_offset(u16 address) const noexcept {
    return static_cast<u8>(address - desc_.base_address);
}

u8 Tcd::read(u16 address) noexcept {
    u8 offset = reg_offset(address);
    if (offset == REG_CAPTUREA || offset == REG_CAPTUREA + 1) {
        return (offset == REG_CAPTUREA) ? (capturea_ & 0xFF) : ((capturea_ >> 8) & 0xFF);
    }
    if (offset == REG_CAPTUREB || offset == REG_CAPTUREB + 1) {
        return (offset == REG_CAPTUREB) ? (captureb_ & 0xFF) : ((captureb_ >> 8) & 0xFF);
    }
    if (offset == REG_CMPASET || offset == REG_CMPASET + 1) {
        return (offset == REG_CMPASET) ? (cmpaset_ & 0xFF) : ((cmpaset_ >> 8) & 0x0F);
    }
    if (offset == REG_CMPACLR || offset == REG_CMPACLR + 1) {
        return (offset == REG_CMPACLR) ? (cmpacl_ & 0xFF) : ((cmpacl_ >> 8) & 0x0F);
    }
    if (offset == REG_CMPBSET || offset == REG_CMPBSET + 1) {
        return (offset == REG_CMPBSET) ? (cmpbset_ & 0xFF) : ((cmpbset_ >> 8) & 0x0F);
    }
    if (offset == REG_CMPBCLR || offset == REG_CMPBCLR + 1) {
        return (offset == REG_CMPBCLR) ? (cmpbcl_ & 0xFF) : ((cmpbcl_ >> 8) & 0x0F);
    }
    switch (offset) {
    case REG_CTRLA: return ctrla_;
    case REG_CTRLB: return ctrlb_;
    case REG_CTRLC: return ctrlc_;
    case REG_CTRLD: return ctrld_;
    case REG_CTRLE: return ctrle_;
    case REG_EVCTRLA: return evctrla_;
    case REG_EVCTRLB: return evctrlB_;
    case REG_INTCTRL: return intctrl_;
    case REG_INTFLAGS: return intflags_;
    case REG_STATUS: return status_;
    case REG_INPUTCTRLA: return inputctrla_;
    case REG_INPUTCTRLB: return inputctrlb_;
    case REG_FAULTCTRL: return faultctrl_;
    case REG_DLYCTRL: return dlyctrl_;
    case REG_DLYVAL: return dlyval_;
    case REG_DITCTRL: return ditctrl_;
    case REG_DITVAL: return ditval_;
    case REG_DBGCTRL: return dbgctrl_;
    default: return 0;
    }
}

void Tcd::write(u16 address, u8 value) noexcept {
    u8 offset = reg_offset(address);
    if (offset > 0x2E) return;

    switch (offset) {
    case REG_CTRLA:
        ctrla_ = value & 0x67;
        enabled_ = (ctrla_ & CTRLA_ENABLE) != 0;
        status_ |= STATUS_ENRDY;
        if (!enabled_) counter_ = 0;
        break;
    case REG_CTRLB: ctrlb_ = value & 0x03; break;
    case REG_CTRLC: ctrlc_ = value & 0xCB; break;
    case REG_CTRLD: ctrld_ = value; break;
    case REG_CTRLE:
        ctrle_ = value;
        if (value & CTRLE_RESTART) counter_ = 0;
        if (value & CTRLE_SCAPTUREA) capturea_ = counter_;
        if (value & CTRLE_SCAPTUREB) captureb_ = counter_;
        if (value & CTRLE_DISEOC) { enabled_ = false; ctrla_ &= ~CTRLA_ENABLE; }
        break;
    case REG_EVCTRLA: evctrla_ = value & 0xC7; break;
    case REG_EVCTRLB: evctrlB_ = value & 0xC7; break;
    case REG_INTCTRL: intctrl_ = value & 0x0D; update_interrupt_pending(); break;
    case REG_INTFLAGS:
        intflags_ &= ~value;
        update_interrupt_pending();
        break;
    case REG_INPUTCTRLA: inputctrla_ = value & 0x0F; break;
    case REG_INPUTCTRLB: inputctrlb_ = value & 0x0F; break;
    case REG_FAULTCTRL: faultctrl_ = value; break;
    case REG_DLYCTRL: dlyctrl_ = value & 0x3F; break;
    case REG_DLYVAL: dlyval_ = value; break;
    case REG_DITCTRL: ditctrl_ = value & 0x03; break;
    case REG_DITVAL: ditval_ = value & 0x0F; break;
    case REG_DBGCTRL: dbgctrl_ = value & 0x05; break;
    case REG_CAPTUREA: case REG_CAPTUREA + 1: break; // Read-only
    case REG_CAPTUREB: case REG_CAPTUREB + 1: break; // Read-only
    case REG_CMPASET:
        cmpaset_ = (cmpaset_ & 0xF00) | value; break;
    case REG_CMPASET + 1:
        cmpaset_ = (cmpaset_ & 0x0FF) | ((static_cast<u16>(value & 0x0F)) << 8); break;
    case REG_CMPACLR:
        cmpacl_ = (cmpacl_ & 0xF00) | value; break;
    case REG_CMPACLR + 1:
        cmpacl_ = (cmpacl_ & 0x0FF) | ((static_cast<u16>(value & 0x0F)) << 8); break;
    case REG_CMPBSET:
        cmpbset_ = (cmpbset_ & 0xF00) | value; break;
    case REG_CMPBSET + 1:
        cmpbset_ = (cmpbset_ & 0x0FF) | ((static_cast<u16>(value & 0x0F)) << 8); break;
    case REG_CMPBCLR:
        cmpbcl_ = (cmpbcl_ & 0xF00) | value; break;
    case REG_CMPBCLR + 1:
        cmpbcl_ = (cmpbcl_ & 0x0FF) | ((static_cast<u16>(value & 0x0F)) << 8); break;
    default: break;
    }
}

void Tcd::tick(u64 elapsed_cycles) noexcept {
    if (!enabled_ || desc_.base_address == 0) return;
    run_counter(elapsed_cycles);
}

void Tcd::run_counter(u64 cycles) noexcept {
    u8 syncpres = (ctrla_ & CTRLA_SYNCPRES) >> 1;
    u8 cntpres = (ctrla_ & CTRLA_CNTPRES) >> 3;
    u16 prescaler = kSyncPrescalers[syncpres] * kCntPrescalers[cntpres];
    u64 total_ticks = cycles * kCounterClockCycles / prescaler;
    if (total_ticks == 0) return;

    u16 period = cmpbcl_;
    if (period == 0) period = 0x1000;

    for (u64 i = 0; i < total_ticks; ++i) {
        counter_++;

        u8 wgmode = ctrlb_ & 0x03;
        if (wgmode == WGMODE_ONERAMP) {
            if (cmpaset_ > 0 && counter_ == cmpaset_) {
                intflags_ |= INT_TRIGA;
                update_interrupt_pending();
            }
            if (cmpbset_ > 0 && counter_ == cmpbset_) {
                intflags_ |= INT_TRIGB;
                update_interrupt_pending();
            }
            if (counter_ >= period) {
                counter_ = 0;
                intflags_ |= INT_OVF;
                update_interrupt_pending();
            }
        } else {
            // Simplified: count to period and reset
            if (counter_ >= period) {
                counter_ = 0;
                intflags_ |= INT_OVF;
                update_interrupt_pending();
            }
        }
    }
    update_outputs();
}

bool Tcd::get_wo_level(u8 index) const noexcept {
    if (!enabled_) return false;
    u8 wgmode = ctrlb_ & 0x03;
    if (wgmode == WGMODE_ONERAMP) {
        switch (index) {
        case 0: // WOA: high between CMPASET and CMPACLR
            if (cmpaset_ == 0) return false;
            if (cmpacl_ == 0) return counter_ >= cmpaset_;
            return counter_ >= cmpaset_ && counter_ < cmpacl_;
        case 1: // WOB: high between CMPBSET and CMPBCLR
            if (cmpbset_ == 0) return false;
            if (cmpbcl_ == 0) return counter_ >= cmpbset_;
            return counter_ >= cmpbset_ && counter_ < cmpbcl_;
        default:
            return false;
        }
    }
    // Other modes: simplified — single slope compare
    if (index == 0 && cmpaset_ > 0) return counter_ < cmpaset_;
    if (index == 1 && cmpbset_ > 0) return counter_ < cmpbset_;
    return false;
}

void Tcd::update_outputs() noexcept {
    if (!pin_mux_) return;
    auto drive = [&](u16 addr, u8 bit, u8 index) {
        if (addr == 0) return;
        bool level = get_wo_level(index);
        pin_mux_->update_pin_by_address(addr, bit, PinOwner::timer, true, level);
    };
    drive(desc_.woa_pin_address, desc_.woa_pin_bit, 0);
    drive(desc_.wob_pin_address, desc_.wob_pin_bit, 1);
    drive(desc_.woc_pin_address, desc_.woc_pin_bit, 2);
    drive(desc_.wod_pin_address, desc_.wod_pin_bit, 3);
}

bool Tcd::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (!int_pending_) return false;
    if ((intctrl_ & INT_OVF) && (intflags_ & INT_OVF)) {
        request = InterruptRequest{.vector_index = desc_.ovf_vector_index, .source_id = 0U};
        return true;
    }
    if ((intctrl_ & (INT_TRIGA | INT_TRIGB)) && (intflags_ & (INT_TRIGA | INT_TRIGB))) {
        request = InterruptRequest{.vector_index = desc_.trig_vector_index, .source_id = 0U};
        return true;
    }
    return false;
}

bool Tcd::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!int_pending_) return false;
    if ((intctrl_ & INT_OVF) && (intflags_ & INT_OVF)) {
        request = InterruptRequest{.vector_index = desc_.ovf_vector_index, .source_id = 0U};
        intflags_ &= ~INT_OVF;
        update_interrupt_pending();
        return true;
    }
    if ((intctrl_ & (INT_TRIGA | INT_TRIGB)) && (intflags_ & (INT_TRIGA | INT_TRIGB))) {
        request = InterruptRequest{.vector_index = desc_.trig_vector_index, .source_id = 0U};
        intflags_ &= ~(INT_TRIGA | INT_TRIGB);
        update_interrupt_pending();
        return true;
    }
    return false;
}

void Tcd::update_interrupt_pending() noexcept {
    int_pending_ = false;
    if ((intctrl_ & INT_OVF) && (intflags_ & INT_OVF)) int_pending_ = true;
    if ((intctrl_ & INT_TRIGA) && (intflags_ & INT_TRIGA)) int_pending_ = true;
    if ((intctrl_ & INT_TRIGB) && (intflags_ & INT_TRIGB)) int_pending_ = true;
}

} // namespace vioavr::core
