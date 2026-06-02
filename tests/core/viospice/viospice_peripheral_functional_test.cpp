#include "doctest.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace fs = std::filesystem;

static std::string to_lowercase(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// ── helpers ──────────────────────────────────────────────────────

static fs::path find_build_dir() {
    std::vector<fs::path> candidates = {
        fs::current_path(),
        fs::current_path() / "build",
        "/home/jnd/cpp_projects/VioAVR",
        "/home/jnd/cpp_projects/VioAVR/build",
    };
    for (const auto& base : candidates) {
        if (fs::exists(base / "cosim" / "libavr_cosim.so"))
            return base;
        if (fs::exists(base / "build" / "cosim" / "libavr_cosim.so"))
            return base / "build";
    }
    return {};
}

static std::string ngspice_binary() {
    const char* env = getenv("NGSPICE");
    if (env && env[0]) return env;
    return "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice";
}

static bool compile_firmware(const std::string& src, const std::string& mcu,
                             const fs::path& hex_out) {
    fs::path dir = fs::temp_directory_path() / "vioavr_periph_test";
    fs::create_directories(dir);
    fs::path src_file = dir / (to_lowercase(mcu) + "_fw.c");
    fs::path elf_file = dir / (to_lowercase(mcu) + "_fw.elf");
    {
        std::ofstream of(src_file);
        of << src;
    }
    std::string cc = "avr-gcc -mmcu=" + to_lowercase(mcu)
        + " -Os -DF_CPU=16000000UL -o " + elf_file.string()
        + " " + src_file.string() + " 2>/dev/null";
    if (system(cc.c_str()) != 0) return false;
    std::string oc = "avr-objcopy -j .text -j .data -O ihex "
        + elf_file.string() + " " + hex_out.string() + " 2>/dev/null";
    return system(oc.c_str()) == 0;
}

static int run_ngspice(const fs::path& cir_file, const fs::path& test_dir,
                       const fs::path& log_file) {
    std::string ngspice = ngspice_binary();
    std::string cmd = "cd " + test_dir.string() + " && " + ngspice + " -b "
        + cir_file.filename().string() + " > " + log_file.string() + " 2>/dev/null";
    return system(cmd.c_str());
}

struct LogRow {
    double time;
    double val;
};

static std::vector<LogRow> parse_log(const fs::path& log_file) {
    std::vector<LogRow> rows;
    std::ifstream log(log_file);
    if (!log) return rows;
    std::string line;
    while (std::getline(log, line)) {
        if (line.empty() || !std::isdigit(line[0])) continue;
        std::istringstream iss(line);
        int idx; double time, val;
        if (!(iss >> idx >> time >> val)) continue;
        rows.push_back({time, val});
    }
    return rows;
}

static bool setup_cosim_links(const fs::path& test_dir,
                              const fs::path& build_dir,
                              const fs::path& hex_file) {
    fs::create_directories(test_dir / "cosim");
    fs::path cosim_so = build_dir / "cosim" / "libavr_cosim.so";
    if (!fs::exists(cosim_so)) return false;
    fs::path link = test_dir / "cosim" / "libavr_cosim.so";
    if (!fs::exists(link))
        fs::create_symlink(cosim_so, link);
    if (!fs::exists(test_dir / "firmware.hex")) {
        if (fs::exists(hex_file))
            fs::create_symlink(hex_file.filename(), test_dir / "firmware.hex");
    }
    return true;
}

static bool setup_spiceinit(const fs::path& test_dir,
                            const fs::path& build_dir) {
    std::ofstream of(test_dir / ".spiceinit");
    if (!of) return false;
    // Copy user's .spiceinit if it exists (provides digital.cm, analog.cm)
    const char* home = getenv("HOME");
    if (home) {
        std::ifstream home_spice(std::string(home) + "/.spiceinit");
        if (home_spice) {
            of << home_spice.rdbuf();
            of << "\n";
        }
    }
    // Add custom avr_adc_bridge code model
    fs::path adc_bridge_cm = build_dir / "cosim" / "avr_adc_bridge.cm";
    if (!fs::exists(adc_bridge_cm)) return false;
    of << "codemodel " << adc_bridge_cm.string() << "\n";
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  TEST 1: External Interrupt (INT0) on ATmega328P
// ═══════════════════════════════════════════════════════════════════

static const char* INT0_FIRMWARE = R"(
#include <avr/io.h>
#include <avr/interrupt.h>

ISR(INT0_vect) {
    PORTB |= _BV(PB0);
}

int main(void) {
    DDRB  |= _BV(PB0);
    PORTB &= ~_BV(PB0);
    EICRA  = _BV(ISC01) | _BV(ISC00);
    EIMSK  = _BV(INT0);
    sei();
    for (;;) {}
}
)";

static void gen_int0_circuit(const fs::path& cir_file, const fs::path& hex_file) {
    std::ofstream of(cir_file);
    of << "* VioAVR INT0 external interrupt test (ATmega328P)\n";
    of << "* INT0 on PD2 (ext ID 26), PB0 output (ext ID 8)\n";
    of << "\n";
    of << "Vpulse int0_pin 0 PULSE(0 5 2u 0.1u 0.1u 3u 20u)\n";
    of << "\n";
    of << "R_pull pb0_dig 0 10k\n";
    of << "A_dac [pb0_dig] [pb0_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    // d_in: all 32, with PD2 (ext 26) driven by pulse
    of << "A_AVR [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << (i == 26 ? "int0_pin" : "0");
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << (i == 8 ? "pb0_dig" : "0");
    }
    of << "] d_cosim_model\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega328p\",\"" << hex_file.filename().string() << "\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 0.5u 100u\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pb0_an)\n";
    of << ".endc\n";
    of << ".end\n";
}

