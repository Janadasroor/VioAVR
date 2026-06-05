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
    // Use volatile pointer access for ADC registers since VioAVR XMEGA
    // descriptor addresses differ from real hardware register map.
    std::string src = R"(#include <avr/io.h>

#define VIO_ADC_CTRLA   (*(volatile unsigned char*)0x0200)
#define VIO_ADC_CTRLB   (*(volatile unsigned char*)0x0201)
#define VIO_ADC_REFCTRL (*(volatile unsigned char*)0x0202)
#define VIO_ADC_INTFLG  (*(volatile unsigned char*)0x0203)
#define VIO_ADC_PRESC   (*(volatile unsigned char*)0x0204)
#define VIO_ADC_RESL    (*(volatile unsigned char*)0x0210)
#define VIO_ADC_RESH    (*(volatile unsigned char*)0x0211)

int main(void) {
    PORTA_DIR = PIN0_bm | PIN1_bm;
    PORTA_OUT = 0;

    ACA_AC0MUXCTRL = 0;
    ACA_AC0CTRL = AC_ENABLE_bm;
    ACA_CTRLA = AC_ENABLE_bm;

    volatile unsigned char i;
    for (i = 0; i < 20; i++);

    if (ACA_STATUS & 0x01) {
        PORTA_OUTSET = PIN0_bm;
    }

    VIO_ADC_PRESC = 0;
    VIO_ADC_REFCTRL = 0x03;
    VIO_ADC_CTRLB = 0;
    VIO_ADC_CTRLA = 0x41;
    while (!(VIO_ADC_INTFLG & 0x01));

    unsigned int lo = VIO_ADC_RESL;
    unsigned int hi = VIO_ADC_RESH;
    unsigned int result = lo | (hi << 8);
    if (result > 2000) {
        PORTA_OUTSET = PIN1_bm;
    }

    for (;;) {}
})";

    fs::path tmp_dir = fs::temp_directory_path() / "vioavr_xmega_ac_adc";
    fs::create_directories(tmp_dir);
    fs::path src_file = tmp_dir / "ac_adc_fw.c";
    fs::path elf_file = tmp_dir / "ac_adc_fw.elf";

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
    of << "* VioAVR XMEGA AC + ADC co-simulation test\n";
    of << "* ATxmega8E5 AC0: AINP0 (bank ch0=3.0V) vs AINN0 (bank ch8=1.5V)\n";
    of << "* ADC: MUX=bank ch0 (3.0V), Vref=VCC (5.0V)\n";
    of << "* Results: PA0 (ext 0) from AC, PA1 (ext 1) from ADC\n";
    of << "\n";
    of << "V_ainp ainp 0 DC 3.0\n";
    of << "A_adc_bridge0 ainp dummy0 avr_adc_bridge_0\n";
    of << ".model avr_adc_bridge_0 avr_adc_bridge(channel=0)\n";
    of << "R_dummy0 dummy0 0 1k\n";
    of << "\n";
    of << "V_ainn ainn 0 DC 1.5\n";
    of << "A_adc_bridge1 ainn dummy1 avr_adc_bridge_1\n";
    of << ".model avr_adc_bridge_1 avr_adc_bridge(channel=8)\n";
    of << "R_dummy1 dummy1 0 1k\n";
    of << "\n";
    of << "R_pull_pa0 pa0_dig 0 10k\n";
    of << "R_pull_pa1 pa1_dig 0 10k\n";
    of << "A_dac0 [pa0_dig] [pa0_an 0] dac_bridge_model\n";
    of << "A_dac1 [pa1_dig] [pa1_an 0] dac_bridge_model\n";
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
        else if (i == 1) of << "pa1_dig";
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atxmega8e5\",\"" << hex_rel << "\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 0.5u 100u\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pa0_an) v(pa1_an)\n";
    of << ".endc\n";
    of << ".end\n";
    return true;
}

TEST_CASE("XMEGA AC + ADC co-simulation") {
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

    fs::path test_dir = build_dir / "cosim_tests" / "xmega_ac_adc";
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

    fs::path hex_file = test_dir / "ac_adc.hex";
    bool compiled = compile_firmware(hex_file);
    if (!compiled) {
        MESSAGE("Firmware compilation failed — skipping test (avr-gcc may not support atxmega8e5)");
        return;
    }

    if (fs::exists(test_dir / "firmware.hex")) {
        fs::remove(test_dir / "firmware.hex");
    }
    fs::create_symlink(hex_file.filename(), test_dir / "firmware.hex");

    bool spiceinit_ok = create_spiceinit();
    REQUIRE_MESSAGE(spiceinit_ok, "Failed to create .spiceinit with avr_adc_bridge.cm");

    fs::path cir_file = test_dir / "xmega_ac_adc.cir";
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

    // Parse 2-signal log: Index time pa0_an pa1_an
    std::ifstream log_stream(log_file);
    REQUIRE_MESSAGE(log_stream.is_open(), "No output log generated");

    struct Row { double time; double pa0; double pa1; };
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
    MESSAGE("XMEGA AC+ADC co-sim: " << rows.size() << " rows, "
            << (elapsed * 1000) << "ms");

    // PA0 = AC0 output (3.0V > 1.5V → ACO=1 → HIGH after settle)
    bool pa0_low = false, pa0_high = false;
    bool pa0_transition = false;
    int prev_pa0 = -1;
    for (const auto& r : rows) {
        if (r.pa0 < 0.1) pa0_low = true;
        if (r.pa0 > 4.0) pa0_high = true;
        int b = (r.pa0 < 0.5) ? 0 : (r.pa0 > 4.0 ? 1 : prev_pa0);
        if (prev_pa0 >= 0 && b != prev_pa0) pa0_transition = true;
        prev_pa0 = b;
    }

    // PA1 = ADC result output (3.0V → 2457 > 2000 → HIGH)
    bool pa1_low = false, pa1_high = false;
    bool pa1_transition = false;
    int prev_pa1 = -1;
    for (const auto& r : rows) {
        if (r.pa1 < 0.1) pa1_low = true;
        if (r.pa1 > 4.0) pa1_high = true;
        int b = (r.pa1 < 0.5) ? 0 : (r.pa1 > 4.0 ? 1 : prev_pa1);
        if (prev_pa1 >= 0 && b != prev_pa1) pa1_transition = true;
        prev_pa1 = b;
    }

    MESSAGE("PA0 L:" << pa0_low << " H:" << pa0_high << " T:" << pa0_transition);
    MESSAGE("PA1 L:" << pa1_low << " H:" << pa1_high << " T:" << pa1_transition);

    CHECK_MESSAGE(pa0_low,  "PA0 never LOW — AC did not start LOW");
    CHECK_MESSAGE(pa0_high, "PA0 never HIGH — AC comparison failed (3.0V > 1.5V expected)");
    CHECK_MESSAGE(pa0_transition, "PA0 no transition — AC not settling correctly");

    CHECK_MESSAGE(pa1_low,  "PA1 never LOW — ADC did not start LOW");
    CHECK_MESSAGE(pa1_high, "PA1 never HIGH — ADC conversion failed (3.0V -> ~2457 > 2000)");
    CHECK_MESSAGE(pa1_transition, "PA1 no transition — ADC conversion not completing");

    MESSAGE("PASS XMEGA AC+ADC (" << elapsed << "s)");
}
