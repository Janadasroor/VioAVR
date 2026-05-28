#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device_catalog.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;
using namespace vioavr::core;

struct NgspiceResult {
    std::string chip;
    bool passed;
    bool should_skip;
    std::string error;
    double runtime_sec;
};

static std::string to_lowercase(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

static bool chip_supported_by_gcc(const std::string& chip) {
    std::string chip_lower = to_lowercase(chip);
    fs::path tmp = fs::temp_directory_path() / "vioavr_cosim_test" / (chip_lower + "_check.c");
    fs::create_directories(tmp.parent_path());
    {
        std::ofstream of(tmp);
        of << "int main(void) { return 0; }\n";
    }
    std::string cmd = "avr-gcc -mmcu=" + chip_lower + " -c " + tmp.string() +
                      " -o " + tmp.string() + ".o 2>/dev/null";
    int rc = system(cmd.c_str());
    fs::remove(tmp);
    fs::remove(tmp.string() + ".o");
    return (rc == 0);
}

static bool compile_firmware(const std::string& chip, const fs::path& output_hex) {
    std::string chip_lower = to_lowercase(chip);
    std::string src = R"(#define F_CPU 16000000UL
#include <avr/io.h>
#if defined(DDRA)
  #define DIR_REG DDRA
  #define OUT_REG PORTA
#elif defined(DDRB)
  #define DIR_REG DDRB
  #define OUT_REG PORTB
#elif defined(DDRC)
  #define DIR_REG DDRC
  #define OUT_REG PORTC
#elif defined(DDRD)
  #define DIR_REG DDRD
  #define OUT_REG PORTD
#elif defined(DDRE)
  #define DIR_REG DDRE
  #define OUT_REG PORTE
#elif defined(PORTA_DIR)
  #define DIR_REG PORTA_DIR
  #define OUT_REG PORTA_OUT
#elif defined(PORTB_DIR)
  #define DIR_REG PORTB_DIR
  #define OUT_REG PORTB_OUT
#elif defined(PORTC_DIR)
  #define DIR_REG PORTC_DIR
  #define OUT_REG PORTC_OUT
#elif defined(PORTD_DIR)
  #define DIR_REG PORTD_DIR
  #define OUT_REG PORTD_OUT
#else
  #error "No GPIO port found"
#endif
int main(void) {
    DIR_REG |= 1;
    while (1) {
        OUT_REG ^= 1;
        for (volatile unsigned long i = 0; i < 50; i++) { }
    }
})";

    fs::path tmp_dir = fs::temp_directory_path() / "vioavr_cosim_test";
    fs::create_directories(tmp_dir);
    fs::path src_file = tmp_dir / (chip_lower + "_fw.c");
    fs::path elf_file = tmp_dir / (chip_lower + "_fw.elf");

    {
        std::ofstream of(src_file);
        of << src;
    }

    std::string compile_cmd = "avr-gcc -mmcu=" + chip_lower +
        " -Os -DF_CPU=16000000UL -o " + elf_file.string() + " " + src_file.string() +
        " 2>/dev/null";
    if (system(compile_cmd.c_str()) != 0) return false;

    std::string objcopy_cmd = "avr-objcopy -j .text -j .data -O ihex " +
        elf_file.string() + " " + output_hex.string() + " 2>/dev/null";
    if (system(objcopy_cmd.c_str()) != 0) return false;

    return true;
}

static bool generate_circuit(const fs::path& cir_path, const std::string& chip,
                             const fs::path& hex_path, const fs::path& cosim_so,
                             char first_port) {
    std::string chip_lower = to_lowercase(chip);
    std::string cosim_rel = "./cosim/libavr_cosim.so";
    std::string hex_rel = hex_path.filename().string();
    char pl = static_cast<char>(std::tolower(static_cast<unsigned char>(first_port)));

    std::ofstream of(cir_path);
    of << "* VioAVR co-simulation test for " << chip << "\n";
    of << "\n";
    of << "R_pull p" << pl << "0_dig 0 10k\n";
    of << "A_dac [p" << pl << "0_dig] [p" << pl << "0_an] dac_bridge_model_dac\n";
    of << ".model dac_bridge_model_dac dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";

    of << "A_AVR [";
    for (int i = 0; i < 24; i++) {
        if (i > 0) of << " ";
        of << "0";
    }
    of << "] [";
    for (int i = 0; i < 24; i++) {
        if (i > 0) of << " ";
        if (i == 0) of << "pa0_dig";
        else if (i == 8) of << "pb0_dig";
        else if (i == 16) of << "pc0_dig";
        else of << "0";
    }
    of << "] d_cosim_model\n";

    of << ".model d_cosim_model d_cosim(simulation=\"" << cosim_rel;
    of << "\" sim_args=[\"" << chip_lower << "\",\"" << hex_rel << "\"] queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 5us 300us\n";
    of << "\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(p" << pl << "0_an)\n";
    of << ".endc\n";
    of << ".end\n";
    return true;
}

