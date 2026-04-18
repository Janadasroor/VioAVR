#pragma once

#include "vioavr/core/types.hpp"

#include <array>
#include <string_view>

namespace vioavr::core {

enum class AdcAutoTriggerSource : u8 {
    none = 0,
    free_running,
    analog_comparator,
    external_interrupt_0,
    timer0_compare_a,
    timer0_compare_b,
    timer0_compare_c,
    timer0_overflow,
    timer0_capture,
    timer1_compare_a,
    timer1_compare_b,
    timer1_compare_c,
    timer1_overflow,
    timer1_capture,
    timer2_compare_a,
    timer2_compare_b,
    timer2_compare_c,
    timer2_overflow,
    timer2_capture,
    timer3_compare_a,
    timer3_compare_b,
    timer3_compare_c,
    timer3_overflow,
    timer3_capture,
    timer4_compare_a,
    timer4_compare_b,
    timer4_compare_c,
    timer4_compare_d,
    timer4_overflow,
    timer4_capture,
    timer5_compare_a,
    timer5_compare_b,
    timer5_compare_c,
    timer5_overflow,
    timer5_capture,
    psc0_sync,
    psc1_sync,
    psc2_sync,
    analog_comparator_1,
    analog_comparator_2,
    analog_comparator_3,
    unsupported = 0xFFU
};

struct AdcMuxEntry {
    u8 positive_channel {0xFFU};
    u8 negative_channel {0xFFU};
    float gain {1.0f};
    bool differential {false};
};

struct AdcDescriptor {
    u16 adcl_address {};
    u16 adch_address {};
    u16 adcsra_address {};
    u16 adcsrb_address {};
    u16 admux_address {};
    u8 vector_index {};
    u8 adcsra_reset {0x00U};
    u8 adcsrb_reset {0x00U};
    u8 admux_reset {0x00U};
    u16 didr0_address {};
    std::array<u16, 16> adc_pin_address {}; // Increased to 16 for mega2560
    std::array<u8, 16> adc_pin_bit {};
    std::array<AdcAutoTriggerSource, 16> auto_trigger_map {};
    u8 adsc_mask {0x40U};
    u8 adate_mask {0x20U};
    u8 adif_mask {0x10U};
    u8 adie_mask {0x08U};
    u8 aden_mask {0x80U};
    u8 adlar_mask {0x20U}; // Usually in ADMUX
    u8 adts_mask {0x07U};   // Mask for ADTS bits in ADCSRB
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
    std::array<AdcMuxEntry, 64> mux_table {};
};

struct AnalogComparatorDescriptor {
    u16 acsr_address {};
    u16 accon_address {}; // E.g. AC0CON, AC1CON
    u16 didr_address {};  // E.g. DIDR1
    u8 vector_index {};
    u16 aip_pin_address {};
    u8 aip_pin_bit {};
    u16 aim_pin_address {};
    u8 aim_pin_bit {};
    
    // Status/Control masks (can be in ACSR or ACCON)
    u8 acd_mask {0x80U};  // Disable bit
    u8 acbg_mask {0x40U}; // Bandgap select
    u8 aco_mask {0x20U};  // Output bit
    u8 acif_mask {0x10U}; // Flag bit
    u8 acie_mask {0x08U}; // Enable interrupt bit
    u8 acic_mask {0x04U}; // Input capture bit
    u8 acis_mask {0x03U}; // Mode select bits
};

struct Timer8Descriptor {
    u16 tcnt_address {};
    u16 ocra_address {};
    u16 ocrb_address {};
    u16 tifr_address {};
    u16 timsk_address {};
    u16 tccra_address {};
    u16 tccrb_address {};
    u16 assr_address {};
    u8 tccra_reset {0x00U};
    u8 tccrb_reset {0x00U};
    u8 assr_reset {0x00U};
    u8 compare_a_vector_index {};
    u8 compare_b_vector_index {};
    u8 overflow_vector_index {};
    u16 ocra_pin_address {};
    u8 ocra_pin_bit {};
    u16 ocrb_pin_address {};
    u8 ocrb_pin_bit {};
    u16 t_pin_address {};
    u8 t_pin_bit {};
    u16 tosc1_pin_address {};
    u8 tosc1_pin_bit {};
    u16 tosc2_pin_address {};
    u8 tosc2_pin_bit {};
    u8 wgm0_mask {0x03U}; // bits 0,1 of TCCRA
    u8 wgm2_mask {0x08U}; // bit 3 of TCCRB
    u8 cs_mask {0x07U};   // bits 0,1,2 of TCCRB
    u8 as2_mask {0x20U};  // bit 5 of ASSR
    u8 tcn2ub_mask {0x10U}; // bit 4 of ASSR
    u8 compare_a_enable_mask {0x02U}; // bit 1 of TIMSK/TIFR
    u8 compare_b_enable_mask {0x04U}; // bit 2 of TIMSK/TIFR
    u8 overflow_enable_mask {0x01U};  // bit 0 of TIMSK/TIFR
    u8 foca_mask {0x80U}; // bit 7 of TCCRB
    u8 focb_mask {0x40U}; // bit 6 of TCCRB
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
    u8 timer_index {0U};

