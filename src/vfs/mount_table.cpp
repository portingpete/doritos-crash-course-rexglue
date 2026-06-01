#include "vfs/mount_table.h"

#include <sstream>

namespace doritos::vfs {

std::vector<Mount> BuildDefaultMounts(const config::PortConfig& config) {
  std::vector<Mount> mounts = {
      {"game:/", config.game_root, true},
      {"d:/", config.game_root, true},
      {"save:/", "save", false},
      {"cache:/", "cache", false},
      {"user:/", "user", false},
  };
  if (!config.title_update_path.empty()) {
    mounts.push_back({"update:/", config.title_update_path, true});
  }
  return mounts;
}

std::string DescribeMounts(const std::vector<Mount>& mounts) {
  std::ostringstream out;
  for (const auto& mount : mounts) {
    out << mount.guest << " -> " << mount.host.string()
        << (mount.read_only ? " [read-only]" : " [writable]") << '\n';
  }
  return out.str();
}

}  // namespace doritos::vfs



