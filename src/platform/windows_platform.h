#pragma once

#include <string>

namespace doritos::platform {

struct HostPlatformInfo {
  std::string os;
  std::string architecture;
};

HostPlatformInfo CollectHostPlatformInfo();

}  // namespace doritos::platform