    // ADC Triggering
    AdcAutoTriggerSource compare_a_trigger_source {AdcAutoTriggerSource::none};
    AdcAutoTriggerSource compare_b_trigger_source {AdcAutoTriggerSource::none};
    AdcAutoTriggerSource overflow_trigger_source {AdcAutoTriggerSource::none};
};

struct ExtInterruptDescriptor {
    u16 eicra_address {};
    u16 eicrb_address {};
    u16 eimsk_address {};
    u16 eifr_address {};
    std::array<u8, 8> vector_indices {};
};

struct UartDescriptor {
    u16 udr_address {};
    u16 ucsra_address {};
    u16 ucsrb_address {};
    u16 ucsrc_address {};
    u16 ubrrl_address {};
    u16 ubrrh_address {};
    u8 ucsra_reset {0x00U};
    u8 ucsrb_reset {0x00U};
    u8 ucsrc_reset {0x00U};
    u8 rx_vector_index {};
    u8 udre_vector_index {};
    u8 tx_vector_index {};
    u8 u2x_mask {0x02U};
    u8 rxc_mask {0x80U};
    u8 txc_mask {0x40U};
    u8 udre_mask {0x20U};
    u8 rxen_mask {0x10U};
    u8 txen_mask {0x08U};
    u8 rxcie_mask {0x80U};
    u8 txcie_mask {0x40U};
    u8 udrie_mask {0x20U};
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
    u8 uart_index {0U};
};

struct PinChangeInterruptDescriptor {
    u16 pcicr_address {};
    u16 pcifr_address {};
    u16 pcmsk_address {};
    u8 pcicr_enable_mask {};
    u8 pcifr_flag_mask {};
    u8 vector_index {};
};

struct SpiDescriptor {
    u16 spcr_address {};
    u16 spsr_address {};
    u16 spdr_address {};
    u8 spcr_reset {0x00U};
    u8 spsr_reset {0x00U};
    u8 vector_index {};
    u8 spe_mask {0x40U};
    u8 spie_mask {0x80U};
    u8 mstr_mask {0x10U};
    u8 spif_mask {0x80U};
    u8 wcol_mask {0x40U};
    u8 sp2x_mask {0x01U};
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
};

struct XmemDescriptor {
    u16 xmcra_address {0U};
    u16 xmcrb_address {0U};
    u16 mcucr_address {0U};
    u8 sre_mask {0x80U};
    u8 srl_mask {0x70U}; 
    u8 srw0_mask {0x03U}; 
    u8 srw1_mask {0x0CU}; 
    u8 xmm_mask {0x07U};  // Released pins
    u8 xmbk_mask {0x80U}; // Memory bank
};

struct TwiDescriptor {
    u16 twbr_address {};
    u16 twsr_address {};
    u16 twar_address {};
    u16 twdr_address {};
    u16 twcr_address {};
    u16 twamr_address {};
    u8 vector_index {};
    u8 twint_mask {0x80U};
    u8 twen_mask {0x04U};
    u8 twie_mask {0x01U};
    u8 twsto_mask {0x10U};
    u8 twsta_mask {0x20U};
    u8 twea_mask {0x40U};
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
};

struct MappedMemoryDescriptor {
    u16 data_start {0U};
    u16 size {0U};
};

struct EepromDescriptor {
    u16 eecr_address {};
    u16 eedr_address {};
    u16 eearl_address {};
    u16 eearh_address {};
    u8 eecr_reset {0x00U};
    u8 vector_index {};
    u16 size {};
    MappedMemoryDescriptor mapped_data {};
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
};

struct NvmCtrlDescriptor {
    u16 ctrla_address {};
    u16 ctrlb_address {};
    u16 status_address {};
    u16 intctrl_address {};
    u16 intflags_address {};
    u16 addr_address {}; 
    u16 data_address {}; 
    u8 vector_index {};
};

struct CpuIntDescriptor {
    u16 ctrla_address {};
    u16 status_address {};
    u16 lvl0pri_address {};
    u16 lvl1vec_address {};
};

struct WdtDescriptor {
    u16 wdtcsr_address {};
    u8 wdtcsr_reset {0x00U};
    u8 vector_index {};
    u8 wdie_mask {0x40U};
    u8 wde_mask {0x08U};
    u8 wdce_mask {0x10U};
};

struct CanDescriptor {
    u16 cangcon_address {};
    u16 cangsta_address {};
    u16 cangit_address {};
    u16 cangie_address {};
    u16 canen1_address {};
    u16 canen2_address {};
    u16 canie1_address {};
    u16 canie2_address {};
    u16 cansit1_address {};
    u16 cansit2_address {};
    u16 canbt1_address {};
    u16 canbt2_address {};
    u16 canbt3_address {};
    u16 cantcon_address {};
    u16 cantim_address {};  // 16-bit
    u16 canttc_address {};  // 16-bit
    u16 cantec_address {};
    u16 canrec_address {};
    u16 canhpmob_address {};
    u16 canpage_address {};
    u16 canstmob_address {};
    u16 cancdmob_address {};
    u16 canidt_address {}; // Base of CANIDT (usually CANIDT4)
    u16 canidm_address {}; // Base of CANIDM (usually CANIDM4)
    u16 canstm_address {};  // 16-bit
    u16 canmsg_address {};
    u8 canit_vector_index {};
    u8 ovrit_vector_index {};
    u8 mob_count {15U};
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
};

struct UsbDescriptor {
    u16 uhwcon_address {};
    u16 usbcon_address {};
    u16 usbsta_address {};
    u16 usbint_address {};
    u16 udcon_address {};
    u16 udint_address {};
    u16 udien_address {};
    u16 udaddr_address {};
    u16 udfnum_address {};
    u16 udmfn_address {};
    u16 uenum_address {};
    u16 uerst_address {};
    u16 ueint_address {};
    u16 ueintx_address {};
    u16 ueienx_address {};
    u16 uedatx_address {};
    u16 uebclx_address {};
    u16 uebchx_address {};
    u16 ueconx_address {};
    u16 uecfg0x_address {};
    u16 uecfg1x_address {};
    u16 uesta0x_address {};
    u16 uesta1x_address {};
    u8 gen_vector_index {};
    u8 com_vector_index {};
    u16 pllcsr_address {};
    