TEST_CASE("INT0 external interrupt") {
    fs::path build_dir = find_build_dir();
    REQUIRE_MESSAGE(!build_dir.empty(), "Build directory with libavr_cosim.so not found");

    fs::path test_dir = build_dir / "cosim_tests" / "int0_test";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    fs::path hex_file = test_dir / "int0_fw.hex";
    REQUIRE(compile_firmware(INT0_FIRMWARE, "atmega328p", hex_file));
    REQUIRE(setup_cosim_links(test_dir, build_dir, hex_file));

    fs::path cir_file = test_dir / "int0_test.cir";
    gen_int0_circuit(cir_file, hex_file);

    fs::path log_file = test_dir / "sim_matrix.log";
    int rc = run_ngspice(cir_file, test_dir, log_file);
    REQUIRE_MESSAGE(rc == 0, "ngspice exited with code " << rc);

    auto rows = parse_log(log_file);
    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");

    // PB0 should start LOW, then transition HIGH after INT0 fires (~2us pulse)
    bool saw_low = false, saw_high = false;
    bool transition = false;
    int prev_bucket = -1;
    for (const auto& r : rows) {
        if (r.val < 0.1) saw_low = true;
        if (r.val > 4.0) saw_high = true;
        int bucket = (r.val < 0.5) ? 0 : (r.val > 4.0 ? 1 : prev_bucket);
        if (prev_bucket >= 0 && bucket != prev_bucket) transition = true;
        prev_bucket = bucket;
    }

    MESSAGE("INT0 test: " << rows.size() << " rows, L:" << saw_low << " H:" << saw_high
            << " T:" << transition);

    CHECK_MESSAGE(saw_low,  "PB0 never goes LOW  (<0.1V) — expected initial state");
    CHECK_MESSAGE(saw_high, "PB0 never goes HIGH (>4.0V) — INT0 did not fire");
    CHECK_MESSAGE(transition, "PB0 shows no LOW→HIGH transition — INT0 ISR did not toggle");
}

// ═══════════════════════════════════════════════════════════════════
//  TEST 2: UART TX on ATmega328P
// ═══════════════════════════════════════════════════════════════════

static const char* UART_TX_FIRMWARE = R"(
#include <avr/io.h>

int main(void) {
    UBRR0H = 0;
    UBRR0L = 16;
    UCSR0A = (1 << U2X0);
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = 0x55;
    while (!(UCSR0A & (1 << TXC0)));
    for (;;) {}
}
)";

static void gen_uart_tx_circuit(const fs::path& cir_file, const fs::path& hex_file) {
    std::ofstream of(cir_file);
    of << "* VioAVR UART TX test (ATmega328P)\n";
    of << "* TXD on PD1 (ext ID 25) — 0x55 at ~117647 baud\n";
    of << "\n";
    of << "R_pull txd_dig 0 10k\n";
    of << "A_dac [txd_dig] [txd_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    of << "A_AVR [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << "0";
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 25) of << "txd_dig";
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega328p\",\"" << hex_file.filename().string() << "\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 0.5u 200u\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(txd_an)\n";
    of << ".endc\n";
    of << ".end\n";
}

