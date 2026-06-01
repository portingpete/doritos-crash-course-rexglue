#include "net/network_mode.h"

namespace doritos::net {

bool NetworkingEnabled(std::string mode) {
  return mode == "lan" || mode == "service";
}

std::string DescribeNetworkingMode(std::string mode) {
  if (mode == "disabled") {
    return "networking disabled; online calls must log and fail safely";
  }
  if (mode == "stubbed") {
    return "networking stubbed with visible call logging";
  }
  if (mode == "lan") {
    return "LAN replacement mode";
  }
  if (mode == "service") {
    return "private-service mode";
  }
  return "unknown networking mode";
}

}  // namespace doritos::net



