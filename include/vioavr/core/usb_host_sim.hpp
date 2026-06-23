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
    std::vector<uint8_t> sketch_data_;
    u64 last_event_cycle_{0};

    int sub_state_{0};

    bool ep_toggle_[7]{};

    // AVR109 state
    u32 flash_word_addr_{0};
    u32 prog_offset_{0};
    u32 page_write_size_{0};

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

    static constexpr u64 kMinEventInterval = 200;
    static constexpr u64 kSetupWait = 3000;
    static constexpr u64 kDataWait = 1000;
    static constexpr u8 kTxini = 0x01;
    static constexpr u8 kRxouti = 0x04;
    static constexpr u8 kRxstpi = 0x08;
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
    last_event_cycle_ = 0;
}

inline void UsbHostCaterina::send_setup_packet(u8 bmReqType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength) noexcept {
    Usb::SetupPacket sp{bmReqType, bRequest, wValue, wIndex, wLength};
    usb_->simulate_setup_packet(sp);
}

inline void UsbHostCaterina::send_in_token(u8 ep) noexcept {
    usb_->simulate_in_token(ep);
}

inline void UsbHostCaterina::send_out_packet(u8 ep, std::span<const uint8_t> data) noexcept {
    bool d1 = ep_toggle_[ep];
    usb_->simulate_out_packet(ep, d1, data);
    ep_toggle_[ep] = !d1;
}

inline std::vector<uint8_t> UsbHostCaterina::read_ep_data(u8 ep) const noexcept {
    return usb_->get_endpoint_data(ep);
}

inline bool UsbHostCaterina::is_ep_busy(u8 ep) const noexcept {
    return usb_->is_endpoint_busy(ep);
}

inline bool UsbHostCaterina::is_ep_response_ready(u8 ep) noexcept {
    // Returns true when firmware has read the SETUP (RXSTPI cleared) AND has
    // data ready for the host (busy || byte_count > 0). This avoids confusing
    // pending SETUP data (byte_count=8, RXSTPI set) with a firmware response.
    select_ep(ep);
    u8 ueintx = bus_->read_data(desc_.ueintx_address);
    if (ueintx & desc_.ueintx_rxstpi_mask)
        return false; // SETUP still pending, firmware hasn't read it yet
    u8 bc = bus_->read_data(desc_.uebclx_address);
    bool busy = usb_->is_endpoint_busy(ep);
    static int dbg = 0;
    if (dbg++ < 5)
        std::fprintf(stderr, "[RESP-RDY] ep=%d ueintx=%02x bc=%d busy=%d result=%d\n",
            ep, ueintx, bc, busy, bc > 0 || busy);
    return bc > 0 || busy;
}

