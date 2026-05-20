#pragma once

#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/sync_engine.hpp"
#include "vioavr/core/tracing.hpp"
#include "vioavr/core/types.hpp"
#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/register_file.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <memory>
#include <string>

// Include component headers for inline implementations
#include "vioavr/core/clkctrl.hpp"
#include "vioavr/core/slpctrl.hpp"
#include "vioavr/core/rstctrl.hpp"
#include "vioavr/core/sync_engine.hpp"

namespace vioavr::core {

class WatchdogTimer;
class Wdt8x;
class ClkCtrl;
class SlpCtrl;
class RstCtrl;

struct DecodedInstruction {
    std::array<u16, 2> words {};
    u16 opcode {};
    u32 word_address {};
    u8 word_size {1};
};

struct CpuSnapshot {
    std::array<u8, kRegisterFileSize> gpr {};
    u32 program_counter {};
    u16 stack_pointer {};
    u8 sreg {};
    u64 cycles {};
    bool interrupt_pending {};
    bool in_interrupt_handler {};
    CpuState state {CpuState::halted};
};

class AvrCpu {
public:
    explicit AvrCpu(MemoryBus& bus) noexcept;

    void reset(ResetCause cause = ResetCause::power_on) noexcept;
    void set_sync_engine(SyncEngine* sync_engine) noexcept;
    void set_trace_hook(ITraceHook* trace_hook) noexcept;
    void set_watchdog_timer(WatchdogTimer* watchdog_timer) noexcept;
    void run(u64 cycle_budget);
    void run_duration(double seconds);
    void step();
    void halt() noexcept { state_ = CpuState::paused; }
    void resume() noexcept;

    [[nodiscard]] u64 cycles_per_second() const noexcept;
    [[nodiscard]] u32 get_sleep_wake_latency() const noexcept;

    [[nodiscard]] constexpr CpuSnapshot snapshot() const noexcept
    {
        return {
            .gpr = gpr_,
            .program_counter = program_counter_,
            .stack_pointer = stack_pointer_,
            .sreg = sreg(),
            .cycles = cycles_,
            .interrupt_pending = interrupt_pending_,
            .in_interrupt_handler = interrupt_depth_ != 0U,
            .state = state_
        };
    }

    [[nodiscard]] constexpr std::span<u8, kRegisterFileSize> registers() noexcept
    {
        return gpr_;
    }

    [[nodiscard]] constexpr std::span<const u8, kRegisterFileSize> registers() const noexcept
    {
        return gpr_;
    }

    [[nodiscard]] constexpr u32 program_counter() const noexcept
    {
        return program_counter_;
    }
    inline void write_data_bus(u16 address, u8 value) noexcept;
    [[nodiscard]] inline u8 read_data_bus(u16 address) noexcept;

    void set_program_counter(u32 value) noexcept
    {
        program_counter_ = value;
    }

    [[nodiscard]] constexpr u16 stack_pointer() const noexcept
    {
        return stack_pointer_;
    }

    void set_stack_pointer(u16 value) noexcept
    {
        stack_pointer_ = value;
    }

    [[nodiscard]] constexpr u8 sreg() const noexcept
    {
        u8 val = 0;
        if (flag_c_) val |= 0x01U;
        if (flag_z_) val |= 0x02U;
        if (flag_n_) val |= 0x04U;
        if (flag_v_) val |= 0x08U;
        if (flag_s_) val |= 0x10U;
        if (flag_h_) val |= 0x20U;
        if (flag_t_) val |= 0x40U;
        if (flag_i_) val |= 0x80U;
        return val;
    }

    void set_sreg(u8 value) noexcept
    {
        write_sreg(value);
    }

    inline void write_sreg(u8 value) noexcept;
    inline void set_flag(SregFlag flag_bit, bool value) noexcept;

    [[nodiscard]] constexpr u8 rampz() const noexcept
    {
        return rampz_;
    }

    void set_rampz(u8 value) noexcept
    {
        rampz_ = value;
    }

    [[nodiscard]] constexpr u8 eind() const noexcept
    {
        return eind_;
    }

