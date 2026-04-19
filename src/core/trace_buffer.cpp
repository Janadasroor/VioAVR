#include "vioavr/core/trace_buffer.hpp"

namespace vioavr::core {

TraceBuffer::TraceBuffer(size_t capacity) 
    : capacity_(capacity) 
{
    buffer_.resize(capacity_);
}

std::vector<CpuSnapshot> TraceBuffer::snapshots() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CpuSnapshot> result;
    result.reserve(size_);
    
    if (size_ < capacity_) {
        for (size_t i = 0; i < size_; ++i) {
            result.push_back(buffer_[i]);
        }
    } else {
        for (size_t i = 0; i < capacity_; ++i) {
            result.push_back(buffer_[(head_ + i) % capacity_]);
        }
    }
    return result;
}

void TraceBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    head_ = 0;
    size_ = 0;
}

void TraceBuffer::on_instruction(u32 address, u16 opcode, std::string_view mnemonic) {
    (void)address;
    (void)opcode;
    (void)mnemonic;
    
    if (!cpu_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    buffer_[head_] = cpu_->snapshot();
    
    head_ = (head_ + 1) % capacity_;
    if (size_ < capacity_) size_++;
}

void TraceBuffer::on_interrupt(u8 vector) {
    (void)vector;
    // We could add a special flag to the snapshot if needed
}

} // namespace vioavr::core
