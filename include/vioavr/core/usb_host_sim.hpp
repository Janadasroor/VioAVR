#pragma once

#include "vioavr/core/usb.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace vioavr::core {

class UsbHostCaterina {
public:
    enum class State : u8 {
        Init,
        EnumGetDescriptorDev,
        EnumGetDescriptorDevStatus,
        EnumSetAddress,
        EnumGetDescriptorDev2,
        EnumGetDescriptorDev2Status,
        EnumGetDescriptorCfg,
        EnumGetDescriptorCfgStatus,
        EnumSetConfig,
        CdcSetLineCoding,
        CdcSetControlLineState,
        Avr109ProgEnter,
        Avr109Identify,
        Avr109ChipType,
        Avr109Signature,
        Avr109Version,
        Avr109ChipErase,
        Avr109SetAddr,
        Avr109BlockWrite,
        Avr109BlockWriteData,
        Avr109Exit,
        Done
    };

    UsbHostCaterina(Usb* usb, MemoryBus* bus) noexcept;

    void set_sketch_data(std::vector<uint8_t> data) noexcept { sketch_data_ = std::move(data); }

    [[nodiscard]] bool is_done() const noexcept { return state_ == State::Done; }
    [[nodiscard]] State state() const noexcept { return state_; }

    void tick(u64 cpu_cycles) noexcept;

private:
    Usb* usb_;
    MemoryBus* bus_;
    const UsbDescriptor& desc_;

    State state_{State::Init};
    State last_logged_state_{State::Init};
    std::vector<uint8_t> sketch_data_;
    u64 last_event_cycle_{0};
    u64 state_entered_cycle_{0};  // cycle at which we entered the current state

    int sub_state_{0};

    bool ep_toggle_[7]{};

    // AVR109 state
    u32 flash_word_addr_{0};
    u32 prog_offset_{0};
    u32 page_write_size_{0};

    // Control transfer state: did the host send an OUT data stage?
    // When true, only busy=true (explicit commit) counts as status-stage ready.
    // When false, TXINI=1 alone (auto-set after ClearSETUP) counts as ZLP-ready.
    bool pending_out_data_stage_{false};

    // USB register helpers
    [[nodiscard]] u8 read_ueintx(u8 ep) noexcept;
    [[nodiscard]] u16 read_uebcx(u8 ep) noexcept;
    [[nodiscard]] u8 read_ueconx(u8 ep) noexcept;
    void select_ep(u8 ep) noexcept;

    // USB event injection
    void send_setup_packet(u8 bmReqType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength) noexcept;
    void send_in_token(u8 ep) noexcept;
    void send_out_packet(u8 ep, std::span<const uint8_t> data) noexcept;
    [[nodiscard]] std::vector<uint8_t> read_ep_data(u8 ep) const noexcept;
    [[nodiscard]] bool is_ep_busy(u8 ep) const noexcept;
    [[nodiscard]] bool is_ep_response_ready(u8 ep) noexcept;

    void transition(State next, u64 cpu_cycles) noexcept {
        if (next != state_) {
            std::fprintf(stderr, "[USB-HOST] state %d -> %d at cyc=%lu\n",
                (int)state_, (int)next, (unsigned long)cpu_cycles);
            state_ = next;
            state_entered_cycle_ = cpu_cycles;
            sub_state_ = 0;
            pending_out_data_stage_ = false; // reset per-state control transfer tracking
        }
    }

    static constexpr u64 kMinEventInterval = 500;    // min CPU cycles between host ticks
    static constexpr u64 kSetupWait       = 10000;   // cycles to wait after sending a SETUP
    static constexpr u64 kDataWait        = 5000;    // cycles to wait for data/ack
    static constexpr u64 kStateTimeout    = 5'000'000; // give up on a state after 5M cycles
    static constexpr u8 kTxini   = 0x01;
    static constexpr u8 kRxouti  = 0x04;
    static constexpr u8 kRxstpi  = 0x08;
    static constexpr u8 kFifocon = 0x80;
};

// ---- Inline implementations ----

inline UsbHostCaterina::UsbHostCaterina(Usb* usb, MemoryBus* bus) noexcept
    : usb_(usb), bus_(bus), desc_(bus->device().usbs[0])
{}

inline u8 UsbHostCaterina::read_ueintx(u8 ep) noexcept {
    select_ep(ep);
    return bus_->read_data(desc_.ueintx_address);
}