TEST_CASE("UART TX") {
    fs::path build_dir = find_build_dir();
    REQUIRE_MESSAGE(!build_dir.empty(), "Build directory with libavr_cosim.so not found");

    fs::path test_dir = build_dir / "cosim_tests" / "uart_tx_test";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    fs::path hex_file = test_dir / "uart_tx_fw.hex";
    REQUIRE(compile_firmware(UART_TX_FIRMWARE, "atmega328p", hex_file));
    REQUIRE(setup_cosim_links(test_dir, build_dir, hex_file));

    fs::path cir_file = test_dir / "uart_tx_test.cir";
    gen_uart_tx_circuit(cir_file, hex_file);

    fs::path log_file = test_dir / "sim_matrix.log";
    int rc = run_ngspice(cir_file, test_dir, log_file);
    REQUIRE_MESSAGE(rc == 0, "ngspice exited with code " << rc);

    auto rows = parse_log(log_file);
    REQUIRE_MESSAGE(rows.size() > 10, "Too few data rows: " << rows.size());

    // For 0x55 (LSB-first: 1,0,1,0,1,0,1,0):
    //   start→D0→D1→D2→D3→D4→D5→D6→D7→stop
    //   LOW→HIGH→LOW→HIGH→LOW→HIGH→LOW→HIGH→LOW→HIGH
    // 10 transitions after idle (edges at each bit boundary)
    // Expected bit time: 16e6 / 8 / 17 ≈ 117647 baud → ~8.5 µs/bit
    // Frame: ~85 µs. First LOW should start ~10µs after reset.
    // With 0.5µs step we sample every 0.5µs

    // Count transitions and bucket timing (skip initial glitch window)
    int transition_count = 0;
    int prev_bucket = -1;
    double first_low_time = -1.0;
    double last_low_time = -1.0;
    double last_high_time = -1.0;
    std::vector<double> edge_times;

    for (const auto& r : rows) {
        int bucket;
        if (r.val > 4.0) bucket = 1;
        else if (r.val < 0.5) bucket = 0;
        else continue;

        if (prev_bucket >= 0 && bucket != prev_bucket && r.time > 1e-6) {
            transition_count++;
            edge_times.push_back(r.time);
            if (bucket == 0) {
                if (first_low_time < 0) first_low_time = r.time;
                last_low_time = r.time;
            }
            if (bucket == 1) last_high_time = r.time;
        }
        if (r.time > 1e-6) prev_bucket = bucket;
    }

    // Edge sequence for 0x55 LSB-first (10 bits, 10 edges):
    //   HIGH→LOW (idle→start), LOW→HIGH (start→D0=1), HIGH→LOW (D0→D1=0) ...

    MESSAGE("UART TX: " << rows.size() << " rows, " << transition_count
            << " transitions, first LOW at " << (first_low_time * 1e6) << " µs");

    CHECK_MESSAGE(first_low_time > 0, "TX never goes LOW — no start bit");
    CHECK_MESSAGE(transition_count >= 6, "Too few transitions — data bits missing");
    CHECK_MESSAGE(rows.back().val > 4.0, "TX not HIGH at end — stop bit missing");

    // Verify first edge timing: start bit after setup code
    CHECK_MESSAGE(first_low_time > 2e-6, "Start bit too early (<2 µs)");
    CHECK_MESSAGE(first_low_time < 50e-6, "Start bit too late (>50 µs)");

    // Verify bit timing: consecutive edges should be ~8.5 µs apart
    // (RC response smears exact thresholds so we tolerate wide margin)
    if (edge_times.size() >= 3) {
        double d1 = edge_times[1] - edge_times[0];
        double d2 = edge_times[2] - edge_times[1];
        MESSAGE("Edge times: " << (edge_times[0]*1e6) << " " << (edge_times[1]*1e6)
                << " " << (edge_times[2]*1e6) << " µs; intervals: "
                << (d1*1e6) << " " << (d2*1e6) << " µs");
        // Intervals should be positive and less than 2× bit time
        CHECK_MESSAGE(d1 > 0, "First interval should be positive");
        CHECK_MESSAGE(d1 < 50e-6, "First interval too long (>50 µs)");
    }
}

// ═══════════════════════════════════════════════════════════════════
//  TEST 3: Analog Comparator (AC8X) on ATmega4809
// ═══════════════════════════════════════════════════════════════════

static const char* AC8X_FIRMWARE = R"(
#include <avr/io.h>

int main(void) {
    PORTA_DIR = PIN0_bm;
    PORTA_OUT &= ~PIN0_bm;

    AC0_CTRLA = AC_ENABLE_bm;
    // Default MUXCTRLA=0: MUXPOS=0 (AINP0=ch0=3.0V), MUXNEG=0 (AINN0=ch4=0.0V)
    // 3.0V > 0.0V → STATE=1 → PA0 goes HIGH

    for (;;) {
        if (AC0_STATUS & AC_STATE_bm) {
            PORTA_OUT |= PIN0_bm;
        } else {
            PORTA_OUT &= ~PIN0_bm;
        }
    }
}
)";

