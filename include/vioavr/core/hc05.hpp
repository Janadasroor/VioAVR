#pragma once

#include "vioavr/core/types.hpp"
#include <functional>
#include <cstring>

namespace vioavr::core {

class Hc05 {
public:
    static constexpr u8 FIFO_SIZE = 128;
    static constexpr u8 AT_BUF_SIZE = 64;
    static constexpr u8 HC05_NAME_MAX = 31;

    enum class State : u8 {
        AT_MODE,
        INIT,
        CONNECTED,
    };

    Hc05();
    ~Hc05() = default;

    Hc05(const Hc05&) = delete;
    Hc05& operator=(const Hc05&) = delete;

    // === UART byte interface ===
    void rx_byte(u8 byte);
    bool has_tx_byte() const;
    u8 read_tx_byte();
    u16 tx_byte_count() const { return tx_fifo_count_; }

    // === Injection (data from paired HC-05) ===
    bool has_injected_data() const { return ext_rx_count_ > 0; }
    u8 read_injected_byte();

    // === Pairing ===
    void pair(Hc05* other);
    void unpair();
    bool is_paired() const { return paired_ != nullptr; }
    Hc05* paired_with() const { return paired_; }

    // === Config ===
    void set_name(const char* name);
    const char* name() const { return name_; }
    void set_pin_code(const char* pin);
    const char* pin_code() const { return pin_code_; }
    void set_baud(u32 baud) { baud_ = baud; }
    u32 baud() const { return baud_; }
    void set_role(bool master) { role_ = master ? 1 : 0; }
    bool role() const { return role_ != 0; }
    void set_cmode(u8 mode) { cmode_ = mode; }
    u8 cmode() const { return cmode_; }
    void set_address(const u8 addr[3]);
    const u8* address() const { return address_; }

    // === State ===
    State state() const { return state_; }
    bool is_connected() const { return state_ == State::CONNECTED; }

    // === External data injection (virtual COM port) ===
    void inject_external_data(const u8* data, u16 len);
    bool has_external_data() const { return ext_rx_count_ > 0; }

    // === Reset ===
    void reset();

private:
    void process_at_command();
    void send_response(const char* resp);
    void send_byte(u8 byte);
    void forward_to_paired(u8 byte);
    void at_init();
    void at_connect();
    void at_disconnect();

    // FIFO helpers
    static bool fifo_push(u8* buf, u8& head, u8& count, u8 byte);
    static bool fifo_pop(u8* buf, u8& tail, u8& count, u8& byte);

    // AT command buffer
    char at_buffer_[AT_BUF_SIZE];
    u8 at_buffer_pos_;
    bool at_was_terminated_;
    u8 at_escape_count_;

    // TX and external RX FIFO
    u8 tx_fifo_[FIFO_SIZE];
    u8 tx_fifo_head_;
    u8 tx_fifo_tail_;
    u8 tx_fifo_count_;

    u8 ext_rx_fifo_[FIFO_SIZE];
    u8 ext_rx_head_;
    u8 ext_rx_tail_;
    u8 ext_rx_count_;

    // Config
    char name_[HC05_NAME_MAX + 1];
    char pin_code_[16];
    u32 baud_;
    u8 role_;    // 0=slave, 1=master
    u8 cmode_;   // 0=fixed address, 1=any
    u8 address_[3];

    // State
    State state_;
    Hc05* paired_;
};

} // namespace vioavr::core