inline u16 UsbHostCaterina::read_uebcx(u8 ep) noexcept {
    select_ep(ep);
    u16 lo = bus_->read_data(desc_.uebclx_address);
    u16 hi = bus_->read_data(desc_.uebchx_address);
    return static_cast<u16>((hi << 8) | lo);
}

inline u8 UsbHostCaterina::read_ueconx(u8 ep) noexcept {
    select_ep(ep);
    return bus_->read_data(desc_.ueconx_address);
}

inline void UsbHostCaterina::select_ep(u8 ep) noexcept {
    bus_->write_data(desc_.uenum_address, ep);
    // NOTE: do NOT reset last_event_cycle_ here — that would bypass the tick-rate
    // limiter and starve the CPU of cycles, causing a hang.
}

inline void UsbHostCaterina::send_setup_packet(u8 bmReqType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength) noexcept {
    Usb::SetupPacket sp{bmReqType, bRequest, wValue, wIndex, wLength};
    usb_->simulate_setup_packet(sp);
}

inline void UsbHostCaterina::send_in_token(u8 ep) noexcept {
    // Clear the OUT-data-stage flag when we send the status IN token
    if (ep == 0) pending_out_data_stage_ = false;
    usb_->simulate_in_token(ep);
}

inline void UsbHostCaterina::send_out_packet(u8 ep, std::span<const uint8_t> data) noexcept {
    bool d1 = ep_toggle_[ep];
    usb_->simulate_out_packet(ep, d1, data);
    ep_toggle_[ep] = !d1;
    // Track that we sent a non-empty OUT data stage on EP0 (control transfer).
    // The firmware must explicitly ACK via Endpoint_ClearIN() (busy=true).
    // TXINI=1 alone is NOT sufficient — that would be a premature status trigger.
    if (ep == 0 && !data.empty())
        pending_out_data_stage_ = true;
}

inline std::vector<uint8_t> UsbHostCaterina::read_ep_data(u8 ep) const noexcept {
    return usb_->get_endpoint_data(ep);
}

inline bool UsbHostCaterina::is_ep_busy(u8 ep) const noexcept {
    return usb_->is_endpoint_busy(ep);
}

inline bool UsbHostCaterina::is_ep_response_ready(u8 ep) noexcept {
    // Returns true when the endpoint has a committed response ready for the host to take.
    // Two cases depending on control transfer type:
    //   1. No-data control (e.g. SET_CONFIG, SET_ADDRESS, SET_CTRL_LINE_STATE):
    //      TXINI=1 + RXSTPI=0 is sufficient. SIE auto-sets TXINI after ClearSETUP,
    //      and the firmware does NOT need to call Endpoint_ClearIN() for the ZLP.
    //   2. Data-OUT control (e.g. SET_LINE_CODING with 7-byte OUT data):
    //      Only busy=true (explicit FIFOCON commit via Endpoint_ClearIN()) counts.
    //      Must wait for firmware to read the OUT data and ACK via status ZLP commit.
    select_ep(ep);
    u8 ueintx = bus_->read_data(desc_.ueintx_address);
    bool busy = usb_->is_endpoint_busy(ep);
    bool rxstpi = (ueintx & desc_.ueintx_rxstpi_mask) != 0;
    bool txini  = (ueintx & desc_.ueintx_txini_mask) != 0;
    // Debug: print periodically for diagnostic states
    static u64 s_last_dbg = 0;
    bool is_debug_state = (state_ == State::EnumSetConfig ||
                           state_ == State::CdcSetControlLineState ||
                           state_ == State::CdcSetLineCoding ||
                           state_ == State::EnumSetAddress);
    if ((state_ == State::CdcSetLineCoding) || (is_debug_state && (last_event_cycle_ - s_last_dbg > 200000 || s_last_dbg == 0))) {
        s_last_dbg = last_event_cycle_;
        std::fprintf(stderr, "[DBG-S%d] is_ep_resp_ready ep=%d ueintx=0x%02x rxstpi=%d txini=%d busy=%d pending_out=%d cyc=%lu\n",
            (int)state_, ep, ueintx, rxstpi, txini, busy, pending_out_data_stage_, (unsigned long)bus_->cpu_cycles());
    }
    if (rxstpi)
        return false; // SETUP still pending, firmware hasn't read it yet
    if (pending_out_data_stage_)
        return busy; // Must wait for explicit commit after firmware reads OUT data
    // No-data control: TXINI=1 (auto ZLP) or explicit busy commit both count
    return txini || busy;
}