    // Bitmasks
    u8 usbcon_usbe_mask {0x80U};
    u8 usbcon_frzclk_mask {0x20U};
    u8 udint_sofi_mask {0x04U};
    u8 udint_eorsti_mask {0x08U};
    u8 ueintx_txini_mask {0x01U};
    u8 ueintx_rxstpi_mask {0x08U};
    u8 ueintx_rxouti_mask {0x04U};
    u8 udaddr_adden_mask {0x80U};

    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
};

struct PscDescriptor {
    u16 pctl_address {};
    u16 psoc_address {};
    u16 pconf_address {};
    u16 pim_address {};
    u16 pifr_address {};
    u16 picr_address {};
    u16 ocrsa_address {};
    u16 ocrra_address {};
    u16 ocrsb_address {};
    u16 ocrrb_address {};
    u16 pfrc0a_address {};
    u16 pfrc0b_address {};
    
    u8 psc_index {};
    u8 gen_vector_index {};
    u8 ec_vector_index {};
    u8 capt_vector_index {};

    u8 prun_mask {0x01U};
    u8 mode_mask {0x18U};
    u8 clksel_mask {0x02U};
    u8 ec_flag_mask {0x01U};
    u8 capt_flag_mask {0x08U};
    
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
};

struct DacDescriptor {
    u16 dacon_address {};
    u16 dacl_address {};
    u16 dach_address {};
    u8 daen_mask {};
    u8 daate_mask {};
    u8 dats_mask {};
    u8 dacoe_mask {};
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
};

struct Timer16Descriptor {
    u16 tcnt_address {};
    u16 ocra_address {};
    u16 ocrb_address {};
    u16 ocrc_address {};
    u16 icr_address {};
    u16 tifr_address {};
    u16 timsk_address {};
    u16 tccra_address {};
    u16 tccrb_address {};
    u16 tccrc_address {};
    u8 tccra_reset {0x00U};
    u8 tccrb_reset {0x00U};
    u8 tccrc_reset {0x00U};
    u8 timer_index {1U};
    u8 capture_vector_index {};
    u8 compare_a_vector_index {};
    u8 compare_b_vector_index {};
    u8 compare_c_vector_index {};
    u8 overflow_vector_index {};
    u16 ocra_pin_address {};
    u8 ocra_pin_bit {};
    u16 ocrb_pin_address {};
    u8 ocrb_pin_bit {};
    u16 ocrc_pin_address {};
    u8 ocrc_pin_bit {};
    u16 icp_pin_address {};
    u8 icp_pin_bit {};
    u16 t_pin_address {};
    u8 t_pin_bit {};
    u8 wgm10_mask {0x03U};
    u8 wgm12_mask {0x18U};
    u8 cs_mask {0x07U};
    u8 ices_mask {0x40U};
    u8 icnc_mask {0x80U};
    u8 capture_enable_mask {0x20U};
    u8 compare_a_enable_mask {0x02U};
    u8 compare_b_enable_mask {0x04U};
    u8 compare_c_enable_mask {0x08U};
    u8 overflow_enable_mask {0x01U};
    u8 foca_mask {0x80U}; // bit 7 of TCCRC
    u8 focb_mask {0x40U}; // bit 6 of TCCRC
    u8 focc_mask {0x20U}; // bit 5 of TCCRC
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};

