#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <string>
#include "vioavr/core/bridge_shm.hpp"

using namespace vioavr::core;

int main(int argc, char** argv) {
    const char* shm_name = "/vioavr_shm_default";
    if (argc > 1) shm_name = argv[1];
    
    int fd = shm_open(shm_name, O_RDWR, 0666);
    if (fd == -1) {
        std::cerr << "{\"error\": \"Failed to open SHM: " << shm_name << "\"}" << std::endl;
        return 1;
    }

    VioBridgeShm* shm = (VioBridgeShm*)mmap(nullptr, sizeof(VioBridgeShm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        std::cerr << "{\"error\": \"Failed to map SHM\"}" << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "EXIT") break;
        
        if (line.rfind("CMD ", 0) == 0) {
            // Format: CMD <type> [arg]
            std::string sub = line.substr(4);
            size_t space = sub.find(' ');
            uint32_t type = std::stoul(sub.substr(0, space));
            if (type == 1) { // RESET
                shm->command.store(1);
            } else if (type == 2) { // LOAD
                std::string arg = sub.substr(space + 1);
                strncpy(shm->command_arg, arg.c_str(), 255);
                shm->command.store(2);
            } else if (type == 4) { // STEP
                shm->request_cycles = 1000;
            } else if (type == 8) { // RUN
                shm->request_cycles = 16000;
            } else if (type == 16) { // VCD
                uint32_t current = shm->command.load();
                shm->command.store(current ^ (1 << 16)); // Toggle bit 16
            }
            sem_post(&shm->sem_req);
        }

        // Always output state
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
    }

    munmap(shm, sizeof(VioBridgeShm));
    close(fd);
    return 0;
}
