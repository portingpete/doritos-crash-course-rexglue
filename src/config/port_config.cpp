#include "config/port_config.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace doritos::config {
namespace {

std::string Trim(std::string value) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

std::string Unquote(std::string value) {
  value = Trim(std::move(value));
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    return value.substr(1, value.size() - 2);
  }
  return value;
}

bool ParseBool(const std::unordered_map<std::string, std::string>& values, const char* key,
               bool fallback) {
  auto it = values.find(key);
  if (it == values.end()) {
    return fallback;
  }
  auto value = it->second;
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value == "true" || value == "1" || value == "yes" || value == "on";
}

int ParseInt(const std::unordered_map<std::string, std::string>& values, const char* key,
             int fallback) {
  auto it = values.find(key);
  if (it == values.end()) {
    return fallback;
  }
  return std::stoi(it->second);
}

std::string ParseString(const std::unordered_map<std::string, std::string>& values, const char* key,
                        std::string fallback) {
  auto it = values.find(key);
  return it == values.end() ? std::move(fallback) : it->second;
}

}  // namespace

PortConfig LoadPortConfig(const std::filesystem::path& path) {
  PortConfig config;
  std::ifstream file(path);
  if (!file) {
    return config;
  }

  std::unordered_map<std::string, std::string> values;
  std::string line;
  while (std::getline(file, line)) {
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line = line.substr(0, comment);
    }
    const auto equals = line.find('=');
    if (equals == std::string::npos) {
      continue;
    }
    auto key = Trim(line.substr(0, equals));
    auto value = Unquote(line.substr(equals + 1));
    if (!key.empty()) {
      values.emplace(std::move(key), std::move(value));
    }
  }

  config.game_root = ParseString(values, "game_root", config.game_root.string());
  config.executable_path = ParseString(values, "executable_path", config.executable_path.string());
  config.title_update_path = ParseString(values, "title_update_path", "");
  config.log_level = logging::ParseLevel(ParseString(values, "log_level", "info"));
  config.graphics_backend = ParseString(values, "graphics_backend", config.graphics_backend);
  config.audio_backend = ParseString(values, "audio_backend", config.audio_backend);
  config.input_backend = ParseString(values, "input_backend", config.input_backend);
  config.networking_mode = ParseString(values, "networking_mode", config.networking_mode);
  config.window_width = ParseInt(values, "window_width", config.window_width);
  config.window_height = ParseInt(values, "window_height", config.window_height);
  config.fullscreen = ParseBool(values, "fullscreen", config.fullscreen);
  config.break_on_unimplemented_import =
      ParseBool(values, "break_on_unimplemented_import", config.break_on_unimplemented_import);
  config.trace_imports = ParseBool(values, "trace_imports", config.trace_imports);
  config.trace_file_io = ParseBool(values, "trace_file_io", config.trace_file_io);
  config.trace_memory = ParseBool(values, "trace_memory", config.trace_memory);
  config.trace_graphics = ParseBool(values, "trace_graphics", config.trace_graphics);
  config.trace_audio = ParseBool(values, "trace_audio", config.trace_audio);
  config.trace_network = ParseBool(values, "trace_network", config.trace_network);

  return config;
}

}  // namespace doritos::config



