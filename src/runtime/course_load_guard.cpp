#include "runtime/course_load_guard.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string_view>

#include "doritos_port_config.h"
#include <rex/ppc.h>
#include <rex/system/function_dispatcher.h>

#include "logging/log.h"

namespace {

constexpr uint32_t kTileWorkerStepAddress = 0x824E9940u;
std::atomic<PPCFunc*> g_original_tile_worker_step{};
std::atomic_bool g_logged_install{};
std::atomic_bool g_logged_null_service{};

void Tracef(const char* format, uint32_t self, uint32_t count, uint32_t entries,
            uint32_t work, uint32_t state) {
  char buffer[384]{};
  const int written =
      std::snprintf(buffer, sizeof(buffer), format, self, count, entries, work, state);
  if (written <= 0) {
    return;
  }
  doritos::logging::Write(doritos::logging::Level::Warning, "course-load",
                          std::string_view(buffer, static_cast<size_t>(written)));
}

PPC_FUNC_IMPL(DoritosCourseLoadGuard_824E9940) {
  PPC_FUNC_PROLOGUE();

  const uint32_t self = ctx.r3.u32;
  const uint32_t service = PPC_LOAD_U32_D(self, 4);
  if (service != 0) {
    if (PPCFunc* original = g_original_tile_worker_step.load()) {
      original(ctx, base);
    }
    return;
  }

  const uint32_t count = PPC_LOAD_U32_D(self, 292);
  const uint32_t entries = PPC_LOAD_U32_D(self, 296);
  const uint32_t work = PPC_LOAD_U32_D(self, 392);
  const uint32_t state = PPC_LOAD_U32_D(self, 400);

  if (!g_logged_null_service.exchange(true)) {
    Tracef("guarded 0x824E9940 stale/null service: this=%08X count=%u entries=%08X work=%08X state=%08X; returning pending",
           self, count, entries, work, state);
  }

  ctx.r3.s64 = 0;
}

}  // namespace

namespace doritos::runtime {

void InstallCourseLoadGuard(rex::runtime::FunctionDispatcher* dispatcher) {
  if (!dispatcher) {
    return;
  }

  PPCFunc* original = dispatcher->GetFunction(kTileWorkerStepAddress);
  if (!original || original == DoritosCourseLoadGuard_824E9940) {
    return;
  }

  g_original_tile_worker_step.store(original);
  dispatcher->SetFunction(kTileWorkerStepAddress, DoritosCourseLoadGuard_824E9940);

  if (!g_logged_install.exchange(true)) {
    doritos::logging::Write(
        doritos::logging::Level::Info, "course-load",
        "installed guard for guest function 0x824E9940 stale/null service worker");
  }
}

}  // namespace doritos::runtime