    // ADC Triggering
    AdcAutoTriggerSource compare_a_trigger_source {AdcAutoTriggerSource::none};
    AdcAutoTriggerSource compare_b_trigger_source {AdcAutoTriggerSource::none};
    AdcAutoTriggerSource compare_c_trigger_source {AdcAutoTriggerSource::none};
    AdcAutoTriggerSource overflow_trigger_source {AdcAutoTriggerSource::none};
    AdcAutoTriggerSource capture_trigger_source {AdcAutoTriggerSource::none};
};

struct Timer10Descriptor {
    u16 tcnt_address {};
    u16 tc4h_address {};
    u16 ocra_address {};
    u16 ocrb_address {};
    u16 ocrc_address {};
    u16 ocrd_address {};
    u16 ocra_pin_address {0U};
    u8 ocra_pin_bit {0xFFU};
    u16 ocra_neg_pin_address {0U};
    u8 ocra_neg_pin_bit {0xFFU};
    u16 ocrb_pin_address {0U};
    u8 ocrb_pin_bit {0xFFU};
    u16 ocrb_neg_pin_address {0U};
    u8 ocrb_neg_pin_bit {0xFFU};
    u16 ocrd_pin_address {0U};
    u8 ocrd_pin_bit {0xFFU};
    u16 ocrd_neg_pin_address {0U};
    u8 ocrd_neg_pin_bit {0xFFU};
    u16 tccra_address {};
    u16 tccrb_address {};
    u16 tccrc_address {};
    u16 tccrd_address {};
    u16 tccre_address {};
    u16 dt4_address {};
    u16 pllcsr_address {};
    u16 timsk_address {};
    u16 tifr_address {};
    u8 tccra_reset {};
    u8 tccrb_reset {};
    u8 tccrc_reset {};
    u8 tccrd_reset {};
    u8 tccre_reset {};
    u8 compare_a_vector_index {};
    u8 compare_b_vector_index {};
    u8 compare_d_vector_index {};
    u8 overflow_vector_index {};
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};

    // ADC Triggering
    AdcAutoTriggerSource compare_a_trigger_source {AdcAutoTriggerSource::none};
    AdcAutoTriggerSource compare_b_trigger_source {AdcAutoTriggerSource::none};
    AdcAutoTriggerSource compare_d_trigger_source {AdcAutoTriggerSource::none};
    AdcAutoTriggerSource overflow_trigger_source {AdcAutoTriggerSource::none};
};

struct PortDescriptor {
    std::string_view name {};
    u16 pin_address {};
    u16 ddr_address {};
    u16 port_address {};
};


struct DeviceDescriptor {
    std::string_view name {};
    u32 flash_words {};
    u16 sram_bytes {};
    u16 eeprom_bytes {};
    u8 interrupt_vector_count {};
    u8 interrupt_vector_size {2U};
    u16 flash_page_size {0x80U};
    AddressRange register_file_range {0x0000, 0x001F};
    AddressRange io_range {0x0020, 0x005F};
    AddressRange extended_io_range {0x0060, 0x00FF};

