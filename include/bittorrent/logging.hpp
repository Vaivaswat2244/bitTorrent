#pragma once

#include <string>
#include <sstream>

namespace btt {

enum class LogLevel { TRACE=0, DEBUG, INFO, WARN, ERROR };

void init_logging(LogLevel level = LogLevel::INFO, const std::string& logfile = "");
void log(LogLevel level, const std::string& message);

} // namespace btt

// Stream-style helpers
#define LOG_TRACE(...) do { std::ostringstream _oss; _oss << __VA_ARGS__; btt::log(btt::LogLevel::TRACE, _oss.str()); } while(0)
#define LOG_DEBUG(...) do { std::ostringstream _oss; _oss << __VA_ARGS__; btt::log(btt::LogLevel::DEBUG, _oss.str()); } while(0)
#define LOG_INFO(...)  do { std::ostringstream _oss; _oss << __VA_ARGS__; btt::log(btt::LogLevel::INFO,  _oss.str()); } while(0)
#define LOG_WARN(...)  do { std::ostringstream _oss; _oss << __VA_ARGS__; btt::log(btt::LogLevel::WARN,  _oss.str()); } while(0)
#define LOG_ERROR(...) do { std::ostringstream _oss; _oss << __VA_ARGS__; btt::log(btt::LogLevel::ERROR, _oss.str()); } while(0)
