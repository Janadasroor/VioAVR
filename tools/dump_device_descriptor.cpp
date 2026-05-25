#include <cstdio>
#include <cstring>
#include <string_view>
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega328p.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/devices/atmega2560.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"
#include "vioavr/core/devices/attiny85.hpp"
#include "vioavr/core/devices/at90pwm3.hpp"
#include "vioavr/core/devices/atmega808.hpp"
#include "vioavr/core/devices/atmega1608.hpp"
#include "vioavr/core/devices/atmega3208.hpp"
#include "vioavr/core/devices/atmega4808.hpp"
#include "vioavr/core/devices/atmega1609.hpp"
#include "vioavr/core/devices/atmega3209.hpp"
#include "vioavr/core/devices/atmega809.hpp"
#include "vioavr/core/devices/atmega324pb.hpp"
#include "vioavr/core/devices/atmega168pb.hpp"
#include "vioavr/core/devices/atmega88pb.hpp"
#include "vioavr/core/devices/atmega328pb.hpp"

using namespace vioavr::core::devices;

static constexpr struct { const char* name; const vioavr::core::DeviceDescriptor* desc; } devices[] = {
    {"atmega328p", &atmega328p},
    {"atmega4809", &atmega4809},
    {"atmega2560", &atmega2560},
    {"atmega32u4", &atmega32u4},
    {"attiny85", &attiny85},
    {"at90pwm3", &at90pwm3},
    {"atmega808", &atmega808},
    {"atmega1608", &atmega1608},
    {"atmega3208", &atmega3208},
    {"atmega4808", &atmega4808},
    {"atmega1609", &atmega1609},
    {"atmega3209", &atmega3209},
    {"atmega809", &atmega809},
    {"atmega324pb", &atmega324pb},
    {"atmega168pb", &atmega168pb},
    {"atmega88pb", &atmega88pb},
    {"atmega328pb", &atmega328pb},
};

using vioavr::core::DeviceDescriptor;
using vioavr::core::u8;

static void print_ports(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->port_count; ++i) {
        auto& p = d->ports[i];
        printf("  port[%u]: name=%s pin=0x%04x ddr=0x%04x port=0x%04x"
               " dirset=0x%04x dirclr=0x%04x dirtgl=0x%04x"
               " outset=0x%04x outclr=0x%04x outtgl=0x%04x"
               " pin_ctrl_base=0x%04x intflags=0x%04x"
               " vport_base=0x%04x vector_idx=%u\n",
               (unsigned)i, p.name.data(), p.pin_address, p.ddr_address, p.port_address,
               p.dirset_address, p.dirclr_address, p.dirtgl_address,
               p.outset_address, p.outclr_address, p.outtgl_address,
               p.pin_ctrl_base, p.intflags_address,
               p.vport_base, (unsigned)p.vector_index);
    }
}

static void print_tca(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->tca_count; ++i) {
        auto& t = d->timers_tca[i];
        printf("  tca[%u]: ctrla=0x%04x ctrlb=0x%04x ctrlc=0x%04x ctrld=0x%04x"
               " ctrleclr=0x%04x ctrleset=0x%04x ctrlfclr=0x%04x ctrlfset=0x%04x"
               " evctrl=0x%04x intctrl=0x%04x intflags=0x%04x dbgctrl=0x%04x"
               " temp=0x%04x cnt=0x%04x per=0x%04x cmp0=0x%04x cmp1=0x%04x cmp2=0x%04x"
               " perbuf=0x%04x cmp0buf=0x%04x cmp1buf=0x%04x cmp2buf=0x%04x"
               " ovf_vec=%u cmp0_vec=%u cmp1_vec=%u cmp2_vec=%u hunf_vec=%u"
               " lcmp0_vec=%u lcmp1_vec=%u lcmp2_vec=%u"
               " user_ev=0x%04x ovf_gen=%u cmp0_gen=%u cmp1_gen=%u cmp2_gen=%u\n",
               (unsigned)i,
               t.ctrla_address, t.ctrlb_address, t.ctrlc_address, t.ctrld_address,
               t.ctrleclr_address, t.ctrleset_address, t.ctrlfclr_address, t.ctrlfset_address,
               t.evctrl_address, t.intctrl_address, t.intflags_address, t.dbgctrl_address,
               t.temp_address, t.tcnt_address, t.period_address,
               t.cmp0_address, t.cmp1_address, t.cmp2_address,
               t.perbuf_address, t.cmp0buf_address, t.cmp1buf_address, t.cmp2buf_address,
               (unsigned)t.luf_ovf_vector_index, (unsigned)t.cmp0_vector_index,
               (unsigned)t.cmp1_vector_index, (unsigned)t.cmp2_vector_index,
               (unsigned)t.hunf_vector_index,
               (unsigned)t.lcmp0_vector_index, (unsigned)t.lcmp1_vector_index, (unsigned)t.lcmp2_vector_index,
               t.user_event_address, (unsigned)t.ovf_generator_id,
               (unsigned)t.cmp0_generator_id, (unsigned)t.cmp1_generator_id, (unsigned)t.cmp2_generator_id);
    }
}