static NgspiceResult test_chip(const std::string& chip, const fs::path& build_dir) {
    std::string chip_lower = to_lowercase(chip);
    NgspiceResult result;
    result.chip = chip;
    result.passed = false;
    result.should_skip = false;

    fs::path test_dir = build_dir / "cosim_tests" / chip_lower;
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);
    fs::create_directories(test_dir / "cosim");

    fs::path cosim_so = build_dir / "cosim" / "libavr_cosim.so";
    if (!fs::exists(cosim_so)) {
        result.error = "libavr_cosim.so not found";
        return result;
    }
    fs::create_symlink(cosim_so, test_dir / "cosim" / "libavr_cosim.so");

    fs::path hex_file = test_dir / (chip_lower + "_fw.hex");
    if (!fs::exists(hex_file)) {
        if (!compile_firmware(chip_lower, hex_file)) {
            result.error = "firmware compilation failed";
            result.should_skip = true;
            return result;
        }
    }

    if (fs::exists(test_dir / "firmware.hex")) {
        fs::remove(test_dir / "firmware.hex");
    }
    fs::create_symlink(hex_file.filename(), test_dir / "firmware.hex");

    // Determine first valid GPIO port (matches firmware's DDRA > DDRB > DDRC priority)
    // Firmware checks DDRA first, then DDRB, etc. — so probe port A, then B, then C.
    char first_port = 'A';
    const auto* desc = DeviceCatalog::find(chip);
    if (desc) {
        static const char port_letters[] = {'A','B','C','D','E','F','G','H'};
        for (int pi = 0; pi < 8 && pi < desc->port_count + 1; pi++) {
            for (u8 i = 0; i < desc->port_count && i < 16; i++) {
                auto pname = desc->ports[i].name;
                if (pname.empty()) continue;
                char pch = static_cast<char>(std::toupper(static_cast<unsigned char>(pname.back())));
                if (pch != port_letters[pi]) continue;
                if (desc->ports[i].port_address != 0 && desc->ports[i].port_address >= 0x20) {
                    first_port = pch;
                    goto found_port;
                }
            }
        }
        found_port: ;
    }

    fs::path cir_file = test_dir / (chip_lower + "_cosim.cir");
    generate_circuit(cir_file, chip, hex_file, cosim_so, first_port);


    double start = clock();
    std::string ngspice = "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice";
    std::string log_file = (test_dir / "sim_matrix.log").string();
    std::string cmd = "cd " + test_dir.string() + " && " + ngspice + " -b " +
                      cir_file.filename().string() + " > " + log_file + " 2>/dev/null";
    int exit_code = system(cmd.c_str());
    double elapsed = (clock() - start) / CLOCKS_PER_SEC;
    result.runtime_sec = elapsed;

    if (exit_code != 0) {
        result.error = "ngspice exited with code " + std::to_string(exit_code);
        return result;
    }

    std::ifstream log(log_file);
    if (!log) {
        result.error = "no output log generated";
        return result;
    }

    // Check the single probed signal for toggling (LOW < 0.1V, HIGH > 4.0V)
    // Data rows: Index time v(pXb0_an)
    bool saw_low = false, saw_high = false;
    std::string line;
    int data_rows = 0;
    while (std::getline(log, line)) {
        if (line.empty() || !std::isdigit(line[0])) continue;
        std::istringstream iss(line);
        int idx; double time, val;
        iss >> idx >> time >> val;
        if (iss.fail()) continue;
        data_rows++;
        if (val < 0.1) saw_low = true;
        if (val > 4.0) saw_high = true;
    }

    if (data_rows == 0) {
        result.error = "no data rows in output";
        return result;
    }

    std::string port_str(1, first_port);
    if (!saw_low || !saw_high) {
        result.error = "PORT" + port_str + " bit 0 does not toggle (L:" + (saw_low?"Y":"N") + " H:" + (saw_high?"Y":"N") + ", " + std::to_string(data_rows) + " rows)";
        return result;
    }

    result.passed = true;
    return result;
}

TEST_CASE("ngspice co-simulation for all supported devices") {
    auto device_names = DeviceCatalog::list_devices();

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

    std::vector<NgspiceResult> results;
    int passed = 0, failed = 0, skipped = 0;
    double total_time = 0;

    for (const auto& name : device_names) {
        std::string chip(name);
        const auto* dev = DeviceCatalog::find(name);

        // Skip chips without valid GPIO or with non-standard IO layout
        if (dev) {
            bool has_gpio = false;
            for (u8 i = 0; i < dev->port_count; i++) {
                if (dev->ports[i].ddr_address != 0 && dev->ports[i].port_address != 0) {
                    // Also verify addresses are in data space (>=0x20), not raw I/O offsets
                    has_gpio = dev->ports[i].ddr_address >= 0x20;
                    break;
                }
            }
            if (!has_gpio) {
                skipped++;
                continue;
            }
        }

        if (!chip_supported_by_gcc(chip)) {
            skipped++;
            continue;
        }

        auto result = test_chip(chip, build_dir);

        total_time += result.runtime_sec;
        results.push_back(result);

        if (result.passed) {
            passed++;
            MESSAGE("PASS " << chip << " (" << result.runtime_sec << "s)");
        } else if (result.should_skip) {
            skipped++;
            MESSAGE("SKIP " << chip << " - " << result.error << " (" << result.runtime_sec << "s)");
        } else {
            failed++;
            MESSAGE("FAIL " << chip << " - " << result.error << " (" << result.runtime_sec << "s)");
        }
    }

    MESSAGE("\n=== Results: " << passed << " passed, " << failed << " failed, "
            << skipped << " skipped, " << (passed + failed + skipped) << " total, "
            << total_time << "s ===");

    CHECK(failed == 0);
}