inline void UsbHostCaterina::tick(u64 cpu_cycles) noexcept {
    if (cpu_cycles < last_event_cycle_ + kMinEventInterval)
        return;

    // Per-state timeout: if stuck in a state for too long, give up to prevent hang.
    if (state_ != State::Init && state_ != State::Done &&
        state_entered_cycle_ > 0 &&
        (cpu_cycles - state_entered_cycle_) > kStateTimeout) {
        std::fprintf(stderr, "[USB-HOST] TIMEOUT state=%d after %lu cycles. Aborting.\n",
            (int)state_, (unsigned long)(cpu_cycles - state_entered_cycle_));
        state_ = State::Done;
        return;
    }

    switch (state_) {
    // ============================================================
    //  USB ENUMERATION
    // ============================================================
    case State::Init: {
        u8 ec = read_ueconx(0);
        if (ec & 0x01) {
            // EP0 enabled — start enumeration
            send_setup_packet(0x80, 6, 0x0100, 0, 18);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            transition(State::EnumGetDescriptorDev, cpu_cycles);
            std::fprintf(stderr, "[USB-HOST] EP0 enabled -> GetDescDev cyc=%lu\n",
                (unsigned long)cpu_cycles);
        } else {
            last_event_cycle_ = cpu_cycles; // retry in kMinEventInterval
            if (cpu_cycles % 50000 < kMinEventInterval)
                std::fprintf(stderr, "[USB-HOST] Init waiting cyc=%lu ec=%02x\n",
                    (unsigned long)cpu_cycles, ec);
        }
        break;
    }

    case State::EnumGetDescriptorDev: {
        // Wait for firmware to load response into EP0 (RXSTPI cleared + TXINI/busy set)
        if (is_ep_response_ready(0)) {
            send_in_token(0);
            last_event_cycle_ = cpu_cycles + kDataWait;
            transition(State::EnumGetDescriptorDevStatus, cpu_cycles);
        } else {
            last_event_cycle_ = cpu_cycles + kMinEventInterval;
        }
        break;
    }

    case State::EnumGetDescriptorDevStatus: {
        if (sub_state_ == 0) {
            last_event_cycle_ = cpu_cycles + kDataWait;  // let firmware finish the IN
            sub_state_ = 1;
        } else if (sub_state_ == 1) {
            send_out_packet(0, {});  // status OUT ZLP
            sub_state_ = 2;
            last_event_cycle_ = cpu_cycles + kDataWait;
        } else {
            transition(State::EnumSetAddress, cpu_cycles);
        }
        break;
    }

    case State::EnumSetAddress: {
        if (sub_state_ == 0) {
            send_setup_packet(0x00, 5, 1, 0, 0);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
        } else if (is_ep_response_ready(0)) {
            send_in_token(0);
            last_event_cycle_ = cpu_cycles + kDataWait;
            transition(State::EnumGetDescriptorDev2, cpu_cycles);
        } else {
            last_event_cycle_ = cpu_cycles + kMinEventInterval;
        }
        break;
    }

    case State::EnumGetDescriptorDev2: {
        if (sub_state_ == 0) {
            send_setup_packet(0x80, 6, 0x0100, 0, 18);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
        } else if (is_ep_response_ready(0)) {
            send_in_token(0);
            last_event_cycle_ = cpu_cycles + kDataWait;
            transition(State::EnumGetDescriptorDev2Status, cpu_cycles);
        } else {
            last_event_cycle_ = cpu_cycles + kMinEventInterval;
        }
        break;
    }

    case State::EnumGetDescriptorDev2Status: {
        if (sub_state_ == 0) {
            last_event_cycle_ = cpu_cycles + kDataWait;  // let firmware finish the IN
            sub_state_ = 1;
        } else if (sub_state_ == 1) {
            send_out_packet(0, {});  // status OUT ZLP
            sub_state_ = 2;
            last_event_cycle_ = cpu_cycles + kDataWait;
        } else {
            transition(State::EnumGetDescriptorCfg, cpu_cycles);
        }
        break;
    }

    case State::EnumGetDescriptorCfg: {
        if (sub_state_ == 0) {
            send_setup_packet(0x80, 6, 0x0200, 0, 255); // GET_DESCRIPTOR Configuration
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
        } else if (sub_state_ == 1) {
            // Read data packets until firmware has no more to send
            if (is_ep_response_ready(0)) {
                send_in_token(0);
                // Check immediately if more data is coming (don't transition yet)
                sub_state_ = 2;
                last_event_cycle_ = cpu_cycles + kDataWait;
            } else {
                last_event_cycle_ = cpu_cycles + kMinEventInterval;
            }
        } else if (sub_state_ == 2) {
            // Check if more data is ready: firmware must commit (busy=true) for more data.
            // TXINI=1 alone is NOT sufficient (that's just the post-ACK re-assertion).
            if (is_ep_busy(0)) {
                // More data committed — read another packet
                send_in_token(0);
                last_event_cycle_ = cpu_cycles + kDataWait;
                // stay in sub_state_=2
            } else {
                // No committed data — config descriptor fully received
                transition(State::EnumGetDescriptorCfgStatus, cpu_cycles);
            }
        }
        break;
    }

    case State::EnumGetDescriptorCfgStatus: {
        if (sub_state_ == 0) {
            last_event_cycle_ = cpu_cycles + kDataWait;  // let firmware finish the IN
            sub_state_ = 1;
        } else if (sub_state_ == 1) {
            send_out_packet(0, {});  // status OUT ZLP
            sub_state_ = 2;
            last_event_cycle_ = cpu_cycles + kDataWait;
        } else {
            transition(State::EnumSetConfig, cpu_cycles);
        }
        break;
    }

    case State::EnumSetConfig: {
        if (sub_state_ == 0) {
            send_setup_packet(0x00, 9, 1, 0, 0); // SET_CONFIGURATION = 1
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
        } else if (is_ep_response_ready(0)) {
            std::fprintf(stderr, "[SETCFG] cyc=%lu sending IN token\n", (unsigned long)cpu_cycles);
            send_in_token(0);
            last_event_cycle_ = cpu_cycles + kDataWait;
            transition(State::CdcSetLineCoding, cpu_cycles);
        } else {
            last_event_cycle_ = cpu_cycles + kMinEventInterval;
        }
        break;
    }

    case State::CdcSetLineCoding: {
        if (sub_state_ == 0) {
            // CDC SET_LINE_CODING request (wLength=7 for 7-byte line coding struct)
            send_setup_packet(0x21, 0x20, 0, 0, 7);
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else if (sub_state_ == 1) {
            // Data stage: 7-byte line coding (9600 baud, 8N1)
            uint8_t lc[7] = {0x80, 0x25, 0, 0, 0, 0, 0x08};
            send_out_packet(0, lc);
            sub_state_ = 2;
            last_event_cycle_ = cpu_cycles + kDataWait;
        } else if (is_ep_response_ready(0)) {
            // Status stage: firmware ACKs the OUT data via TXINI
            send_in_token(0);
            last_event_cycle_ = cpu_cycles + kDataWait;
            transition(State::CdcSetControlLineState, cpu_cycles);
        } else {
            last_event_cycle_ = cpu_cycles + kMinEventInterval;
        }
        break;
    }

    case State::CdcSetControlLineState: {
        if (sub_state_ == 0) {
            // CDC SET_CONTROL_LINE_STATE (DTR=1, RTS=1 -> wValue=3)
            send_setup_packet(0x21, 0x22, 3, 0, 0);
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else if (is_ep_response_ready(0)) {
            send_in_token(0);
            last_event_cycle_ = cpu_cycles + kDataWait;
            transition(State::Avr109ProgEnter, cpu_cycles);
        } else {
            last_event_cycle_ = cpu_cycles + kMinEventInterval;
        }
        break;
    }

    case State::Avr109ProgEnter: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'P'});
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else {
            auto data = read_ep_data(3);
            if (!data.empty() && data[0] == '\r') {
                send_in_token(3);
                transition(State::Avr109Identify, cpu_cycles);
                last_event_cycle_ = cpu_cycles + kDataWait;
            }
        }
        break;
    }

    case State::Avr109Identify: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'S'});
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else {
            auto data = read_ep_data(3);
            if (data.size() >= 7) {
                send_in_token(3);
                transition(State::Avr109ChipType, cpu_cycles);
                last_event_cycle_ = cpu_cycles + kDataWait;
            }
        }
        break;
    }

    case State::Avr109ChipType: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'t'});
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else {
            auto data = read_ep_data(3);
            if (data.size() >= 2) {
                send_in_token(3);
                transition(State::Avr109Signature, cpu_cycles);
                last_event_cycle_ = cpu_cycles + kDataWait;
            }
        }
        break;
    }

    case State::Avr109Signature: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'s'});
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else {
            auto data = read_ep_data(3);
            if (data.size() >= 2) {
                send_in_token(3);
                transition(State::Avr109Version, cpu_cycles);
                last_event_cycle_ = cpu_cycles + kDataWait;
            }
        }
        break;
    }

    case State::Avr109Version: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'V'});
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else {
            auto data = read_ep_data(3);
            if (data.size() >= 2) {
                send_in_token(3);
                transition(State::Avr109ChipErase, cpu_cycles);
                last_event_cycle_ = cpu_cycles + kDataWait;
            }
        }
        break;
    }

    case State::Avr109ChipErase: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'e'});
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else {
            auto data = read_ep_data(3);
            if (!data.empty() && data[0] == '\r') {
                send_in_token(3);
                flash_word_addr_ = 0;
                prog_offset_ = 0;
                transition(State::Avr109SetAddr, cpu_cycles);
                last_event_cycle_ = cpu_cycles + kDataWait;
            }
        }
        break;
    }

    case State::Avr109SetAddr: {
        if (flash_word_addr_ * 2 >= sketch_data_.size()) {
            transition(State::Avr109Exit, cpu_cycles);
            last_event_cycle_ = cpu_cycles + kDataWait;
        } else if (sub_state_ == 0) {
            u8 addr_hi = static_cast<u8>((flash_word_addr_ >> 8) & 0xFF);
            u8 addr_lo = static_cast<u8>(flash_word_addr_ & 0xFF);
            send_out_packet(2, std::vector<uint8_t>{'A', addr_hi, addr_lo});
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else {
            auto data = read_ep_data(3);
            if (!data.empty() && data[0] == '\r') {
                send_in_token(3);
                transition(State::Avr109BlockWrite, cpu_cycles);
                last_event_cycle_ = cpu_cycles + kDataWait;
            }
        }
        break;
    }

    case State::Avr109BlockWrite: {
        u32 page_start = flash_word_addr_ * 2;
        u32 page_size = 256;
        if (page_start + page_size > sketch_data_.size()) page_size = sketch_data_.size() - page_start;
        if (page_size == 0) {
            transition(State::Avr109Exit, cpu_cycles);
            last_event_cycle_ = cpu_cycles + kDataWait;
        } else {
            page_write_size_ = page_size;
            state_ = State::Avr109BlockWriteData;
            sub_state_ = 0;
            last_event_cycle_ = cpu_cycles;
        }
        break;
    }

    case State::Avr109BlockWriteData: {
        u32 page_start = flash_word_addr_ * 2;
        if (sub_state_ == 0) {
            u32 chunk = (page_write_size_ > 12) ? 12 : page_write_size_;
            std::vector<uint8_t> pkt{'B', (u8)(page_write_size_ >> 8), (u8)(page_write_size_ & 0xFF), 'F'};
            for (u32 i = 0; i < chunk; ++i) pkt.push_back(sketch_data_[page_start + i]);
            send_out_packet(2, pkt);
            prog_offset_ = chunk;
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else if (sub_state_ == 1) {
            auto resp = read_ep_data(3);
            if (!resp.empty() && resp[0] == '\r') {
                send_in_token(3);
                flash_word_addr_ += 128;
                transition(State::Avr109SetAddr, cpu_cycles);
                last_event_cycle_ = cpu_cycles + kDataWait;
            } else if (prog_offset_ < page_write_size_) {
                u32 chunk = (page_write_size_ - prog_offset_ > 16) ? 16 : (page_write_size_ - prog_offset_);
                std::vector<uint8_t> pkt;
                for (u32 i = 0; i < chunk; ++i) pkt.push_back(sketch_data_[page_start + prog_offset_ + i]);
                send_out_packet(2, pkt);
                prog_offset_ += chunk;
                last_event_cycle_ = cpu_cycles + kDataWait;
            } else sub_state_ = 2;
        } else {
            auto data = read_ep_data(3);
            if (!data.empty() && data[0] == '\r') {
                send_in_token(3);
                flash_word_addr_ += 128;
                transition(State::Avr109SetAddr, cpu_cycles);
                last_event_cycle_ = cpu_cycles + kDataWait;
            }
        }
        break;
    }

    case State::Avr109Exit: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'E'});
            sub_state_ = 1;
            last_event_cycle_ = cpu_cycles + kSetupWait;
        } else {
            auto data = read_ep_data(3);
            if (!data.empty() && data[0] == '\r') {
                send_in_token(3);
                state_ = State::Done;
            }
        }
        break;
    }
    case State::Done: break;
    }
}

} // namespace vioavr::core
