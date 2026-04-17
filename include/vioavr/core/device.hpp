#pragma once

#include "vioavr/core/types.hpp"

#include <array>
#include <string_view>

namespace vioavr::core {

enum class AdcAutoTriggerSource : u8 {
    free_running = 0,
    analog_comparator = 1,
    external_interrupt_0 = 2,
    timer0_compare = 3,
    timer0_overflow = 4,
    timer1_compare_b = 5,
    timer1_overflow = 6,
    timer1_capture = 7,
    unsupported = 0xFFU
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
    std::array<AdcAutoTriggerSource, 8> auto_trigger_map {};
    u8 adsc_mask {0x40U};
    u8 adate_mask {0x20U};
    u8 adif_mask {0x10U};
    u8 adie_mask {0x08U};
    u8 aden_mask {0x80U};
    u8 adlar_mask {0x20U}; // Usually in ADMUX
    u16 pr_address {0U};
    u8 pr_bit {0xFFU};
};

struct AnalogComparatorDescriptor {
    u16 acsr_address {};
    u16 didr1_address {};
    u8 vector_index {};
    u16 ain0_pin_address {};
    u8 ain0_pin_bit {};
    u16 ain1_pin_address {};
    u8 ain1_pin_bit {};
    u8 aci_mask {0x10U};
    u8 acie_mask {0x08U};
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

struct EepromDescriptor {
    u16 eecr_address {};
    u16 eedr_address {};
    u16 eearl_address {};
    u16 eearh_address {};
    u8 vector_index {};
    u16 size {};
};

struct WdtDescriptor {
    u16 wdtcsr_address {};
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

    u16 fuse_address {0x0000U};
    u16 lockbit_address {0x0000U};
    u16 signature_address {0x0000U};

    u8 port_count {0U};
    std::array<PortDescriptor, 16> ports {};

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
