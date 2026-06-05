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

static bool compile_firmware(const fs::path& output_hex) {
    std::string src = R"(#include <avr/io.h>
int main(void) {
    TCC4_CTRLA = 0x03;
    TCC4_CTRLB = 0x10;
    TCC4_CTRLD = 0x03;
    TCC4_PER = 399;
    TCC4_CCA = 200;
    for (;;) {}
})";

    fs::path tmp_dir = fs::temp_directory_path() / "vioavr_xmega_tc_pwm";
    fs::create_directories(tmp_dir);
    fs::path src_file = tmp_dir / "tc_pwm_fw.c";
    fs::path elf_file = tmp_dir / "tc_pwm_fw.elf";

    {
        std::ofstream of(src_file);
        of << src;
    }

    std::string compile_cmd = "avr-gcc -mmcu=atxmega8e5 -Os -o "
        + elf_file.string() + " " + src_file.string() + " 2>/dev/null";
    if (system(compile_cmd.c_str()) != 0) return false;

    std::string objcopy_cmd = "avr-objcopy -j .text -j .data -O ihex "
        + elf_file.string() + " " + output_hex.string() + " 2>/dev/null";
    if (system(objcopy_cmd.c_str()) != 0) return false;

    return true;
}

static bool generate_circuit(const fs::path& cir_path, const fs::path& hex_path) {
    std::string hex_rel = hex_path.filename().string();

    std::ofstream of(cir_path);
    of << "* VioAVR XMEGA TC PWM co-simulation test\n";
    of << "* ATxmega8E5 TCC4 single-slope PWM on PC4 (WO0)\n";
    of << "* PER=399, CCA=200 at 32 MHz → 12.5µs period, ~50% duty\n";
    of << "* PC4 = port_idx 2 (PORTC) × 8 + bit 4 = external ID 20\n";
    of << "\n";
    of << "R_pull pc4_dig 0 10k\n";
    of << "A_dac [pc4_dig] [pc4_an] dac_bridge_model_dac\n";
    of << ".model dac_bridge_model_dac dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    of << "A_AVR [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        of << "0";
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 20) of << "pc4_dig";
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atxmega8e5\",\"" << hex_rel << "\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 0.5u 500u\n";
    of << "\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pc4_an)\n";
    of << ".endc\n";
    of << ".end\n";
    return true;
}

TEST_CASE("XMEGA TC PWM co-simulation") {
    auto find_build_dir = []() -> fs::path {
        std::vector<fs::path> candidates = {
            fs::current_path(),
            fs::current_path() / "build",
            "/home/jnd/cpp_projects/VioAVR",
            "/home/jnd/cpp_projects/VioAVR/build",
        };
        for (const auto& base : candidates) {
            fs::path probe = fs::exists(base / "cosim" / "libavr_cosim.so")
                ? base : (fs::exists(base / "build" / "cosim" / "libavr_cosim.so")
                    ? base / "build" : fs::path{});
            if (!probe.empty()) return probe;
        }
        return {};
    };

    fs::path build_dir = find_build_dir();
    REQUIRE_MESSAGE(!build_dir.empty(), "Build directory with libavr_cosim.so not found");

    fs::path test_dir = build_dir / "cosim_tests" / "xmega_tc_pwm";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);
    fs::create_directories(test_dir / "cosim");

    fs::path cosim_so = build_dir / "cosim" / "libavr_cosim.so";
    REQUIRE_MESSAGE(fs::exists(cosim_so), "libavr_cosim.so not found");
    fs::create_symlink(cosim_so, test_dir / "cosim" / "libavr_cosim.so");

    fs::path hex_file = test_dir / "tc_pwm.hex";
    bool compiled = compile_firmware(hex_file);
    REQUIRE_MESSAGE(compiled, "Firmware compilation failed");

    if (fs::exists(test_dir / "firmware.hex")) {
        fs::remove(test_dir / "firmware.hex");
    }
    fs::create_symlink(hex_file.filename(), test_dir / "firmware.hex");

    fs::path cir_file = test_dir / "xmega_tc_pwm.cir";
    generate_circuit(cir_file, hex_file);

    double start = clock();
    std::string ngspice = "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice";
    std::string log_file = (test_dir / "sim_matrix.log").string();
    std::string cmd = "cd " + test_dir.string() + " && " + ngspice + " -b "
        + cir_file.filename().string() + " > " + log_file + " 2>/dev/null";
    int exit_code = system(cmd.c_str());
    double elapsed = (clock() - start) / CLOCKS_PER_SEC;

    REQUIRE_MESSAGE(exit_code == 0,
        "ngspice exited with code " << std::to_string(exit_code)
        << " (" << elapsed << "s)");

    std::ifstream log(log_file);
    REQUIRE_MESSAGE(log.is_open(), "No output log generated");

    // Check PC4 for PWM toggling (LOW < 0.1V, HIGH > 4.0V)
    bool saw_low = false, saw_high = false;
    bool saw_multiple_transitions = false;
    int data_rows = 0;
    int transitions = 0;
    int rising_edges = 0;
    int prev_bucket = -1;
    double rising_edge_times[4] = {-1.0, -1.0, -1.0, -1.0};
    std::string line;

    while (std::getline(log, line)) {
        if (line.empty() || !std::isdigit(line[0])) continue;
        std::istringstream iss(line);
        int idx; double time, val;
        iss >> idx >> time >> val;
        if (iss.fail()) continue;
        data_rows++;
        if (val < 0.1) saw_low = true;
        if (val > 4.0) saw_high = true;
        int bucket = (val < 0.5) ? 0 : (val > 4.0 ? 1 : prev_bucket);
        if (prev_bucket >= 0 && bucket != prev_bucket) {
            transitions++;
            if (bucket == 1 && rising_edges < 4)
                rising_edge_times[rising_edges++] = time;
        }
        prev_bucket = bucket;
        if (saw_low && saw_high && transitions >= 3) saw_multiple_transitions = true;
    }

    MESSAGE("XMEGA TC PWM co-sim: " << data_rows << " rows, "
            << transitions << " transitions, "
            << rising_edges << " rising edges, "
            << (elapsed * 1000) << "ms");

    CHECK_MESSAGE(data_rows > 0, "No data rows in output");
    CHECK_MESSAGE(saw_low, "PC4 never goes LOW (<0.1V)");
    CHECK_MESSAGE(saw_high, "PC4 never goes HIGH (>4.0V)");
    CHECK_MESSAGE(saw_multiple_transitions,
                  "PC4 shows fewer than 3 transitions (expected PWM toggling)");

    // Verify PWM period: PER=399, CCA=200 at 32 MHz → 12.5µs period
    int period_samples = 0;
    for (int i = 0; i < rising_edges - 1; i++) {
        double period = rising_edge_times[i + 1] - rising_edge_times[i];
        if (period > 1e-9) {
            MESSAGE("PWM period[" << i << "]: " << (period * 1e6) << " µs");
            period_samples++;
            if (period_samples == 1) {
                CHECK_MESSAGE(period > 10e-6,
                    "PWM period too short: " << (period * 1e6) << " µs (expected ~12.5)");
                CHECK_MESSAGE(period < 20e-6,
                    "PWM period too long: " << (period * 1e6) << " µs (expected ~12.5)");
            }
        }
    }

    MESSAGE("PASS XMEGA TC PWM (" << elapsed << "s)");
}
