#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/avr_jit.hpp"
#include "vioavr/core/wdt8x.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/watchdog_timer.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/timer10.hpp"
#include "vioavr/core/cpu_int.hpp"
#include "vioavr/core/clkctrl.hpp"
#include "vioavr/core/slpctrl.hpp"
#include "vioavr/core/rstctrl.hpp"

#include <array>
#include <cstddef>
#include <algorithm>

namespace vioavr::core {

AvrCpu::AvrCpu(MemoryBus& bus) noexcept 
    : bus_(&bus), 
      control_regs_(std::make_unique<CpuControl>(*this, bus.device())),
      register_file_(std::make_unique<RegisterFile>(*this, bus.device().register_file_range)),
      jit_(std::make_unique<jit::AvrJit>())
{
    if (bus_ != nullptr) {
        bus_->attach_peripheral(*control_regs_);
        bus_->attach_peripheral(*register_file_);

        // Connect bus to timers that need it for external clocking
        for (auto* peripheral : bus_->peripherals()) {
            if (auto* t8 = dynamic_cast<Timer8*>(peripheral)) {
                t8->set_bus(*bus_);
            } else if (auto* t16 = dynamic_cast<Timer16*>(peripheral)) {
                t16->set_bus(*bus_);
            } else if (auto* t10 = dynamic_cast<Timer10*>(peripheral)) {
                t10->set_bus(*bus_);
            } else if (auto* wdt = dynamic_cast<WatchdogTimer*>(peripheral)) {
                watchdog_timer_ = wdt;
            } else if (auto* wdt8x = dynamic_cast<Wdt8x*>(peripheral)) {
                wdt8x_ = wdt8x;
            } else if (auto* clk = dynamic_cast<ClkCtrl*>(peripheral)) {
                clk_ctrl_ = clk;
            } else if (auto* slp = dynamic_cast<SlpCtrl*>(peripheral)) {
                slp_ctrl_ = slp;
            } else if (auto* rst = dynamic_cast<RstCtrl*>(peripheral)) {
                rst_ctrl_ = rst;
            }
        }
    }
    fast_decode_table_.fill(0xFFFFU);
    const auto table = instruction_table();
    for (u32 i = 0; i < 65536; ++i) {
        const u16 opcode = static_cast<u16>(i);
        for (size_t j = 0; j < table.size(); ++j) {
            if ((opcode & table[j].mask) == table[j].pattern) {
                fast_decode_table_[opcode] = static_cast<u16>(j);
                break;
            }
        }
    }
    reset();
}

AvrCpu::~AvrCpu() = default;


void AvrCpu::reset(ResetCause cause) noexcept
{
    Logger::info("Resetting AVR CPU");

    if (bus_ != nullptr) {
        bus_->reset();
    }

    gpr_.fill(0U);
    program_counter_ = bus_ != nullptr ? bus_->reset_word_address() : 0U;

    if (bus_ != nullptr) {
        stack_pointer_ = bus_->device().spl_reset | (bus_->device().sph_reset << 8);
        if (stack_pointer_ == 0U) {
            stack_pointer_ = bus_->device().sram_range().end;
        }
        write_sreg(bus_->device().sreg_reset);
    } else {
        stack_pointer_ = 0U;
        write_sreg(0U);
    }

    cycles_ = 0U;
    reset_triggered_ = true;
    interrupt_pending_ = false;
    interrupt_depth_ = 0U;
    if (jit_) jit_->invalidate_all();
    state_ = (bus_ != nullptr && bus_->loaded_program_words() > 0U) ? CpuState::running : CpuState::halted;

    if (rst_ctrl_) {
        u8 cause_mask = 0x01U;
        switch (cause) {
            case ResetCause::power_on:  cause_mask = 0x01U; break;
            case ResetCause::external:  cause_mask = 0x04U; break;
            case ResetCause::brown_out: cause_mask = 0x02U; break;
            case ResetCause::watchdog:  cause_mask = 0x08U; break;
        }
        rst_ctrl_->set_reset_cause(cause_mask);
    } else if (control_regs_) {
        control_regs_->set_reset_cause(cause);
    }

    interrupt_delay_ = 0U;
    if (sync_engine_ != nullptr) {
        sync_engine_->on_reset();
    }
}

jit::JitDebugStats AvrCpu::jit_debug_stats() const noexcept
{
    if (!jit_) return {0, 0, 0};
    auto s = jit_->debug_stats();
    return {s.translate_count, s.execute_count, s.execute_cycles};
}

void AvrCpu::set_sync_engine(SyncEngine* sync_engine) noexcept
{
    sync_engine_ = sync_engine;
    if (sync_engine != nullptr) {
        interrupt_check_interval_ = 1;
    }
}

void AvrCpu::set_trace_hook(ITraceHook* trace_hook) noexcept
{
    trace_hook_ = trace_hook;
}

void AvrCpu::set_watchdog_timer(WatchdogTimer* watchdog_timer) noexcept
{
    watchdog_timer_ = watchdog_timer;
}

void AvrCpu::run(const u64 cycle_budget)
{
    if (state_ == CpuState::paused) {
        return;
    }

    if (state_ != CpuState::sleeping) {
        state_ = CpuState::running;
    }
    const u64 cycle_target = cycles_ + cycle_budget;
    
    // Fast loop: only check interrupts/peripherals if mask is non-zero
    const u16* flash = bus_->flash_ptr();
    const size_t loaded_words = bus_->loaded_program_words();
    const size_t flash_size = bus_->flash_size_words();
    const auto table = instruction_table();
    u8* __restrict__ regs = gpr_.data();
    u8* __restrict__ bus_data = bus_->data_space().data();
    const u16 sram_begin = bus_->device().sram_range().begin;
    const u16 sram_end = bus_->device().sram_range().end;

    u32 pending_cycles = 0;
    const u64 check_interval = interrupt_check_interval_;

    jit::JitState jit_state;

    while (state_ == CpuState::running && cycles_ + pending_cycles < cycle_target) {
        // Handle pending interrupt before fetching next instruction
        if (__builtin_expect(interrupt_pending_ != 0, 0)) {
            if (pending_cycles > 0) {
                advance_cycles(pending_cycles);
                pending_cycles = 0;
            }
            if (interrupt_delay_ > 0U) {
                --interrupt_delay_;
            }
            refresh_interrupt_pending();
            if (interrupt_pending_) {
                if (service_interrupt_if_needed()) {
                    if (state_ != CpuState::running) break;
                    continue;
                }
            }
            if (state_ != CpuState::running) break;
        }

        // Handle CPU Stall (e.g. during EEPROM write or SPM)
        if (__builtin_expect(bus_->should_stall_cpu(program_counter_), 0)) {
            if (pending_cycles > 0) {
                advance_cycles(pending_cycles);
                pending_cycles = 0;
            }
            bus_->consume_stall_cycle();
            advance_cycles(1U);
            continue;
        }

        if (__builtin_expect(program_counter_ >= loaded_words, 0)) {
            if (pending_cycles > 0) {
                advance_cycles(pending_cycles);
                pending_cycles = 0;
            }
            state_ = CpuState::halted;
            break;
        }

        // JIT execution path
        if (__builtin_expect(jit_enabled_ && jit_ && (trace_hook_ == nullptr || !trace_hook_->is_active()), 0)) {
            if (pending_cycles > 0) {
                advance_cycles(pending_cycles);
                pending_cycles = 0;
            }

                bool jit_ok = jit_->has_block(program_counter_);
            if (!jit_ok) {
                jit_ok = jit_->translate(program_counter_, flash, loaded_words);
            }

            if (jit_ok) {
                std::copy(gpr_.begin(), gpr_.end(), jit_state.gpr);
                jit_state.pc = program_counter_;
                jit_state.sp = stack_pointer_;
                jit_state.sreg = sreg_;
                jit_state.cycles = cycles_;
                jit_state.bus = bus_;

                const u64 cycles_before = cycles_;

                jit_->execute(program_counter_, &jit_state);

                std::copy(jit_state.gpr, jit_state.gpr + 32, gpr_.begin());
                program_counter_ = jit_state.pc;
                stack_pointer_ = jit_state.sp;
                write_sreg(jit_state.sreg);

                const u64 delta = jit_state.cycles - cycles_before;
                cycles_ = jit_state.cycles;

                if (bus_ != nullptr) {
                    bus_->tick_peripherals(delta, active_clock_domains());
                    if (bus_->has_pending_pin_changes())
                        publish_pending_pin_changes();
                }
                if (spm_lock_timeout_ > 0U) {
                    spm_lock_timeout_ = (delta >= spm_lock_timeout_) ? 0U : static_cast<u8>(spm_lock_timeout_ - delta);
                }
                if (sync_engine_ != nullptr)
                    sync_engine_->on_cycles_advanced(cycles_, delta);
                refresh_interrupt_pending();

                continue;
            }
        }

        u32 pc = program_counter_;
        const u16 opcode = flash[pc];
        
        // Trace hook / GDB breakpoint support: fall through to slow path when active
        bool slow_path_needed = false;
        if (__builtin_expect(trace_hook_ != nullptr, 0)) {
            slow_path_needed = true;
        } else {
            // Fast dispatch by opcode bits (no LUT access for common instructions)
            switch (opcode >> 10) {
        case 0: // 0x0000-0x03FF: NOP
            if (__builtin_expect(opcode == 0x0000, 1)) {
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 3: // 0x0C00-0x0FFF: ADD
            if (__builtin_expect((opcode & 0xFC00) == 0x0C00, 1)) {
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                const u8 lhs = regs[dest];
                const u8 rhs = regs[src];
                const u8 res = static_cast<u8>(lhs + rhs);
                regs[dest] = res;
                flag_h_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x08;
                flag_v_ = ((lhs & rhs & ~res) | (~lhs & ~rhs & res)) & 0x80;
                flag_n_ = (res & 0x80);
                flag_z_ = (res == 0);
                flag_c_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x80;
                flag_s_ = flag_n_ ^ flag_v_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 7: // 0x1C00-0x1FFF: ADC
            if (__builtin_expect((opcode & 0xFC00) == 0x1C00, 1)) {
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                const u8 lhs = regs[dest];
                const u8 rhs = regs[src];
                const bool carry_in = flag_c_;
                const u8 res = static_cast<u8>(lhs + rhs + (carry_in ? 1U : 0U));
                regs[dest] = res;
                const bool prev_zero = flag_z_;
                flag_h_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x08;
                flag_v_ = ((lhs & rhs & ~res) | (~lhs & ~rhs & res)) & 0x80;
                flag_n_ = (res & 0x80);
                flag_c_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x80;
                flag_z_ = prev_zero && (res == 0);
                flag_s_ = flag_n_ ^ flag_v_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 5: // 0x1400-0x17FF: CP
            if (__builtin_expect((opcode & 0xFC00) == 0x1400, 1)) {
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                const u8 lhs = regs[dest];
                const u8 rhs = regs[src];
                const u8 res = static_cast<u8>(lhs - rhs);
                flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
                flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
                flag_n_ = (res & 0x80);
                flag_z_ = (res == 0);
                flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
                flag_s_ = flag_n_ ^ flag_v_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 6: // 0x1800-0x1BFF: SUB
            if (__builtin_expect((opcode & 0xFC00) == 0x1800, 1)) {
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                const u8 lhs = regs[dest];
                const u8 rhs = regs[src];
                const u8 res = static_cast<u8>(lhs - rhs);
                regs[dest] = res;
                flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
                flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
                flag_n_ = (res & 0x80);
                flag_z_ = (res == 0);
                flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
                flag_s_ = flag_n_ ^ flag_v_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 8: // 0x2000-0x23FF: AND
            if (__builtin_expect((opcode & 0xFC00) == 0x2000, 1)) {
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                const u8 res = static_cast<u8>(regs[dest] & regs[src]);
                regs[dest] = res;
                flag_v_ = false;
                flag_n_ = (res & 0x80);
                flag_z_ = (res == 0);
                flag_s_ = flag_n_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 9: // 0x2400-0x27FF: EOR
            if (__builtin_expect((opcode & 0xFC00) == 0x2400, 1)) {
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                const u8 res = static_cast<u8>(regs[dest] ^ regs[src]);
                regs[dest] = res;
                flag_v_ = false;
                flag_n_ = (res & 0x80);
                flag_z_ = (res == 0);
                flag_s_ = flag_n_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 11: // 0x2C00-0x2FFF: MOV
            if (__builtin_expect((opcode & 0xFC00) == 0x2C00, 1)) {
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                regs[dest] = regs[src];
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 1: // 0x0400-0x07FF: CPC
            if (__builtin_expect((opcode & 0xFC00) == 0x0400, 1)) {
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                const bool carry_in = flag_c_;
                const u8 lhs = regs[dest];
                const u8 rhs = regs[src];
                const u8 res = static_cast<u8>(lhs - rhs - (carry_in ? 1U : 0U));
                const bool prev_zero = flag_z_;
                flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
                flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
                flag_n_ = (res & 0x80);
                flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
                flag_z_ = prev_zero && (res == 0);
                flag_s_ = flag_n_ ^ flag_v_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 2: // 0x0800-0x0BFF: SBC
            if (__builtin_expect((opcode & 0xFC00) == 0x0800, 1)) {
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                const bool carry_in = flag_c_;
                const u8 lhs = regs[dest];
                const u8 rhs = regs[src];
                const u8 res = static_cast<u8>(lhs - rhs - (carry_in ? 1U : 0U));
                regs[dest] = res;
                const bool prev_zero = flag_z_;
                flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
                flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
                flag_n_ = (res & 0x80);
                flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
                flag_z_ = prev_zero && (res == 0);
                flag_s_ = flag_n_ ^ flag_v_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 12: // 0x3000-0x3FFF: CPI
            if (__builtin_expect((opcode & 0xF000) == 0x3000, 1)) {
                const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
                const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
                const u8 lhs = regs[dest];
                const u8 res = static_cast<u8>(lhs - imm);
                flag_h_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x08;
                flag_v_ = ((lhs & ~imm & ~res) | (~lhs & imm & res)) & 0x80;
                flag_n_ = (res & 0x80);
                flag_z_ = (res == 0);
                flag_c_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x80;
                flag_s_ = flag_n_ ^ flag_v_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 16: // 0x4000-0x4FFF: SBCI
            if (__builtin_expect((opcode & 0xF000) == 0x4000, 1)) {
                const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
                const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
                const bool carry_in = flag_c_;
                const u8 lhs = regs[dest];
                const u8 res = static_cast<u8>(lhs - imm - (carry_in ? 1U : 0U));
                regs[dest] = res;
                const bool prev_zero = flag_z_;
                flag_h_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x08;
                flag_v_ = ((lhs & ~imm & ~res) | (~lhs & imm & res)) & 0x80;
                flag_n_ = (res & 0x80);
                flag_c_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x80;
                flag_z_ = prev_zero && (res == 0);
                flag_s_ = flag_n_ ^ flag_v_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 20: // 0x5000-0x5FFF: SUBI
            if (__builtin_expect((opcode & 0xF000) == 0x5000, 1)) {
                const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
                const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
                const u8 lhs = regs[dest];
                const u8 res = static_cast<u8>(lhs - imm);
                regs[dest] = res;
                flag_h_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x08;
                flag_v_ = ((lhs & ~imm & ~res) | (~lhs & imm & res)) & 0x80;
                flag_n_ = (res & 0x80);
                flag_z_ = (res == 0);
                flag_c_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x80;
                flag_s_ = flag_n_ ^ flag_v_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 24: // 0x6000-0x6FFF: ORI
            if (__builtin_expect((opcode & 0xF000) == 0x6000, 1)) {
                const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
                const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
                regs[dest] |= imm;
                const u8 res = regs[dest];
                flag_v_ = false;
                flag_n_ = (res & 0x80);
                flag_z_ = (res == 0);
                flag_s_ = flag_n_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 28: // 0x7000-0x7FFF: ANDI
            if (__builtin_expect((opcode & 0xF000) == 0x7000, 1)) {
                const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
                const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
                regs[dest] &= imm;
                const u8 res = regs[dest];
                flag_v_ = false;
                flag_n_ = (res & 0x80);
                flag_z_ = (res == 0);
                flag_s_ = flag_n_;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 32: case 33: case 34: case 35:
        case 36: case 37: case 38: case 39:
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46: case 47: // 0x8000-0xBFFF: LDD, STD, IN, OUT
            if ((opcode & 0xD208) == 0x8008) { // LDD Y+q
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
                const u16 addr = static_cast<u16>((static_cast<u16>(regs[29]) << 8U) | (regs[28] + q));
                if (addr >= sram_begin && addr <= sram_end) [[likely]]
                    regs[dest] = bus_data[addr];
                else
                    regs[dest] = read_data_bus(addr);
                pc += 1U;
                pending_cycles += 2;
                break;
            }
            if ((opcode & 0xD208) == 0x8000) { // LDD Z+q
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
                const u16 addr = static_cast<u16>((static_cast<u16>(regs[31]) << 8U) | (regs[30] + q));
                if (addr >= sram_begin && addr <= sram_end) [[likely]]
                    regs[dest] = bus_data[addr];
                else
                    regs[dest] = read_data_bus(addr);
                pc += 1U;
                pending_cycles += 2;
                break;
            }
            if ((opcode & 0xD208) == 0x8208) { // STD Y+q
                const u8 src = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
                const u16 addr = static_cast<u16>((static_cast<u16>(regs[29]) << 8U) | (regs[28] + q));
                if (addr >= sram_begin && addr <= sram_end) [[likely]]
                    bus_data[addr] = regs[src];
                else
                    write_data_bus(addr, regs[src]);
                pc += 1U;
                pending_cycles += 2;
                break;
            }
            if ((opcode & 0xD208) == 0x8200) { // STD Z+q
                const u8 src = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
                const u16 addr = static_cast<u16>((static_cast<u16>(regs[31]) << 8U) | (regs[30] + q));
                if (addr >= sram_begin && addr <= sram_end) [[likely]]
                    bus_data[addr] = regs[src];
                else
                    write_data_bus(addr, regs[src]);
                pc += 1U;
                pending_cycles += 2;
                break;
            }
            if ((opcode & 0xF800) == 0xB000) { // IN
                if (pending_cycles > 0) {
                    advance_cycles(pending_cycles);
                    pending_cycles = 0;
                }
                const u8 reg = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 io_offset = static_cast<u8>(((opcode >> 5U) & 0x30U) | (opcode & 0x0FU));
                regs[reg] = read_data_bus(MemoryBus::low_io_address(io_offset));
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            if ((opcode & 0xF800) == 0xB800) { // OUT
                if (pending_cycles > 0) {
                    advance_cycles(pending_cycles);
                    pending_cycles = 0;
                }
                const u8 reg = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 io_offset = static_cast<u8>(((opcode >> 5U) & 0x30U) | (opcode & 0x0FU));
                write_data_bus(MemoryBus::low_io_address(io_offset), regs[reg]);
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            if ((opcode & 0xFE0F) == 0x9000) { // LDS
                if (pending_cycles > 0) {
                    advance_cycles(pending_cycles);
                    pending_cycles = 0;
                }
                const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                if (pc + 1U >= flash_size) {
                    state_ = CpuState::halted; break;
                }
                const u16 addr = flash[pc + 1U];
                regs[dest] = read_data_bus(addr);
                pc += 2U;
                pending_cycles += 2;
                break;
            }
            if ((opcode & 0xFE0F) == 0x9200) { // STS
                if (pending_cycles > 0) {
                    advance_cycles(pending_cycles);
                    pending_cycles = 0;
                }
                const u8 src = static_cast<u8>((opcode >> 4U) & 0x1FU);
                if (pc + 1U >= flash_size) {
                    state_ = CpuState::halted; break;
                }
                const u16 addr = flash[pc + 1U];
                write_data_bus(addr, regs[src]);
                pc += 2U;
                pending_cycles += 2;
                break;
            }
            if ((opcode | 0x0010) == 0x95F8) [[unlikely]] { // SPM & SPM Z+
                const u16 spmcsr_addr = bus_->device().spmcsr_address;
                if (spmcsr_addr != 0) {
                    const u8 spmcsr = bus_->read_data(spmcsr_addr);
                    if ((spmcsr & 0x01) != 0 && spm_lock_timeout_ > 0) {
                        if ((spmcsr & 0x40) == 0 || (spmcsr & 0x10) != 0) {
                            bus_->execute_spm(
                                static_cast<u8>(spmcsr & 0x1FU),
                                z_pointer(),
                                static_cast<u16>(regs[0] | (static_cast<u16>(regs[1]) << 8)),
                                pc
                            );
                            write_data_bus(spmcsr_addr, static_cast<u8>(spmcsr & 0xE0U));
                        }
                    }
                }
                spm_lock_timeout_ = 0;
                if ((opcode & 0x0001) != 0) {
                    const u16 z = static_cast<u16>((static_cast<u16>(regs[31]) << 8) | regs[30]);
                    regs[30] = static_cast<u8>((z + 2) & 0xFF);
                    regs[31] = static_cast<u8>(((z + 2) >> 8) & 0xFF);
                }
                pc += 1U;
                advance_cycles(1);
                break;
            }
            slow_path_needed = true; break;
        case 48: case 49: case 50: case 51: // 0xC000-0xFFFF: RJMP
            if (__builtin_expect((opcode & 0xF000) == 0xC000, 1)) {
                i32 displacement = static_cast<i32>(opcode & 0x0FFFU);
                if ((displacement & 0x0800) != 0) {
                    displacement -= 0x1000;
                }
                pc = static_cast<u32>(static_cast<i32>(pc) + 1 + displacement);
                pending_cycles += 2;
                break;
            }
            slow_path_needed = true; break;
        case 56: case 57: case 58: case 59: // 0xE000-0xFFFF: LDI / SER
            if (__builtin_expect((opcode & 0xF000) == 0xE000, 1)) {
                if ((opcode & 0xFF0F) == 0xEF0F) { slow_path_needed = true; break; } // SER, not inlined
                const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
                const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
                regs[dest] = imm;
                pc += 1U;
                pending_cycles += 1;
                break;
            }
            slow_path_needed = true; break;
        case 60: case 61: case 62: case 63: // 0xF000-0xFFFF: branches, SBRS, SBRC
            if (__builtin_expect((opcode & 0xFE08) == 0xFE00 || (opcode & 0xFE08) == 0xFC00, 0)) {
                const u8 reg = static_cast<u8>((opcode >> 4U) & 0x1FU);
                const u8 bit = static_cast<u8>(opcode & 0x07U);
                const bool bit_set = (regs[reg] >> bit) & 1U;
                // SBRS (bit9=1): skip if set; SBRC (bit9=0): skip if clear
                const bool should_skip = (opcode & 0x0200) ? bit_set : !bit_set;
                if (should_skip) {
                    if (pc + 1U >= flash_size) {
                        state_ = CpuState::halted; break;
                    }
                    const u16 next_opcode = flash[pc + 1U];
                    const bool is_2word = (next_opcode & 0xFE0EU) == 0x940CU ||
                                          (next_opcode & 0xFE0EU) == 0x940EU ||
                                          (next_opcode & 0xFE0FU) == 0x9000U ||
                                          (next_opcode & 0xFE0FU) == 0x9200U;
                    pc += is_2word ? 3U : 2U;
                    pending_cycles += is_2word ? 3U : 2U;
                } else {
                    pc += 1U;
                    pending_cycles += 1;
                }
                break;
            }
            switch (opcode & 0xFC07) {
            case 0xF001: // BREQ
                if (flag_z_) {
                    i32 d = static_cast<i32>((opcode >> 3U) & 0x7FU);
                    if (d & 0x40) d -= 0x80;
                    pc = static_cast<u32>(static_cast<i32>(pc) + 1 + d);
                    pending_cycles += 2;
                } else {
                    pc += 1U;
                    pending_cycles += 1;
                }
                break;
            case 0xF401: // BRNE
                if (!flag_z_) {
                    i32 d = static_cast<i32>((opcode >> 3U) & 0x7FU);
                    if (d & 0x40) d -= 0x80;
                    pc = static_cast<u32>(static_cast<i32>(pc) + 1 + d);
                    pending_cycles += 2;
                } else {
                    pc += 1U;
                    pending_cycles += 1;
                }
                break;
            case 0xF000: // BRCS / BRLO
                if (flag_c_) {
                    i32 d = static_cast<i32>((opcode >> 3U) & 0x7FU);
                    if (d & 0x40) d -= 0x80;
                    pc = static_cast<u32>(static_cast<i32>(pc) + 1 + d);
                    pending_cycles += 2;
                } else {
                    pc += 1U;
                    pending_cycles += 1;
                }
                break;
            case 0xF400: // BRCC / BRSH
                if (!flag_c_) {
                    i32 d = static_cast<i32>((opcode >> 3U) & 0x7FU);
                    if (d & 0x40) d -= 0x80;
                    pc = static_cast<u32>(static_cast<i32>(pc) + 1 + d);
                    pending_cycles += 2;
                } else {
                    pc += 1U;
                    pending_cycles += 1;
                }
                break;
            default:
                slow_path_needed = true; break;
            }
            break;
        default:
            slow_path_needed = true; break;
        }
        }
        
        if (__builtin_expect(slow_path_needed, 0)) {
            program_counter_ = pc;
            if (handle_slow_path(opcode, pending_cycles, regs, flash, flash_size, bus_data, sram_begin, sram_end)) break;
            pc = program_counter_;
        }
        program_counter_ = pc;

        if (pending_cycles >= check_interval) {
            advance_cycles(pending_cycles);
            pending_cycles = 0;
            synchronize_if_needed();
        }
    }

    if (pending_cycles > 0) {
        advance_cycles(pending_cycles);
    }

    // Fallback to standard step for complex states or pending interrupts
    while ((state_ == CpuState::running || state_ == CpuState::sleeping) && cycles_ < cycle_target) {
        step();
    }
}
bool AvrCpu::handle_slow_path(const u16 opcode, u32& pending_cycles, u8* regs, const u16* flash, const size_t flash_size, u8* bus_data, const u16 sram_begin, const u16 sram_end) noexcept
{
        {
            const u16 descriptor_index = fast_decode_table_[opcode];
            if (__builtin_expect(descriptor_index == 0xFFFFU, 0)) {
                state_ = CpuState::halted;
                return true;
            }
            if (__builtin_expect(trace_hook_ != nullptr, 0)) {
                if (pending_cycles > 0) {
                    advance_cycles(pending_cycles);
                    pending_cycles = 0;
                }
                const auto& desc = instruction_table()[descriptor_index];
                std::string_view mnemonic = desc.mnemonic;
                if (mnemonic == "AND" && 
                    decode_destination_register(opcode) == decode_source_register(opcode)) {
                    mnemonic = "TST";
                }
                trace_hook_->on_instruction(program_counter_, opcode, mnemonic);
                if (state_ == CpuState::paused) {
                    return true;
                }
            }

            switch (descriptor_index) {
            case 22: case 25: // SUB, CPI
                {
                    const u8 destination = (descriptor_index == 25) ? 
                        static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU)) :
                        decode_destination_register(opcode);
                    const u8 rhs = (descriptor_index == 25) ?
                        static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU)) :
                        regs[decode_source_register(opcode)];
                    const u8 lhs = regs[destination];
                    const u8 res = static_cast<u8>(lhs - rhs);
                    if (descriptor_index == 22) regs[destination] = res;
                    flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
                    flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
                    flag_n_ = (res & 0x80);
                    flag_z_ = (res == 0);
                    flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
                    flag_s_ = flag_n_ ^ flag_v_;
                    program_counter_ += 1U;
                    pending_cycles += 1;
                }
                break;
            case 26: // SUBI
                {
                    const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
                    const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
                    const u8 lhs = regs[dest];
                    const u8 res = static_cast<u8>(lhs - imm);
                    regs[dest] = res;
                    flag_h_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x08;
                    flag_v_ = ((lhs & ~imm & ~res) | (~lhs & imm & res)) & 0x80;
                    flag_n_ = (res & 0x80);
                    flag_z_ = (res == 0);
                    flag_c_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x80;
                    flag_s_ = flag_n_ ^ flag_v_;
                    program_counter_ += 1U;
                    pending_cycles += 1;
                }
                break;
            case 27: case 38: // ANDI, ORI
                {
                    const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
                    const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
                    if (descriptor_index == 27) regs[dest] &= imm;
                    else regs[dest] |= imm;
                    const u8 res = regs[dest];
                    flag_v_ = false;
                    flag_n_ = (res & 0x80);
                    flag_z_ = (res == 0);
                    flag_s_ = flag_n_;
                    program_counter_ += 1U;
                    pending_cycles += 1;
                }
                break;
             case 44: case 45: // IN, OUT
                {
                    if (pending_cycles > 0) {
                        advance_cycles(pending_cycles);
                        pending_cycles = 0;
                    }
                    const u8 reg = (descriptor_index == 44) ? 
                        decode_destination_register(opcode) : 
                        static_cast<u8>((opcode >> 4U) & 0x1FU);
                    const u8 io_offset = static_cast<u8>(((opcode >> 5U) & 0x30U) | (opcode & 0x0FU));
                    const u16 addr = MemoryBus::low_io_address(io_offset);
                    if (descriptor_index == 44) regs[reg] = bus_->read_data(addr);
                    else bus_->write_data(addr, regs[reg]);
                    program_counter_ += 1U;
                    pending_cycles += 1;
                }
                break;
             case 104: case 105: // BREQ, BRNE
                {
                    bool cond = (descriptor_index == 104) ? flag_z_ : !flag_z_;
                    if (cond) {
                        i32 disp = static_cast<i32>((opcode >> 3U) & 0x7FU);
                        if (disp & 0x40) disp -= 0x80;
                        program_counter_ = static_cast<u32>(static_cast<i32>(program_counter_) + 1 + disp);
                        pending_cycles += 2;
                    } else {
                        program_counter_ += 1U;
                        pending_cycles += 1;
                    }
                }
                break;
            case 15: // ADC
                {
                    const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                    const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                    const u8 lhs = regs[dest];
                    const u8 rhs = regs[src];
                    const bool carry_in = flag_c_;
                    const u8 res = static_cast<u8>(lhs + rhs + (carry_in ? 1U : 0U));
                    regs[dest] = res;
                    const bool prev_zero = flag_z_;
                    flag_h_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x08;
                    flag_v_ = ((lhs & rhs & ~res) | (~lhs & ~rhs & res)) & 0x80;
                    flag_n_ = (res & 0x80);
                    flag_c_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x80;
                    flag_z_ = prev_zero && (res == 0);
                    flag_s_ = flag_n_ ^ flag_v_;
                    program_counter_ += 1U;
                    pending_cycles += 1;
                }
                break;
            case 20: // SBC
                {
                    const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                    const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                    const bool carry_in = flag_c_;
                    const u8 lhs = regs[dest];
                    const u8 rhs = regs[src];
                    const u8 res = static_cast<u8>(lhs - rhs - (carry_in ? 1U : 0U));
                    regs[dest] = res;
                    const bool prev_zero = flag_z_;
                    flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
                    flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
                    flag_n_ = (res & 0x80);
                    flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
                    flag_z_ = prev_zero && (res == 0);
                    flag_s_ = flag_n_ ^ flag_v_;
                    program_counter_ += 1U;
                    pending_cycles += 1;
                }
                break;
            case 21: // SBCI
                {
                    const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
                    const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
                    const bool carry_in = flag_c_;
                    const u8 lhs = regs[dest];
                    const u8 res = static_cast<u8>(lhs - imm - (carry_in ? 1U : 0U));
                    regs[dest] = res;
                    const bool prev_zero = flag_z_;
                    flag_h_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x08;
                    flag_v_ = ((lhs & ~imm & ~res) | (~lhs & imm & res)) & 0x80;
                    flag_n_ = (res & 0x80);
                    flag_c_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x80;
                    flag_z_ = prev_zero && (res == 0);
                    flag_s_ = flag_n_ ^ flag_v_;
                    program_counter_ += 1U;
                    pending_cycles += 1;
                }
                break;
            case 39: // EOR
                {
                    const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                    const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
                    const u8 res = static_cast<u8>(regs[dest] ^ regs[src]);
                    regs[dest] = res;
                    flag_v_ = false;
                    flag_n_ = (res & 0x80);
                    flag_z_ = (res == 0);
                    flag_s_ = flag_n_;
                    program_counter_ += 1U;
                    pending_cycles += 1;
                }
                break;
            case 54: // LDD Y+q
                {
                    const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                    const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
                    const u16 addr = static_cast<u16>((static_cast<u16>(regs[29]) << 8U) | (regs[28] + q));
                    if (addr >= sram_begin && addr <= sram_end) [[likely]]
                        regs[dest] = bus_data[addr];
                    else
                        regs[dest] = read_data_bus(addr);
                    program_counter_ += 1U;
                    pending_cycles += 2;
                }
                break;
            case 58: // LDD Z+q
                {
                    const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
                    const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
                    const u16 addr = static_cast<u16>((static_cast<u16>(regs[31]) << 8U) | (regs[30] + q));
                    if (addr >= sram_begin && addr <= sram_end) [[likely]]
                        regs[dest] = bus_data[addr];
                    else
                        regs[dest] = read_data_bus(addr);
                    program_counter_ += 1U;
                    pending_cycles += 2;
                }
                break;
            case 75: // STD Y+q
                {
                    const u8 src = static_cast<u8>((opcode >> 4U) & 0x1FU);
                    const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
                    const u16 addr = static_cast<u16>((static_cast<u16>(regs[29]) << 8U) | (regs[28] + q));
                    if (addr >= sram_begin && addr <= sram_end) [[likely]]
                        bus_data[addr] = regs[src];
                    else
                        write_data_bus(addr, regs[src]);
                    program_counter_ += 1U;
                    pending_cycles += 2;
                }
                break;
            case 79: // STD Z+q
                {
                    const u8 src = static_cast<u8>((opcode >> 4U) & 0x1FU);
                    const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
                    const u16 addr = static_cast<u16>((static_cast<u16>(regs[31]) << 8U) | (regs[30] + q));
                    if (addr >= sram_begin && addr <= sram_end) [[likely]]
                        bus_data[addr] = regs[src];
                    else
                        write_data_bus(addr, regs[src]);
                    program_counter_ += 1U;
                    pending_cycles += 2;
                }
                break;
             case 100: // BRCS / BRLO
                {
                    if (flag_c_) {
                        i32 displacement = static_cast<i32>((opcode >> 3U) & 0x7FU);
                        if (displacement & 0x40) displacement -= 0x80;
                        program_counter_ = static_cast<u32>(static_cast<i32>(program_counter_) + 1 + displacement);
                        pending_cycles += 2;
                    } else {
                        program_counter_ += 1U;
                        pending_cycles += 1;
                    }
                }
                break;
             case 101: // BRCC / BRSH
                {
                    if (!flag_c_) {
                        i32 disp = static_cast<i32>((opcode >> 3U) & 0x7FU);
                        if (disp & 0x40) disp -= 0x80;
                        program_counter_ = static_cast<u32>(static_cast<i32>(program_counter_) + 1 + disp);
                        pending_cycles += 2;
                    } else {
                        program_counter_ += 1U;
                        pending_cycles += 1;
                    }
                }
                break;
            default:
                {
                    const auto& desc = instruction_table()[descriptor_index];
                    DecodedInstruction inst;
                    inst.opcode = opcode;
                    inst.word_address = program_counter_;
                    inst.words[0] = opcode;
                    if ((opcode & 0xFE0EU) == 0x940CU || (opcode & 0xFE0EU) == 0x940EU ||
                        (opcode & 0xFE0FU) == 0x9000U || (opcode & 0xFE0FU) == 0x9200U) {
                        inst.word_size = 2U;
                        if (program_counter_ + 1U < flash_size)
                            inst.words[1] = flash[program_counter_ + 1U];
                        else
                            inst.words[1] = 0xFFFFU;
                    } else {
                        inst.word_size = 1U;
                    }
                    if (pending_cycles > 0) {
                        advance_cycles(pending_cycles);
                        pending_cycles = 0;
                    }
                    (this->*desc.handler)(inst);
                }
                break;
            }
        }
    return false;
}

void AvrCpu::dispatch_instruction(const DecodedInstruction& instruction)
{
    const u16 descriptor_index = fast_decode_table_[instruction.opcode];
    if (descriptor_index == 0xFFFFU) {
        state_ = CpuState::halted;
        return;
    }

    const auto& descriptor = instruction_table()[descriptor_index];
    
    switch (descriptor_index) {
    case 0: execute_nop(instruction); break;
    case 1: execute_ldi(instruction); break;
    case 8: execute_mov(instruction); break;
    case 84: execute_rjmp(instruction); break;
    default:
        (this->*descriptor.handler)(instruction);
        break;
    }
}

u64 AvrCpu::cycles_per_second() const noexcept
{
    if (clk_ctrl_ != nullptr) return clk_ctrl_->cpu_frequency_hz();
    return control_regs_ != nullptr ? control_regs_->effective_frequency() : (bus_ != nullptr ? bus_->device().cpu_frequency_hz : 0U);
}

void AvrCpu::run_duration(const double seconds)
{
    if (state_ == CpuState::halted || state_ == CpuState::paused) {
        return;
    }

    const double cycles_to_run = (seconds * static_cast<double>(cycles_per_second())) + cycle_accumulator_;
    const u64 budget = static_cast<u64>(cycles_to_run);
    cycle_accumulator_ = cycles_to_run - static_cast<double>(budget);

    if (budget > 0U) {
        run(budget);
    }
}

void AvrCpu::resume() noexcept
{
    if (state_ == CpuState::paused) {
        // Return to functional state (running or sleeping)
        state_ = control_regs_->is_sleeping() ? CpuState::sleeping : CpuState::running;
    }
}

u32 AvrCpu::get_sleep_wake_latency() const noexcept
{
    // Legacy mapping or Mega-0 mapping
    // From Idle: 6 cycles. 
    // From Power-down: 6 cycles + startup.
    // We use 6 as a standard for now.
    if (!control_regs_) return 4U;

    switch (control_regs_->current_sleep_mode()) {
    case SleepMode::idle: return 6U;
    case SleepMode::adc_noise_reduction: return 6U;
    case SleepMode::power_down: return 6U;
    case SleepMode::power_save: return 6U;
    case SleepMode::standby: return 6U;
    case SleepMode::extended_standby: return 6U;
    default: return 4U;
    }
}

void AvrCpu::step()
{
    if (bus_ == nullptr || bus_->loaded_program_words() == 0U) {
        state_ = CpuState::halted;
        return;
    }

    if (__builtin_expect(bus_->should_stall_cpu(program_counter_), 0)) {
        advance_cycles(1U);
        return;
    }

    // Halt if PC is beyond the loaded program boundary
    if (__builtin_expect(program_counter_ >= bus_->loaded_program_words(), 0)) {
        state_ = CpuState::halted;
        return;
    }

    if (state_ == CpuState::paused) {
        return;
    }

    refresh_interrupt_pending();

    if (state_ == CpuState::sleeping) {
        if (!interrupt_pending_) {
            advance_cycles(1U);
        }

        if (interrupt_pending_) {
            // Wake up latency (4-6 cycles depending on mode)
            const u32 latency = get_sleep_wake_latency();
            advance_cycles(latency);
            
            state_ = CpuState::running;
            cpu_control().exit_sleep();

            if (service_interrupt_if_needed()) {
                synchronize_if_needed();
            }
            return;
        }
        return;
    }

    if (interrupt_delay_ > 0U) {
        --interrupt_delay_;
    } else if (interrupt_pending_) {
        if (service_interrupt_if_needed()) {
            synchronize_if_needed();
            return;
        }
    }

    reset_triggered_ = false;
    const u16 opcode = static_cast<u16>(bus_->read_program_word(program_counter_));
    u8* regs = gpr_.data();
    const auto sram = bus_->device().sram_range();
    const u16 sram_begin = sram.begin;
    const u16 sram_end = sram.end;
    u8* bus_data = bus_->data_space().data();
    const u16* flash = bus_->flash_ptr();
    const size_t flash_size = bus_->flash_size_words();
    
    switch (opcode >> 10) {
    case 0: // NOP
        if (__builtin_expect(opcode == 0x0000, 1)) {
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 3: // ADD
        if (__builtin_expect((opcode & 0xFC00) == 0x0C00, 1)) {
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
            const u8 lhs = regs[dest];
            const u8 rhs = regs[src];
            const u8 res = static_cast<u8>(lhs + rhs);
            regs[dest] = res;
            flag_h_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x08;
            flag_v_ = ((lhs & rhs & ~res) | (~lhs & ~rhs & res)) & 0x80;
            flag_n_ = (res & 0x80);
            flag_z_ = (res == 0);
            flag_c_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x80;
            flag_s_ = flag_n_ ^ flag_v_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 7: // ADC
        if (__builtin_expect((opcode & 0xFC00) == 0x1C00, 1)) {
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
            const u8 lhs = regs[dest];
            const u8 rhs = regs[src];
            const bool carry_in = flag_c_;
            const u8 res = static_cast<u8>(lhs + rhs + (carry_in ? 1U : 0U));
            regs[dest] = res;
            const bool prev_zero = flag_z_;
            flag_h_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x08;
            flag_v_ = ((lhs & rhs & ~res) | (~lhs & ~rhs & res)) & 0x80;
            flag_n_ = (res & 0x80);
            flag_c_ = ((lhs & rhs) | (rhs & ~res) | (~res & lhs)) & 0x80;
            flag_z_ = prev_zero && (res == 0);
            flag_s_ = flag_n_ ^ flag_v_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 5: // CP
        if (__builtin_expect((opcode & 0xFC00) == 0x1400, 1)) {
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
            const u8 lhs = regs[dest];
            const u8 rhs = regs[src];
            const u8 res = static_cast<u8>(lhs - rhs);
            flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
            flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
            flag_n_ = (res & 0x80);
            flag_z_ = (res == 0);
            flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
            flag_s_ = flag_n_ ^ flag_v_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 6: // SUB
        if (__builtin_expect((opcode & 0xFC00) == 0x1800, 1)) {
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
            const u8 lhs = regs[dest];
            const u8 rhs = regs[src];
            const u8 res = static_cast<u8>(lhs - rhs);
            regs[dest] = res;
            flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
            flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
            flag_n_ = (res & 0x80);
            flag_z_ = (res == 0);
            flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
            flag_s_ = flag_n_ ^ flag_v_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 8: // AND
        if (__builtin_expect((opcode & 0xFC00) == 0x2000, 1)) {
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
            const u8 res = static_cast<u8>(regs[dest] & regs[src]);
            regs[dest] = res;
            flag_v_ = false;
            flag_n_ = (res & 0x80);
            flag_z_ = (res == 0);
            flag_s_ = flag_n_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 9: // EOR
        if (__builtin_expect((opcode & 0xFC00) == 0x2400, 1)) {
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
            const u8 res = static_cast<u8>(regs[dest] ^ regs[src]);
            regs[dest] = res;
            flag_v_ = false;
            flag_n_ = (res & 0x80);
            flag_z_ = (res == 0);
            flag_s_ = flag_n_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 11: // MOV
        if (__builtin_expect((opcode & 0xFC00) == 0x2C00, 1)) {
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
            regs[dest] = regs[src];
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 1: // CPC
        if (__builtin_expect((opcode & 0xFC00) == 0x0400, 1)) {
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
            const bool carry_in = flag_c_;
            const u8 lhs = regs[dest];
            const u8 rhs = regs[src];
            const u8 res = static_cast<u8>(lhs - rhs - (carry_in ? 1U : 0U));
            const bool prev_zero = flag_z_;
            flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
            flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
            flag_n_ = (res & 0x80);
            flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
            flag_z_ = prev_zero && (res == 0);
            flag_s_ = flag_n_ ^ flag_v_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 2: // SBC
        if (__builtin_expect((opcode & 0xFC00) == 0x0800, 1)) {
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 src = static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
            const bool carry_in = flag_c_;
            const u8 lhs = regs[dest];
            const u8 rhs = regs[src];
            const u8 res = static_cast<u8>(lhs - rhs - (carry_in ? 1U : 0U));
            regs[dest] = res;
            const bool prev_zero = flag_z_;
            flag_h_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x08;
            flag_v_ = ((lhs & ~rhs & ~res) | (~lhs & rhs & res)) & 0x80;
            flag_n_ = (res & 0x80);
            flag_c_ = ((~lhs & rhs) | (rhs & res) | (res & ~lhs)) & 0x80;
            flag_z_ = prev_zero && (res == 0);
            flag_s_ = flag_n_ ^ flag_v_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 12: // CPI
        if (__builtin_expect((opcode & 0xF000) == 0x3000, 1)) {
            const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
            const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
            const u8 lhs = regs[dest];
            const u8 res = static_cast<u8>(lhs - imm);
            flag_h_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x08;
            flag_v_ = ((lhs & ~imm & ~res) | (~lhs & imm & res)) & 0x80;
            flag_n_ = (res & 0x80);
            flag_z_ = (res == 0);
            flag_c_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x80;
            flag_s_ = flag_n_ ^ flag_v_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 16: // SBCI
        if (__builtin_expect((opcode & 0xF000) == 0x4000, 1)) {
            const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
            const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
            const bool carry_in = flag_c_;
            const u8 lhs = regs[dest];
            const u8 res = static_cast<u8>(lhs - imm - (carry_in ? 1U : 0U));
            regs[dest] = res;
            const bool prev_zero = flag_z_;
            flag_h_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x08;
            flag_v_ = ((lhs & ~imm & ~res) | (~lhs & imm & res)) & 0x80;
            flag_n_ = (res & 0x80);
            flag_c_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x80;
            flag_z_ = prev_zero && (res == 0);
            flag_s_ = flag_n_ ^ flag_v_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 20: // SUBI
        if (__builtin_expect((opcode & 0xF000) == 0x5000, 1)) {
            const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
            const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
            const u8 lhs = regs[dest];
            const u8 res = static_cast<u8>(lhs - imm);
            regs[dest] = res;
            flag_h_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x08;
            flag_v_ = ((lhs & ~imm & ~res) | (~lhs & imm & res)) & 0x80;
            flag_n_ = (res & 0x80);
            flag_z_ = (res == 0);
            flag_c_ = ((~lhs & imm) | (imm & res) | (res & ~lhs)) & 0x80;
            flag_s_ = flag_n_ ^ flag_v_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 24: // ORI
        if (__builtin_expect((opcode & 0xF000) == 0x6000, 1)) {
            const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
            const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
            regs[dest] |= imm;
            const u8 res = regs[dest];
            flag_v_ = false;
            flag_n_ = (res & 0x80);
            flag_z_ = (res == 0);
            flag_s_ = flag_n_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 28: // ANDI
        if (__builtin_expect((opcode & 0xF000) == 0x7000, 1)) {
            const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
            const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
            regs[dest] &= imm;
            const u8 res = regs[dest];
            flag_v_ = false;
            flag_n_ = (res & 0x80);
            flag_z_ = (res == 0);
            flag_s_ = flag_n_;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 32: case 33: case 34: case 35:
    case 36: case 37: case 38: case 39:
    case 40: case 41: case 42: case 43:
    case 44: case 45: case 46: case 47: // 0x8000-0xBFFF: LDD, STD, IN, OUT, LDS, STS
        if ((opcode & 0xD208) == 0x8008) { // LDD Y+q
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
            const u16 addr = static_cast<u16>((static_cast<u16>(regs[29]) << 8U) | (regs[28] + q));
            if (addr >= sram_begin && addr <= sram_end) [[likely]]
                regs[dest] = bus_data[addr];
            else
                regs[dest] = read_data_bus(addr);
            program_counter_ += 1U; advance_cycles(2U); return;
        }
        if ((opcode & 0xD208) == 0x8000) { // LDD Z+q
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
            const u16 addr = static_cast<u16>((static_cast<u16>(regs[31]) << 8U) | (regs[30] + q));
            if (addr >= sram_begin && addr <= sram_end) [[likely]]
                regs[dest] = bus_data[addr];
            else
                regs[dest] = read_data_bus(addr);
            program_counter_ += 1U; advance_cycles(2U); return;
        }
        if ((opcode & 0xD208) == 0x8208) { // STD Y+q
            const u8 src = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
            const u16 addr = static_cast<u16>((static_cast<u16>(regs[29]) << 8U) | (regs[28] + q));
            if (addr >= sram_begin && addr <= sram_end) [[likely]]
                bus_data[addr] = regs[src];
            else
                write_data_bus(addr, regs[src]);
            program_counter_ += 1U; advance_cycles(2U); return;
        }
        if ((opcode & 0xD208) == 0x8200) { // STD Z+q
            const u8 src = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 q = static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
            const u16 addr = static_cast<u16>((static_cast<u16>(regs[31]) << 8U) | (regs[30] + q));
            if (addr >= sram_begin && addr <= sram_end) [[likely]]
                bus_data[addr] = regs[src];
            else
                write_data_bus(addr, regs[src]);
            program_counter_ += 1U; advance_cycles(2U); return;
        }
        if ((opcode & 0xF800) == 0xB000) { // IN
            const u8 reg = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 io_offset = static_cast<u8>(((opcode >> 5U) & 0x30U) | (opcode & 0x0FU));
            regs[reg] = read_data_bus(MemoryBus::low_io_address(io_offset));
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        if ((opcode & 0xF800) == 0xB800) { // OUT
            const u8 reg = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 io_offset = static_cast<u8>(((opcode >> 5U) & 0x30U) | (opcode & 0x0FU));
            write_data_bus(MemoryBus::low_io_address(io_offset), regs[reg]);
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        if ((opcode & 0xFE0F) == 0x9000) { // LDS
            const u8 dest = static_cast<u8>((opcode >> 4U) & 0x1FU);
            if (program_counter_ + 1U >= flash_size) { state_ = CpuState::halted; return; }
            const u16 addr = bus_->read_program_word(program_counter_ + 1U);
            regs[dest] = read_data_bus(addr);
            program_counter_ += 2U; advance_cycles(2U); return;
        }
        if ((opcode & 0xFE0F) == 0x9200) { // STS
            const u8 src = static_cast<u8>((opcode >> 4U) & 0x1FU);
            if (program_counter_ + 1U >= flash_size) { state_ = CpuState::halted; return; }
            const u16 addr = bus_->read_program_word(program_counter_ + 1U);
            write_data_bus(addr, regs[src]);
            program_counter_ += 2U; advance_cycles(2U); return;
        }
        break;
    case 48: case 49: case 50: case 51: // RJMP
        if (__builtin_expect((opcode & 0xF000) == 0xC000, 1)) {
            i32 displacement = static_cast<i32>(opcode & 0x0FFFU);
            if ((displacement & 0x0800) != 0) displacement -= 0x1000;
            program_counter_ = static_cast<u32>(static_cast<i32>(program_counter_) + 1 + displacement);
            advance_cycles(2U); return;
        }
        break;
    case 56: case 57: case 58: case 59: // LDI / SER
        if (__builtin_expect((opcode & 0xF000) == 0xE000, 1)) {
            if ((opcode & 0xFF0F) == 0xEF0F) break; // SER, not inlined
            const u8 dest = static_cast<u8>(16U + ((opcode >> 4U) & 0x0FU));
            const u8 imm = static_cast<u8>(((opcode >> 4U) & 0xF0U) | (opcode & 0x0FU));
            regs[dest] = imm;
            program_counter_ += 1U; advance_cycles(1U); return;
        }
        break;
    case 60: case 61: case 62: case 63: // Branches (BREQ, BRNE, BRCS, BRCC) + SBRS, SBRC
        if ((opcode & 0xFE08) == 0xFE00 || (opcode & 0xFE08) == 0xFC00) {
            const u8 reg = static_cast<u8>((opcode >> 4U) & 0x1FU);
            const u8 bit = static_cast<u8>(opcode & 0x07U);
            const bool bit_set = (regs[reg] >> bit) & 1U;
            const bool should_skip = (opcode & 0x0200) ? bit_set : !bit_set;
            if (should_skip) {
                if (program_counter_ + 1U >= flash_size) {
                    state_ = CpuState::halted; return;
                }
                const u16 next_opcode = flash[program_counter_ + 1U];
                const bool is_2word = (next_opcode & 0xFE0EU) == 0x940CU ||
                                      (next_opcode & 0xFE0EU) == 0x940EU ||
                                      (next_opcode & 0xFE0FU) == 0x9000U ||
                                      (next_opcode & 0xFE0FU) == 0x9200U;
                program_counter_ += is_2word ? 3U : 2U;
                advance_cycles(is_2word ? 3U : 2U);
            } else {
                program_counter_ += 1U;
                advance_cycles(1U);
            }
            return;
        }
        switch (opcode & 0xFC07) {
        case 0xF001: // BREQ
            if (flag_z_) {
                i32 d = static_cast<i32>((opcode >> 3U) & 0x7FU);
                if (d & 0x40) d -= 0x80;
                program_counter_ = static_cast<u32>(static_cast<i32>(program_counter_) + 1 + d);
                advance_cycles(2U);
            } else {
                program_counter_ += 1U;
                advance_cycles(1U);
            }
            return;
        case 0xF401: // BRNE
            if (!flag_z_) {
                i32 d = static_cast<i32>((opcode >> 3U) & 0x7FU);
                if (d & 0x40) d -= 0x80;
                program_counter_ = static_cast<u32>(static_cast<i32>(program_counter_) + 1 + d);
                advance_cycles(2U);
            } else {
                program_counter_ += 1U;
                advance_cycles(1U);
            }
            return;
        case 0xF000: // BRCS / BRLO
            if (flag_c_) {
                i32 d = static_cast<i32>((opcode >> 3U) & 0x7FU);
                if (d & 0x40) d -= 0x80;
                program_counter_ = static_cast<u32>(static_cast<i32>(program_counter_) + 1 + d);
                advance_cycles(2U);
            } else {
                program_counter_ += 1U;
                advance_cycles(1U);
            }
            return;
        case 0xF400: // BRCC / BRSH
            if (!flag_c_) {
                i32 d = static_cast<i32>((opcode >> 3U) & 0x7FU);
                if (d & 0x40) d -= 0x80;
                program_counter_ = static_cast<u32>(static_cast<i32>(program_counter_) + 1 + d);
                advance_cycles(2U);
            } else {
                program_counter_ += 1U;
                advance_cycles(1U);
            }
            return;
        default:
            break;
        }
        break;
    default:
        break;
    }
    
    // Slow path: LUT-based dispatch for rare instructions
    {
        const u16 descriptor_index = fast_decode_table_[opcode];
        if (descriptor_index != 0xFFFFU) {
            const auto& descriptor = instruction_table()[descriptor_index];
            
            DecodedInstruction instruction;
            instruction.opcode = opcode;
            instruction.word_address = program_counter_;
            instruction.words[0] = opcode;
            
            if ((opcode & 0xFE0EU) == 0x940CU || (opcode & 0xFE0EU) == 0x940EU ||
                (opcode & 0xFE0FU) == 0x9000U || (opcode & 0xFE0FU) == 0x9200U) {
                instruction.word_size = 2U;
                instruction.words[1] = static_cast<u16>(bus_->read_program_word(program_counter_ + 1U));
            } else {
                instruction.word_size = 1U;
            }
            
            if (trace_hook_ != nullptr) {
                std::string_view mnemonic = descriptor.mnemonic;
                if (mnemonic == "AND" && 
                    decode_destination_register(opcode) == decode_source_register(opcode)) {
                    mnemonic = "TST";
                }
                trace_hook_->on_instruction(instruction.word_address, instruction.opcode, mnemonic);
                if (state_ == CpuState::paused) {
                    return;
                }
            }
            
            (this->*descriptor.handler)(instruction);
            synchronize_if_needed();
            return;
        }
        state_ = CpuState::halted;
    }
}

DecodedInstruction AvrCpu::fetch() const noexcept
{
    const u16 opcode = static_cast<u16>(bus_ != nullptr ? bus_->read_program_word(program_counter_) : 0U);
    const u8 word_size = classify_word_size(opcode);
    return {
        .words = {
            opcode,
            static_cast<u16>(bus_ != nullptr ? bus_->read_program_word(program_counter_ + 1U) : 0U)
        },
        .opcode = opcode,
        .word_address = program_counter_,
        .word_size = word_size
    };
}

constexpr u8 AvrCpu::classify_word_size(const u16 opcode) noexcept
{
    if ((opcode & 0xFE0EU) == 0x940CU) {
        return 2U;  // JMP / CALL
    }

    if ((opcode & 0xFE0EU) == 0x940EU) {
        return 2U;  // CALL
    }

    if ((opcode & 0xFE0FU) == 0x9000U) {
        return 2U;  // LDS
    }

    if ((opcode & 0xFE0FU) == 0x9200U) {
        return 2U;  // STS
    }

    return 1U;
}

std::span<const AvrCpu::InstructionDescriptor> AvrCpu::instruction_table() noexcept
{
    static constexpr auto table = std::to_array<InstructionDescriptor>({
        {0xFFFFU, 0x0000U, "NOP", &AvrCpu::execute_nop},
        {0xF000U, 0xE000U, "LDI", &AvrCpu::execute_ldi},
        {0xFF00U, 0x0200U, "MULS", &AvrCpu::execute_muls},
        {0xFF88U, 0x0300U, "MULSU", &AvrCpu::execute_mulsu},
        {0xFF88U, 0x0308U, "FMUL", &AvrCpu::execute_fmul},
        {0xFF88U, 0x0380U, "FMULS", &AvrCpu::execute_fmuls},
        {0xFF88U, 0x0388U, "FMULSU", &AvrCpu::execute_fmulsu},
        {0xFF00U, 0x0100U, "MOVW", &AvrCpu::execute_movw},
        {0xFC00U, 0x2C00U, "MOV", &AvrCpu::execute_mov},
        {0xFC00U, 0x9C00U, "MUL", &AvrCpu::execute_mul},
        {0xFE0FU, 0x9400U, "COM", &AvrCpu::execute_com},
        {0xFE0FU, 0x9401U, "NEG", &AvrCpu::execute_neg},
        {0xFE0FU, 0x9402U, "SWAP", &AvrCpu::execute_swap},
        {0xFF00U, 0x9600U, "ADIW", &AvrCpu::execute_adiw},
        {0xFC00U, 0x0C00U, "ADD", &AvrCpu::execute_add},
        {0xFC00U, 0x1C00U, "ADC", &AvrCpu::execute_adc},
        {0xFE0FU, 0x9403U, "INC", &AvrCpu::execute_inc},
        {0xFE0FU, 0x9405U, "ASR", &AvrCpu::execute_asr},
        {0xFE0FU, 0x9406U, "LSR", &AvrCpu::execute_lsr},
        {0xFE0FU, 0x9407U, "ROR", &AvrCpu::execute_ror},
        {0xFC00U, 0x0800U, "SBC", &AvrCpu::execute_sbc},
        {0xF000U, 0x4000U, "SBCI", &AvrCpu::execute_sbci},
        {0xFC00U, 0x1800U, "SUB", &AvrCpu::execute_sub},
        {0xFE0FU, 0x940AU, "DEC", &AvrCpu::execute_dec},
        {0xFF00U, 0x9700U, "SBIW", &AvrCpu::execute_sbiw},
        {0xF000U, 0x3000U, "CPI", &AvrCpu::execute_cpi},
        {0xF000U, 0x5000U, "SUBI", &AvrCpu::execute_subi},
        {0xF000U, 0x7000U, "ANDI", &AvrCpu::execute_andi},
        {0xFC00U, 0x1400U, "CP", &AvrCpu::execute_cp},
        {0xFC00U, 0x0400U, "CPC", &AvrCpu::execute_cpc},
        {0xFC00U, 0x1000U, "CPSE", &AvrCpu::execute_cpse},
        {0xFE08U, 0xFC00U, "SBRC", &AvrCpu::execute_sbrc},
        {0xFE08U, 0xFE00U, "SBRS", &AvrCpu::execute_sbrs},
        {0xFE08U, 0xFA00U, "BST", &AvrCpu::execute_bst},
        {0xFE08U, 0xF800U, "BLD", &AvrCpu::execute_bld},
        {0xFF0FU, 0xEF0FU, "SER", &AvrCpu::execute_ser},
        {0xFC00U, 0x2000U, "AND", &AvrCpu::execute_and},
        {0xFC00U, 0x2800U, "OR", &AvrCpu::execute_or},
        {0xF000U, 0x6000U, "ORI", &AvrCpu::execute_ori},
        {0xFC00U, 0x2400U, "EOR", &AvrCpu::execute_eor},
        {0xFF00U, 0x9800U, "CBI", &AvrCpu::execute_cbi},
        {0xFF00U, 0x9900U, "SBIC", &AvrCpu::execute_sbic},
        {0xFF00U, 0x9A00U, "SBI", &AvrCpu::execute_sbi},
        {0xFF00U, 0x9B00U, "SBIS", &AvrCpu::execute_sbis},
        {0xF800U, 0xB000U, "IN", &AvrCpu::execute_in},
        {0xF800U, 0xB800U, "OUT", &AvrCpu::execute_out},
        {0xFE0FU, 0x900CU, "LD X", &AvrCpu::execute_ld_x},
        {0xFE0FU, 0x900DU, "LD X+", &AvrCpu::execute_ld_x_postinc},
        {0xFE0FU, 0x900EU, "LD -X", &AvrCpu::execute_ld_x_predec},
        {0xFE0FU, 0x8008U, "LD Y", &AvrCpu::execute_ld_y},
        {0xFE0FU, 0x9008U, "LD Y", &AvrCpu::execute_ld_y},
        {0xFE0FU, 0x9108U, "LD Y", &AvrCpu::execute_ld_y},
        {0xFE0FU, 0x9009U, "LD Y+", &AvrCpu::execute_ld_y_postinc},
        {0xFE0FU, 0x900AU, "LD -Y", &AvrCpu::execute_ld_y_predec},
        {0xD208U, 0x8008U, "LDD Y+q", &AvrCpu::execute_ldd_y},
        {0xFE0FU, 0x8000U, "LD Z", &AvrCpu::execute_ld_z},
        {0xFE0FU, 0x9001U, "LD Z+", &AvrCpu::execute_ld_z_postinc},
        {0xFE0FU, 0x9002U, "LD -Z", &AvrCpu::execute_ld_z_predec},
        {0xD208U, 0x8000U, "LDD Z+q", &AvrCpu::execute_ldd_z},
        {0xFFFFU, 0x95C8U, "LPM", &AvrCpu::execute_lpm},
        {0xFFFFU, 0x95D8U, "ELPM", &AvrCpu::execute_elpm},
        {0xFE0FU, 0x9004U, "LPM Z", &AvrCpu::execute_lpm_z},
        {0xFE0FU, 0x9006U, "ELPM Z", &AvrCpu::execute_elpm_z},
        {0xFE0FU, 0x9005U, "LPM Z+", &AvrCpu::execute_lpm_z_postinc},
        {0xFE0FU, 0x9007U, "ELPM Z+", &AvrCpu::execute_elpm_z_postinc},
        {0xFFFFU, 0x95E8U, "SPM", &AvrCpu::execute_spm},
        {0xFE0FU, 0x9000U, "LDS", &AvrCpu::execute_lds},
        {0xFE0FU, 0x920CU, "ST X", &AvrCpu::execute_st_x},
        {0xFE0FU, 0x920DU, "ST X+", &AvrCpu::execute_st_x_postinc},
        {0xFE0FU, 0x920EU, "ST -X", &AvrCpu::execute_st_x_predec},
        {0xFE0FU, 0x8208U, "ST Y", &AvrCpu::execute_st_y},
        {0xFE0FU, 0x9208U, "ST Y", &AvrCpu::execute_st_y},
        {0xFE0FU, 0x9308U, "ST Y", &AvrCpu::execute_st_y},
        {0xFE0FU, 0x9209U, "ST Y+", &AvrCpu::execute_st_y_postinc},
        {0xFE0FU, 0x920AU, "ST -Y", &AvrCpu::execute_st_y_predec},
        {0xD208U, 0x8208U, "STD Y+q", &AvrCpu::execute_std_y},
        {0xFE0FU, 0x8200U, "ST Z", &AvrCpu::execute_st_z},
        {0xFE0FU, 0x9201U, "ST Z+", &AvrCpu::execute_st_z_postinc},
        {0xFE0FU, 0x9202U, "ST -Z", &AvrCpu::execute_st_z_predec},
        {0xD208U, 0x8200U, "STD Z+q", &AvrCpu::execute_std_z},
        {0xFE0FU, 0x9200U, "STS", &AvrCpu::execute_sts},
        {0xFE0FU, 0x920FU, "PUSH", &AvrCpu::execute_push},
        {0xFE0FU, 0x900FU, "POP", &AvrCpu::execute_pop},
        {0xF000U, 0xD000U, "RCALL", &AvrCpu::execute_rcall},
        {0xF000U, 0xC000U, "RJMP", &AvrCpu::execute_rjmp},
        {0xFFFFU, 0x9409U, "IJMP", &AvrCpu::execute_ijmp},
        {0xFE0EU, 0x940EU, "CALL", &AvrCpu::execute_call},
        {0xFE0EU, 0x940CU, "JMP", &AvrCpu::execute_jmp},
        {0xFFFFU, 0x9419U, "EIJMP", &AvrCpu::execute_eijmp},
        {0xFFFFU, 0x9509U, "ICALL", &AvrCpu::execute_icall},
        {0xFFFFU, 0x9519U, "EICALL", &AvrCpu::execute_eicall},
        {0xFFFFU, 0x9508U, "RET", &AvrCpu::execute_ret},
        {0xFFFFU, 0x9518U, "RETI", &AvrCpu::execute_reti},
        {0xFFFFU, 0x9588U, "SLEEP", &AvrCpu::execute_sleep},
        {0xFFFFU, 0x9598U, "BREAK", &AvrCpu::execute_break},
        {0xFFFFU, 0x95A8U, "WDR", &AvrCpu::execute_wdr},
        {0xFFFFU, 0x94F8U, "CLI", &AvrCpu::execute_cli},
        {0xFFFFU, 0x9478U, "SEI", &AvrCpu::execute_sei},
        {0xFF8FU, 0x9488U, "BCLR", &AvrCpu::execute_bclr},
        {0xFF8FU, 0x9408U, "BSET", &AvrCpu::execute_bset},
        {0xFC07U, 0xF000U, "BRCS", &AvrCpu::execute_brcs},
        {0xFC07U, 0xF400U, "BRCC", &AvrCpu::execute_brcc},
        {0xFC07U, 0xF002U, "BRMI", &AvrCpu::execute_brmi},
        {0xFC07U, 0xF402U, "BRPL", &AvrCpu::execute_brpl},
        {0xFC07U, 0xF001U, "BREQ", &AvrCpu::execute_breq},
        {0xFC07U, 0xF401U, "BRNE", &AvrCpu::execute_brne},
        {0xFC07U, 0xF003U, "BRVS", &AvrCpu::execute_brbs},
        {0xFC07U, 0xF403U, "BRVC", &AvrCpu::execute_brbc},
        {0xFC07U, 0xF004U, "BRLT", &AvrCpu::execute_brbs},
        {0xFC07U, 0xF404U, "BRGE", &AvrCpu::execute_brbc},
        {0xFC07U, 0xF005U, "BRHS", &AvrCpu::execute_brbs},
        {0xFC07U, 0xF405U, "BRHC", &AvrCpu::execute_brbc},
        {0xFC07U, 0xF006U, "BRTS", &AvrCpu::execute_brbs},
        {0xFC07U, 0xF406U, "BRTC", &AvrCpu::execute_brbc},
        {0xFC07U, 0xF007U, "BRIE", &AvrCpu::execute_brbs},
        {0xFC07U, 0xF407U, "BRID", &AvrCpu::execute_brbc},
    });
    return table;
}

void AvrCpu::decode_and_execute(const DecodedInstruction& instruction)
{
    const u16 descriptor_index = fast_decode_table_[instruction.opcode];
    if (descriptor_index != 0xFFFFU) {
        const auto& descriptor = instruction_table()[descriptor_index];
        
        if (trace_hook_ != nullptr) {
            // Handle TST mnemonic for tracing
            std::string_view mnemonic = descriptor.mnemonic;
            if (mnemonic == "AND" && 
                decode_destination_register(instruction.opcode) == decode_source_register(instruction.opcode)) {
                mnemonic = "TST";
            }
            trace_hook_->on_instruction(instruction.word_address, instruction.opcode, mnemonic);
            if (state_ == CpuState::paused) return;
        }

        (this->*descriptor.handler)(instruction);
        return;
    }

    state_ = CpuState::halted;
}

constexpr u8 AvrCpu::decode_destination_register(const u16 opcode) noexcept
{
    return static_cast<u8>((opcode >> 4U) & 0x1FU);
}

constexpr u8 AvrCpu::decode_source_register(const u16 opcode) noexcept
{
    return static_cast<u8>((opcode & 0x0FU) | ((opcode >> 5U) & 0x10U));
}

constexpr i32 AvrCpu::decode_relative_offset(const u16 opcode, const u8 width_bits) noexcept
{
    const i32 mask = (1 << width_bits) - 1;
    i32 displacement = static_cast<i32>((opcode >> 3U) & mask);
    const i32 sign_bit = 1 << (width_bits - 1U);
    if ((displacement & sign_bit) != 0) {
        displacement -= (1 << width_bits);
    }
    return displacement;
}

constexpr SregFlag AvrCpu::decode_sreg_bit(const u16 opcode) noexcept
{
    return static_cast<SregFlag>((opcode >> 4U) & 0x07U);
}

constexpr SregFlag AvrCpu::decode_branch_sreg_bit(const u16 opcode) noexcept
{
    return static_cast<SregFlag>(opcode & 0x07U);
}

constexpr u16 AvrCpu::read_register_pair(const u8 low_register_index) const noexcept
{
    return static_cast<u16>(gpr_[low_register_index] |
                            (static_cast<u16>(gpr_[low_register_index + 1U]) << 8U));
}

void AvrCpu::write_register_pair(const u8 low_register_index, const u16 value) noexcept
{
    write_register(low_register_index, static_cast<u8>(value & 0x00FFU));
    write_register(static_cast<u8>(low_register_index + 1U), static_cast<u8>((value >> 8U) & 0x00FFU));
}

constexpr u16 AvrCpu::x_pointer() const noexcept
{
    return read_register_pair(26U);
}

constexpr u16 AvrCpu::y_pointer() const noexcept
{
    return read_register_pair(28U);
}

constexpr u16 AvrCpu::z_pointer() const noexcept
{
    return read_register_pair(30U);
}

constexpr u8 AvrCpu::decode_displacement_q(const u16 opcode) noexcept
{
    return static_cast<u8>(((opcode >> 8U) & 0x20U) | ((opcode >> 7U) & 0x18U) | (opcode & 0x07U));
}

void AvrCpu::set_x_pointer(const u16 value) noexcept
{
    write_register_pair(26U, value);
}

void AvrCpu::set_y_pointer(const u16 value) noexcept
{
    write_register_pair(28U, value);
}

void AvrCpu::set_z_pointer(const u16 value) noexcept
{
    write_register_pair(30U, value);
}


u8 AvrCpu::active_clock_domains_slow() const noexcept
{
    if (state_ == CpuState::running) {
        return 0xFFU; // All domains active
    }

    if (state_ == CpuState::sleeping) {
        SleepMode mode;
        if (slp_ctrl_) {
            switch (slp_ctrl_->sleep_mode()) {
                case 0: mode = SleepMode::idle; break;
                case 1: mode = SleepMode::standby; break;
                case 2: mode = SleepMode::power_down; break;
                default: mode = SleepMode::power_down; break;
            }
        } else {
            mode = control_regs_->current_sleep_mode();
        }
        u8 domains = static_cast<u8>(ClockDomain::watchdog); // Watchdog always active

        switch (mode) {
        case SleepMode::idle:
            // All clocks active except CPU clock
            domains |= static_cast<u8>(ClockDomain::io) | 
                       static_cast<u8>(ClockDomain::adc) | 
                       static_cast<u8>(ClockDomain::async_timer);
            break;
        case SleepMode::adc_noise_reduction:
            // ADC, TWI, Timer2, Watchdog, and I/O can be active
            domains |= static_cast<u8>(ClockDomain::adc) | 
                       static_cast<u8>(ClockDomain::io) | 
                       static_cast<u8>(ClockDomain::async_timer);
            break;
        case SleepMode::power_down:
            // Only Watchdog and TWI (Address Match) / INT / Pin Change can wake.
            // Clock-based peripherals except Async Timer stop.
            break;
        case SleepMode::power_save:
            // Same as Power Down but Async Timer (Timer 2) keeps running
            domains |= static_cast<u8>(ClockDomain::async_timer);
            break;
        case SleepMode::standby:
        case SleepMode::extended_standby:
            // Oscillator runs, but clocks to peripherals gated similarly to Power Down/Save
            if (mode == SleepMode::extended_standby) {
                domains |= static_cast<u8>(ClockDomain::async_timer);
            }
            break;
        default:
            break;
        }
        return domains;
    }

    return 0U; // Halted or unknown state
}

void AvrCpu::synchronize_if_needed()
{
    if (sync_engine_ == nullptr) {
        return;
    }

    if (sync_engine_->should_pause(cycles_)) {
        state_ = CpuState::paused;
        sync_engine_->wait_until_resumed();
        state_ = CpuState::running;
    }
}

void AvrCpu::publish_pending_pin_changes()
{
    if (bus_ == nullptr || sync_engine_ == nullptr) {
        return;
    }

    PinStateChange change;
    while (bus_->consume_pin_change(change)) {
        change.cycle_stamp = cycles_;
        sync_engine_->on_pin_state_changed(change);
    }
}


bool AvrCpu::service_interrupt_if_needed()
{
    refresh_interrupt_pending();
    if (!interrupt_pending_ || !flag(SregFlag::interrupt) || bus_ == nullptr) {
        if (interrupt_pending_) {
             Logger::debug("AvrCpu::service_interrupt_if_needed: interrupt pending but I flag is " + std::to_string(flag(SregFlag::interrupt)));
        }
        return false;
    }

    InterruptRequest request;
    if (!bus_->consume_interrupt_request(request, active_clock_domains())) {
        interrupt_pending_ = false;
        return false;
    }
    if (trace_hook_ != nullptr) {
        trace_hook_->on_interrupt(request.vector_index);
    }

    const u32 old_pc = program_counter_;
    set_flag(SregFlag::interrupt, false);
    program_counter_ = interrupt_vector_word_address(request.vector_index);
    interrupt_pending_ = false;
    state_ = CpuState::running;
    ++interrupt_depth_;

    // Handle CPUINT Status
    if (auto* cpu_int = bus_->cpu_int()) {
        if (cpu_int->is_lvl1_vector(request.vector_index)) {
            cpu_int->set_executing_lvl1(true);
        } else {
            cpu_int->set_executing_lvl0(true);
        }
    }

    if (bus_->device().flash_words > 65536) {
        // 3-byte PC devices: 5 cycles
        // Cycle 1: Start vectoring sequence
        advance_cycles(1U);

        // Cycle 2: Push PC low byte
        write_data_bus(stack_pointer_, static_cast<u8>(old_pc & 0xFFU));
        --stack_pointer_;
        advance_cycles(1U);

        // Cycle 3: Push PC mid byte
        write_data_bus(stack_pointer_, static_cast<u8>((old_pc >> 8U) & 0xFFU));
        --stack_pointer_;
        advance_cycles(1U);

        // Cycle 4: Push PC high byte
        write_data_bus(stack_pointer_, static_cast<u8>((old_pc >> 16U) & 0xFFU));
        --stack_pointer_;
        advance_cycles(1U);

        // Cycle 5: Jump vector
        advance_cycles(1U);
    } else {
        // 2-byte PC devices: 4 cycles
        // Cycle 1: Start vectoring sequence
        advance_cycles(1U);

        // Cycle 2: Push PC low byte
        write_data_bus(stack_pointer_, static_cast<u8>(old_pc & 0xFFU));
        --stack_pointer_;
        advance_cycles(1U);

        // Cycle 3: Push PC high byte
        write_data_bus(stack_pointer_, static_cast<u8>((old_pc >> 8U) & 0xFFU));
        --stack_pointer_;
        advance_cycles(1U);

        // Cycle 4: Jump vector
        advance_cycles(1U);
    }
    return true;
}

u32 AvrCpu::interrupt_vector_word_address(u8 vector_index) const noexcept
{
    u8 vec_size = bus_->device().interrupt_vector_size;
    u32 base_addr = 0;

    if (auto* cpu_int = bus_->cpu_int()) {
        if (cpu_int->compact_vector_table()) {
            vec_size = 2;
        }
        if (cpu_int->ivsel()) {
            // On AVR8X, IVSEL typically moves vectors to the start of the boot section.
            // We use the boot_start_address from the device descriptor if available.
            base_addr = bus_->device().boot_start_address;
        }
    }

    // interrupt_vector_size is in bytes, convert to words
    return base_addr + static_cast<u32>(vector_index * (vec_size / 2U));
}

void AvrCpu::update_add_flags(const u8 lhs, const u8 rhs, const u8 result, const bool carry_in) noexcept
{
    const bool r7 = (result & 0x80U) != 0U;
    const bool d7 = (lhs & 0x80U) != 0U;
    const bool s7 = (rhs & 0x80U) != 0U;
    const bool r3 = (result & 0x08U) != 0U;
    const bool d3 = (lhs & 0x08U) != 0U;
    const bool s3 = (rhs & 0x08U) != 0U;

    // H: Rd3 & Rr3 | Rr3 & !R3 | !R3 & Rd3
    set_flag(SregFlag::halfCarry, (d3 && s3) || (s3 && !r3) || (!r3 && d3));
    // V: Rd7 & Rr7 & !R7 | !Rd7 & !Rr7 & R7
    set_flag(SregFlag::overflow, (d7 && s7 && !r7) || (!d7 && !s7 && r7));
    set_flag(SregFlag::negative, r7);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    // C: Rd7 & Rr7 | Rr7 & !R7 | !R7 & Rd7
    set_flag(SregFlag::carry, (d7 && s7) || (s7 && !r7) || (!r7 && d7));
    (void)carry_in;
}

void AvrCpu::update_sub_flags(const u8 lhs, const u8 rhs, const u8 result, const bool carry_in) noexcept
{
    const bool r7 = (result & 0x80U) != 0U;
    const bool d7 = (lhs & 0x80U) != 0U;
    const bool s7 = (rhs & 0x80U) != 0U;
    const bool r3 = (result & 0x08U) != 0U;
    const bool d3 = (lhs & 0x08U) != 0U;
    const bool s3 = (rhs & 0x08U) != 0U;
    (void)carry_in;

    // H: !Rd3 & Rr3 | Rr3 & R3 | R3 & !Rd3
    set_flag(SregFlag::halfCarry, (!d3 && s3) || (s3 && r3) || (r3 && !d3));
    // V: Rd7 & !Rr7 & !R7 | !Rd7 & Rr7 & R7
    set_flag(SregFlag::overflow, (d7 && !s7 && !r7) || (!d7 && s7 && r7));
    set_flag(SregFlag::negative, r7);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    // C: !Rd7 & Rr7 | Rr7 & R7 | R7 & !Rd7
    set_flag(SregFlag::carry, (!d7 && s7) || (s7 && r7) || (r7 && !d7));
}

void AvrCpu::update_logic_flags(const u8 result) noexcept
{
    set_flag(SregFlag::negative, (result & 0x80U) != 0U);
    set_flag(SregFlag::overflow, false);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
}

void AvrCpu::update_adiw_flags(const u16 lhs, const u16 result) noexcept
{
    const bool lhs15 = (lhs & 0x8000U) != 0U;
    const bool res15 = (result & 0x8000U) != 0U;
    set_flag(SregFlag::negative, res15);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::overflow, !lhs15 && res15);
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    set_flag(SregFlag::carry, !res15 && lhs15);
}

void AvrCpu::update_sbiw_flags(const u16 lhs, const u16 result) noexcept
{
    const bool lhs15 = (lhs & 0x8000U) != 0U;
    const bool res15 = (result & 0x8000U) != 0U;
    set_flag(SregFlag::negative, res15);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::overflow, lhs15 && !res15);
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    set_flag(SregFlag::carry, res15 && !lhs15);
}

void AvrCpu::push_byte(const u8 value) noexcept
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    write_data_bus(stack_pointer_, value);
    --stack_pointer_;
}

u8 AvrCpu::pop_byte() noexcept
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return 0U;
    }

    ++stack_pointer_;
    return read_data_bus(stack_pointer_);
}

void AvrCpu::push_word(const u16 value) noexcept
{
    push_byte(static_cast<u8>(value & 0xFFU));
    push_byte(static_cast<u8>((value >> 8U) & 0xFFU));
}

u16 AvrCpu::pop_word() noexcept
{
    u16 value = static_cast<u16>(pop_byte()) << 8U;
    value |= static_cast<u16>(pop_byte());
    return value;
}

void AvrCpu::push_pc(u32 address) noexcept
{
    push_byte(static_cast<u8>(address & 0xFFU));
    push_byte(static_cast<u8>((address >> 8U) & 0xFFU));
    if (bus_->device().flash_words > 65536) {
        push_byte(static_cast<u8>((address >> 16U) & 0xFFU));
    }
}

u32 AvrCpu::pop_pc() noexcept
{
    u32 address = 0;
    if (bus_->device().flash_words > 65536) {
        address = static_cast<u32>(pop_byte()) << 16U;
        address |= static_cast<u32>(pop_byte()) << 8U;
        address |= static_cast<u32>(pop_byte());
    } else {
        address = static_cast<u32>(pop_byte()) << 8U;
        address |= static_cast<u32>(pop_byte());
    }
    return address;
}

void AvrCpu::execute_load_indirect(const u8 destination,
                                   const u16 address,
                                   const bool post_increment,
                                   const u8 pointer_low_register) noexcept
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    write_register(destination, read_data_bus(address));
    advance_cycles(2U);
    if (post_increment) {
        write_register_pair(pointer_low_register, static_cast<u16>(address + 1U));
    }
    ++program_counter_;
}

void AvrCpu::execute_store_indirect(const u16 address,
                                    const u8 source,
                                    const bool post_increment,
                                    const u8 pointer_low_register) noexcept
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    write_data_bus(address, gpr_[source]);
    advance_cycles(2U);
    if (post_increment) {
        write_register_pair(pointer_low_register, static_cast<u16>(address + 1U));
    }
    ++program_counter_;
}

void AvrCpu::execute_nop(const DecodedInstruction& instruction)
{
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_ldi(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x0FU));
    const u8 immediate = static_cast<u8>(((instruction.opcode >> 4U) & 0xF0U) | (instruction.opcode & 0x0FU));
    write_register(destination, immediate);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_movw(const DecodedInstruction& instruction)
{
    const u8 destination_low = static_cast<u8>(((instruction.opcode >> 4U) & 0x0FU) * 2U);
    const u8 source_low = static_cast<u8>((instruction.opcode & 0x0FU) * 2U);
    write_register_pair(destination_low, read_register_pair(source_low));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_mov(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    write_register(destination, gpr_[source]);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_mul(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const u16 product = static_cast<u16>(gpr_[destination]) * static_cast<u16>(gpr_[source]);
    write_register(0U, static_cast<u8>(product & 0x00FFU));
    write_register(1U, static_cast<u8>((product >> 8U) & 0x00FFU));
    set_flag(SregFlag::zero, product == 0U);
    set_flag(SregFlag::carry, (product & 0x8000U) != 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(2U);
}

void AvrCpu::execute_muls(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x0FU));
    const u8 source = static_cast<u8>(16U + (instruction.opcode & 0x0FU));
    const auto lhs = static_cast<std::int16_t>(static_cast<std::int8_t>(gpr_[destination]));
    const auto rhs = static_cast<std::int16_t>(static_cast<std::int8_t>(gpr_[source]));
    const u16 product = static_cast<u16>(lhs * rhs);
    write_register(0U, static_cast<u8>(product & 0x00FFU));
    write_register(1U, static_cast<u8>((product >> 8U) & 0x00FFU));
    set_flag(SregFlag::zero, product == 0U);
    set_flag(SregFlag::carry, (product & 0x8000U) != 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(2U);
}

void AvrCpu::execute_mulsu(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x07U));
    const u8 source = static_cast<u8>(16U + (instruction.opcode & 0x07U));
    const auto lhs = static_cast<std::int16_t>(static_cast<std::int8_t>(gpr_[destination]));
    const auto rhs = static_cast<std::uint16_t>(gpr_[source]);
    const u16 product = static_cast<u16>(lhs * static_cast<std::int16_t>(rhs));
    write_register(0U, static_cast<u8>(product & 0x00FFU));
    write_register(1U, static_cast<u8>((product >> 8U) & 0x00FFU));
    set_flag(SregFlag::zero, product == 0U);
    set_flag(SregFlag::carry, (product & 0x8000U) != 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(2U);
}

void AvrCpu::execute_fmul(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x07U));
    const u8 source = static_cast<u8>(16U + (instruction.opcode & 0x07U));
    const u16 product = static_cast<u16>(gpr_[destination]) * static_cast<u16>(gpr_[source]);
    const u16 result = static_cast<u16>(product << 1U);
    write_register(0U, static_cast<u8>(result & 0x00FFU));
    write_register(1U, static_cast<u8>((result >> 8U) & 0x00FFU));
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::carry, (product & 0x8000U) != 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(2U);
}

void AvrCpu::execute_fmuls(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x07U));
    const u8 source = static_cast<u8>(16U + (instruction.opcode & 0x07U));
    const auto lhs = static_cast<std::int16_t>(static_cast<std::int8_t>(gpr_[destination]));
    const auto rhs = static_cast<std::int16_t>(static_cast<std::int8_t>(gpr_[source]));
    const u16 product = static_cast<u16>(lhs * rhs);
    const u16 result = static_cast<u16>(product << 1U);
    write_register(0U, static_cast<u8>(result & 0x00FFU));
    write_register(1U, static_cast<u8>((result >> 8U) & 0x00FFU));
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::carry, (product & 0x8000U) != 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(2U);
}

void AvrCpu::execute_fmulsu(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x07U));
    const u8 source = static_cast<u8>(16U + (instruction.opcode & 0x07U));
    const auto lhs = static_cast<std::int16_t>(static_cast<std::int8_t>(gpr_[destination]));
    const auto rhs = static_cast<std::uint16_t>(gpr_[source]);
    const u16 product = static_cast<u16>(lhs * static_cast<std::int16_t>(rhs));
    const u16 result = static_cast<u16>(product << 1U);
    write_register(0U, static_cast<u8>(result & 0x00FFU));
    write_register(1U, static_cast<u8>((result >> 8U) & 0x00FFU));
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::carry, (product & 0x8000U) != 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(2U);
}

void AvrCpu::execute_com(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 result = static_cast<u8>(~gpr_[destination]);
    write_register(destination, result);
    set_flag(SregFlag::negative, (result & 0x80U) != 0U);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::overflow, false);
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    set_flag(SregFlag::carry, true);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_neg(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 lhs = gpr_[destination];
    const u8 result = static_cast<u8>(0U - lhs);
    write_register(destination, result);
    update_sub_flags(0U, lhs, result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_swap(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 value = gpr_[destination];
    write_register(destination, static_cast<u8>((value << 4U) | (value >> 4U)));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_adiw(const DecodedInstruction& instruction)
{
    const u8 pair_selector = static_cast<u8>((instruction.opcode >> 4U) & 0x03U);
    const u8 low_register = static_cast<u8>(24U + (pair_selector * 2U));
    const u8 immediate = static_cast<u8>(((instruction.opcode >> 2U) & 0x30U) |
                                         (instruction.opcode & 0x0FU));
    const u16 lhs = read_register_pair(low_register);
    const u16 result = static_cast<u16>(lhs + immediate);
    write_register_pair(low_register, result);
    update_adiw_flags(lhs, result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(2U);
}

void AvrCpu::execute_add(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const u8 lhs = gpr_[destination];
    const u8 rhs = gpr_[source];
    const u8 result = static_cast<u8>(lhs + rhs);
    write_register(destination, result);
    update_add_flags(lhs, rhs, result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_adc(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const bool carry_in = flag(SregFlag::carry);
    const u8 lhs = gpr_[destination];
    const u8 rhs = gpr_[source];
    const u8 result = static_cast<u8>(lhs + rhs + (carry_in ? 1U : 0U));
    write_register(destination, result);
    const bool prev_zero = flag(SregFlag::zero);
    update_add_flags(lhs, rhs, result, carry_in);
    set_flag(SregFlag::zero, prev_zero && result == 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_inc(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 result = static_cast<u8>(gpr_[destination] + 1U);
    write_register(destination, result);
    set_flag(SregFlag::negative, (result & 0x80U) != 0U);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::overflow, result == 0x80U);
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_asr(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 value = gpr_[destination];
    const bool carry_out = (value & 0x01U) != 0U;
    const u8 result = static_cast<u8>((value >> 1U) | (value & 0x80U));
    write_register(destination, result);
    set_flag(SregFlag::negative, (result & 0x80U) != 0U);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::carry, carry_out);
    set_flag(SregFlag::overflow, flag(SregFlag::negative) != flag(SregFlag::carry));
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_lsr(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const bool carry_out = (gpr_[destination] & 0x01U) != 0U;
    const u8 result = static_cast<u8>(gpr_[destination] >> 1U);
    write_register(destination, result);
    set_flag(SregFlag::negative, false);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::carry, carry_out);
    set_flag(SregFlag::overflow, flag(SregFlag::negative) != flag(SregFlag::carry));
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_ror(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const bool carry_in = flag(SregFlag::carry);
    const bool carry_out = (gpr_[destination] & 0x01U) != 0U;
    const u8 result = static_cast<u8>((gpr_[destination] >> 1U) | (carry_in ? 0x80U : 0U));
    write_register(destination, result);
    set_flag(SregFlag::negative, (result & 0x80U) != 0U);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::carry, carry_out);
    set_flag(SregFlag::overflow, flag(SregFlag::negative) != flag(SregFlag::carry));
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_sbc(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const bool carry_in = flag(SregFlag::carry);
    const u8 lhs = gpr_[destination];
    const u8 rhs = gpr_[source];
    const u8 result = static_cast<u8>(lhs - rhs - (carry_in ? 1U : 0U));
    write_register(destination, result);
    const bool prev_zero = flag(SregFlag::zero);
    update_sub_flags(lhs, rhs, result, carry_in);
    set_flag(SregFlag::zero, prev_zero && result == 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_sbci(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x0FU));
    const u8 immediate = static_cast<u8>(((instruction.opcode >> 4U) & 0xF0U) | (instruction.opcode & 0x0FU));
    const bool carry_in = flag(SregFlag::carry);
    const u8 lhs = gpr_[destination];
    const u8 result = static_cast<u8>(lhs - immediate - (carry_in ? 1U : 0U));
    write_register(destination, result);
    const bool prev_zero = flag(SregFlag::zero);
    update_sub_flags(lhs, immediate, result, carry_in);
    set_flag(SregFlag::zero, prev_zero && result == 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_sub(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const u8 lhs = gpr_[destination];
    const u8 rhs = gpr_[source];
    const u8 result = static_cast<u8>(lhs - rhs);
    write_register(destination, result);
    update_sub_flags(lhs, rhs, result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_dec(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 result = static_cast<u8>(gpr_[destination] - 1U);
    write_register(destination, result);
    set_flag(SregFlag::negative, (result & 0x80U) != 0U);
    set_flag(SregFlag::zero, result == 0U);
    set_flag(SregFlag::overflow, result == 0x7FU);
    set_flag(SregFlag::sign, flag(SregFlag::negative) != flag(SregFlag::overflow));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_sbiw(const DecodedInstruction& instruction)
{
    const u8 pair_selector = static_cast<u8>((instruction.opcode >> 4U) & 0x03U);
    const u8 low_register = static_cast<u8>(24U + (pair_selector * 2U));
    const u8 immediate = static_cast<u8>(((instruction.opcode >> 2U) & 0x30U) |
                                         (instruction.opcode & 0x0FU));
    const u16 lhs = read_register_pair(low_register);
    const u16 result = static_cast<u16>(lhs - immediate);
    write_register_pair(low_register, result);
    update_sbiw_flags(lhs, result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(2U);
}

void AvrCpu::execute_cpi(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x0FU));
    const u8 immediate = static_cast<u8>(((instruction.opcode >> 4U) & 0xF0U) | (instruction.opcode & 0x0FU));
    const u8 lhs = gpr_[destination];
    const u8 result = static_cast<u8>(lhs - immediate);
    update_sub_flags(lhs, immediate, result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_cp(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const u8 lhs = gpr_[destination];
    const u8 rhs = gpr_[source];
    const u8 result = static_cast<u8>(lhs - rhs);
    update_sub_flags(lhs, rhs, result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_cpc(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const bool carry_in = flag(SregFlag::carry);
    const u8 lhs = gpr_[destination];
    const u8 rhs = gpr_[source];
    const u8 result = static_cast<u8>(lhs - rhs - (carry_in ? 1U : 0U));
    const bool prev_zero = flag(SregFlag::zero);
    update_sub_flags(lhs, rhs, result, carry_in);
    set_flag(SregFlag::zero, prev_zero && result == 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_cpse(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const bool equal = gpr_[destination] == gpr_[source];
    if (equal) {
        const u16 next_opcode = bus_ != nullptr ? bus_->read_program_word(instruction.word_address + 1U) : 0U;
        const u8 skip_words = classify_word_size(next_opcode);
        program_counter_ = instruction.word_address + 1U + skip_words;
        advance_cycles(skip_words == 2U ? 3U : 2U);
        return;
    }

    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_sbrc(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 bit_index = static_cast<u8>(instruction.opcode & 0x07U);
    const bool bit_is_clear = (gpr_[destination] & (1U << bit_index)) == 0U;
    if (bit_is_clear) {
        const u16 next_opcode = bus_ != nullptr ? bus_->read_program_word(instruction.word_address + 1U) : 0U;
        const u8 skip_words = classify_word_size(next_opcode);
        program_counter_ = instruction.word_address + 1U + skip_words;
        advance_cycles(skip_words == 2U ? 3U : 2U);
        return;
    }

    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_sbrs(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 bit_index = static_cast<u8>(instruction.opcode & 0x07U);
    const bool bit_is_set = (gpr_[destination] & (1U << bit_index)) != 0U;
    if (bit_is_set) {
        const u16 next_opcode = bus_ != nullptr ? bus_->read_program_word(instruction.word_address + 1U) : 0U;
        const u8 skip_words = classify_word_size(next_opcode);
        program_counter_ = instruction.word_address + 1U + skip_words;
        advance_cycles(skip_words == 2U ? 3U : 2U);
        return;
    }

    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_bst(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 bit_index = static_cast<u8>(instruction.opcode & 0x07U);
    set_flag(SregFlag::transfer, (gpr_[destination] & (1U << bit_index)) != 0U);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_bld(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 bit_index = static_cast<u8>(instruction.opcode & 0x07U);
    const u8 mask = static_cast<u8>(1U << bit_index);
    if (flag(SregFlag::transfer)) {
        write_register(destination, static_cast<u8>(gpr_[destination] | mask));
    } else {
        write_register(destination, static_cast<u8>(gpr_[destination] & static_cast<u8>(~mask)));
    }
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_tst(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 result = gpr_[destination];
    update_logic_flags(result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_ser(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x0FU));
    write_register(destination, 0xFFU);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_subi(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x0FU));
    const u8 immediate = static_cast<u8>(((instruction.opcode >> 4U) & 0xF0U) | (instruction.opcode & 0x0FU));
    const u8 lhs = gpr_[destination];
    const u8 result = static_cast<u8>(lhs - immediate);
    write_register(destination, result);
    update_sub_flags(lhs, immediate, result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_andi(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x0FU));
    const u8 immediate = static_cast<u8>(((instruction.opcode >> 4U) & 0xF0U) | (instruction.opcode & 0x0FU));
    const u8 result = static_cast<u8>(gpr_[destination] & immediate);
    write_register(destination, result);
    update_logic_flags(result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_and(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const u8 result = static_cast<u8>(gpr_[destination] & gpr_[source]);
    write_register(destination, result);
    update_logic_flags(result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_or(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const u8 result = static_cast<u8>(gpr_[destination] | gpr_[source]);
    write_register(destination, result);
    update_logic_flags(result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_ori(const DecodedInstruction& instruction)
{
    const u8 destination = static_cast<u8>(16U + ((instruction.opcode >> 4U) & 0x0FU));
    const u8 immediate = static_cast<u8>(((instruction.opcode >> 4U) & 0xF0U) | (instruction.opcode & 0x0FU));
    const u8 result = static_cast<u8>(gpr_[destination] | immediate);
    write_register(destination, result);
    update_logic_flags(result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_eor(const DecodedInstruction& instruction)
{
    const u8 destination = decode_destination_register(instruction.opcode);
    const u8 source = decode_source_register(instruction.opcode);
    const u8 result = static_cast<u8>(gpr_[destination] ^ gpr_[source]);
    write_register(destination, result);
    update_logic_flags(result);
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_cbi(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u8 io_offset = static_cast<u8>((instruction.opcode >> 3U) & 0x1FU);
    const u8 bit_index = static_cast<u8>(instruction.opcode & 0x07U);
    const u16 address = MemoryBus::low_io_address(io_offset);
    bus_->set_in_rmw(true);
    const u8 value = static_cast<u8>(read_data_bus(address) & static_cast<u8>(~(1U << bit_index)));
    bus_->set_in_rmw(false);
    write_data_bus(address, value);
    ++program_counter_;
    advance_cycles(2U);
}

void AvrCpu::execute_sbic(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u8 io_offset = static_cast<u8>((instruction.opcode >> 3U) & 0x1FU);
    const u8 bit_index = static_cast<u8>(instruction.opcode & 0x07U);
    const u16 address = MemoryBus::low_io_address(io_offset);
    const bool bit_is_clear = (read_data_bus(address) & (1U << bit_index)) == 0U;
    if (bit_is_clear) {
        const u8 skip_words = classify_word_size(bus_->read_program_word(instruction.word_address + 1U));
        program_counter_ = instruction.word_address + 1U + skip_words;
        advance_cycles(skip_words == 2U ? 3U : 2U);
        return;
    }

    ++program_counter_;
    advance_cycles(1U);
}

void AvrCpu::execute_sbi(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u8 io_offset = static_cast<u8>((instruction.opcode >> 3U) & 0x1FU);
    const u8 bit_index = static_cast<u8>(instruction.opcode & 0x07U);
    const u16 address = MemoryBus::low_io_address(io_offset);
    bus_->set_in_rmw(true);
    const u8 value = static_cast<u8>(read_data_bus(address) | static_cast<u8>(1U << bit_index));
    bus_->set_in_rmw(false);
    write_data_bus(address, value);
    ++program_counter_;
    advance_cycles(2U);
}

void AvrCpu::execute_sbis(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u8 io_offset = static_cast<u8>((instruction.opcode >> 3U) & 0x1FU);
    const u8 bit_index = static_cast<u8>(instruction.opcode & 0x07U);
    const u16 address = MemoryBus::low_io_address(io_offset);
    const bool bit_is_set = (read_data_bus(address) & (1U << bit_index)) != 0U;
    if (bit_is_set) {
        const u8 skip_words = classify_word_size(bus_->read_program_word(instruction.word_address + 1U));
        program_counter_ = instruction.word_address + 1U + skip_words;
        advance_cycles(skip_words == 2U ? 3U : 2U);
        return;
    }

    ++program_counter_;
    advance_cycles(1U);
}

void AvrCpu::execute_in(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u8 destination =
        static_cast<u8>(((instruction.opcode >> 4U) & 0x1FU));
    const u8 io_offset = static_cast<u8>(((instruction.opcode >> 5U) & 0x30U) | (instruction.opcode & 0x0FU));
    write_register(destination, read_data_bus(MemoryBus::low_io_address(io_offset)));
    ++program_counter_;
    advance_cycles(1U);
}

void AvrCpu::execute_out(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u8 source = static_cast<u8>(((instruction.opcode >> 4U) & 0x1FU));
    const u8 io_offset = static_cast<u8>(((instruction.opcode >> 5U) & 0x30U) | (instruction.opcode & 0x0FU));
    write_data_bus(MemoryBus::low_io_address(io_offset), gpr_[source]);
    ++program_counter_;
    advance_cycles(1U);
}

void AvrCpu::execute_ld_x(const DecodedInstruction& instruction)
{
    execute_load_indirect(decode_destination_register(instruction.opcode), x_pointer(), false, 26U);
}

void AvrCpu::execute_ld_x_postinc(const DecodedInstruction& instruction)
{
    execute_load_indirect(decode_destination_register(instruction.opcode), x_pointer(), true, 26U);
}

void AvrCpu::execute_ld_x_predec(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(x_pointer() - 1U);
    set_x_pointer(address);
    execute_load_indirect(decode_destination_register(instruction.opcode), address, false, 26U);
}

void AvrCpu::execute_ld_y(const DecodedInstruction& instruction)
{
    execute_load_indirect(decode_destination_register(instruction.opcode), y_pointer(), false, 28U);
}

void AvrCpu::execute_ld_y_postinc(const DecodedInstruction& instruction)
{
    execute_load_indirect(decode_destination_register(instruction.opcode), y_pointer(), true, 28U);
}

void AvrCpu::execute_ld_y_predec(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(y_pointer() - 1U);
    set_y_pointer(address);
    execute_load_indirect(decode_destination_register(instruction.opcode), address, false, 28U);
}

void AvrCpu::execute_ldd_y(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(y_pointer() + decode_displacement_q(instruction.opcode));
    execute_load_indirect(decode_destination_register(instruction.opcode), address, false, 28U);
}

void AvrCpu::execute_ld_z(const DecodedInstruction& instruction)
{
    execute_load_indirect(decode_destination_register(instruction.opcode), z_pointer(), false, 30U);
}

void AvrCpu::execute_ld_z_postinc(const DecodedInstruction& instruction)
{
    execute_load_indirect(decode_destination_register(instruction.opcode), z_pointer(), true, 30U);
}

void AvrCpu::execute_ld_z_predec(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(z_pointer() - 1U);
    set_z_pointer(address);
    execute_load_indirect(decode_destination_register(instruction.opcode), address, false, 30U);
}

void AvrCpu::execute_ldd_z(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(z_pointer() + decode_displacement_q(instruction.opcode));
    execute_load_indirect(decode_destination_register(instruction.opcode), address, false, 30U);
}

void AvrCpu::execute_lpm(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    write_register(0U, bus_->read_program_byte(z_pointer(), program_counter_));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(3U);
}

void AvrCpu::execute_lpm_z(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    write_register(decode_destination_register(instruction.opcode), bus_->read_program_byte(z_pointer(), program_counter_));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(3U);
}

void AvrCpu::execute_lpm_z_postinc(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u16 address = z_pointer();
    write_register(decode_destination_register(instruction.opcode), bus_->read_program_byte(address, program_counter_));
    set_z_pointer(static_cast<u16>(address + 1U));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(3U);
}

void AvrCpu::execute_elpm(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u32 addr = (static_cast<u32>(rampz_) << 16U) | z_pointer();
    write_register(0U, bus_->read_program_byte(addr, program_counter_));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(3U);
}

void AvrCpu::execute_elpm_z(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u32 addr = (static_cast<u32>(rampz_) << 16U) | z_pointer();
    write_register(decode_destination_register(instruction.opcode), bus_->read_program_byte(addr, program_counter_));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(3U);
}

void AvrCpu::execute_elpm_z_postinc(const DecodedInstruction& instruction)
{
    if (bus_ == nullptr) {
        state_ = CpuState::halted;
        return;
    }

    const u32 addr = (static_cast<u32>(rampz_) << 16U) | z_pointer();
    const u8 destination = decode_destination_register(instruction.opcode);
    write_register(destination, bus_->read_program_byte(addr, program_counter_));
    write_register_pair(30U, static_cast<u16>(z_pointer() + 1U));
    program_counter_ = instruction.word_address + 1U;
    advance_cycles(3U);
}

void AvrCpu::execute_spm(const DecodedInstruction& instruction)
{
    (void)instruction;
    const u16 spmcsr_addr = bus_->device().spmcsr_address;
    if (spmcsr_addr == 0U) {
        advance_cycles(1U);
        program_counter_ = instruction.word_address + 1U;
        return;
    }

    const u8 spmcsr = bus_->read_data(spmcsr_addr);
    const bool spmen = (spmcsr & 0x01U) != 0U;
    
    if (!spmen || spm_lock_timeout_ == 0U) {
        Logger::debug("AvrCpu: SPM ignored. SPMEN=" + std::to_string(spmen) + " timeout=" + std::to_string(spm_lock_timeout_));
        advance_cycles(1U);
        program_counter_ = instruction.word_address + 1U;
        return;
    }

    // If RWWSB is set, only RWWSRE is allowed
    const bool rwwsb = (spmcsr & 0x40U) != 0U;
    const bool rwwsre = (spmcsr & 0x10U) != 0U;

    if (rwwsb && !rwwsre) {
        write_data_bus(spmcsr_addr, static_cast<u8>(spmcsr & 0xE0U)); // Clear all command bits
        advance_cycles(1U);
        program_counter_ = instruction.word_address + 1U;
        return;
    }

    const u32 z = z_pointer();
    const u16 data = static_cast<u16>(gpr_[0] | (static_cast<u16>(gpr_[1]) << 8U));

    bus_->execute_spm(spmcsr, z, data, program_counter_);
    
    // Clear all SPMCSR command bits [4:0] after execution (hardware behaviour per datasheet)
    write_data_bus(spmcsr_addr, static_cast<u8>(spmcsr & 0xE0U));
    spm_lock_timeout_ = 0U;

    advance_cycles(1U);
    program_counter_ = instruction.word_address + 1U;
}

void AvrCpu::execute_lds(const DecodedInstruction& instruction)
{
    execute_load_indirect(decode_destination_register(instruction.opcode), instruction.words[1], false, 0U);
    program_counter_ = instruction.word_address + 2U;
}

void AvrCpu::execute_st_x(const DecodedInstruction& instruction)
{
    execute_store_indirect(x_pointer(), decode_destination_register(instruction.opcode), false, 26U);
}

void AvrCpu::execute_st_x_postinc(const DecodedInstruction& instruction)
{
    execute_store_indirect(x_pointer(), decode_destination_register(instruction.opcode), true, 26U);
}

void AvrCpu::execute_st_x_predec(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(x_pointer() - 1U);
    set_x_pointer(address);
    execute_store_indirect(address, decode_destination_register(instruction.opcode), false, 26U);
}

void AvrCpu::execute_st_y(const DecodedInstruction& instruction)
{
    execute_store_indirect(y_pointer(), decode_destination_register(instruction.opcode), false, 28U);
}

void AvrCpu::execute_st_y_postinc(const DecodedInstruction& instruction)
{
    execute_store_indirect(y_pointer(), decode_destination_register(instruction.opcode), true, 28U);
}

void AvrCpu::execute_st_y_predec(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(y_pointer() - 1U);
    set_y_pointer(address);
    execute_store_indirect(address, decode_destination_register(instruction.opcode), false, 28U);
}

void AvrCpu::execute_std_y(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(y_pointer() + decode_displacement_q(instruction.opcode));
    execute_store_indirect(address, decode_destination_register(instruction.opcode), false, 28U);
}

void AvrCpu::execute_st_z(const DecodedInstruction& instruction)
{
    execute_store_indirect(z_pointer(), decode_destination_register(instruction.opcode), false, 30U);
}

void AvrCpu::execute_st_z_postinc(const DecodedInstruction& instruction)
{
    execute_store_indirect(z_pointer(), decode_destination_register(instruction.opcode), true, 30U);
}

void AvrCpu::execute_st_z_predec(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(z_pointer() - 1U);
    set_z_pointer(address);
    execute_store_indirect(address, decode_destination_register(instruction.opcode), false, 30U);
}

void AvrCpu::execute_std_z(const DecodedInstruction& instruction)
{
    const u16 address = static_cast<u16>(z_pointer() + decode_displacement_q(instruction.opcode));
    execute_store_indirect(address, decode_destination_register(instruction.opcode), false, 30U);
}

void AvrCpu::execute_sts(const DecodedInstruction& instruction)
{
    // STS is 2 words. execute_store_indirect increments PC by 1.
    execute_store_indirect(instruction.words[1], decode_destination_register(instruction.opcode), false, 0U);
    program_counter_ = instruction.word_address + 2U;
}

void AvrCpu::execute_rcall(const DecodedInstruction& instruction)
{
    i32 displacement = static_cast<i32>(instruction.opcode & 0x0FFFU);
    if ((displacement & 0x0800) != 0) {
        displacement -= 0x1000;
    }
    push_pc(instruction.word_address + 1U);
    program_counter_ = static_cast<u32>(static_cast<i32>(instruction.word_address) + 1 + displacement);
    advance_cycles(bus_->device().pc_width_bytes() == 3U ? 4U : 3U);
}

void AvrCpu::execute_rjmp(const DecodedInstruction& instruction)
{
    i32 displacement = static_cast<i32>(instruction.opcode & 0x0FFFU);
    if ((displacement & 0x0800) != 0) {
        displacement -= 0x1000;
    }
    program_counter_ =
        static_cast<u32>(static_cast<i32>(instruction.word_address) + 1 + displacement);
    advance_cycles(2U);
}

void AvrCpu::execute_ijmp(const DecodedInstruction& instruction)
{
    (void)instruction;
    program_counter_ = static_cast<u32>(z_pointer());
    advance_cycles(2U);
}

void AvrCpu::execute_eijmp(const DecodedInstruction& instruction)
{
    (void)instruction;
    program_counter_ = (static_cast<u32>(eind_) << 16U) | z_pointer();
    advance_cycles(2U);
}

void AvrCpu::execute_jmp(const DecodedInstruction& instruction)
{
    const u32 target = (((static_cast<u32>(instruction.opcode) & 0x01F0U) << 13U) |
                        ((static_cast<u32>(instruction.opcode) & 0x0001U) << 16U) |
                        instruction.words[1]);
    program_counter_ = target;
    advance_cycles(3U);
}

void AvrCpu::execute_push(const DecodedInstruction& instruction)
{
    advance_cycles(2U);
    const u8 source = decode_destination_register(instruction.opcode);
    push_byte(gpr_[source]);
    program_counter_ = instruction.word_address + 1U;
}

void AvrCpu::execute_pop(const DecodedInstruction& instruction)
{
    advance_cycles(2U);
    const u8 destination = decode_destination_register(instruction.opcode);
    write_register(destination, pop_byte());
    program_counter_ = instruction.word_address + 1U;
}

void AvrCpu::execute_icall(const DecodedInstruction& instruction)
{
    push_pc(instruction.word_address + 1U);
    program_counter_ = static_cast<u32>(z_pointer());
    advance_cycles(bus_->device().pc_width_bytes() == 3U ? 4U : 3U);
}

void AvrCpu::execute_eicall(const DecodedInstruction& instruction)
{
    push_pc(instruction.word_address + 1U);
    program_counter_ = (static_cast<u32>(eind_) << 16U) | z_pointer();
    advance_cycles(4U);
}

void AvrCpu::execute_call(const DecodedInstruction& instruction)
{
    const u32 target = (((static_cast<u32>(instruction.opcode) & 0x01F0U) << 13U) |
                        ((static_cast<u32>(instruction.opcode) & 0x0001U) << 16U) |
                        instruction.words[1]);
    push_pc(instruction.word_address + 2U);
    program_counter_ = target;
    advance_cycles(bus_->device().pc_width_bytes() == 3U ? 5U : 4U);
}

void AvrCpu::execute_ret(const DecodedInstruction& instruction)
{
    (void)instruction;
    program_counter_ = pop_pc();
    const u32 cycles = (bus_->device().pc_width_bytes() >= 3U) ? 5U : 4U;
    advance_cycles(cycles);
}

void AvrCpu::execute_reti(const DecodedInstruction& instruction)
{
    (void)instruction;
    program_counter_ = pop_pc();
    set_flag(SregFlag::interrupt, true);
    if (interrupt_depth_ != 0U) {
        --interrupt_depth_;
    }
    interrupt_delay_ = 1U;
    const u32 cycles = (bus_->device().pc_width_bytes() >= 3U) ? 5U : 4U;
    advance_cycles(cycles);
    if (bus_ != nullptr) {
        bus_->on_reti();
    }
}

void AvrCpu::execute_bclr(const DecodedInstruction& instruction)
{
    set_flag(decode_sreg_bit(instruction.opcode), false);
    ++program_counter_;
    advance_cycles(1U);
}

void AvrCpu::execute_bset(const DecodedInstruction& instruction)
{
    const auto flag_bit = decode_sreg_bit(instruction.opcode);
    set_flag(flag_bit, true);
    if (flag_bit == SregFlag::interrupt) {
        interrupt_delay_ = 1U;
    }
    ++program_counter_;
    advance_cycles(1U);
}

void AvrCpu::execute_sleep(const DecodedInstruction& instruction)
{
    (void)instruction;
    ++program_counter_;
    advance_cycles(1U);

    // Check if sleep is enabled
    bool enabled = slp_ctrl_ ? slp_ctrl_->sleep_enabled() : cpu_control().sleep_enabled();
    if (enabled) {
        if (slp_ctrl_) {
            state_ = CpuState::sleeping;
            const u8 avr8x_mode = slp_ctrl_->sleep_mode();
            switch (avr8x_mode) {
                case 0: Logger::debug("Entering Idle mode (AVR8X)"); break;
                case 1: Logger::debug("Entering Standby mode (AVR8X)"); break;
                case 2: Logger::debug("Entering Power-down mode (AVR8X)"); break;
                default: break;
            }
        } else {
            cpu_control().enter_sleep();
            const SleepMode mode = cpu_control().current_sleep_mode();

            switch (mode) {
            case SleepMode::idle:
                state_ = CpuState::sleeping;
                break;
            case SleepMode::adc_noise_reduction:
                Logger::debug("Entering ADC Noise Reduction mode");
                state_ = CpuState::sleeping;
                break;
            case SleepMode::power_down:
                Logger::debug("Entering Power-down mode");
                state_ = CpuState::sleeping;
                break;
            case SleepMode::power_save:
                Logger::debug("Entering Power-save mode");
                state_ = CpuState::sleeping;
                break;
            case SleepMode::standby:
                Logger::debug("Entering Standby mode");
                state_ = CpuState::sleeping;
                break;
            case SleepMode::extended_standby:
                // Same as Power Save but oscillator keeps running (1-cycle wake)
                Logger::debug("Entering Extended Standby mode");
                state_ = CpuState::sleeping;
                break;
            default:
                // Reserved modes - treat as Idle
                state_ = CpuState::sleeping;
                break;
            }
        }
    } else {
        // Sleep not enabled - instruction executes but no sleep occurs
        Logger::debug("SLEEP executed but SE bit not set");
    }
}

void AvrCpu::execute_break(const DecodedInstruction& instruction)
{
    (void)instruction;
    ++program_counter_;
    advance_cycles(1U);
    state_ = CpuState::paused;
}

void AvrCpu::execute_wdr(const DecodedInstruction& instruction)
{
    (void)instruction;
    
    if (watchdog_timer_ != nullptr) {
        watchdog_timer_->reset_watchdog();
    }
    if (wdt8x_ != nullptr) {
        wdt8x_->reset_timer();
    }

    if (reset_triggered_) {
        return;
    }

    ++program_counter_;
    advance_cycles(1U);
}

void AvrCpu::execute_cli(const DecodedInstruction& instruction)
{
    (void)instruction;
    set_flag(SregFlag::interrupt, false);
    ++program_counter_;
    advance_cycles(1U);
}

void AvrCpu::execute_sei(const DecodedInstruction& instruction)
{
    (void)instruction;
    set_flag(SregFlag::interrupt, true);
    ++program_counter_;
    interrupt_delay_ = 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_brbc(const DecodedInstruction& instruction)
{
    const bool taken = !flag(decode_branch_sreg_bit(instruction.opcode));
    if (taken) {
        const i32 displacement = decode_relative_offset(instruction.opcode, 7U);
        program_counter_ = static_cast<u32>(static_cast<i32>(instruction.word_address) + 1 + displacement);
        advance_cycles(2U);
        return;
    }

    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_brbs(const DecodedInstruction& instruction)
{
    const bool taken = flag(decode_branch_sreg_bit(instruction.opcode));
    if (taken) {
        const i32 displacement = decode_relative_offset(instruction.opcode, 7U);
        program_counter_ = static_cast<u32>(static_cast<i32>(instruction.word_address) + 1 + displacement);
        advance_cycles(2U);
        return;
    }

    program_counter_ = instruction.word_address + 1U;
    advance_cycles(1U);
}

void AvrCpu::execute_brcs(const DecodedInstruction& instruction)
{
    execute_brbs(instruction);
}

void AvrCpu::execute_brcc(const DecodedInstruction& instruction)
{
    execute_brbc(instruction);
}

void AvrCpu::execute_brmi(const DecodedInstruction& instruction)
{
    execute_brbs(instruction);
}

void AvrCpu::execute_brpl(const DecodedInstruction& instruction)
{
    execute_brbc(instruction);
}

void AvrCpu::execute_breq(const DecodedInstruction& instruction)
{
    execute_brbs(instruction);
}

void AvrCpu::execute_brne(const DecodedInstruction& instruction)
{
    execute_brbc(instruction);
}


}  // namespace vioavr::core