static void print_tcb(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->tcb_count; ++i) {
        auto& t = d->timers_tcb[i];
        printf("  tcb[%u]: ctrla=0x%04x ctrlb=0x%04x evctrl=0x%04x intctrl=0x%04x"
               " intflags=0x%04x status=0x%04x dbgctrl=0x%04x temp=0x%04x"
               " cnt=0x%04x ccmp=0x%04x vec=%u user_ev=0x%04x capt_gen=%u\n",
               (unsigned)i,
               t.ctrla_address, t.ctrlb_address, t.evctrl_address, t.intctrl_address,
               t.intflags_address, t.status_address, t.dbgctrl_address, t.temp_address,
               t.cnt_address, t.ccmp_address,
               (unsigned)t.vector_index, t.user_event_address, (unsigned)t.capt_generator_id);
    }
}

static void print_uart8x(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->uart8x_count; ++i) {
        auto& u = d->uarts8x[i];
        printf("  uart8x[%u]: ctrla=0x%04x ctrlb=0x%04x ctrlc=0x%04x ctrld=0x%04x"
               " status=0x%04x baud=0x%04x rxdata=0x%04x txdata=0x%04x"
               " dbgctrl=0x%04x evctrl=0x%04x"
               " rx_vec=%u tx_vec=%u dre_vec=%u"
               " user_ev=0x%04x txd_pin=0x%04x:%u rxd_pin=0x%04x:%u idx=%u\n",
               (unsigned)i,
               u.ctrla_address, u.ctrlb_address, u.ctrlc_address, u.ctrld_address,
               u.status_address, u.baud_address, u.rxdata_address, u.txdata_address,
               u.dbgctrl_address, u.evctrl_address,
               (unsigned)u.rx_vector_index, (unsigned)u.tx_vector_index, (unsigned)u.dre_vector_index,
               u.user_event_address, u.txd_pin_address, (unsigned)u.txd_pin_bit,
               u.rxd_pin_address, (unsigned)u.rxd_pin_bit, (unsigned)u.index);
    }
}

static void print_adc8x(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->adc8x_count; ++i) {
        auto& a = d->adcs8x[i];
        printf("  adc8x[%u]: ctrla=0x%04x ctrlb=0x%04x ctrlc=0x%04x ctrld=0x%04x"
               " ctrle=0x%04x sampctrl=0x%04x muxpos=0x%04x muxneg=0x%04x"
               " cmd=0x%04x evctrl=0x%04x intctrl=0x%04x intflags=0x%04x"
               " dbgctrl=0x%04x temp=0x%04x res=0x%04x winlt=0x%04x winht=0x%04x"
               " resrdy_vec=%u wcomp_vec=%u user_ev=0x%04x resrd_gen=%u\n",
               (unsigned)i,
               a.ctrla_address, a.ctrlb_address, a.ctrlc_address, a.ctrld_address,
               a.ctrle_address, a.sampctrl_address, a.muxpos_address, a.muxneg_address,
               a.command_address, a.evctrl_address, a.intctrl_address, a.intflags_address,
               a.dbgctrl_address, a.temp_address, a.res_address, a.winlt_address, a.winht_address,
               (unsigned)a.res_ready_vector_index, (unsigned)a.wcomp_vector_index,
               a.user_event_address, (unsigned)a.resrd_generator_id);
    }
}

