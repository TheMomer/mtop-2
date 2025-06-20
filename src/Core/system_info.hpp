#ifndef SYSTEM_INFO_HPP
#define SYSTEM_INFO_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "parser.hpp"

struct ProcessInfo {
    int pid;
    std::string name;
    std::string state;
    double cpu_percent;
    uint64_t memory_kb;
    std::string user;
    int uid;
    bool is_kernel_thread;
};

struct SystemStats {
    double cpu_percent;
    uint64_t total_memory_kb;
    uint64_t used_memory_kb;
    uint64_t free_memory_kb;
    double load_avg[3];
    int process_count;
    std::vector<ProcessInfo> processes;
};

class SystemInfo {
public:
    SystemInfo(const MtopConfig& config);
    ~SystemInfo() = default;
    
    SystemStats getStats();
    void updateStats();
    void updateConfig(const MtopConfig& new_config);
    
private:
    SystemStats stats;
    MtopConfig config;
    uint64_t prev_total_time;
    uint64_t prev_idle_time;
    
    void readCpuStats();
    void readMemoryStats();
    void readProcesses();
    void readLoadAverage();
    std::string getUserName(int uid);
    double calculateCpuPercent(uint64_t total_time, uint64_t idle_time);
    
    // Process filtering
    bool shouldShowProcess(const ProcessInfo& proc) const;
    void sortProcesses();
    void applyProcessFilters();
};

#endif // SYSTEM_INFO_HPP