    void set_eind(u8 value) noexcept
    {
        eind_ = value;
    }

    [[nodiscard]] MemoryBus& bus() noexcept
    {
        return *bus_;
    }

    [[nodiscard]] CpuControl& control() noexcept
    {
        return *control_regs_;
    }

    [[nodiscard]] const MemoryBus& bus() const noexcept
    {
        return *bus_;
    }

    [[nodiscard]] CpuControl& cpu_control() noexcept
    {
        return *control_regs_;
    }

    inline void write_register(u8 index, u8 value) noexcept;
    void set_wdt8x(Wdt8x* wdt) noexcept { wdt8x_ = wdt; }

    // Public for fidelity tests
    void execute_reti(const DecodedInstruction& instruction);
    [[nodiscard]] bool service_interrupt_if_needed();
    void push_pc(u32 address) noexcept;
    [[nodiscard]] u32 interrupt_vector_word_address(u8 vector_index) const noexcept;
    void set_clk_ctrl(ClkCtrl* clk) noexcept { clk_ctrl_ = clk; }
    void set_slp_ctrl(SlpCtrl* slp) noexcept { slp_ctrl_ = slp; }
    void set_rst_ctrl(RstCtrl* rst) noexcept { rst_ctrl_ = rst; }
    [[nodiscard]] u8 active_clock_domains() const noexcept;

    [[nodiscard]] constexpr u64 cycles() const noexcept
    {
        return cycles_;
    }

    [[nodiscard]] constexpr CpuState state() const noexcept
    {
        return state_;
    }

private:
    using InstructionHandler = void (AvrCpu::*)(const DecodedInstruction&);

    struct InstructionDescriptor {
        u16 mask {};
        u16 pattern {};
        std::string_view mnemonic {};
        InstructionHandler handler {};
    };

    [[nodiscard]] DecodedInstruction fetch() const noexcept;
    [[nodiscard]] static constexpr u8 classify_word_size(u16 opcode) noexcept;
    [[nodiscard]] static std::span<const InstructionDescriptor> instruction_table() noexcept;
    void dispatch_instruction(const DecodedInstruction& instruction);
    void decode_and_execute(const DecodedInstruction& instruction);
    inline void advance_cycles(u64 delta_cycles);
    void synchronize_if_needed();
    void publish_pending_pin_changes();
    void refresh_interrupt_pending();


    [[nodiscard]] constexpr bool flag(SregFlag flag_bit) const noexcept
    {
        switch (flag_bit) {
            case SregFlag::carry: return flag_c_;
            case SregFlag::zero: return flag_z_;
            case SregFlag::negative: return flag_n_;
            case SregFlag::overflow: return flag_v_;
            case SregFlag::sign: return flag_s_;
            case SregFlag::halfCarry: return flag_h_;
            case SregFlag::transfer: return flag_t_;
            case SregFlag::interrupt: return flag_i_;
            default: return false;
        }
    }

    [[nodiscard]] static constexpr u8 decode_destination_register(u16 opcode) noexcept;
    [[nodiscard]] static constexpr u8 decode_source_register(u16 opcode) noexcept;
    [[nodiscard]] static constexpr i32 decode_relative_offset(u16 opcode, u8 width_bits) noexcept;
    [[nodiscard]] static constexpr SregFlag decode_sreg_bit(u16 opcode) noexcept;
    [[nodiscard]] static constexpr SregFlag decode_branch_sreg_bit(u16 opcode) noexcept;
    [[nodiscard]] constexpr u16 read_register_pair(u8 low_register_index) const noexcept;
    void write_register_pair(u8 low_register_index, u16 value) noexcept;
    [[nodiscard]] constexpr u16 x_pointer() const noexcept;
    [[nodiscard]] constexpr u16 y_pointer() const noexcept;
    [[nodiscard]] constexpr u16 z_pointer() const noexcept;
    void set_x_pointer(u16 value) noexcept;
    void set_y_pointer(u16 value) noexcept;
    void set_z_pointer(u16 value) noexcept;
    [[nodiscard]] static constexpr u8 decode_displacement_q(u16 opcode) noexcept;
    void update_add_flags(u8 lhs, u8 rhs, u8 result, bool carry_in = false) noexcept;
    void update_sub_flags(u8 lhs, u8 rhs, u8 result, bool carry_in = false) noexcept;
    void update_logic_flags(u8 result) noexcept;
    void update_adiw_flags(u16 lhs, u16 result) noexcept;
    void update_sbiw_flags(u16 lhs, u16 result) noexcept;
    void push_byte(u8 value) noexcept;
    [[nodiscard]] u8 pop_byte() noexcept;
    void push_word(u16 value) noexcept;
    [[nodiscard]] u16 pop_word() noexcept;

