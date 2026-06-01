#include "platform/windows_platform.h"

namespace doritos::platform {

HostPlatformInfo CollectHostPlatformInfo() {
  HostPlatformInfo info;
#if defined(_WIN32)
  info.os = "Windows";
#else
  info.os = "non-Windows";
#endif
#if defined(_M_X64) || defined(__x86_64__)
  info.architecture = "x64";
#elif defined(_M_ARM64) || defined(__aarch64__)
  info.architecture = "arm64";
#else
  info.architecture = "unknown";
#endif
  return info;
}

}  // namespace doritos::platform



