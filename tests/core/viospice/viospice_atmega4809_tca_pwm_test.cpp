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

static bool compile_firmware(const fs::path& output_hex) {
    std::string src = R"(#include <avr/io.h>
int main(void) {
    PORTA_DIR = PIN0_bm;
    TCA0_SINGLE_CTRLB = TCA_SINGLE_CMP0EN_bm;
    TCA0_SINGLE_PER = 255;
    TCA0_SINGLE_CMP0 = 64;
    TCA0_SINGLE_CTRLA = TCA_SINGLE_ENABLE_bm | TCA_SINGLE_CLKSEL_DIV1_gc;
    for (;;) {}
})";

    fs::path tmp_dir = fs::temp_directory_path() / "vioavr_m4809_tca_pwm";
    fs::create_directories(tmp_dir);
    fs::path src_file = tmp_dir / "tca_pwm_fw.c";
    fs::path elf_file = tmp_dir / "tca_pwm_fw.elf";

    {
        std::ofstream of(src_file);
        of << src;
    }

    std::string compile_cmd = "avr-gcc -mmcu=atmega4809 -Os -DF_CPU=16000000UL -o "
        + elf_file.string() + " " + src_file.string() + " 2>/dev/null";
    if (system(compile_cmd.c_str()) != 0) return false;

    std::string objcopy_cmd = "avr-objcopy -j .text -j .data -O ihex "
        + elf_file.string() + " " + output_hex.string() + " 2>/dev/null";
    if (system(objcopy_cmd.c_str()) != 0) return false;

    return true;
}

static bool generate_circuit(const fs::path& cir_path, const fs::path& hex_path,
                             const fs::path& cosim_so) {
    std::string hex_rel = hex_path.filename().string();
    std::string cosim_rel = "./cosim/libavr_cosim.so";

    std::ofstream of(cir_path);
    of << "* VioAVR ATmega4809 TCA single-slope PWM on WO0 (PA0)\n";
    of << "\n";
    of << "R_pull pa0_dig 0 10k\n";
    of << "A_dac [pa0_dig] [pa0_an] dac_bridge_model\n";
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
        if (i < 8) of << "pa" << i << "_dig";
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << ".model d_cosim_model d_cosim(simulation=\"" << cosim_rel;
    of << "\" sim_args=[\"atmega4809\",\"" << hex_rel
       << "\"] queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 2us 200us\n";
    of << "\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pa0_an)\n";
    of << ".endc\n";
    of << ".end\n";
    return true;
}

TEST_CASE("ATmega4809 TCA single-slope PWM on PA0") {
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

    fs::path test_dir = build_dir / "cosim_tests" / "atmega4809_tca_pwm";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);
    fs::create_directories(test_dir / "cosim");

    fs::path cosim_so = build_dir / "cosim" / "libavr_cosim.so";
    REQUIRE_MESSAGE(fs::exists(cosim_so), "libavr_cosim.so not found");
    fs::create_symlink(cosim_so, test_dir / "cosim" / "libavr_cosim.so");

    fs::path hex_file = test_dir / "tca_pwm.hex";
    bool compiled = compile_firmware(hex_file);
    REQUIRE_MESSAGE(compiled, "Firmware compilation failed");

    if (fs::exists(test_dir / "firmware.hex")) {
        fs::remove(test_dir / "firmware.hex");
    }
    fs::create_symlink(hex_file.filename(), test_dir / "firmware.hex");

    fs::path cir_file = test_dir / "atmega4809_tca_pwm.cir";
    generate_circuit(cir_file, hex_file, cosim_so);

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

    bool saw_low = false, saw_high = false;
    int data_rows = 0, transitions = 0;
    int prev_bucket = -1;
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
        if (prev_bucket >= 0 && bucket != prev_bucket) transitions++;
        prev_bucket = bucket;
    }

    MESSAGE("ATmega4809 TCA PWM co-sim: " << data_rows << " rows, "
            << transitions << " transitions, "
            << (elapsed * 1000) << "ms");

    CHECK_MESSAGE(data_rows > 0, "No data rows in output");
    CHECK_MESSAGE(saw_low, "PA0 (TCA WO0) never goes LOW (<0.1V)");
    CHECK_MESSAGE(saw_high, "PA0 (TCA WO0) never goes HIGH (>4.0V)");
    CHECK_MESSAGE(transitions >= 3,
                  "PA0 shows fewer than 3 transitions (expected PWM toggling)");

    MESSAGE("PASS ATmega4809 TCA PWM (" << elapsed << "s)");
}