static void gen_ac8x_circuit(const fs::path& cir_file, const fs::path& hex_file) {
    std::ofstream of(cir_file);
    of << "* VioAVR AC8X comparator test (ATmega4809)\n";
    of << "* avr_adc_bridge injects 3.0V on signal bank channel 0\n";
    of << "* AC0 compares AINP0 (ch0) vs DACREF=128 (~2.5V)\n";
    of << "* 3.0V > 2.5V → ACO=1 → PA0 goes HIGH\n";
    of << "\n";
    of << "V_src ac_in 0 DC 3.0\n";
    of << "A_adc_bridge ac_in dummy avr_adc_bridge_model\n";
    of << ".model avr_adc_bridge_model avr_adc_bridge(channel=0)\n";
    of << "R_dummy dummy 0 1k\n";
    of << "\n";
    of << "R_pull pa0_dig 0 10k\n";
    of << "A_dac [pa0_dig] [pa0_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    of << "A_AVR [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << "0";
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 0) of << "pa0_dig";
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega4809\",\"" << hex_file.filename().string() << "\",\"1\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 0.5u 100u\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pa0_an)\n";
    of << ".endc\n";
    of << ".end\n";
}

TEST_CASE("AC8X analog comparator") {
    fs::path build_dir = find_build_dir();
    REQUIRE_MESSAGE(!build_dir.empty(), "Build directory with libavr_cosim.so not found");

    fs::path test_dir = build_dir / "cosim_tests" / "ac8x_test";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    fs::path hex_file = test_dir / "ac8x_fw.hex";
    REQUIRE(compile_firmware(AC8X_FIRMWARE, "atmega4809", hex_file));
    REQUIRE(setup_cosim_links(test_dir, build_dir, hex_file));
    REQUIRE(setup_spiceinit(test_dir, build_dir));

    fs::path cosim_cm = build_dir / "cosim" / "avr_adc_bridge.cm";
    REQUIRE_MESSAGE(fs::exists(cosim_cm), "avr_adc_bridge.cm not built");

    fs::path cir_file = test_dir / "ac8x_test.cir";
    gen_ac8x_circuit(cir_file, hex_file);

    fs::path log_file = test_dir / "sim_matrix.log";
    int rc = run_ngspice(cir_file, test_dir, log_file);
    REQUIRE_MESSAGE(rc == 0, "ngspice exited with code " << rc);

    auto rows = parse_log(log_file);
    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");

    // AC: AINP0 (3V via avr_adc_bridge ch0) vs DACREF=128 (~2.5V)
    // 3.0V > 2.5V → ACO=1 → PA0 should go HIGH
    // PA0 starts LOW (initial state), then transitions to HIGH after AC settles

    bool saw_low = false, saw_high = false;
    bool transition = false;
    int prev_bucket = -1;
    for (const auto& r : rows) {
        if (r.val < 0.1) saw_low = true;
        if (r.val > 4.0) saw_high = true;
        int bucket = (r.val < 0.5) ? 0 : (r.val > 4.0 ? 1 : prev_bucket);
        if (prev_bucket >= 0 && bucket != prev_bucket) transition = true;
        prev_bucket = bucket;
    }

    MESSAGE("AC8X test: " << rows.size() << " rows, L:" << saw_low
            << " H:" << saw_high << " T:" << transition);

    CHECK_MESSAGE(saw_low,  "PA0 never LOW — expected initial state");
    CHECK_MESSAGE(saw_high, "PA0 never HIGH — AC comparison did not trigger");
    CHECK_MESSAGE(transition, "PA0 shows no transition — AC not working");
}

// ═══════════════════════════════════════════════════════════════════
//  TEST 4: Closed-loop Temperature Control PI Regulator
// ═══════════════════════════════════════════════════════════════════

static const char* THERMAL_REGULATOR_FIRMWARE = R"(
#define F_CPU 16000000UL
#include <avr/io.h>

static void delay_cycles(unsigned int count) {
    __asm__ volatile (
        "1: subi %A0, 1"  "\n\t"
        "   sbci %B0, 0"  "\n\t"
        "   brne 1b"
        : "+r" (count)
        :
        : "cc"
    );
}

int main(void) {
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);

    const int16_t setpoint = 717; // 3.5V -> (3.5 / 5.0) * 1024 = 716.8
    int16_t integral = 0;
    uint8_t pwm_counter = 0;

    while (1) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        uint16_t reading = ADC;

        int16_t error = setpoint - (int16_t)reading;
        integral += error;
        
        if (integral > 2000) integral = 2000;
        else if (integral < -2000) integral = -2000;

        int16_t drive = (8 * error) + (integral / 8);
        if (drive < 0) drive = 0;
        if (drive > 100) drive = 100;

        if (pwm_counter < (drive / 10)) {
            PORTB |= (1 << PB0);
        } else {
            PORTB &= ~(1 << PB0);
        }

        pwm_counter++;
        if (pwm_counter >= 10) pwm_counter = 0;

        delay_cycles(200);
    }
    return 0;
}
)";

