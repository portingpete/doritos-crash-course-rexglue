#pragma once

#include <filesystem>
#include <string>

#include "logging/log.h"

namespace doritos::config {

struct PortConfig {
  std::filesystem::path game_root = "assets\\game";
  std::filesystem::path executable_path = "default.xex_uncrypted.xex";
  std::filesystem::path title_update_path;
  logging::Level log_level = logging::Level::Info;
  std::string graphics_backend = "d3d12";
  std::string audio_backend = "sdl";
  std::string input_backend = "xinput";
  std::string networking_mode = "disabled";
  int window_width = 1280;
  int window_height = 720;
  bool fullscreen = false;
  bool break_on_unimplemented_import = true;
  bool trace_imports = true;
  bool trace_file_io = true;
  bool trace_memory = false;
  bool trace_graphics = true;
  bool trace_audio = true;
  bool trace_network = true;
};

PortConfig LoadPortConfig(const std::filesystem::path& path);

}  // namespace doritos::config



