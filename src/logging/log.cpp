#include "logging/log.h"

#include <chrono>
#include <cctype>
#include <fstream>
#include <iostream>
#include <mutex>

namespace doritos::logging {
namespace {

std::mutex g_mutex;
Config g_config;
std::ofstream g_file;

std::string Lower(std::string_view value) {
  std::string out(value);
  for (char& c : out) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return out;
}

bool Enabled(Level level) {
  return static_cast<int>(level) <= static_cast<int>(g_config.level);
}

std::string Timestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &time);
#else
  localtime_r(&time, &tm);
#endif
  char buffer[32]{};
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
  return buffer;
}

}  // namespace

Level ParseLevel(std::string_view value) {
  const auto lower = Lower(value);
  if (lower == "error") return Level::Error;
  if (lower == "warning" || lower == "warn") return Level::Warning;
  if (lower == "debug") return Level::Debug;
  if (lower == "trace") return Level::Trace;
  return Level::Info;
}

std::string_view ToString(Level level) {
  switch (level) {
    case Level::Error:
      return "error";
    case Level::Warning:
      return "warning";
    case Level::Info:
      return "info";
    case Level::Debug:
      return "debug";
    case Level::Trace:
      return "trace";
  }
  return "info";
}

void Initialize(const Config& config) {
  std::lock_guard lock(g_mutex);
  g_config = config;
  if (!g_config.file.empty()) {
    std::filesystem::create_directories(g_config.file.parent_path());
    g_file.open(g_config.file, std::ios::out | std::ios::app);
  }
}

void Shutdown() {
  std::lock_guard lock(g_mutex);
  if (g_file.is_open()) {
    g_file.flush();
    g_file.close();
  }
}

void Write(Level level, std::string_view category, std::string_view message) {
  if (!Enabled(level)) {
    return;
  }

  std::lock_guard lock(g_mutex);
  const std::string line =
      Timestamp() + " [" + std::string(ToString(level)) + "] [" + std::string(category) + "] " +
      std::string(message) + "\n";
  if (g_config.mirror_to_stderr) {
    std::cerr << line;
  }
  if (g_file.is_open()) {
    g_file << line;
    g_file.flush();
  }
}

}  // namespace doritos::logging



