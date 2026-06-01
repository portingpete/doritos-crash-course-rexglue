#include "diagnostics/guest_capture.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <rex/cvar.h>
#include <rex/graphics/graphics_system.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/ui/presenter.h>

#include "logging/log.h"

REXCVAR_DEFINE_BOOL(doritos_capture_guest_output, false, "Doritos/Verification",
                    "Capture the guest framebuffer to a PNG after a delay");
REXCVAR_DEFINE_STRING(doritos_capture_guest_output_path, "", "Doritos/Verification",
                      "PNG path for doritos_capture_guest_output");
REXCVAR_DEFINE_UINT32(doritos_capture_guest_output_delay_ms, 35000, "Doritos/Verification",
                      "Delay before capturing the guest framebuffer")
    .range(1000, 300000);

namespace {

std::filesystem::path DefaultCapturePath() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &time);
#else
  localtime_r(&time, &tm);
#endif

  std::ostringstream name;
  name << "doritos-guest-output-" << std::put_time(&tm, "%Y%m%d-%H%M%S") << ".bmp";
  return std::filesystem::current_path() / "logs" / "screenshots" / name.str();
}

std::filesystem::path CapturePath() {
  const std::string configured = REXCVAR_GET(doritos_capture_guest_output_path);
  if (!configured.empty()) {
    return configured;
  }
  return DefaultCapturePath();
}

void WriteU16(std::ofstream& out, uint16_t value) {
  out.put(static_cast<char>(value & 0xFFu));
  out.put(static_cast<char>((value >> 8) & 0xFFu));
}

void WriteU32(std::ofstream& out, uint32_t value) {
  out.put(static_cast<char>(value & 0xFFu));
  out.put(static_cast<char>((value >> 8) & 0xFFu));
  out.put(static_cast<char>((value >> 16) & 0xFFu));
  out.put(static_cast<char>((value >> 24) & 0xFFu));
}

bool SaveBmp(const std::filesystem::path& path, const rex::ui::RawImage& image) {
  if (!image.width || !image.height || image.data.empty()) {
    return false;
  }

  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    return false;
  }

  const uint32_t row_size = (image.width * 3u + 3u) & ~3u;
  const uint32_t image_size = row_size * image.height;
  const uint32_t file_size = 14u + 40u + image_size;

  out.put('B');
  out.put('M');
  WriteU32(out, file_size);
  WriteU16(out, 0);
  WriteU16(out, 0);
  WriteU32(out, 54);

  WriteU32(out, 40);
  WriteU32(out, image.width);
  WriteU32(out, image.height);
  WriteU16(out, 1);
  WriteU16(out, 24);
  WriteU32(out, 0);
  WriteU32(out, image_size);
  WriteU32(out, 2835);
  WriteU32(out, 2835);
  WriteU32(out, 0);
  WriteU32(out, 0);

  std::vector<uint8_t> row(row_size);
  for (uint32_t y = image.height; y > 0; --y) {
    const uint8_t* src = image.data.data() + size_t(y - 1) * image.stride;
    for (uint32_t x = 0; x < image.width; ++x) {
      const uint8_t* pixel = src + size_t(x) * 4u;
      row[size_t(x) * 3u + 0u] = pixel[2];
      row[size_t(x) * 3u + 1u] = pixel[1];
      row[size_t(x) * 3u + 2u] = pixel[0];
    }
    out.write(reinterpret_cast<const char*>(row.data()), static_cast<std::streamsize>(row.size()));
  }
  return bool(out);
}

void CaptureGuestOutput(rex::Runtime* runtime) {
  auto* graphics_system =
      runtime ? static_cast<rex::graphics::GraphicsSystem*>(runtime->graphics_system()) : nullptr;
  auto* presenter = graphics_system ? graphics_system->presenter() : nullptr;
  if (!presenter) {
    doritos::logging::Write(doritos::logging::Level::Warning, "diagnostics",
                            "guest output capture skipped: presenter unavailable");
    return;
  }

  rex::ui::RawImage image;
  if (!presenter->CaptureGuestOutput(image) || image.data.empty()) {
    doritos::logging::Write(doritos::logging::Level::Warning, "diagnostics",
                            "guest output capture failed");
    return;
  }

  const auto path = CapturePath();
  if (!SaveBmp(path, image)) {
    doritos::logging::Write(doritos::logging::Level::Warning, "diagnostics",
                            "guest output capture save failed: " + path.string());
    return;
  }

  doritos::logging::Write(doritos::logging::Level::Info, "diagnostics",
                          "guest output captured: " + path.string());
}

}  // namespace

namespace doritos::diagnostics {

void StartGuestOutputCapture(rex::Runtime* runtime, std::atomic_bool& stop_requested,
                             std::thread& worker) {
  if (!REXCVAR_GET(doritos_capture_guest_output) || worker.joinable()) {
    return;
  }

  const uint32_t delay_ms = REXCVAR_GET(doritos_capture_guest_output_delay_ms);
  doritos::logging::Write(doritos::logging::Level::Info, "diagnostics",
                          "guest output capture armed");
  worker = std::thread([runtime, &stop_requested, delay_ms]() {
    uint32_t waited_ms = 0;
    while (!stop_requested.load() && waited_ms < delay_ms) {
      constexpr uint32_t kStepMs = 100;
      std::this_thread::sleep_for(std::chrono::milliseconds(kStepMs));
      waited_ms += kStepMs;
    }
    if (!stop_requested.load()) {
      CaptureGuestOutput(runtime);
    }
  });
}

}  // namespace doritos::diagnostics