static void print_ac8x(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->ac8x_count; ++i) {
        auto& a = d->acs8x[i];
        printf("  ac8x[%u]: ctrla=0x%04x muxctrla=0x%04x dacctrla=0x%04x"
               " intctrl=0x%04x status=0x%04x vec=%u"
               " user_ev=0x%04x out_gen=%u muxpos_base=%u muxneg_base=%u\n",
               (unsigned)i,
               a.ctrla_address, a.muxctrla_address, a.dacctrla_address,
               a.intctrl_address, a.status_address,
               (unsigned)a.vector_index, a.user_event_address,
               (unsigned)a.out_generator_id, (unsigned)a.muxpos_base, (unsigned)a.muxneg_base);
    }
}

static void print_rtc(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->rtc_count; ++i) {
        auto& r = d->timers_rtc[i];
        printf("  rtc[%u]: ctrla=0x%04x status=0x%04x intctrl=0x%04x intflags=0x%04x"
               " temp=0x%04x dbgctrl=0x%04x clksel=0x%04x cnt=0x%04x per=0x%04x cmp=0x%04x"
               " pitctrla=0x%04x pitstatus=0x%04x pitintctrl=0x%04x pitintflags=0x%04x"
               " ovf_vec=%u pit_vec=%u ovf_gen=%u cmp_gen=%u\n",
               (unsigned)i,
               r.ctrla_address, r.status_address, r.intctrl_address, r.intflags_address,
               r.temp_address, r.dbgctrl_address, r.clksel_address,
               r.cnt_address, r.per_address, r.cmp_address,
               r.pitctrla_address, r.pitstatus_address, r.pitintctrl_address, r.pitintflags_address,
               (unsigned)r.ovf_vector_index, (unsigned)r.pit_vector_index,
               (unsigned)r.ovf_generator_id, (unsigned)r.cmp_generator_id);
    }
}

static void print_evsys(const DeviceDescriptor* d) {
    printf("  evsys: strobe=0x%04x channels=0x%04x users=0x%04x ch_count=%u user_count=%u\n",
           d->evsys.strobe_address, d->evsys.channels_address, d->evsys.users_address,
           (unsigned)d->evsys.channel_count, (unsigned)d->evsys.user_count);
}

static void print_sysregs(const DeviceDescriptor* d) {
    printf("  spl=0x%04x sph=0x%04x sreg=0x%04x rampz=0x%04x eind=0x%04x\n",
           d->spl_address, d->sph_address, d->sreg_address,
           d->rampz_address, d->eind_address);
    printf("  spmcsr=0x%04x prr=0x%04x prr0=0x%04x prr1=0x%04x\n",
           d->spmcsr_address, d->prr_address, d->prr0_address, d->prr1_address);
    printf("  smcr=0x%04x mcusr=0x%04x mcucr=0x%04x clkpr=0x%04x osccal=0x%04x ccp=0x%04x pllcsr=0x%04x\n",
           d->smcr_address, d->mcusr_address, d->mcucr_address,
           d->clkpr_address, d->osccal_address, d->ccp_address, d->pllcsr_address);
}

static void print_wdt8x(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->wdt8x_count; ++i) {
        auto& w = d->wdts8x[i];
        printf("  wdt8x[%u]: ctrla=0x%04x winctrla=0x%04x intctrl=0x%04x status=0x%04x vec=%u\n",
               (unsigned)i,
               w.ctrla_address, w.winctrla_address, w.intctrl_address, w.status_address,
               (unsigned)w.vector_index);
    }
}

