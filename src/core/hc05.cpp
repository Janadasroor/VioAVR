#include "vioavr/core/hc05.hpp"
#include <cstring>
#include <cstdio>

namespace vioavr::core {

Hc05::Hc05()
    : at_buffer_pos_(0)
    , at_was_terminated_(false)
    , at_escape_count_(0)
    , tx_fifo_head_(0)
    , tx_fifo_tail_(0)
    , tx_fifo_count_(0)
    , ext_rx_head_(0)
    , ext_rx_tail_(0)
    , ext_rx_count_(0)
    , baud_(9600)
    , role_(0)
    , cmode_(1)
    , state_(State::AT_MODE)
    , paired_(nullptr)
{
    name_[0] = '\0';
    pin_code_[0] = '\0';
    address_[0] = 0;
    address_[1] = 0;
    address_[2] = 0;
    set_name("HC-05");
    set_pin_code("1234");
    set_address(reinterpret_cast<const u8*>("\x00\x11\x22"));
}

void Hc05::set_name(const char* name) {
    u8 len = static_cast<u8>(std::strlen(name));
    if (len > HC05_NAME_MAX) len = HC05_NAME_MAX;
    std::memcpy(name_, name, len);
    name_[len] = '\0';
}

void Hc05::set_pin_code(const char* pin) {
    u8 len = static_cast<u8>(std::strlen(pin));
    if (len > 15) len = 15;
    std::memcpy(pin_code_, pin, len);
    pin_code_[len] = '\0';
}

void Hc05::set_address(const u8 addr[3]) {
    std::memcpy(address_, addr, 3);
}

void Hc05::pair(Hc05* other) {
    paired_ = other;
}

void Hc05::unpair() {
    paired_ = nullptr;
}

void Hc05::reset() {
    at_buffer_pos_ = 0;
    at_was_terminated_ = false;
    at_escape_count_ = 0;
    tx_fifo_head_ = 0;
    tx_fifo_tail_ = 0;
    tx_fifo_count_ = 0;
    ext_rx_head_ = 0;
    ext_rx_tail_ = 0;
    ext_rx_count_ = 0;
    state_ = State::AT_MODE;
    paired_ = nullptr;
}

bool Hc05::fifo_push(u8* buf, u8& head, u8& count, u8 byte) {
    if (count >= FIFO_SIZE) return false;
    buf[head] = byte;
    head = (head + 1) % FIFO_SIZE;
    count++;
    return true;
}

bool Hc05::fifo_pop(u8* buf, u8& tail, u8& count, u8& byte) {
    if (count == 0) return false;
    byte = buf[tail];
    tail = (tail + 1) % FIFO_SIZE;
    count--;
    return true;
}

void Hc05::send_byte(u8 byte) {
    fifo_push(tx_fifo_, tx_fifo_head_, tx_fifo_count_, byte);
}

void Hc05::send_response(const char* resp) {
    u8 len = static_cast<u8>(std::strlen(resp));
    for (u8 i = 0; i < len; i++)
        send_byte(static_cast<u8>(resp[i]));
    send_byte('\r');
    send_byte('\n');
}

void Hc05::rx_byte(u8 byte) {
    bool is_term = (byte == '\n' || byte == '\r');

    // Consume trailing terminators after AT command processing,
    // even if state changed to CONNECTED (prevents \n leak)
    if (at_was_terminated_) {
        at_was_terminated_ = false;
        if (is_term) return;
    }

    if (state_ == State::CONNECTED) {
        if (byte == '+') {
            at_escape_count_++;
            if (at_escape_count_ >= 3) {
                at_escape_count_ = 0;
                state_ = State::AT_MODE;
                at_buffer_pos_ = 0;
                send_response("OK");
            }
            return;
        }
        if (at_escape_count_ > 0) {
            for (u8 i = 0; i < at_escape_count_; i++)
                forward_to_paired('+');
            at_escape_count_ = 0;
        }
        forward_to_paired(byte);
        return;
    }

    if (is_term) {
        if (at_buffer_pos_ > 0) {
            at_was_terminated_ = true;
            at_buffer_[at_buffer_pos_] = '\0';
            process_at_command();
        }
        at_buffer_pos_ = 0;
        return;
    }

    if (at_buffer_pos_ >= AT_BUF_SIZE) {
        at_buffer_pos_ = 0;
        send_response("ERROR");
        return;
    }
    at_buffer_[at_buffer_pos_++] = static_cast<char>(byte);
}

void Hc05::process_at_command() {
    char* cmd = at_buffer_;

    while (*cmd == ' ' || *cmd == '\t') cmd++;
    if (*cmd == '\0') return;

    if (std::strncmp(cmd, "AT", 2) != 0) {
        send_response("ERROR");
        return;
    }
    cmd += 2;

    if (*cmd == '\0') {
        send_response("OK");
        return;
    }

    if (*cmd == '+') {
        cmd++;

        if (std::strcmp(cmd, "RESET") == 0) {
            reset();
            send_response("OK");
            return;
        }

        if (std::strncmp(cmd, "NAME", 4) == 0) {
            cmd += 4;
            if (*cmd == '?') {
                char resp[64];
                std::snprintf(resp, sizeof(resp), "+NAME:%s", name_);
                send_response(resp);
                return;
            }
            if (*cmd == '=') {
                cmd++;
                u8 len = static_cast<u8>(std::strlen(cmd));
                if (len > HC05_NAME_MAX) len = HC05_NAME_MAX;
                std::memcpy(name_, cmd, len);
                name_[len] = '\0';
                send_response("OK");
                return;
            }
            send_response("ERROR");
            return;
        }

        if (std::strncmp(cmd, "PSWD", 4) == 0) {
            cmd += 4;
            if (*cmd == '?') {
                char resp[32];
                std::snprintf(resp, sizeof(resp), "+PSWD:%s", pin_code_);
                send_response(resp);
                return;
            }
            if (*cmd == '=') {
                cmd++;
                u8 len = static_cast<u8>(std::strlen(cmd));
                if (len > 15) len = 15;
                std::memcpy(pin_code_, cmd, len);
                pin_code_[len] = '\0';
                send_response("OK");
                return;
            }
            send_response("ERROR");
            return;
        }

        if (std::strncmp(cmd, "UART", 4) == 0) {
            cmd += 4;
            if (*cmd == '?') {
                char resp[64];
                std::snprintf(resp, sizeof(resp), "+UART:%u,0,0", baud_);
                send_response(resp);
                return;
            }
            if (*cmd == '=') {
                cmd++;
                int b = 0, s = 0, p = 0;
                if (std::sscanf(cmd, "%d,%d,%d", &b, &s, &p) >= 1) {
                    baud_ = static_cast<u32>(b);
                    send_response("OK");
                    return;
                }
            }
            send_response("ERROR");
            return;
        }

        if (std::strncmp(cmd, "ROLE", 4) == 0) {
            cmd += 4;
            if (*cmd == '?') {
                char resp[16];
                std::snprintf(resp, sizeof(resp), "+ROLE:%d", role_);
                send_response(resp);
                return;
            }
            if (*cmd == '=') {
                cmd++;
                int r = 0;
                if (std::sscanf(cmd, "%d", &r) == 1 && (r == 0 || r == 1)) {
                    role_ = static_cast<u8>(r);
                    send_response("OK");
                    return;
                }
            }
            send_response("ERROR");
            return;
        }

        if (std::strncmp(cmd, "CMODE", 5) == 0) {
            cmd += 5;
            if (*cmd == '?') {
                char resp[16];
                std::snprintf(resp, sizeof(resp), "+CMODE:%d", cmode_);
                send_response(resp);
                return;
            }
            if (*cmd == '=') {
                cmd++;
                int m = 0;
                if (std::sscanf(cmd, "%d", &m) == 1 && (m == 0 || m == 1)) {
                    cmode_ = static_cast<u8>(m);
                    send_response("OK");
                    return;
                }
            }
            send_response("ERROR");
            return;
        }

        if (std::strcmp(cmd, "STATE?") == 0) {
            const char* s = (state_ == State::CONNECTED) ? "CONNECTED" : "INITIALIZED";
            char resp[32];
            std::snprintf(resp, sizeof(resp), "+STATE:%s", s);
            send_response(resp);
            return;
        }

        if (std::strcmp(cmd, "VERSION?") == 0) {
            send_response("+VERSION:2.0-20200601");
            return;
        }

        if (std::strcmp(cmd, "ADDR?") == 0) {
            char resp[32];
            std::snprintf(resp, sizeof(resp), "+ADDR:%02x:%02x:%02x",
                         address_[0], address_[1], address_[2]);
            send_response(resp);
            return;
        }

        if (std::strcmp(cmd, "INIT") == 0) {
            at_init();
            return;
        }

        if (std::strncmp(cmd, "CONN", 4) == 0) {
            at_connect();
            return;
        }

        if (std::strcmp(cmd, "DISC") == 0) {
            at_disconnect();
            return;
        }

        if (std::strcmp(cmd, "INQ") == 0) {
            if (role_ == 1) {
                send_response("+INQ:00:11:22,1F,7FFF");
            }
            send_response("OK");
            return;
        }

        send_response("ERROR");
        return;
    }

    send_response("OK");
}

void Hc05::at_init() {
    if (state_ == State::AT_MODE) {
        if (paired_) {
            state_ = State::INIT;
        } else {
            state_ = State::CONNECTED;
        }
        send_response("OK");
    } else {
        send_response("ERROR");
    }
}

void Hc05::at_connect() {
    if (state_ != State::INIT) {
        send_response("ERROR");
        return;
    }
    if (!paired_) {
        send_response("ERROR");
        return;
    }
    if (role_ == 1) {
        state_ = State::CONNECTED;
        paired_->state_ = State::CONNECTED;
        send_response("CONNECT");
    } else {
        send_response("ERROR");
    }
}

void Hc05::at_disconnect() {
    if (!paired_) {
        send_response("ERROR");
        return;
    }
    if (paired_->state_ == State::CONNECTED) {
        paired_->state_ = State::INIT;
        paired_->send_response("DISCONNECT");
    }
    state_ = State::INIT;
    send_response("OK");
}

void Hc05::forward_to_paired(u8 byte) {
    if (paired_ && paired_->state_ == State::CONNECTED) {
        paired_->ext_rx_fifo_[paired_->ext_rx_head_] = byte;
        paired_->ext_rx_head_ = (paired_->ext_rx_head_ + 1) % FIFO_SIZE;
        paired_->ext_rx_count_++;
    } else {
        // No paired device — echo back to sender (loopback mode)
        send_byte(byte);
    }
}

bool Hc05::has_tx_byte() const {
    return tx_fifo_count_ > 0;
}

u8 Hc05::read_tx_byte() {
    u8 byte = 0;
    fifo_pop(tx_fifo_, tx_fifo_tail_, tx_fifo_count_, byte);
    return byte;
}

u8 Hc05::read_injected_byte() {
    u8 byte = 0;
    if (ext_rx_count_ > 0) {
        byte = ext_rx_fifo_[ext_rx_tail_];
        ext_rx_tail_ = (ext_rx_tail_ + 1) % FIFO_SIZE;
        ext_rx_count_--;
    }
    return byte;
}

void Hc05::inject_external_data(const u8* data, u16 len) {
    for (u16 i = 0; i < len; i++) {
        if (ext_rx_count_ >= FIFO_SIZE) break;
        ext_rx_fifo_[ext_rx_head_] = data[i];
        ext_rx_head_ = (ext_rx_head_ + 1) % FIFO_SIZE;
        ext_rx_count_++;
    }
}

} // namespace vioavr::core