inline void UsbHostCaterina::tick(u64 cpu_cycles) noexcept {
    if (cpu_cycles < last_event_cycle_ + kMinEventInterval)
        return;

    switch (state_) {
    // ============================================================
    //  USB ENUMERATION
    // ============================================================
    case State::Init: {
        u8 ec = read_ueconx(0);
        std::fprintf(stderr, "[USB-HOST] Init cyc=%lu ec=%02x state=%d\n",
            (unsigned long)cpu_cycles, ec, (int)state_);
        if (ec & 0x01) {
            send_setup_packet(0x80, 6, 0x0100, 0, 18);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            state_ = State::EnumGetDescriptorDev;
            std::fprintf(stderr, "[USB-HOST] -> GetDescDev cyc=%lu\n", (unsigned long)cpu_cycles);
        } else if (cpu_cycles > 40000) {
            static bool once = false;
            if (!once) { once = true;
                std::fprintf(stderr, "[USB-INIT] cyc=%lu ec=%02x (EP0 not enabled yet)\n",
                    (unsigned long)cpu_cycles, ec);
            }
        }
        break;
    }

    case State::EnumGetDescriptorDev: {
        // Wait for firmware to load data into EP0 (bank busy = firmware data ready)
        bool busy = is_ep_response_ready(0);
        if ((cpu_cycles > 39020 && cpu_cycles < 49000)) {
            select_ep(0);
            u8 ec_now = bus_->read_data(desc_.ueconx_address);
            u8 ueintx_now = bus_->read_data(desc_.ueintx_address);
            u8 bc = bus_->read_data(desc_.uebclx_address);
            u8 bsy_val = bus_->read_data(desc_.uesta1x_address);
            std::fprintf(stderr, "[USB-GETD] cyc=%lu busy=%d ec=%02x ueintx=%02x bc=%02x uesta1=%02x\n",
                (unsigned long)cpu_cycles, busy, ec_now, ueintx_now, bc, bsy_val);
        }
        if (busy) {
            send_in_token(0);
            last_event_cycle_ = cpu_cycles;
            state_ = State::EnumGetDescriptorDevStatus;
        } else if (cpu_cycles > 50000) {
            static int dbg = 0;
            if (++dbg < 5) {
                select_ep(0);
                u8 ec_now = bus_->read_data(desc_.ueconx_address);
                u8 ueintx_now = bus_->read_data(desc_.ueintx_address);
                u8 con = bus_->read_data(desc_.usbcon_address);
                u8 udint_val = bus_->read_data(desc_.udint_address);
                u8 udien_val = bus_->read_data(desc_.udien_address);
                u8 udaddr = bus_->read_data(desc_.udaddr_address);
                u8 sreg = bus_->read_data(bus_->device().sreg_address);
                std::fprintf(stderr, "[USB-DBG] cyc=%lu ec=%02x ueintx=%02x con=%02x udint=%02x udien=%02x uaddr=%02x sreg=%02x\n",
                    (unsigned long)cpu_cycles, ec_now, ueintx_now, con, udint_val, udien_val, udaddr, sreg);
            }
        }
        break;
    }

    case State::EnumGetDescriptorDevStatus: {
        if (sub_state_ == 0) {
            last_event_cycle_ = cpu_cycles + kDataWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            send_out_packet(0, {});
            last_event_cycle_ = cpu_cycles + kDataWait;
            state_ = State::EnumSetAddress;
            sub_state_ = 0;
            break;
        }
        break;
    }

    case State::EnumSetAddress: {
        if (sub_state_ == 0) {
            send_setup_packet(0x00, 5, 1, 0, 0);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            // Wait for firmware to prepare status stage (bank busy)
            if (is_ep_response_ready(0)) {
                send_in_token(0);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::EnumGetDescriptorDev2;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    case State::EnumGetDescriptorDev2: {
        if (sub_state_ == 0) {
            send_setup_packet(0x80, 6, 0x0100, 0, 18);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            if (is_ep_response_ready(0)) {
                send_in_token(0);
                last_event_cycle_ = cpu_cycles;
                sub_state_ = 0;
                state_ = State::EnumGetDescriptorDev2Status;
            }
            break;
        }
        break;
    }

    case State::EnumGetDescriptorDev2Status: {
        if (sub_state_ == 0) {
            last_event_cycle_ = cpu_cycles + kDataWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            send_out_packet(0, {});
            last_event_cycle_ = cpu_cycles + kDataWait;
            state_ = State::EnumGetDescriptorCfg;
            sub_state_ = 0;
            break;
        }
        break;
    }

    case State::EnumGetDescriptorCfg: {
        if (sub_state_ == 0) {
            send_setup_packet(0x80, 6, 0x0200, 0, 255); // Get config descriptor
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            if (is_ep_response_ready(0)) {
                send_in_token(0);
                last_event_cycle_ = cpu_cycles;
                sub_state_ = 0;
                state_ = State::EnumGetDescriptorCfgStatus;
            }
            break;
        }
        break;
    }

    case State::EnumGetDescriptorCfgStatus: {
        if (sub_state_ == 0) {
            last_event_cycle_ = cpu_cycles + kDataWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            send_out_packet(0, {});
            last_event_cycle_ = cpu_cycles + kDataWait;
            state_ = State::EnumSetConfig;
            sub_state_ = 0;
            break;
        }
        break;
    }

    case State::EnumSetConfig: {
        if (sub_state_ == 0) {
            send_setup_packet(0x00, 9, 1, 0, 0);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            if (is_ep_response_ready(0)) {
                std::fprintf(stderr, "[SETCFG-BUSY] cyc=%lu busy=1 sending IN token\n", (unsigned long)cpu_cycles);
                send_in_token(0);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::CdcSetLineCoding;
                sub_state_ = 0;
            } else if (cpu_cycles % 2000 < 100) {
                select_ep(0);
                u8 ec = bus_->read_data(desc_.ueconx_address);
                u8 fl = bus_->read_data(desc_.ueintx_address);
                u8 ei = bus_->read_data(desc_.ueienx_address);
                u8 bc = bus_->read_data(desc_.uebclx_address);
                u8 sta1 = bus_->read_data(desc_.uesta1x_address);
                u8 con = bus_->read_data(desc_.usbcon_address);
                std::fprintf(stderr, "[SETCFG-WAIT] cyc=%lu ec=%02x fl=%02x ei=%02x bc=%02x sta1=%02x con=%02x\n",
                    (unsigned long)cpu_cycles, ec, fl, ei, bc, sta1, con);
            }
            break;
        }
        break;
    }

    case State::CdcSetLineCoding: {
        if (sub_state_ == 0) {
            send_setup_packet(0x21, 0x20, 0, 0, 8);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            // Data stage: send line coding bytes
            uint8_t lc[8] = {0x80, 0x25, 0, 0, 0, 0, 0x08, 0};
            send_out_packet(0, lc);
            last_event_cycle_ = cpu_cycles + kDataWait;
            sub_state_ = 2;
            break;
        }
        if (sub_state_ == 2) {
            if (is_ep_busy(0)) {
                send_in_token(0);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::CdcSetControlLineState;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    case State::CdcSetControlLineState: {
        if (sub_state_ == 0) {
            send_setup_packet(0x21, 0x22, 3, 0, 0);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            if (is_ep_response_ready(0)) {
                send_in_token(0);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::Avr109ProgEnter;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    // ============================================================
    //  AVR109 PROTOCOL  (EP2=OUT for commands, EP3=IN for responses)
    // ============================================================
    case State::Avr109ProgEnter: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'P'});
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            auto data = read_ep_data(3);
            if (!data.empty() && data[0] == '\r') {
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::Avr109Identify;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    case State::Avr109Identify: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'S'});
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            auto data = read_ep_data(3);
            if (data.size() >= 7) {
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::Avr109ChipType;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    case State::Avr109ChipType: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'t'});
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            auto data = read_ep_data(3);
            if (data.size() >= 2) {
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::Avr109Signature;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    case State::Avr109Signature: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'s'});
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            auto data = read_ep_data(3);
            if (data.size() >= 2) {
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::Avr109Version;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    case State::Avr109Version: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'V'});
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            auto data = read_ep_data(3);
            if (data.size() >= 2) {
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::Avr109ChipErase;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    case State::Avr109ChipErase: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'e'});
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            auto data = read_ep_data(3);
            if (data.size() >= 1 && data[0] == '\r') {
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::Avr109SetAddr;
                sub_state_ = 0;
                flash_word_addr_ = 0;
                prog_offset_ = 0;
            }
            break;
        }
        break;
    }

    case State::Avr109SetAddr: {
        if (flash_word_addr_ * 2 >= sketch_data_.size()) {
            state_ = State::Avr109Exit;
            sub_state_ = 0;
            break;
        }
        if (sub_state_ == 0) {
            u8 addr_hi = static_cast<u8>((flash_word_addr_ >> 8) & 0xFF);
            u8 addr_lo = static_cast<u8>(flash_word_addr_ & 0xFF);
            send_out_packet(2, std::vector<uint8_t>{'A', addr_hi, addr_lo});
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            auto data = read_ep_data(3);
            if (!data.empty() && data[0] == '\r') {
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::Avr109BlockWrite;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    case State::Avr109BlockWrite: {
        u32 page_start = flash_word_addr_ * 2;
        u32 page_size = 256;
        if (page_start + page_size > sketch_data_.size()) {
            page_size = sketch_data_.size() - page_start;
        }
        if (page_size == 0) {
            state_ = State::Avr109Exit;
            sub_state_ = 0;
            break;
        }
        page_write_size_ = page_size;
        state_ = State::Avr109BlockWriteData;
        sub_state_ = 0;
        break;
    }

    case State::Avr109BlockWriteData: {
        u32 page_start = flash_word_addr_ * 2;

        if (sub_state_ == 0) {
            u32 bytes_left = page_write_size_;
            u32 chunk = (bytes_left > 12) ? 12 : bytes_left;

            std::vector<uint8_t> pkt;
            pkt.push_back('B');
            pkt.push_back(static_cast<uint8_t>((page_write_size_ >> 8) & 0xFF));
            pkt.push_back(static_cast<uint8_t>(page_write_size_ & 0xFF));
            pkt.push_back('F');
            for (u32 i = 0; i < chunk; ++i)
                pkt.push_back(sketch_data_[page_start + i]);
            send_out_packet(2, pkt);
            last_event_cycle_ = cpu_cycles + kSetupWait;
            prog_offset_ = chunk;
            sub_state_ = 1;
            break;
        }

        if (sub_state_ == 1) {
            // Wait for OUT to be consumed, then send more data or check response
            auto resp = read_ep_data(3);
            if (!resp.empty() && resp[0] == '\r') {
                // Got ack before all data sent? Unlikely but handle
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                flash_word_addr_ += 128;
                state_ = State::Avr109SetAddr;
                sub_state_ = 0;
                break;
            }
            if (prog_offset_ < page_write_size_) {
                u32 bytes_left = page_write_size_ - prog_offset_;
                u32 chunk = (bytes_left > 16) ? 16 : bytes_left;
                std::vector<uint8_t> pkt;
                for (u32 i = 0; i < chunk; ++i)
                    pkt.push_back(sketch_data_[page_start + prog_offset_ + i]);
                send_out_packet(2, pkt);
                last_event_cycle_ = cpu_cycles + kDataWait;
                prog_offset_ += chunk;
            } else {
                // All data sent, wait for response
                sub_state_ = 2;
            }
            break;
        }

        if (sub_state_ == 2) {
            auto data = read_ep_data(3);
            if (!data.empty() && data[0] == '\r') {
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                flash_word_addr_ += 128;
                state_ = State::Avr109SetAddr;
                sub_state_ = 0;
            }
            break;
        }
        break;
    }

    case State::Avr109Exit: {
        if (sub_state_ == 0) {
            send_out_packet(2, std::vector<uint8_t>{'E'});
            last_event_cycle_ = cpu_cycles + kSetupWait;
            sub_state_ = 1;
            break;
        }
        if (sub_state_ == 1) {
            auto data = read_ep_data(3);
            if (!data.empty() && data[0] == '\r') {
                send_in_token(3);
                last_event_cycle_ = cpu_cycles + kDataWait;
                state_ = State::Done;
            }
            break;
        }
        break;
    }

    case State::Done:
        break;
    }
}

} // namespace vioavr::core