static void print_nvmctrl(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->nvm_ctrl_count; ++i) {
        auto& n = d->nvm_ctrls[i];
        printf("  nvmctrl[%u]: ctrla=0x%04x ctrlb=0x%04x status=0x%04x intctrl=0x%04x"
               " intflags=0x%04x addr=0x%04x data=0x%04x vec=%u\n",
               (unsigned)i,
               n.ctrla_address, n.ctrlb_address, n.status_address, n.intctrl_address,
               n.intflags_address, n.addr_address, n.data_address,
               (unsigned)n.vector_index);
    }
}

static void print_cpuint(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->cpu_int_count; ++i) {
        auto& c = d->cpu_ints[i];
        printf("  cpuint[%u]: ctrla=0x%04x status=0x%04x lvl0pri=0x%04x lvl1vec=0x%04x\n",
               (unsigned)i,
               c.ctrla_address, c.status_address, c.lvl0pri_address, c.lvl1vec_address);
    }
}

static void print_clkctrl(const DeviceDescriptor* d) {
    printf("  clkctrl: ctrla=0x%04x ctrlb=0x%04x mclklock=0x%04x mclkstatus=0x%04x"
           " osc20mctrla=0x%04x osc20mcalib=0x%04x osc32kctrla=0x%04x xosc32kctrla=0x%04x\n",
           d->clkctrl.ctrla_address, d->clkctrl.ctrlb_address, d->clkctrl.mclklock_address,
           d->clkctrl.mclkstatus_address, d->clkctrl.osc20mctrla_address,
           d->clkctrl.osc20mcalib_address, d->clkctrl.osc32kctrla_address,
           d->clkctrl.xosc32kctrla_address);
}

static void print_slpctrl(const DeviceDescriptor* d) {
    printf("  slpctrl: ctrla=0x%04x\n", d->slpctrl.ctrla_address);
}

static void print_rstctrl(const DeviceDescriptor* d) {
    printf("  rstctrl: rstfr=0x%04x\n", d->rstctrl.rstfr_address);
}

static void print_bod(const DeviceDescriptor* d) {
    printf("  bod: ctrla=0x%04x ctrlb=0x%04x vlmctrla=0x%04x intctrl=0x%04x intflags=0x%04x status=0x%04x vlm_vec=%u\n",
           d->bod.ctrla_address, d->bod.ctrlb_address, d->bod.vlmctrla_address,
           d->bod.intctrl_address, d->bod.intflags_address, d->bod.status_address,
           (unsigned)d->bod.vlm_vector_index);
}

static void print_spi8x(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->spi8x_count; ++i) {
        auto& s = d->spis8x[i];
        printf("  spi8x[%u]: ctrla=0x%04x ctrlb=0x%04x intctrl=0x%04x intflags=0x%04x"
               " data=0x%04x vec=%u user_ev=0x%04x idx=%u\n",
               (unsigned)i,
               s.ctrla_address, s.ctrlb_address, s.intctrl_address, s.intflags_address,
               s.data_address, (unsigned)s.vector_index, s.user_event_address, (unsigned)s.index);
    }
}

