#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "config/port_config.h"

namespace doritos::vfs {

struct Mount {
  std::string guest;
  std::filesystem::path host;
  bool read_only = true;
};

std::vector<Mount> BuildDefaultMounts(const config::PortConfig& config);
std::string DescribeMounts(const std::vector<Mount>& mounts);

}  // namespace doritos::vfs