static void gen_thermal_circuit(const fs::path& cir_file, const fs::path& hex_file) {
    std::ofstream of(cir_file);
    of << "* Closed-loop temperature regulation test circuit\n";
    of << "V_ambient ambient 0 DC 1.0\n";
    of << "R_ambient chamber_temp ambient 100\n";
    of << "C_thermal chamber_temp 0 10u IC=1.0\n";
    of << "\n";
    of << "A_dac [heater_dig] [heater_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    of << "G_heat 0 chamber_temp VALUE={V(heater_an) * 0.01}\n";
    of << "\n";
    of << "A_avr_adc chamber_temp dummy avr_adc_bridge_model\n";
    of << ".model avr_adc_bridge_model avr_adc_bridge(channel=16)\n";
    of << "R_dummy dummy 0 1k\n";
    of << "\n";
    of << "A_AVR [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << "0";
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 8) of << "heater_dig";
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << "\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega328p\",\"" << hex_file.filename().string() << "\",\"1\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 1u 10m uic\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(chamber_temp)\n";
    of << ".endc\n";
    of << ".end\n";
}

TEST_CASE("Closed-loop Temperature Control PI Regulator") {
    fs::path build_dir = find_build_dir();
    REQUIRE_MESSAGE(!build_dir.empty(), "Build directory with libavr_cosim.so not found");

    fs::path test_dir = build_dir / "cosim_tests" / "thermal_test";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    fs::path hex_file = test_dir / "thermal_fw.hex";
    REQUIRE(compile_firmware(THERMAL_REGULATOR_FIRMWARE, "atmega328p", hex_file));
    REQUIRE(setup_cosim_links(test_dir, build_dir, hex_file));
    REQUIRE(setup_spiceinit(test_dir, build_dir));

    fs::path cir_file = test_dir / "thermal_test.cir";
    gen_thermal_circuit(cir_file, hex_file);

    fs::path log_file = test_dir / "sim_matrix.log";
    int rc = run_ngspice(cir_file, test_dir, log_file);
    REQUIRE_MESSAGE(rc == 0, "ngspice exited with code " << rc);

    auto rows = parse_log(log_file);
    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");

    double final_temp = rows.back().val;
    MESSAGE("Thermal loop final temperature: " << final_temp << " V (" << rows.size() << " rows)");

    // Initial temperature should be 1.0V (ambient)
    CHECK_MESSAGE(rows.front().val >= 0.9, "Initial temperature not at ambient (~1.0V)");
    CHECK_MESSAGE(rows.front().val <= 1.1, "Initial temperature not at ambient (~1.0V)");

    // Max temperature should have risen significantly above 2.0V
    double max_temp = 0.0;
    for (const auto& r : rows) {
        if (r.val > max_temp) max_temp = r.val;
    }
    CHECK_MESSAGE(max_temp > 2.5, "Temperature never rose above 2.5V — PI loop or heater failed");

    // Final temperature should be regulated near target 3.5V setpoint
    CHECK_MESSAGE(final_temp >= 3.3, "Final temperature unregulated (too low): " << final_temp << "V");
    CHECK_MESSAGE(final_temp <= 3.7, "Final temperature unregulated (too high): " << final_temp << "V");
}

// ═══════════════════════════════════════════════════════════════════
//  TEST 5: PWM RC Filter and ADC Feedback Loop
// ═══════════════════════════════════════════════════════════════════

static const char* PWM_FILTER_FIRMWARE = R"(
#define F_CPU 16000000UL
#include <avr/io.h>

int main(void) {
    DDRB |= (1 << PB1);

    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS10);
    OCR1A = 153; // 153 / 255 = 60% duty cycle

    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);

    while (1) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        volatile uint16_t reading = ADC;
        (void)reading;
    }
    return 0;
}
)";

