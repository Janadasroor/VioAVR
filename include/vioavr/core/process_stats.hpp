#pragma once

#include "vioavr/core/types.hpp"

#include <cstdint>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/task_info.h>
#else
#include <fstream>
#include <unistd.h>
#endif

namespace vioavr::core {

[[nodiscard]] inline u64 process_rss_bytes() noexcept
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<u64>(pmc.WorkingSetSize);
    }
    return 0;
#elif defined(__APPLE__)
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
        return static_cast<u64>(info.resident_size);
    }
    return 0;
#else
    // Linux /proc/self/status
    std::ifstream status("/proc/self/status");
    if (!status) return 0;
    std::string line;
    while (std::getline(status, line)) {
        if (line.find("VmRSS:") == 0) {
            u64 val = 0;
            char unit[8] = {};
            std::sscanf(line.c_str(), "%*s %llu %s", &val, unit);
            // Convert kB to bytes
            return val * 1024ULL;
        }
    }
    return 0;
#endif
}

} // namespace vioavr::core
