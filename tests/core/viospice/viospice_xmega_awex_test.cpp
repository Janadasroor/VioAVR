#include "doctest.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <cmath>

namespace fs = std::filesystem;

static bool compile_firmware(const fs::path& output_hex) {
    // Minimal firmware: configure TC SS PWM on PORTC via PortMux
    // Use direct PORT register access for verification
    std::string src = R"(#include <avr/io.h>

int main(void) {
    PORTC_DIR = PIN4_bm | PIN5_bm;
    PORTC_OUT = 0;

    TCC0_CTRLD = 0x03;
    TCC0_CTRLB = 0x10;
    TCC0_PER = 399;
    TCC0_CCA = 200;
    TCC0_CTRLA = 0x02;

    AWEXC_CTRL = 0x01;
    AWEXC_DTBOTH = 200;

    for (;;) {}
})";

    fs::path tmp_dir = fs::temp_directory_path() / "vioavr_xmega_awex";
    fs::create_directories(tmp_dir);
    fs::path src_file = tmp_dir / "awex_fw.c";
    fs::path elf_file = tmp_dir / "awex_fw.elf";

    {
        std::ofstream of(src_file);
        of << src;
    }

    std::string compile_cmd = "avr-gcc -mmcu=atxmega128a1 -Os -o "
        + elf_file.string() + " " + src_file.string() + " 2>/dev/null";
    if (system(compile_cmd.c_str()) != 0) {
        MESSAGE("avr-gcc -mmcu=atxmega128a1 failed");
        return false;
    }

    std::string objcopy_cmd = "avr-objcopy -j .text -j .data -O ihex "
        + elf_file.string() + " " + output_hex.string() + " 2>/dev/null";
    if (system(objcopy_cmd.c_str()) != 0) {
        MESSAGE("avr-objcopy failed");
        return false;
    }

    // Dump firmware map to verify register addresses
    std::string nm_cmd = "avr-nm " + elf_file.string() + " 2>/dev/null | grep -E '[0-9a-f]+ [Tt] ' | head -5";
    system(nm_cmd.c_str());

    return true;
}

static bool generate_circuit(const fs::path& cir_path, const fs::path& hex_path) {
    std::string hex_rel = hex_path.filename().string();

    std::ofstream of(cir_path);
    of << "* VioAVR XMEGA AWEX co-simulation test\n";
    of << "* ATxmega128A1 TCC0 SS PWM + AWEXC on PORTC\n";
    of << "* DH0=PC4 (ext 20), DL0=PC5 (ext 21)\n";
    of << "* PER=399, CCA=200, clk/8 prescaler = 100 us period\n";
    of << "\n";
    of << "R_dh pc4_dig 0 10k\n";
    of << "R_dl pc5_dig 0 10k\n";
    of << "A_dac_dh [pc4_dig] [pc4_an 0] dac_bridge_model\n";
    of << "A_dac_dl [pc5_dig] [pc5_an 0] dac_bridge_model\n";
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
        if (i == 20) of << "pc4_dig";
        else if (i == 21) of << "pc5_dig";
        else of << "0";
    }
    of << "] d_cosim_model\n";
    of << ".model d_cosim_model d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atxmega128a1\",\"" << hex_rel << "\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 0.5u 800u\n";
    of << ".control\n";
    of << "run\n";
    of << "print v(pc4_an) v(pc5_an) v(pc5_dig)\n";
    of << ".endc\n";
    of << ".end\n";
    return true;
}

