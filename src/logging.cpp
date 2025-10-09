#include "bittorrent/logging.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <ctime>

namespace btt {

static LogLevel g_level = LogLevel::INFO;
static std::mutex g_log_mutex;
static std::ofstream g_log_file;

void init_logging(LogLevel level, const std::string& logfile) {
    std::lock_guard<std::mutex> lk(g_log_mutex);
    g_level = level;
    if (!logfile.empty()) {
        g_log_file.open(logfile, std::ios::app);
    }
}

static const char* level_to_string(LogLevel l) {
    switch (l) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

void log(LogLevel level, const std::string& message) {
    if (level < g_level) return;
    std::lock_guard<std::mutex> lk(g_log_mutex);

    // timestamp
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    char timebuf[64];
    std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);

    std::ostringstream out;
    out << "[" << timebuf << "] [" << level_to_string(level) << "] " << message << '\n';

    if (g_log_file.is_open()) {
        g_log_file << out.str();
        g_log_file.flush();
    } else {
        std::cerr << out.str();
    }
}

} // namespace btt
