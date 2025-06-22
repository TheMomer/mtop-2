#include <ios>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <csignal>
#include <termios.h>
#include <unistd.h>
#include "system_info.hpp"
#include "parser.hpp"
#include <cstdlib>

class Display {
public:
    Display(const MtopConfig& config) : config(config) {
        // Скрываем курсор
        std::cout << "\033[?25l";
    }
    
    ~Display() {
        // Показываем курсор
        std::cout << "\033[?25h";
        std::cout << "\033[0m"; // Сброс цветов
    }
    
    void updateConfig(const MtopConfig& new_config) {
        config = new_config;
    }
    
    void printHeader() {
        std::cout << "\033[1;36m"; // Яркий голубой
        std::cout << "╭───────────────────────────────────╮\n";
        std::cout << "│ \033[1;35mmtop 2\033[1;36m - Modern Top 2 by TheMomer │\n";
        std::cout << "╰───────────────────────────────────╯\033[0m\n";
    }
    
    void printSystemStats(const SystemStats& stats) {
        std::cout << "\033[1;33m"; // Желтый для заголовков
        
        // CPU
        if (config.show_cpu_bar) {
            std::cout << "\033[1m\033[93mCPU: ";
            printProgressBar(stats.cpu_percent, 100.0, config.progress_bar_width);
            std::cout << " " << std::fixed << std::setprecision(1) << stats.cpu_percent << "%\n";
        } else {
            std::cout << "CPU: " << std::fixed << std::setprecision(1) << stats.cpu_percent << "%\n";
        }
        
        // Memory
        double mem_percent = (static_cast<double>(stats.used_memory_kb) / stats.total_memory_kb) * 100.0;
        if (config.show_memory_bar) {
            std::cout << "\033[1m\033[93mMEM: ";
            printProgressBar(mem_percent, 100.0, config.progress_bar_width);
            std::cout << " " << std::fixed << std::setprecision(1) << mem_percent << "% ";
            std::cout << "(" << formatBytes(stats.used_memory_kb * 1024) << "/" 
                      << formatBytes(stats.total_memory_kb * 1024) << ")\n";
        } else {
            std::cout << "MEM: " << std::fixed << std::setprecision(1) << mem_percent << "% ";
            std::cout << "(" << formatBytes(stats.used_memory_kb * 1024) << "/" 
                      << formatBytes(stats.total_memory_kb * 1024) << ")\n";
        }
        
        // Load Average
        if (config.show_load_avg) {
            std::cout << "\033[1m\033[93mLoad: ";
            std::cout << "\033[1;32m";
            std::cout << std::fixed << std::setprecision(2) 
                      << stats.load_avg[0] << " " << stats.load_avg[1] << " " << stats.load_avg[2];
            std::cout << "\033[0m";
        }
        
        std::cout << "  Processes: ";
        std::cout << "\033[1;32m";
        std::cout << stats.process_count;
        std::cout << "\033[0m";
        std::cout << "\n\n";
    }
    
    void printProcesses(const SystemStats& stats) {
        std::cout << "\033[1;34m"; // Синий для заголовка таблицы
        std::cout << "╭─────────┬────────────────────┬─────────┬──────────────┬──────────────╮\n";
        std::cout << "│   PID   │        NAME        │  STATE  │     USER     │    MEMORY    │\n";
        std::cout << "├─────────┼────────────────────┼─────────┼──────────────┼──────────────┤\033[0m\n";
        
        for (const auto& proc : stats.processes) {
            std::cout << "\033[1;34m│ ";
            
            std::cout << std::setw(7) <<  std::right << proc.pid;
            
            std::cout << "\033[1;34m │ ";
            
            // Имя процесса (обрезаем если длинное)
            std::string name = proc.name;
            if (name.length() > 18) {
                name = name.substr(0, 15) + "...";
            }
            
            std::cout << "\033[1;37m";
            std::cout << std::setw(18) << std::left << name;
            std::cout << "\033[0m";
            
            std::cout << "\033[1;34m │ ";
            
            // Состояние с цветом
            if (config.show_process_state) {
                std::string state_color = "\033[1;32m"; // Зеленый по умолчанию
                if (proc.state == "Z") state_color = "\033[1;31m"; // Красный для зомби
                else if (proc.state == "D") state_color = "\033[1;33m"; // Желтый для ожидания

                std::cout << state_color;
                std::cout << std::setw(7) << std::left << proc.state;
                std::cout << "\033[0m";
            }
            
            std::cout << "\033[1;34m │ ";
            
            // Пользователь
            if (config.show_process_user) {
                std::string user = proc.user;
                if (user.length() > 12) {
                    user = user.substr(0, 9) + "...";
                }
                std::cout << "\033[1;36m";
                std::cout << std::setw(12) << std::left << user;
                std::cout << "\033[0m";
            } else {
                std::cout << std::setw(12) << " ";
            }
            
            std::cout << "\033[1;34m │ ";
            
            // Память
            std::cout << "\033[1;35m";
            std::cout << std::setw(12) << std::right << formatBytes(proc.memory_kb * 1024);
            std::cout << "\033[0m\033[1;34m";
            
            std::cout << " │\n";
        }
        
        std::cout << "\033[1;34m╰─────────┴────────────────────┴─────────┴──────────────┴──────────────╯\033[0m\n";
    }
    
private:
    MtopConfig config;
    
    void printProgressBar(double value, double max_value, int width) {
        double percent = value / max_value;
        int filled = static_cast<int>(percent * width);
        
        std::cout << "\033[1;32m["; // Зеленый для прогресс-бара
        for (int i = 0; i < width; ++i) {
            if (i < filled) {
                std::cout << "█";
            } else {
                std::cout << " ";
            }
        }
        std::cout << "]\033[0m";
    }
    
    std::string formatBytes(uint64_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit_index = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unit_index < 4) {
            size /= 1024.0;
            unit_index++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << size << units[unit_index];
        return oss.str();
    }
};

volatile bool running = true;

void signalHandler(int signal) {
    running = false;
}

int main(int argc, char* argv[]) {
    // Парсим конфигурацию
    ConfigParser parser;
    
    // Загружаем конфигурацию по умолчанию
    parser.loadDefaultConfig();
    
    // Парсим аргументы командной строки
    if (!parser.parseCommandLine(argc, argv)) {
        return 0; // Вышли из-за --help или ошибки
    }
    
    MtopConfig config = parser.getConfig();
    
    // Настройка обработки сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    SystemInfo sysInfo(config);
    Display display(config);
    
    std::cout << "\033[1;32mStarting mtop... Press Ctrl+C to exit\033[0m\n";

    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    while (running) {
        system("clear");
        if (config.header) { display.printHeader(); }
        
        sysInfo.updateStats();
        SystemStats stats = sysInfo.getStats();
        
        display.printSystemStats(stats);
        display.printProcesses(stats);
        
        std::cout << "\n\033[1;90mPress Ctrl+C to exit | Update interval: " 
                    << config.update_interval << "s\033[0m" << std::flush;
        
        // Обновляем согласно интервалу из конфигурации
        std::this_thread::sleep_for(std::chrono::seconds(config.update_interval));
    }
    
    std::cout << "\n\033[1;32mGoodbye!\033[0m\n";
    
    return 0;
}