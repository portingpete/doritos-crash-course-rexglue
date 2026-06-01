#include "graphics/backend.h"

namespace doritos::graphics {

std::string DescribeGraphicsBackend(std::string backend) {
  if (backend == "d3d12") {
    return "D3D12 via local ReXGlue graphics backend";
  }
  if (backend == "vulkan") {
    return "Vulkan via local ReXGlue graphics backend";
  }
  return "graphics disabled for diagnostics";
}

}  // namespace doritos::graphics