    [[nodiscard]] u32 pop_pc() noexcept;
    void execute_load_indirect(u8 destination, u16 address, bool post_increment, u8 pointer_low_register) noexcept;
    void execute_store_indirect(u16 address, u8 source, bool post_increment, u8 pointer_low_register) noexcept;
    void execute_nop(const DecodedInstruction& instruction);
    void execute_ldi(const DecodedInstruction& instruction);
    void execute_movw(const DecodedInstruction& instruction);
    void execute_mov(const DecodedInstruction& instruction);
    void execute_mul(const DecodedInstruction& instruction);
    void execute_muls(const DecodedInstruction& instruction);
    void execute_mulsu(const DecodedInstruction& instruction);
    void execute_fmul(const DecodedInstruction& instruction);
    void execute_fmuls(const DecodedInstruction& instruction);
    void execute_fmulsu(const DecodedInstruction& instruction);
    void execute_com(const DecodedInstruction& instruction);
    void execute_neg(const DecodedInstruction& instruction);
    void execute_swap(const DecodedInstruction& instruction);
    void execute_adiw(const DecodedInstruction& instruction);
    void execute_add(const DecodedInstruction& instruction);
    void execute_adc(const DecodedInstruction& instruction);
    void execute_inc(const DecodedInstruction& instruction);
    void execute_asr(const DecodedInstruction& instruction);
    void execute_lsr(const DecodedInstruction& instruction);
    void execute_ror(const DecodedInstruction& instruction);
    void execute_sbc(const DecodedInstruction& instruction);
    void execute_sbci(const DecodedInstruction& instruction);
    void execute_sub(const DecodedInstruction& instruction);
    void execute_dec(const DecodedInstruction& instruction);
    void execute_sbiw(const DecodedInstruction& instruction);
    void execute_cpi(const DecodedInstruction& instruction);
    void execute_subi(const DecodedInstruction& instruction);
    void execute_andi(const DecodedInstruction& instruction);
    void execute_cp(const DecodedInstruction& instruction);
    void execute_cpc(const DecodedInstruction& instruction);
    void execute_cpse(const DecodedInstruction& instruction);
    void execute_sbrc(const DecodedInstruction& instruction);
    void execute_sbrs(const DecodedInstruction& instruction);
    void execute_bst(const DecodedInstruction& instruction);
    void execute_bld(const DecodedInstruction& instruction);
    void execute_tst(const DecodedInstruction& instruction);
    void execute_ser(const DecodedInstruction& instruction);
    void execute_and(const DecodedInstruction& instruction);
    void execute_or(const DecodedInstruction& instruction);
    void execute_ori(const DecodedInstruction& instruction);
    void execute_eor(const DecodedInstruction& instruction);
    void execute_cbi(const DecodedInstruction& instruction);
    void execute_sbic(const DecodedInstruction& instruction);
    void execute_sbi(const DecodedInstruction& instruction);
    void execute_sbis(const DecodedInstruction& instruction);
    void execute_in(const DecodedInstruction& instruction);
    void execute_out(const DecodedInstruction& instruction);
    void execute_ld_x(const DecodedInstruction& instruction);
    void execute_ld_x_postinc(const DecodedInstruction& instruction);
    void execute_ld_x_predec(const DecodedInstruction& instruction);
    void execute_ld_y(const DecodedInstruction& instruction);
    void execute_ld_y_postinc(const DecodedInstruction& instruction);
    void execute_ld_y_predec(const DecodedInstruction& instruction);
    void execute_ldd_y(const DecodedInstruction& instruction);
    void execute_ld_z(const DecodedInstruction& instruction);
    void execute_ld_z_postinc(const DecodedInstruction& instruction);
    void execute_ld_z_predec(const DecodedInstruction& instruction);
    void execute_ldd_z(const DecodedInstruction& instruction);
    void execute_lpm(const DecodedInstruction& instruction);
    void execute_lpm_z(const DecodedInstruction& instruction);
    void execute_lpm_z_postinc(const DecodedInstruction& instruction);
    void execute_elpm(const DecodedInstruction& instruction);
    void execute_elpm_z(const DecodedInstruction& instruction);
    void execute_elpm_z_postinc(const DecodedInstruction& instruction);
    void execute_eicall(const DecodedInstruction& instruction);
    void execute_eijmp(const DecodedInstruction& instruction);
    void execute_spm(const DecodedInstruction& instruction);
    void execute_lds(const DecodedInstruction& instruction);
    void execute_st_x(const DecodedInstruction& instruction);
    void execute_st_x_postinc(const DecodedInstruction& instruction);
    void execute_st_x_predec(const DecodedInstruction& instruction);
    void execute_st_y(const DecodedInstruction& instruction);
    void execute_st_y_postinc(const DecodedInstruction& instruction);
    void execute_st_y_predec(const DecodedInstruction& instruction);
    void execute_std_y(const DecodedInstruction& instruction);
    void execute_st_z(const DecodedInstruction& instruction);
    void execute_st_z_postinc(const DecodedInstruction& instruction);
    void execute_st_z_predec(const DecodedInstruction& instruction);
    void execute_std_z(const DecodedInstruction& instruction);
    void execute_sts(const DecodedInstruction& instruction);
    void execute_rcall(const DecodedInstruction& instruction);
    void execute_rjmp(const DecodedInstruction& instruction);
    void execute_ijmp(const DecodedInstruction& instruction);
    void execute_jmp(const DecodedInstruction& instruction);
    void execute_push(const DecodedInstruction& instruction);
    void execute_pop(const DecodedInstruction& instruction);
    void execute_icall(const DecodedInstruction& instruction);
    void execute_call(const DecodedInstruction& instruction);
    void execute_ret(const DecodedInstruction& instruction);
    void execute_bclr(const DecodedInstruction& instruction);
    void execute_bset(const DecodedInstruction& instruction);
    void execute_sleep(const DecodedInstruction& instruction);
    void execute_break(const DecodedInstruction& instruction);
    void execute_wdr(const DecodedInstruction& instruction);
    void execute_cli(const DecodedInstruction& instruction);
    void execute_sei(const DecodedInstruction& instruction);
    void execute_brbc(const DecodedInstruction& instruction);
    void execute_brbs(const DecodedInstruction& instruction);
    void execute_brcs(const DecodedInstruction& instruction);
    void execute_brcc(const DecodedInstruction& instruction);
    void execute_brmi(const DecodedInstruction& instruction);
    void execute_brpl(const DecodedInstruction& instruction);
    void execute_breq(const DecodedInstruction& instruction);
    void execute_brne(const DecodedInstruction& instruction);

