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

static bool compile_firmware(const fs::path& output_hex, const std::string& source) {
    fs::path tmp_dir = fs::temp_directory_path() / "vioavr_multi_i2c";
    fs::create_directories(tmp_dir);
    fs::path src_file = tmp_dir / "fw.c";
    fs::path elf_file = tmp_dir / "fw.elf";

    {
        std::ofstream of(src_file);
        of << source;
    }

    std::string compile_cmd = "avr-gcc -mmcu=atmega4809 -Os -o "
        + elf_file.string() + " " + src_file.string() + " 2>/dev/null";
    if (system(compile_cmd.c_str()) != 0) return false;

    std::string objcopy_cmd = "avr-objcopy -j .text -j .data -O ihex "
        + elf_file.string() + " " + output_hex.string() + " 2>/dev/null";
    if (system(objcopy_cmd.c_str()) != 0) return false;

    return true;
}

static std::string master_firmware() {
    return R"(#include <avr/io.h>

#define VIO_TWI0_MCTRLA  (*(volatile unsigned char*)0x08A3)
#define VIO_TWI0_MSTATUS (*(volatile unsigned char*)0x08A5)
#define VIO_TWI0_MBAUD   (*(volatile unsigned char*)0x08A6)
#define VIO_TWI0_MADDR   (*(volatile unsigned char*)0x08A7)
#define VIO_TWI0_MDATA   (*(volatile unsigned char*)0x08A8)
#define VIO_TWI0_MCTRLB  (*(volatile unsigned char*)0x08A4)

int main(void) {
    PORTA_DIR = PIN0_bm;
    PORTA_OUT = 0;

    VIO_TWI0_MBAUD = 200;
    VIO_TWI0_MCTRLA = 0x01;

    volatile unsigned int d;
    for (d = 0; d < 200; d++);

    VIO_TWI0_MADDR = (0x50 << 1) | 0;

    for (d = 0; d < 10000; d++) {
        if (VIO_TWI0_MSTATUS & 0x01) break;
    }

    VIO_TWI0_MDATA = 0xA5;

    for (d = 0; d < 10000; d++) {
        if (VIO_TWI0_MSTATUS & 0x01) break;
    }

    VIO_TWI0_MCTRLB = 0x03;

    PORTA_OUTSET = PIN0_bm;

    for (;;) {}
})";
}

static std::string slave_firmware() {
    return R"(#include <avr/io.h>

#define VIO_TWI0_SCTRLA  (*(volatile unsigned char*)0x08A9)
#define VIO_TWI0_SSTATUS (*(volatile unsigned char*)0x08AB)
#define VIO_TWI0_SADDR   (*(volatile unsigned char*)0x08AC)
#define VIO_TWI0_SDATA   (*(volatile unsigned char*)0x08AD)
#define VIO_TWI0_SCTRLB  (*(volatile unsigned char*)0x08AA)

int main(void) {
    PORTB_DIR = PIN0_bm;
    PORTB_OUT = 0;

    VIO_TWI0_SADDR = (0x50 << 1);
    VIO_TWI0_SCTRLA = 0x05;

    volatile unsigned int d;
    for (d = 0; d < 200; d++);

    for (d = 0; d < 200000; d++) {
        if (VIO_TWI0_SSTATUS & 0x01) {
            unsigned char data = VIO_TWI0_SDATA;
            if (data == 0xA5) {
                PORTB_OUTSET = PIN0_bm;
            }
            break;
        }
    }

    for (;;) {}
})";
}

static bool generate_circuit(const fs::path& cir_path) {
    std::ofstream of(cir_path);
    of << "* VioAVR multi-chip I2C co-simulation test\n";
    of << "* ATmega4809 master (PA0=success) + ATmega4809 slave (PB0=success)\n";
    of << "* TWI0: SCL=PA2(ext 2), SDA=PA3(ext 3)\n";
    of << "\n";
    of << "V_vcc vcc 0 5.0\n";
    of << "R_scl scl vcc 4.7k\n";
    of << "R_sda sda vcc 4.7k\n";
    of << "\n";
    of << "R_pull_pa0 pa0_master vcc 10k\n";
    of << "R_pull_pb0 pb0_slave vcc 10k\n";
    of << "A_dac_pa0 [pa0_master] [pa0_master_an 0] dac_bridge_model\n";
    of << "A_dac_pb0 [pb0_slave] [pb0_slave_an 0] dac_bridge_model\n";
    of << "A_dac_scl [scl] [scl_an 0] dac_bridge_model\n";
    of << "A_dac_sda [sda] [sda_an 0] dac_bridge_model\n";
    of << ".model dac_bridge_model dac_bridge(out_low=0.0 out_high=5.0 input_load=1e12)\n";
    of << "\n";
    of << "A_MASTER [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 2) of << "scl";
        else if (i == 3) of << "sda";
        else of << "0";
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 0) of << "pa0_master";
        else if (i == 2) of << "scl";
        else if (i == 3) of << "sda";
        else of << "0";
    }
    of << "] d_cosim_master\n";
    of << ".model d_cosim_master d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega4809\",\"master.hex\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << "A_SLAVE [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 2) of << "scl";
        else if (i == 3) of << "sda";
        else of << "0";
    }
    of << "] [";
    for (int i = 0; i < 32; i++) {
        if (i > 0) of << " ";
        if (i == 8) of << "pb0_slave";
        else of << "0";
    }
    of << "] d_cosim_slave\n";
    of << ".model d_cosim_slave d_cosim(simulation=\"./cosim/libavr_cosim.so\"";
    of << " sim_args=[\"atmega4809\",\"slave.hex\"]";
    of << " queue_size=1024 irreversible=1 delay=1e-9)\n";
    of << "\n";
    of << ".options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n";
    of << ".tran 1u 5000u\n";
    of << ".control\n";
    of << "set width=300\n";
    of << "run\n";
    of << "print v(pa0_master_an) v(pb0_slave_an) v(scl_an) v(sda_an)\n";
    of << ".endc\n";
    of << ".end\n";
    return true;
}

