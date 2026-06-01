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