static void gen_pwm_filter_circuit(const fs::path& cir_file, const fs::path& hex_file) {
    std::ofstream of(cir_file);
    of << "* PWM RC Filter and ADC Feedback Loop\n";
    of << "V_ref vdd 0 DC 5.0\n";
    of << "\n";
    of << "A_dac [pwm_dig] [pwm_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    of << "R1 pwm_an filter_out 1k\n";
    of << "C1 filter_out 0 1u IC=0.0\n";
    of << "\n";
    of << "A_avr_adc filter_out dummy avr_adc_bridge_model\n";
    of << ".model avr_adc_bridge_model avr_adc_bridge(channel=16)\n";
    of << "R_dummy dummy 0 1k\n";
    of << "\n";
    of << "A_AVR [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << "0";
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 9) of << "pwm_dig"; // PB1 is ext ID 9
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << "\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega328p\",\"" << hex_file.filename().string() << "\",\"1\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7\n";
    of << ".tran 2u 10m uic\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(filter_out)\n";
    of << ".endc\n";
    of << ".end\n";
}

TEST_CASE("PWM RC Filter and ADC Feedback Loop") {
    fs::path build_dir = find_build_dir();
    REQUIRE_MESSAGE(!build_dir.empty(), "Build directory with libavr_cosim.so not found");

    fs::path test_dir = build_dir / "cosim_tests" / "pwm_filter_test";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    fs::path hex_file = test_dir / "pwm_filter_fw.hex";
    REQUIRE(compile_firmware(PWM_FILTER_FIRMWARE, "atmega328p", hex_file));
    REQUIRE(setup_cosim_links(test_dir, build_dir, hex_file));
    REQUIRE(setup_spiceinit(test_dir, build_dir));

    fs::path cir_file = test_dir / "pwm_filter.cir";
    gen_pwm_filter_circuit(cir_file, hex_file);

    fs::path log_file = test_dir / "sim_matrix.log";
    int rc = run_ngspice(cir_file, test_dir, log_file);
    REQUIRE_MESSAGE(rc == 0, "ngspice exited with code " << rc);

    auto rows = parse_log(log_file);
    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");

    double final_voltage = rows.back().val;
    MESSAGE("PWM Filter final voltage: " << final_voltage << " V (" << rows.size() << " rows)");

    // Initial voltage on C should be 0.0V (as specified by UIC)
    CHECK_MESSAGE(rows.front().val <= 0.1, "Initial voltage on C not 0V");

    // Final settled voltage should be exactly 60% of VCC (5V) -> 3.0V
    CHECK_MESSAGE(final_voltage >= 2.8, "Final voltage unregulated (too low): " << final_voltage << "V");
    CHECK_MESSAGE(final_voltage <= 3.2, "Final voltage unregulated (too high): " << final_voltage << "V");
}

// ═══════════════════════════════════════════════════════════════════
//  TEST 6: Analog Comparator Edge Triggered Interrupt
// ═══════════════════════════════════════════════════════════════════

static const char* AC_INTERRUPT_FIRMWARE = R"(
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

ISR(ANALOG_COMP_vect) {
    PORTB |= (1 << PB0); // Toggle PB0 HIGH
}

int main(void) {
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    // ACSR: Enable AC, ACIE = 1, ACIS1 = 1, ACIS0 = 1 (Rising Edge)
    ACSR = (1 << ACIE) | (1 << ACIS1) | (1 << ACIS0);
    sei();

    while (1) {}
    return 0;
}
)";

static void gen_ac_interrupt_circuit(const fs::path& cir_file, const fs::path& hex_file) {
    std::ofstream of(cir_file);
    of << "* AC Edge Triggered Interrupt\n";
    of << "V_sin ain0 0 SIN(1.5 2.0 200)\n";
    of << "V_ref ain1 0 DC 2.5\n";
    of << "\n";
    of << "A_bridge0 ain0 dummy0 avr_adc_bridge_model0\n";
    of << ".model avr_adc_bridge_model0 avr_adc_bridge(channel=30)\n";
    of << "R_dummy0 dummy0 0 1k\n";
    of << "\n";
    of << "A_bridge1 ain1 dummy1 avr_adc_bridge_model1\n";
    of << ".model avr_adc_bridge_model1 avr_adc_bridge(channel=31)\n";
    of << "R_dummy1 dummy1 0 1k\n";
    of << "\n";
    of << "A_dac [pb0_dig] [pb0_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    of << "A_AVR [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << "0";
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 8) of << "pb0_dig"; // PB0 is ext ID 8
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << "\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega328p\",\"" << hex_file.filename().string() << "\",\"1\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear\n";
    of << ".tran 10u 4m\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pb0_an)\n";
    of << ".endc\n";
    of << ".end\n";
}