TEST_CASE("Multi-chip I2C co-simulation") {
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

    fs::path test_dir = build_dir / "cosim_tests" / "multi_i2c";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);
    fs::create_directories(test_dir / "cosim");

    fs::path cosim_so = build_dir / "cosim" / "libavr_cosim.so";
    REQUIRE_MESSAGE(fs::exists(cosim_so), "libavr_cosim.so not found");
    fs::path cosim_link = test_dir / "cosim" / "libavr_cosim.so";
    if (!fs::exists(cosim_link))
        fs::create_symlink(cosim_so, cosim_link);

    auto create_spiceinit = [&](const fs::path& dir) -> bool {
        std::ofstream of(dir / ".spiceinit");
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

    fs::path master_hex = test_dir / "master.hex";
    bool master_ok = compile_firmware(master_hex, master_firmware());
    if (!master_ok) {
        MESSAGE("Master firmware compilation failed -- skipping test");
        return;
    }

    fs::path slave_hex = test_dir / "slave.hex";
    bool slave_ok = compile_firmware(slave_hex, slave_firmware());
    if (!slave_ok) {
        MESSAGE("Slave firmware compilation failed -- skipping test");
        return;
    }

    if (fs::exists(test_dir / "firmware.hex")) {
        fs::remove(test_dir / "firmware.hex");
    }
    fs::create_symlink(master_hex.filename(), test_dir / "firmware.hex");

    bool spiceinit_ok = create_spiceinit(test_dir);
    REQUIRE_MESSAGE(spiceinit_ok, "Failed to create .spiceinit");

    fs::path cir_file = test_dir / "multi_i2c.cir";
    generate_circuit(cir_file);

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

    struct Row { double time; double pa0; double pb0; double scl; double sda; };
    std::vector<Row> rows;
    std::string line;
    while (std::getline(log_stream, line)) {
        if (line.empty() || !std::isdigit(line[0])) continue;
        std::istringstream iss(line);
        int idx; double t, v0, v1, v2, v3;
        if (!(iss >> idx >> t >> v0 >> v1 >> v2 >> v3)) continue;
        rows.push_back({t, v0, v1, v2, v3});
    }

    REQUIRE_MESSAGE(rows.size() > 0, "No data rows in output");
    MESSAGE("Multi-chip I2C co-sim: " << rows.size() << " rows, "
            << (elapsed * 1000) << "ms");

    double min_scl = 99, max_scl = -1;
    double min_sda = 99, max_sda = -1;
    bool pa0_low = false, pa0_high = false;
    bool pb0_low = false, pb0_high = false;
    int scl_transitions = 0;
    double last_scl = rows[0].scl;
    for (const auto& r : rows) {
        if (r.pa0 < 0.1) pa0_low = true;
        if (r.pa0 > 4.0) pa0_high = true;
        if (r.pb0 < 0.1) pb0_low = true;
        if (r.pb0 > 4.0) pb0_high = true;
        if (r.scl < min_scl) min_scl = r.scl;
        if (r.scl > max_scl) max_scl = r.scl;
        if (r.sda < min_sda) min_sda = r.sda;
        if (r.sda > max_sda) max_sda = r.sda;
        if ((r.scl > 2.5) != (last_scl > 2.5)) scl_transitions++;
        last_scl = r.scl;
    }

    MESSAGE("SCL: min=" << min_scl << "V max=" << max_scl << "V trans=" << scl_transitions);
    MESSAGE("SDA: min=" << min_sda << "V max=" << max_sda << "V");
    MESSAGE("Master PA0 L:" << pa0_low << " H:" << pa0_high);
    MESSAGE("Slave  PB0 L:" << pb0_low << " H:" << pb0_high);

    CHECK_MESSAGE(scl_transitions > 0, "SCL never toggled -- I2C master not driving bus");
    CHECK_MESSAGE(min_scl < 0.5, "SCL never goes LOW");
    CHECK_MESSAGE(max_scl > 4.0, "SCL never goes HIGH");
    CHECK_MESSAGE(pa0_low,  "Master PA0 never LOW");
    CHECK_MESSAGE(pa0_high, "Master PA0 never HIGH -- I2C transaction did not complete");
    CHECK_MESSAGE(pb0_low,  "Slave PB0 never LOW");
    CHECK_MESSAGE(pb0_high, "Slave PB0 never HIGH -- slave did not receive I2C data");

    MESSAGE("PASS Multi-chip I2C (" << elapsed << "s)");
}
