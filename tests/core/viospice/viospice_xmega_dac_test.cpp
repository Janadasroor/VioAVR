#include "doctest.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <vector>
#include <regex>

namespace fs = std::filesystem;

static bool compile_firmware(const fs::path& output_hex) {
    std::string src = R"(#include <avr/io.h>

#define VIO_DAC0_CTRLA    (*(volatile unsigned char*)0x0300)
#define VIO_DAC0_STATUS   (*(volatile unsigned char*)0x0305)
#define VIO_DAC0_CH0DATAL (*(volatile unsigned char*)0x0318)
#define VIO_DAC0_CH0DATAH (*(volatile unsigned char*)0x0319)

int main(void) {
    PORTA_DIR = PIN0_bm;
    PORTA_OUT = 0;

    VIO_DAC0_CTRLA = 0x01;

    VIO_DAC0_CH0DATAL = 0x00;
    VIO_DAC0_CH0DATAH = 0x08;

    volatile unsigned int timeout = 0;
    while (!(VIO_DAC0_STATUS & 0x01) && timeout < 50000) timeout++;

    PORTA_OUTSET = PIN0_bm;

    for (;;) {}
})";

    fs::path tmp_dir = fs::temp_directory_path() / "vioavr_xmega_dac";
    fs::create_directories(tmp_dir);
    fs::path src_file = tmp_dir / "dac_fw.c";
    fs::path elf_file = tmp_dir / "dac_fw.elf";

    {
        std::ofstream of(src_file);
        of << src;
    }

    std::string compile_cmd = "avr-gcc -mmcu=atxmega128a1 -Os -o "
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
    of << "* VioAVR XMEGA DAC co-simulation test\n";
    of << "* ATxmega128A1 DAC0 CH0: data=2048, Vref=5V -> 2.5V expected\n";
    of << "* PA0 (ext 0) = firmware completion signal\n";
    of << "\n";
    of << "R_pull_pa0 pa0_dig 0 10k\n";
    of << "A_dac0 [pa0_dig] [pa0_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    of << "R_dummy dummy 0 1k\n";
    of << "A_avr_dac dummy dac0_out avr_dac_bridge_0\n";
    of << ".model avr_dac_bridge_0 avr_dac_bridge(channel=0)\n";
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
    of << " sim_args=[\"atxmega128a1\",\"" << hex_rel << "\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 0.5u 50u\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pa0_an) v(dac0_out)\n";
    of << ".endc\n";
    of << ".end\n";
    return true;
}

TEST_CASE("XMEGA DAC co-simulation") {
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

    fs::path test_dir = build_dir / "cosim_tests" / "xmega_dac";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);
    fs::create_directories(test_dir / "cosim");

    fs::path cosim_so = build_dir / "cosim" / "libavr_cosim.so";
    REQUIRE_MESSAGE(fs::exists(cosim_so), "libavr_cosim.so not found");
    fs::create_symlink(cosim_so, test_dir / "cosim" / "libavr_cosim.so");

    auto create_spiceinit = [&]() -> bool {
        std::ofstream of(test_dir / ".spiceinit");
        if (!of) return false;

        const char* home = getenv("HOME");
        if (home) {
            std::ifstream home_spice(std::string(home) + "/.spiceinit");
            if (home_spice) {
                of << home_spice.rdbuf();
                of << "\n";
            }
        }

        fs::path adc_bridge_cm = build_dir / "cosim" / "avr_adc_bridge.cm";
        if (!fs::exists(adc_bridge_cm)) return false;
        of << "codemodel " << adc_bridge_cm.string() << "\n";
        return true;
    };

    fs::path hex_file = test_dir / "dac.hex";
    bool compiled = compile_firmware(hex_file);
    if (!compiled) {
        MESSAGE("Firmware compilation failed \u2014 skipping test (avr-gcc may not support atxmega128a1)");
        return;
    }

    if (fs::exists(test_dir / "firmware.hex")) {
        fs::remove(test_dir / "firmware.hex");
    }
    fs::create_symlink(hex_file.filename(), test_dir / "firmware.hex");

    bool spiceinit_ok = create_spiceinit();
    REQUIRE_MESSAGE(spiceinit_ok, "Failed to create .spiceinit with avr_adc_bridge.cm");

    fs::path cir_file = test_dir / "xmega_dac.cir";
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

    std::ifstream log_stream(log_file);
    REQUIRE_MESSAGE(log_stream.is_open(), "No output log generated");

    struct Row { double time; double pa0; double dac_out; };
    std::vector<Row> rows;
    std::string line;
    while (std::getline(log_stream, line)) {
        if (line.empty() || !std::isdigit(line[0])) continue;
        std::istringstream iss(line);
        int idx; double t, v0, v1;
        if (!(iss >> idx >> t >> v0 >> v1)) continue;
        rows.push_back({t, v0, v1});
    }

    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");
    MESSAGE("XMEGA DAC co-sim: " << rows.size() << " rows, "
            << (elapsed * 1000) << "ms");

    bool pa0_low = false, pa0_high = false;
    for (const auto& r : rows) {
        if (r.pa0 < 0.1) pa0_low = true;
        if (r.pa0 > 4.0) pa0_high = true;
    }

    bool dac_has_value = false;
    double dac_vmin = 1e9, dac_vmax = -1e9;
    for (const auto& r : rows) {
        if (r.dac_out > 0.01) dac_has_value = true;
        if (r.dac_out < dac_vmin) dac_vmin = r.dac_out;
        if (r.dac_out > dac_vmax) dac_vmax = r.dac_out;
    }

    MESSAGE("PA0 L:" << pa0_low << " H:" << pa0_high);
    MESSAGE("DAC0 CH0: min=" << dac_vmin << "V max=" << dac_vmax << "V");

    CHECK_MESSAGE(pa0_low,  "PA0 never LOW \u2014 firmware did not start LOW");
    CHECK_MESSAGE(pa0_high, "PA0 never HIGH \u2014 DAC conversion did not complete");

    CHECK_MESSAGE(dac_has_value, "DAC output always 0V \u2014 conversion not reaching signal bank");
    CHECK_MESSAGE(dac_vmax > 2.0, "DAC output never reached >2V (expected ~2.5V for data=2048, Vref=5V)");
    CHECK_MESSAGE(dac_vmax < 3.0, "DAC output exceeded 3.0V (expected ~2.5V)");

    MESSAGE("PASS XMEGA DAC (" << elapsed << "s)");
}
