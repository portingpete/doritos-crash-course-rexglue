#include "audio/backend.h"

namespace doritos::audio {

std::string DescribeAudioBackend(std::string backend) {
  if (backend == "sdl") {
    return "SDL audio through local ReXGlue";
  }
  return "no-op audio backend";
}

}  // namespace doritos::audio