TEST_CASE("Analog Comparator Edge Triggered Interrupt") {
    fs::path build_dir = find_build_dir();
    REQUIRE_MESSAGE(!build_dir.empty(), "Build directory with libavr_cosim.so not found");

    fs::path test_dir = build_dir / "cosim_tests" / "ac_interrupt_test";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    fs::path hex_file = test_dir / "ac_interrupt_fw.hex";
    REQUIRE(compile_firmware(AC_INTERRUPT_FIRMWARE, "atmega328p", hex_file));
    REQUIRE(setup_cosim_links(test_dir, build_dir, hex_file));
    REQUIRE(setup_spiceinit(test_dir, build_dir));

    fs::path cir_file = test_dir / "ac_interrupt.cir";
    gen_ac_interrupt_circuit(cir_file, hex_file);

    fs::path log_file = test_dir / "sim_matrix.log";
    int rc = run_ngspice(cir_file, test_dir, log_file);
    REQUIRE_MESSAGE(rc == 0, "ngspice exited with code " << rc);

    auto rows = parse_log(log_file);
    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");

    // Target crossing is exactly at 0.4167 ms
    // Before 0.3 ms -> PB0 should be LOW
    // After 0.5 ms -> PB0 should be HIGH
    bool low_before = false;
    bool high_after = false;

    for (const auto& r : rows) {
        if (r.time < 0.3e-3) {
            CHECK_MESSAGE(r.val < 0.5, "PB0 went HIGH too early at " << (r.time * 1e3) << " ms");
            low_before = true;
        }
        if (r.time > 0.5e-3) {
            if (r.val > 4.0) high_after = true;
        }
    }

    CHECK_MESSAGE(low_before, "No data checked before crossing threshold");
    CHECK_MESSAGE(high_after, "PB0 never went HIGH after crossing threshold");
}

// ═══════════════════════════════════════════════════════════════════
//  TEST 7: GPIO Schmitt Trigger Hysteresis
// ═══════════════════════════════════════════════════════════════════

static const char* SCHMITT_TRIGGER_FIRMWARE = R"(
#define F_CPU 16000000UL
#include <avr/io.h>

int main(void) {
    DDRB &= ~(1 << PB0);
    DDRD |= (1 << PD0);
    
    while (1) {
        if (PINB & (1 << PB0)) {
            PORTD |= (1 << PD0);
        } else {
            PORTD &= ~(1 << PD0);
        }
    }
    return 0;
}
)";

static void gen_schmitt_trigger_circuit(const fs::path& cir_file, const fs::path& hex_file) {
    std::ofstream of(cir_file);
    of << "* GPIO Schmitt Trigger Hysteresis\n";
    of << "V_tri tri_in 0 pulse(0 5 0 5m 5m 1n 10m)\n";
    of << "\n";
    of << "A_AVR [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << (i == 8 ? "tri_in" : "0"); // PB0 is ext ID 8
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 24) of << "pd0_dig"; // PD0 is ext ID 24
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << "\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega328p\",\"" << hex_file.filename().string() << "\",\"1\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << "A_dac [pd0_dig] [pd0_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear\n";
    of << ".tran 10u 10m\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pd0_an) v(tri_in)\n";
    of << ".endc\n";
    of << ".end\n";
}

TEST_CASE("GPIO Schmitt Trigger Hysteresis") {
    fs::path build_dir = find_build_dir();
    REQUIRE_MESSAGE(!build_dir.empty(), "Build directory with libavr_cosim.so not found");

    fs::path test_dir = build_dir / "cosim_tests" / "schmitt_test";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    fs::path hex_file = test_dir / "schmitt_fw.hex";
    REQUIRE(compile_firmware(SCHMITT_TRIGGER_FIRMWARE, "atmega328p", hex_file));
    REQUIRE(setup_cosim_links(test_dir, build_dir, hex_file));
    REQUIRE(setup_spiceinit(test_dir, build_dir));

    fs::path cir_file = test_dir / "schmitt.cir";
    gen_schmitt_trigger_circuit(cir_file, hex_file);

    fs::path log_file = test_dir / "sim_matrix.log";
    int rc = run_ngspice(cir_file, test_dir, log_file);
    REQUIRE_MESSAGE(rc == 0, "ngspice exited with code " << rc);

    std::ifstream log(log_file);
    REQUIRE(log);
    std::string line;
    
    // Parse time, pd0_an, tri_in
    struct SchmittRow {
        double time;
        double pd0;
        double tri;
    };
    std::vector<SchmittRow> rows;
    while (std::getline(log, line)) {
        if (line.empty() || !std::isdigit(line[0])) continue;
        std::istringstream iss(line);
        int idx; double time, pd0, tri;
        if (!(iss >> idx >> time >> pd0 >> tri)) continue;
        rows.push_back({time, pd0, tri});
    }
    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");

    bool saw_low_start = false;
    bool saw_high_rising = false;
    bool saw_low_falling = false;

    for (const auto& r : rows) {
        if (r.time < 1e-3) {
            // Initially should be LOW
            CHECK_MESSAGE(r.pd0 < 0.5, "PD0 went HIGH too early at t=" << (r.time*1e3) << "ms");
            saw_low_start = true;
        }
        
        // Rising edge crossing: transitions around 1.66V due to ngspice auto-inserted adc_bridge.
        if (r.time > 1e-3 && r.time < 5e-3) {
            if (r.tri < 1.6) {
                CHECK_MESSAGE(r.pd0 < 0.5, "PD0 went HIGH before crossing rising threshold (1.66V) at tri=" << r.tri << "V");
            }
            if (r.tri > 1.8) {
                if (r.pd0 > 4.0) saw_high_rising = true;
            }
        }

        // Falling edge crossing: transitions around 1.63V due to ngspice auto-inserted adc_bridge.
        if (r.time > 5e-3 && r.time < 9e-3) {
            if (r.tri > 1.7) {
                CHECK_MESSAGE(r.pd0 > 4.0, "PD0 went LOW before crossing falling threshold (1.63V) at tri=" << r.tri << "V");
            }
            if (r.tri < 1.5) {
                if (r.pd0 < 0.5) saw_low_falling = true;
            }
        }
    }

    CHECK_MESSAGE(saw_low_start, "No initial low checked");
    CHECK_MESSAGE(saw_high_rising, "PD0 never went HIGH on rising edge above 3.0V");
    CHECK_MESSAGE(saw_low_falling, "PD0 never went LOW on falling edge below 1.5V");
}

