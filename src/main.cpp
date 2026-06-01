#include <filesystem>
#include <memory>
#include <sstream>
#include <atomic>
#include <thread>

#include <rex/cvar.h>
#include <rex/ui/windowed_app.h>

#include "audio/backend.h"
#include "config/port_config.h"
#include "diagnostics/crash_dumps.h"
#include "diagnostics/exception_trace.h"
#include "diagnostics/guest_capture.h"
#include "graphics/backend.h"
#include "input/mapping.h"
#include "logging/log.h"
#include "net/network_mode.h"
#include "platform/windows_platform.h"
#include "runtime/course_load_guard.h"
#include "runtime/runtime_plan.h"
#include "runtime/xam_input_overrides.h"
#include "runtime/xam_profile_overrides.h"
#include "vfs/mount_table.h"

#if DORITOS_HAS_REXGLUE_GENERATED
#include <rex/rex_app.h>
#include "doritos_port_config.h"
#include "doritos_port_init.h"
#endif

namespace {

std::filesystem::path ProjectConfigPath() {
  return std::filesystem::current_path() / "config" / "doritos-port.toml";
}

void InitializeScaffoldLogging(const doritos::config::PortConfig& config) {
  doritos::logging::Config log_config;
  log_config.level = config.log_level;
  log_config.file = std::filesystem::current_path() / "logs" / "runtime" / "doritos-scaffold.log";
  const auto rex_log_level = rex::cvar::GetFlagByName("log_level");
  if (!rex_log_level.empty()) {
    log_config.level = doritos::logging::ParseLevel(rex_log_level);
  }
  const auto rex_log_file = rex::cvar::GetFlagByName("log_file");
  if (!rex_log_file.empty()) {
    log_config.file = rex_log_file;
  }
  doritos::logging::Initialize(log_config);
}

void LogScaffoldState(const doritos::config::PortConfig& config) {
  auto platform = doritos::platform::CollectHostPlatformInfo();
  doritos::logging::Write(doritos::logging::Level::Info, "boot",
                         "platform=" + platform.os + " arch=" + platform.architecture);
  doritos::logging::Write(doritos::logging::Level::Info, "boot",
                         doritos::runtime::CurrentRuntimeMilestone());
  doritos::logging::Write(doritos::logging::Level::Info, "graphics",
                         doritos::graphics::DescribeGraphicsBackend(config.graphics_backend));
  doritos::logging::Write(doritos::logging::Level::Info, "audio",
                         doritos::audio::DescribeAudioBackend(config.audio_backend));
  doritos::logging::Write(doritos::logging::Level::Info, "input",
                         doritos::input::DefaultInputMapping());
  doritos::logging::Write(doritos::logging::Level::Info, "networking",
                         doritos::net::DescribeNetworkingMode(config.networking_mode));
  doritos::logging::Write(doritos::logging::Level::Info, "filesystem",
                         "\n" + doritos::vfs::DescribeMounts(doritos::vfs::BuildDefaultMounts(config)));
}

#if DORITOS_HAS_REXGLUE_GENERATED

class DORITOSPortApp final : public rex::ReXApp {
 public:
  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<DORITOSPortApp>(new DORITOSPortApp(ctx));
  }

 private:
  explicit DORITOSPortApp(rex::ui::WindowedAppContext& ctx)
      : rex::ReXApp(ctx, "doritos_port", PPCImageConfig) {}

  void OnConfigurePaths(rex::PathConfig& paths) override {
    config_ = doritos::config::LoadPortConfig(ProjectConfigPath());
    if (auto game_directory = GetArgument("game_directory")) {
      config_.game_root = *game_directory;
    }
    paths.game_data_root = config_.game_root;
    paths.user_data_root = "user";
    if (!config_.title_update_path.empty()) {
      paths.update_data_root = config_.title_update_path;
    }
    InitializeScaffoldLogging(config_);
    doritos::diagnostics::InstallCrashDumpHandler("logs/runtime/dumps");
    doritos::diagnostics::InstallFirstChanceExceptionTrace();
    LogScaffoldState(config_);
    doritos::runtime::InstallXamInputOverrides();
    doritos::runtime::InstallXamProfileOverrides();
  }

  void OnLoadXexImage(std::string& xex_image) override {
    xex_image = "game:\\" + config_.executable_path.filename().string();
  }

  void OnPostSetup() override {
    doritos::logging::Write(doritos::logging::Level::Info, "boot",
                           "ReXGlue runtime setup completed; launching guest module next");
    doritos::diagnostics::StartGuestOutputCapture(runtime(), guest_capture_stop_,
                                                 guest_capture_thread_);
  }

  void OnPreLaunchModule() override {
    doritos::runtime::InstallCourseLoadGuard(runtime()->function_dispatcher());
  }

  void OnShutdown() override {
    guest_capture_stop_.store(true);
    if (guest_capture_thread_.joinable()) {
      guest_capture_thread_.join();
    }
    doritos::logging::Write(doritos::logging::Level::Info, "boot", "shutdown");
    doritos::logging::Shutdown();
  }

  doritos::config::PortConfig config_;
  std::atomic_bool guest_capture_stop_{false};
  std::thread guest_capture_thread_;
};

#else

class DORITOSPortApp final : public rex::ui::WindowedApp {
 public:
  explicit DORITOSPortApp(rex::ui::WindowedAppContext& ctx)
      : rex::ui::WindowedApp(ctx, "doritos_port", "[game_directory]") {
    AddPositionalOption("game_directory");
  }

  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx) {
    return std::make_unique<DORITOSPortApp>(ctx);
  }

  bool OnInitialize() override {
    auto config = doritos::config::LoadPortConfig(ProjectConfigPath());
    if (auto game_directory = GetArgument("game_directory")) {
      config.game_root = *game_directory;
    }
    InitializeScaffoldLogging(config);
    doritos::diagnostics::InstallCrashDumpHandler("logs/runtime/dumps");
    doritos::diagnostics::InstallFirstChanceExceptionTrace();
    LogScaffoldState(config);
    doritos::logging::Write(
        doritos::logging::Level::Warning, "rexglue",
        "Generated ReXGlue sources are absent; run tools/codegen.ps1 before expecting boot");
    app_context().CallInUIThreadDeferred([this]() { app_context().QuitFromUIThread(); });
    return true;
  }

 protected:
  void OnDestroy() override {
    doritos::logging::Write(doritos::logging::Level::Info, "boot", "scaffold shutdown");
    doritos::logging::Shutdown();
  }
};

#endif

}  // namespace

REX_DEFINE_APP(doritos_port, DORITOSPortApp::Create)


