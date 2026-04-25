#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include "vioavr/core/bridge_shm.hpp"

using namespace vioavr::core;

int main(int argc, char** argv) {
    const char* shm_name = (argc > 1) ? argv[1] : "/vioavr_shm_default";
    
    int fd = shm_open(shm_name, O_RDONLY, 0666);
    if (fd == -1) {
        std::cerr << "{\"error\": \"Failed to open SHM: " << shm_name << "\"}" << std::endl;
        return 1;
    }

    VioBridgeShm* shm = (VioBridgeShm*)mmap(nullptr, sizeof(VioBridgeShm), PROT_READ, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        std::cerr << "{\"error\": \"Failed to map SHM\"}" << std::endl;
        return 1;
    }

    if (shm->magic != VIOAVR_BRIDGE_MAGIC) {
        std::cerr << "{\"error\": \"Magic mismatch\"}" << std::endl;
        return 1;
    }

    // Output JSON once
    std::cout << "{"
              << "\"status\": " << static_cast<uint32_t>(shm->status.load()) << ","
              << "\"sync_counter\": " << shm->sync_counter.load() << ","
              << "\"cpu\": {"
              << "\"pc\": " << shm->cpu_state.pc << ","
              << "\"sp\": " << shm->cpu_state.sp << ","
              << "\"sreg\": " << (int)shm->cpu_state.sreg << ","
              << "\"gprs\": [";
    for (int i = 0; i < 32; i++) {
        std::cout << (int)shm->cpu_state.gprs[i] << (i < 31 ? "," : "");
    }
    std::cout << "],"
              << "\"cycles\": " << shm->telemetry.total_cycles << ","
              << "\"core_state\": " << (int)shm->telemetry.core_state
              << "},"
              << "\"digital_outputs\": [";
    for (int i = 0; i < 128; i++) {
        std::cout << (int)shm->digital_outputs[i] << (i < 127 ? "," : "");
    }
    std::cout << "],"
              << "\"analog_outputs\": [";
    for (int i = 0; i < 32; i++) {
        std::cout << shm->analog_outputs[i] << (i < 31 ? "," : "");
    }
    std::cout << "]"
              << "}" << std::endl;

    munmap(shm, sizeof(VioBridgeShm));
    close(fd);
    return 0;
}