TEST_CASE("XMEGA AWEX co-simulation") {
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

    fs::path test_dir = build_dir / "cosim_tests" / "xmega_awex";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);
    fs::create_directories(test_dir / "cosim");

    fs::path cosim_so = build_dir / "cosim" / "libavr_cosim.so";
    REQUIRE_MESSAGE(fs::exists(cosim_so), "libavr_cosim.so not found");
    fs::create_symlink(cosim_so, test_dir / "cosim" / "libavr_cosim.so");

    fs::path hex_file = test_dir / "awex.hex";
    bool compiled = compile_firmware(hex_file);
    if (!compiled) {
        MESSAGE("Firmware compilation failed — skipping test");
        return;
    }

    if (fs::exists(test_dir / "firmware.hex")) {
        fs::remove(test_dir / "firmware.hex");
    }
    fs::create_symlink(hex_file.filename(), test_dir / "firmware.hex");

    fs::path cir_file = test_dir / "xmega_awex.cir";
    generate_circuit(cir_file, hex_file);

    double start = clock();
    std::string ngspice = "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice";
    std::string log_file = (test_dir / "sim_matrix.log").string();
    std::string debug_file_str = (test_dir / "awex_debug.log").string();
    std::string cmd = "cd " + test_dir.string() + " && " + ngspice + " -b "
        + cir_file.filename().string() + " > " + log_file + " 2>" + debug_file_str;
    int exit_code = system(cmd.c_str());
    double elapsed = (clock() - start) / CLOCKS_PER_SEC;

    // Read AWEX debug log
    {
        std::ifstream df(debug_file_str);
        std::string dl;
        MESSAGE("AWEX debug (first 100 lines):");
        int dl_cnt = 0;
        while (std::getline(df, dl) && dl_cnt < 100) {
            MESSAGE("  " << dl);
            dl_cnt++;
        }
    }

    REQUIRE_MESSAGE(exit_code == 0,
        "ngspice exited with code " << std::to_string(exit_code)
        << " (" << elapsed << "s)");

    // Parse log: Index time pc4_an pc5_an pc5_dig
    std::ifstream log_stream(log_file);
    REQUIRE_MESSAGE(log_stream.is_open(), "No output log generated");

    struct Row { double time; double dh; double dl; double dl_dig; };
    std::vector<Row> rows;
    std::string line;
    while (std::getline(log_stream, line)) {
        if (line.empty() || !std::isdigit(line[0])) continue;
        std::istringstream iss(line);
        int idx; double t, v0, v1, v2;
        if (!(iss >> idx >> t >> v0 >> v1 >> v2)) continue;
        rows.push_back({t, v0, v1, v2});
    }

    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");
    MESSAGE("XMEGA AWEX co-sim: " << rows.size() << " rows, "
            << (elapsed * 1000) << "ms");

    // Print a few raw samples for debugging
    MESSAGE("First 20 samples:");
    for (int i = 0; i < std::min(20, (int)rows.size()); i++)
        MESSAGE("  t=" << (rows[i].time * 1e6) << " us  PC4_an=" << rows[i].dh << "V  PC5_an=" << rows[i].dl << "V  PC5_dig=" << rows[i].dl_dig << "V");

    // Find DH transitions
    MESSAGE("DH transition times:");
    int prev_dh_val = -1;
    for (size_t i = 0; i < rows.size(); i++) {
        int b = (rows[i].dh < 0.5) ? 0 : (rows[i].dh > 4.0 ? 1 : prev_dh_val);
        if (prev_dh_val >= 0 && b != prev_dh_val) {
            MESSAGE("  t=" << (rows[i].time * 1e6) << " us  DH " << prev_dh_val << "->" << b
                    << "  DL=" << rows[i].dl << "V  DL_dig=" << rows[i].dl_dig << "V");
        }
        prev_dh_val = b;
    }

    MESSAGE("DL samples around first DH transition:");
    // Find rows near 30us (first DH rise)
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i].time > 28e-6 && rows[i].time < 34e-6) {
            MESSAGE("  t=" << (rows[i].time * 1e6) << " us  DH=" << rows[i].dh << "V  DL=" << rows[i].dl << "V");
        }
    }
    // Find where DL transitions
    int prev_dl_val2 = -1;
    for (size_t i = 0; i < rows.size(); i++) {
        int b = (rows[i].dl < 0.5) ? 0 : (rows[i].dl > 4.0 ? 1 : prev_dl_val2);
        if (prev_dl_val2 >= 0 && b != prev_dl_val2) {
            MESSAGE("  DL transition at " << (rows[i].time * 1e6) << " us: " << prev_dl_val2 << "->" << b << "  DH=" << rows[i].dh << "V");
        }
        prev_dl_val2 = b;
    }

    MESSAGE("Last 5 samples:");
    for (int i = std::max(0, (int)rows.size() - 5); i < (int)rows.size(); i++)
        MESSAGE("  t=" << (rows[i].time * 1e6) << " us  PC4=" << rows[i].dh << "V  PC5=" << rows[i].dl << "V  PC5_dig=" << rows[i].dl_dig << "V");

    // Check DH (PC4) toggles
    bool dh_low = false, dh_high = false;
    int dh_transitions = 0;
    int prev_dh = -1;
    for (const auto& r : rows) {
        if (r.dh < 0.1) dh_low = true;
        if (r.dh > 4.0) dh_high = true;
        int b = (r.dh < 0.5) ? 0 : (r.dh > 4.0 ? 1 : prev_dh);
        if (prev_dh >= 0 && b != prev_dh) dh_transitions++;
        prev_dh = b;
    }

    bool dl_low = false, dl_high = false;
    int dl_transitions = 0;
    int prev_dl = -1;
    for (const auto& r : rows) {
        if (r.dl < 0.1) dl_low = true;
        if (r.dl > 4.0) dl_high = true;
        int b = (r.dl < 0.5) ? 0 : (r.dl > 4.0 ? 1 : prev_dl);
        if (prev_dl >= 0 && b != prev_dl) dl_transitions++;
        prev_dl = b;
    }

    // Check complementary (never both HIGH)
    bool both_high_found = false;
    for (const auto& r : rows) {
        if (r.dh > 4.0 && r.dl > 4.0) { both_high_found = true; break; }
    }

    MESSAGE("DH L:" << dh_low << " H:" << dh_high << " T:" << dh_transitions);
    MESSAGE("DL L:" << dl_low << " H:" << dl_high << " T:" << dl_transitions);
    MESSAGE("Both HIGH:" << both_high_found);

    CHECK_MESSAGE(dh_low,  "DH0 (PC4) never LOW — no PWM toggling");
    CHECK_MESSAGE(dh_high, "DH0 (PC4) never HIGH — no PWM toggling");
    CHECK_MESSAGE(dh_transitions >= 3, "DH0 fewer than 3 transitions — PWM not running");

    CHECK_MESSAGE(dl_low,  "DL0 (PC5) never LOW — no complementary output");
    CHECK_MESSAGE(dl_high, "DL0 (PC5) never HIGH — no complementary output");

    CHECK_MESSAGE(!both_high_found, "DH and DL both HIGH simultaneously — not complementary");

    MESSAGE("PASS XMEGA AWEX (" << elapsed << "s)");
}