    MemoryBus* bus_ {};
    SyncEngine* sync_engine_ {};
    ITraceHook* trace_hook_ {};
    WatchdogTimer* watchdog_timer_ {};
    Wdt8x* wdt8x_ {};
    std::unique_ptr<CpuControl> control_regs_;
    std::unique_ptr<RegisterFile> register_file_;
    std::array<u16, 65536> fast_decode_table_ {};
    std::array<u8, kRegisterFileSize> gpr_ {};
    u32 program_counter_ {};
    u16 stack_pointer_ {};
    u8 sreg_ {};
    bool flag_c_ {false};
    bool flag_z_ {false};
    bool flag_n_ {false};
    bool flag_v_ {false};
    bool flag_s_ {false};
    bool flag_h_ {false};
    bool flag_t_ {false};
    bool flag_i_ {false};
    u8 rampz_ {};
    u8 eind_ {};
    ClkCtrl* clk_ctrl_ {nullptr};
    SlpCtrl* slp_ctrl_ {nullptr};
    RstCtrl* rst_ctrl_ {nullptr};
    u64 cycles_ {0};
    double cycle_accumulator_ {0.0};
    bool interrupt_pending_ {};
    u8 interrupt_depth_ {};
    u8 interrupt_delay_ {};
    std::vector<u16> temp_flash_page_buffer_;
    u8 spm_lock_timeout_ {};
    CpuState state_ {CpuState::halted};
    bool reset_triggered_ {false};
};

}  // namespace vioavr::core

