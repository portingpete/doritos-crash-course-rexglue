#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace doritos::logging {

enum class Level {
  Error = 0,
  Warning,
  Info,
  Debug,
  Trace,
};

struct Config {
  Level level = Level::Info;
  std::filesystem::path file;
  bool mirror_to_stderr = true;
};

Level ParseLevel(std::string_view value);
std::string_view ToString(Level level);
void Initialize(const Config& config);
void Shutdown();
void Write(Level level, std::string_view category, std::string_view message);

}  // namespace doritos::logging