static void print_twi8x(const DeviceDescriptor* d) {
    for (u8 i = 0; i < d->twi8x_count; ++i) {
        auto& t = d->twis8x[i];
        printf("  twi8x[%u]: mctrla=0x%04x mctrlb=0x%04x mstatus=0x%04x mbaud=0x%04x"
               " maddr=0x%04x mdata=0x%04x sctrla=0x%04x sctrlb=0x%04x sstatus=0x%04x"
               " saddr=0x%04x sdata=0x%04x saddrmask=0x%04x dbgctrl=0x%04x"
               " master_vec=%u slave_vec=%u user_ev=0x%04x idx=%u\n",
               (unsigned)i,
               t.mctrla_address, t.mctrlb_address, t.mstatus_address, t.mbaud_address,
               t.maddr_address, t.mdata_address,
               t.sctrla_address, t.sctrlb_address, t.sstatus_address,
               t.saddr_address, t.sdata_address, t.saddrmask_address, t.dbgctrl_address,
               (unsigned)t.master_vector_index, (unsigned)t.slave_vector_index,
               t.user_event_address, (unsigned)t.index);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <device_name>\n", argv[0]);
        return 1;
    }

    const DeviceDescriptor* target = nullptr;
    for (auto& d : devices) {
        if (strcmp(d.name, argv[1]) == 0) {
            target = d.desc;
            break;
        }
    }

    if (!target) {
        fprintf(stderr, "Unknown device: %s\n", argv[1]);
        return 1;
    }

    printf("name=%s\n", target->name.data());
    printf("flash_words=%u\n", (unsigned)target->flash_words);
    printf("sram_bytes=%u\n", (unsigned)target->sram_bytes);
    printf("sram_start=0x%04x\n", target->sram_start);
    printf("eeprom_bytes=%u\n", (unsigned)target->eeprom_bytes);
    printf("interrupt_vector_count=%u\n", (unsigned)target->interrupt_vector_count);
    printf("interrupt_vector_size=%u\n", (unsigned)target->interrupt_vector_size);
    printf("flash_page_size=0x%04x\n", target->flash_page_size);
    printf("signature=0x%02x%02x%02x\n",
           (unsigned)target->signature[0], (unsigned)target->signature[1], (unsigned)target->signature[2]);
    printf("cpu_frequency_hz=%lu\n", (unsigned long)target->cpu_frequency_hz);
    printf("port_count=%u\n", (unsigned)target->port_count);
    printf("tca_count=%u\n", (unsigned)target->tca_count);
    printf("tcb_count=%u\n", (unsigned)target->tcb_count);
    printf("uart8x_count=%u\n", (unsigned)target->uart8x_count);
    printf("uart_count=%u\n", (unsigned)target->uart_count);
    printf("adc8x_count=%u\n", (unsigned)target->adc8x_count);
    printf("ac8x_count=%u\n", (unsigned)target->ac8x_count);
    printf("spi8x_count=%u\n", (unsigned)target->spi8x_count);
    printf("twi8x_count=%u\n", (unsigned)target->twi8x_count);
    printf("wdt8x_count=%u\n", (unsigned)target->wdt8x_count);
    printf("evsys_channel_count=%u\n", (unsigned)target->evsys.channel_count);
    printf("evsys_user_count=%u\n", (unsigned)target->evsys.user_count);
    printf("rtc_count=%u\n", (unsigned)target->rtc_count);
    printf("nvm_ctrl_count=%u\n", (unsigned)target->nvm_ctrl_count);
    printf("cpu_int_count=%u\n", (unsigned)target->cpu_int_count);

    printf("\n=== PORTS ===\n");
    print_ports(target);

    printf("\n=== SYSTEM REGISTERS ===\n");
    print_sysregs(target);

    printf("\n=== CLKCTRL ===\n");
    print_clkctrl(target);
    printf("\n=== SLPCTRL ===\n");
    print_slpctrl(target);
    printf("\n=== RSTCTRL ===\n");
    print_rstctrl(target);
    printf("\n=== BOD ===\n");
    print_bod(target);

    printf("\n=== EVSYS ===\n");
    print_evsys(target);

    printf("\n=== TCA ===\n");
    print_tca(target);

    printf("\n=== TCB ===\n");
    print_tcb(target);

    printf("\n=== RTC ===\n");
    print_rtc(target);

    printf("\n=== UART8X ===\n");
    print_uart8x(target);

    printf("\n=== SPI8X ===\n");
    print_spi8x(target);

    printf("\n=== TWI8X ===\n");
    print_twi8x(target);

    printf("\n=== ADC8X ===\n");
    print_adc8x(target);

    printf("\n=== AC8X ===\n");
    print_ac8x(target);

    printf("\n=== WDT8X ===\n");
    print_wdt8x(target);

    printf("\n=== NVMCTRL ===\n");
    print_nvmctrl(target);

    printf("\n=== CPUINT ===\n");
    print_cpuint(target);

    return 0;
}