    // Unified Memory Map Support (AVR8X / megaAVR-0)
    MappedMemoryDescriptor mapped_flash {};
    MappedMemoryDescriptor mapped_eeprom {};
    MappedMemoryDescriptor mapped_fuses {};
    MappedMemoryDescriptor mapped_signatures {};
    MappedMemoryDescriptor mapped_user_signatures {};
    u16 spl_address {0x005DU};
    u16 sph_address {0x005EU};
    u16 sreg_address {0x005FU};
    u16 rampz_address {};
    u16 eind_address {};
    u16 spmcsr_address {};
    u16 prr_address {};
    u16 prr0_address {};
    u16 prr1_address {};
    u16 smcr_address {};
    u16 mcusr_address {};
    u16 mcucr_address {};
    u16 pllcsr_address {};
    u16 xmcra_address {};
    u16 xmcrb_address {};
    XmemDescriptor xmem {};
    u8 pradc_bit {0xFFU};
    u8 prusart0_bit {0xFFU};
    u8 prspi_bit {0xFFU};
    u8 prtwi_bit {0xFFU};
    u8 prtimer0_bit {0xFFU};
    u8 prtimer1_bit {0xFFU};
    u8 prtimer2_bit {0xFFU};
    u8 smcr_sm_mask {0x0EU};
    u8 smcr_se_mask {0x01U};
    u32 flash_rww_end_word {};
    u8 spl_reset {0x00U};
    u8 sph_reset {0x00U};
    u8 sreg_reset {0x00U};
    u64 cpu_frequency_hz {16'000'000U};

    // Peripheral Arrays
    u8 adc_count {0U};
    std::array<AdcDescriptor, 4> adcs {};

    u8 ac_count {0U};
    std::array<AnalogComparatorDescriptor, 4> acs {};

    u8 timer8_count {0U};
    std::array<Timer8Descriptor, 8> timers8 {};

    u8 timer16_count {0U};
    std::array<Timer16Descriptor, 8> timers16 {};

    u8 timer10_count {0U};
    std::array<Timer10Descriptor, 4> timers10 {};

    u8 ext_interrupt_count {0U};
    std::array<ExtInterruptDescriptor, 16> ext_interrupts {};

    u8 uart_count {0U};
    std::array<UartDescriptor, 8> uarts {};

    u8 nvm_ctrl_count {0U};
    std::array<NvmCtrlDescriptor, 1> nvm_ctrls {};

    u8 cpu_int_count {0U};
    std::array<CpuIntDescriptor, 1> cpu_ints {};

    u8 pcint_count {0U};
    std::array<PinChangeInterruptDescriptor, 8> pcints {};

    u8 spi_count {0U};
    std::array<SpiDescriptor, 4> spis {};

    u8 twi_count {0U};
    std::array<TwiDescriptor, 4> twis {};

    u8 eeprom_count {0U};
    std::array<EepromDescriptor, 4> eeproms {};

    u8 wdt_count {0U};
    std::array<WdtDescriptor, 4> wdts {};

    u8 can_count {0U};
    std::array<CanDescriptor, 2> cans {};
    
    u8 usb_count {0U};
    std::array<UsbDescriptor, 1> usbs {};

    u8 psc_count {0U};
    std::array<PscDescriptor, 3> pscs {};

    u8 dac_count {0U};
    std::array<DacDescriptor, 1> dacs {};

    u16 fuse_address {0x0000U};
    u16 lockbit_address {0x0000U};
    u16 signature_address {0x0000U};

    std::array<u8, 3> signature { 0xFFU, 0xFFU, 0xFFU };
    std::array<u8, 16> fuses { 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU };

    u8 port_count {0U};
    std::array<PortDescriptor, 16> ports {};

    [[nodiscard]] constexpr u8 pc_width_bytes() const noexcept {
        return (flash_words > 65536U) ? 3U : 2U;
    }

    [[nodiscard]] constexpr u16 data_end_address() const noexcept
    {
        return static_cast<u16>(extended_io_range.end + sram_bytes);
    }

    [[nodiscard]] constexpr AddressRange sram_range() const noexcept
    {
        return {
            static_cast<u16>(extended_io_range.end + 1U),
            data_end_address()
        };
    }
};

}  // namespace vioavr::core