namespace vioavr::core {

inline void AvrCpu::write_register(const u8 index, const u8 value) noexcept
{
    if (index < kRegisterFileSize) {
        gpr_[index] = value;
    }
}

inline void AvrCpu::advance_cycles(const u64 delta_cycles)
{
    cycles_ += delta_cycles;
    if (bus_ != nullptr) {
        bus_->tick_peripherals(delta_cycles, active_clock_domains());
        if (bus_->has_pending_pin_changes()) {
            publish_pending_pin_changes();
        }
    }
    if (spm_lock_timeout_ > 0U) {
        spm_lock_timeout_ = (delta_cycles >= spm_lock_timeout_) ? 0U : static_cast<u8>(spm_lock_timeout_ - delta_cycles);
    }
    if (sync_engine_ != nullptr) {
        sync_engine_->on_cycles_advanced(cycles_, delta_cycles);
    }
    refresh_interrupt_pending();
}

inline u8 AvrCpu::read_data_bus(const u16 address) noexcept
{
    if (bus_ == nullptr) return 0U;
    
    // Inline wait state check for speed
    const u8 ws = bus_->get_wait_states(address);
    if (ws > 0) {
        advance_cycles(static_cast<u64>(ws));
    }
    
    return bus_->read_data(address);
}

inline void AvrCpu::write_data_bus(const u16 address, const u8 value) noexcept
{
    if (bus_ == nullptr) return;

    const u8 ws = bus_->get_wait_states(address);
    if (ws > 0) {
        advance_cycles(static_cast<u64>(ws));
    }

    if (address == bus_->device().spmcsr_address) {
        if ((value & 0x01U) != 0U) {
            spm_lock_timeout_ = 4U;
        }
    }

    bus_->write_data(address, value);
}

inline void AvrCpu::set_flag(const SregFlag flag_bit, const bool value) noexcept
{
    switch (flag_bit) {
        case SregFlag::carry: flag_c_ = value; break;
        case SregFlag::zero: flag_z_ = value; break;
        case SregFlag::negative: flag_n_ = value; break;
        case SregFlag::overflow: flag_v_ = value; break;
        case SregFlag::sign: flag_s_ = value; break;
        case SregFlag::halfCarry: flag_h_ = value; break;
        case SregFlag::transfer: flag_t_ = value; break;
        case SregFlag::interrupt: flag_i_ = value; break;
    }
}

inline void AvrCpu::write_sreg(const u8 value) noexcept
{
    sreg_ = value;
    flag_c_ = (value & 0x01U) != 0U;
    flag_z_ = (value & 0x02U) != 0U;
    flag_n_ = (value & 0x04U) != 0U;
    flag_v_ = (value & 0x08U) != 0U;
    flag_s_ = (value & 0x10U) != 0U;
    flag_h_ = (value & 0x20U) != 0U;
    flag_t_ = (value & 0x40U) != 0U;

    const bool old_i = flag_i_;
    flag_i_ = (value & 0x80U) != 0U;
    if (flag_i_ && !old_i) {
        interrupt_delay_ = 1U;
    }

    if (trace_hook_ != nullptr) {
        trace_hook_->on_sreg_write(value);
    }
}

} // namespace vioavr::core
