#include "system_info.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <pwd.h>
#include <unistd.h>

SystemInfo::SystemInfo(const MtopConfig& cfg) : config(cfg), prev_total_time(0), prev_idle_time(0) {
    updateStats();
}

SystemStats SystemInfo::getStats() {
    return stats;
}

void SystemInfo::updateConfig(const MtopConfig& new_config) {
    config = new_config;
}

void SystemInfo::updateStats() {
    readCpuStats();
    readMemoryStats();
    readLoadAverage();
    readProcesses();
    applyProcessFilters();
    sortProcesses();
}

void SystemInfo::readCpuStats() {
    std::ifstream file("/proc/stat");
    std::string line;
    
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string cpu_label;
        uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
        
        iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        
        uint64_t total_time = user + nice + system + idle + iowait + irq + softirq + steal;
        uint64_t idle_time = idle + iowait;
        
        stats.cpu_percent = calculateCpuPercent(total_time, idle_time);
        
        prev_total_time = total_time;
        prev_idle_time = idle_time;
    }
}

double SystemInfo::calculateCpuPercent(uint64_t total_time, uint64_t idle_time) {
    if (prev_total_time == 0) return 0.0;
    
    uint64_t total_diff = total_time - prev_total_time;
    uint64_t idle_diff = idle_time - prev_idle_time;
    
    if (total_diff == 0) return 0.0;
    
    return 100.0 * (1.0 - static_cast<double>(idle_diff) / total_diff);
}

void SystemInfo::readMemoryStats() {
    std::ifstream file("/proc/meminfo");
    std::string line;
    
    stats.total_memory_kb = 0;
    stats.free_memory_kb = 0;
    uint64_t available_kb = 0;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        uint64_t value;
        
        iss >> key >> value;
        
        if (key == "MemTotal:") {
            stats.total_memory_kb = value;
        } else if (key == "MemAvailable:") {
            available_kb = value;
        } else if (key == "MemFree:" && available_kb == 0) {
            stats.free_memory_kb = value;
        }
    }
    
    if (available_kb > 0) {
        stats.free_memory_kb = available_kb;
    }
    
    stats.used_memory_kb = stats.total_memory_kb - stats.free_memory_kb;
}

void SystemInfo::readLoadAverage() {
    std::ifstream file("/proc/loadavg");
    if (file.is_open()) {
        file >> stats.load_avg[0] >> stats.load_avg[1] >> stats.load_avg[2];
    }
}

void SystemInfo::readProcesses() {
    stats.processes.clear();
    stats.process_count = 0;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
            if (!entry.is_directory()) continue;
            
            std::string filename = entry.path().filename().string();
            if (!std::all_of(filename.begin(), filename.end(), ::isdigit)) continue;
            
            int pid = std::stoi(filename);
            ProcessInfo proc;
            proc.pid = pid;
            proc.is_kernel_thread = false;
            
            // Читаем /proc/PID/stat
            std::ifstream stat_file("/proc/" + filename + "/stat");
            if (!stat_file.is_open()) continue;
            
            std::string stat_line;
            std::getline(stat_file, stat_line);
            
            // Парсим строку stat правильно - имя процесса может содержать пробелы в скобках
            size_t first_paren = stat_line.find('(');
            size_t last_paren = stat_line.rfind(')');
            
            if (first_paren == std::string::npos || last_paren == std::string::npos) {
                continue;
            }
            
            // Извлекаем имя процесса (между скобками)
            proc.name = stat_line.substr(first_paren + 1, last_paren - first_paren - 1);
            
            // Парсим остальные поля после закрывающей скобки
            std::string remaining = stat_line.substr(last_paren + 1);
            std::istringstream iss(remaining);
            
            std::vector<std::string> fields;
            std::string field;
            while (iss >> field) {
                fields.push_back(field);
            }
            
            if (fields.size() >= 22) {
                proc.state = fields[0]; // Первое поле после имени - состояние
                
                // Проверяем, является ли процесс kernel thread
                int ppid = std::stoi(fields[1]);
                if (ppid == 2 || (proc.name.front() == '[' && proc.name.back() == ']')) {
                    proc.is_kernel_thread = true;
                }
                
                // RSS находится в поле 21 (после имени и состояния)
                uint64_t rss = std::stoull(fields[21]);
                proc.memory_kb = rss * 4; // RSS в страницах по 4KB
            } else {
                continue; // Пропускаем процесс если данных недостаточно
            }
            
            // Читаем /proc/PID/status для получения UID
            std::ifstream status_file("/proc/" + filename + "/status");
            std::string status_line;
            proc.uid = 0;
            
            while (std::getline(status_file, status_line)) {
                if (status_line.substr(0, 4) == "Uid:") {
                    std::istringstream uid_iss(status_line);
                    std::string uid_label;
                    uid_iss >> uid_label >> proc.uid;
                    break;
                }
            }
            
            proc.user = getUserName(proc.uid);
            proc.cpu_percent = 0.0; // Упрощенная версия, без вычисления CPU
            
            stats.processes.push_back(proc);
            stats.process_count++;
        }
    } catch (const std::exception&) {
        // Игнорируем ошибки чтения процессов
    }
}

void SystemInfo::applyProcessFilters() {
    // Фильтруем процессы согласно конфигурации
    auto it = std::remove_if(stats.processes.begin(), stats.processes.end(),
                             [this](const ProcessInfo& proc) {
                                 return !shouldShowProcess(proc);
                             });
    stats.processes.erase(it, stats.processes.end());
    
    // Ограничиваем количество процессов
    if (stats.processes.size() > static_cast<size_t>(config.max_processes)) {
        stats.processes.resize(config.max_processes);
    }
}

bool SystemInfo::shouldShowProcess(const ProcessInfo& proc) const {
    // Проверяем kernel threads
    if (proc.is_kernel_thread && !config.show_kernel_threads) {
        return false;
    }
    
    // Проверяем скрытые процессы
    for (const auto& hidden : config.hide_processes) {
        if (proc.name.find(hidden) != std::string::npos) {
            return false;
        }
    }
    
    // Проверяем фильтр по пользователям
    if (!config.show_only_users.empty()) {
        bool found = false;
        for (const auto& user : config.show_only_users) {
            if (proc.user == user) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    return true;
}

void SystemInfo::sortProcesses() {
    std::sort(stats.processes.begin(), stats.processes.end(),
              [this](const ProcessInfo& a, const ProcessInfo& b) {
                  bool result = false;
                  
                  switch (config.sort_by) {
                      case MtopConfig::SortBy::MEMORY:
                          result = a.memory_kb > b.memory_kb;
                          break;
                      case MtopConfig::SortBy::CPU:
                          result = a.cpu_percent > b.cpu_percent;
                          break;
                      case MtopConfig::SortBy::PID:
                          result = a.pid < b.pid;
                          break;
                      case MtopConfig::SortBy::NAME:
                          result = a.name < b.name;
                          break;
                  }
                  
                  return config.reverse_sort ? !result : result;
              });
}

std::string SystemInfo::getUserName(int uid) {
    struct passwd* pw = getpwuid(uid);
    return pw ? std::string(pw->pw_name) : std::to_string(uid);
}