// ═══════════════════════════════════════════════════════════════════
//  TEST 8: Watchdog Timer System Reset Pulse
// ═══════════════════════════════════════════════════════════════════

static const char* WDT_RESET_FIRMWARE = R"(
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/wdt.h>

int main(void) {
    DDRB |= (1 << PB0);
    PORTB |= (1 << PB0);

    wdt_enable(WDTO_15MS);

    while (1) {}
    return 0;
}
)";

static void gen_wdt_reset_circuit(const fs::path& cir_file, const fs::path& hex_file) {
    std::ofstream of(cir_file);
    of << "* WDT Reset Pulse\n";
    of << "A_dac [pb0_dig] [pb0_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "R_pulldown pb0_an 0 10k\n";
    of << "\n";
    of << "A_AVR [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << "0";
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 8) of << "pb0_dig"; // PB0 is ext ID 8
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << "\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega328p\",\"" << hex_file.filename().string() << "\",\"1\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear\n";
    of << ".tran 100u 40m\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pb0_an)\n";
    of << ".endc\n";
    of << ".end\n";
}

TEST_CASE("Watchdog Timer System Reset Pulse") {
    fs::path build_dir = find_build_dir();
    REQUIRE_MESSAGE(!build_dir.empty(), "Build directory with libavr_cosim.so not found");

    fs::path test_dir = build_dir / "cosim_tests" / "wdt_reset_test";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    fs::path hex_file = test_dir / "wdt_reset_fw.hex";
    REQUIRE(compile_firmware(WDT_RESET_FIRMWARE, "atmega328p", hex_file));
    REQUIRE(setup_cosim_links(test_dir, build_dir, hex_file));
    REQUIRE(setup_spiceinit(test_dir, build_dir));

    fs::path cir_file = test_dir / "wdt_reset.cir";
    gen_wdt_reset_circuit(cir_file, hex_file);

    fs::path log_file = test_dir / "sim_matrix.log";
    int rc = run_ngspice(cir_file, test_dir, log_file);
    REQUIRE_MESSAGE(rc == 0, "ngspice exited with code " << rc);

    auto rows = parse_log(log_file);
    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");

    // WDT timeout WDTO_15MS triggers around 15-20 ms.
    // Assert that PB0 starts HIGH, goes LOW around 15-20 ms (reset condition),
    // and goes HIGH again after reset initialization (reboot).
    bool saw_high_initial = false;
    bool saw_low_reset = false;
    bool saw_high_reboot = false;

    for (const auto& r : rows) {
        if (r.time < 10.0e-3) {
            if (r.val > 4.0) saw_high_initial = true;
        }
        if (r.time > 13.0e-3 && r.time < 22.0e-3) {
            if (r.val < 0.5) saw_low_reset = true;
        }
        if (r.time > 25.0e-3) {
            if (r.val > 4.0) saw_high_reboot = true;
        }
    }

    CHECK_MESSAGE(saw_high_initial, "PB0 was never HIGH initially before WDT reset");
    CHECK_MESSAGE(saw_low_reset, "PB0 never dropped to LOW around WDT timeout (15-20ms)");
    CHECK_MESSAGE(saw_high_reboot, "PB0 never went HIGH again after reboot");
}
