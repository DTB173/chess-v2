#pragma once
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>

class Logger {
    std::ofstream file;
public:
    Logger() {
        std::string log_path = "F:/c++ projects/chess_logs/engine_debug.log";

        file.open(log_path, std::ios::app);
        if (file.is_open()) {
            log("=== ENGINE STARTED ===");
        }
        else {
            // Fallback if absolute path fails
            file.open("C:/temp/engine_debug.log", std::ios::app);
        }
    }

    ~Logger() {
        if (file.is_open()) log("=== ENGINE SHUTDOWN ===");
    }

    void log(const std::string& message) {
        if (!file.is_open()) return;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time);

        file << std::put_time(&tm, "%H:%M:%S")
            << " | " << message << std::endl;
        file.flush();
    }
};

inline Logger engine_log;
