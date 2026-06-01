#include "runtime/xam_input_overrides.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>

#include <rex/chrono/clock.h>
#include <rex/cvar.h>
#include <rex/input/input.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xtypes.h>
#include <rex/ui/virtual_key.h>

#include "logging/log.h"

REXCVAR_DEFINE_STRING(doritos_scripted_input, "", "Doritos/Verification",
                      "Scripted input sequence for deterministic port verification")
    .allowed({"", "start_course"});

namespace {

constexpr uint32_t kXInputFlagGamepad = 0x00000001u;
constexpr uint32_t kXInputFlagAnyUser = 1u << 30;
constexpr uint16_t kXInputGamepadA = rex::input::X_INPUT_GAMEPAD_A;
constexpr uint16_t kXInputPadA = static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadA);
constexpr rex::X_RESULT kXErrorSuccess = 0x00000000u;
constexpr rex::X_RESULT kXErrorBadArguments = 0x00000057u;
constexpr rex::X_RESULT kXErrorDeviceNotConnected = 0x0000048Fu;

uint32_t ActualUserIndex(uint32_t user_index, uint32_t flags) {
  if ((user_index & 0xFFu) == 0xFFu || (flags & kXInputFlagAnyUser)) {
    return 0;
  }
  return user_index;
}

rex::input::InputSystem* InputSystem() {
  auto* kernel_state = REX_KERNEL_STATE();
  if (!kernel_state || !kernel_state->emulator()) {
    return nullptr;
  }
  return static_cast<rex::input::InputSystem*>(kernel_state->emulator()->input_system());
}

bool IsStartCourseScriptEnabled() {
  return REXCVAR_GET(doritos_scripted_input) == "start_course";
}

uint64_t ScriptElapsedMillis() {
  static std::atomic_uint64_t script_start_ms = 0;
  const uint64_t now = rex::chrono::Clock::QueryHostUptimeMillis();
  uint64_t expected = 0;
  script_start_ms.compare_exchange_strong(expected, now);
  return now - script_start_ms.load();
}

uint16_t ScriptedButtonsForElapsedMs(uint64_t elapsed_ms) {
  constexpr uint64_t kFirstPulseMs = 1500;
  constexpr uint64_t kLastPulseMs = 120000;
  constexpr uint64_t kPulsePeriodMs = 4500;
  constexpr uint64_t kPulseHoldMs = 1200;

  if (elapsed_ms < kFirstPulseMs || elapsed_ms > kLastPulseMs) {
    return 0;
  }
  if (((elapsed_ms - kFirstPulseMs) % kPulsePeriodMs) < kPulseHoldMs) {
    return kXInputGamepadA;
  }
  return 0;
}

void LogScriptedInputOnce() {
  static std::atomic_bool logged = false;
  if (!logged.exchange(true)) {
    REXKRNL_INFO("Doritos scripted input active: start_course");
    doritos::logging::Write(doritos::logging::Level::Info, "input",
                            "Doritos scripted input active: start_course");
  }
}

void ApplyScriptedInput(rex::input::X_INPUT_STATE& state) {
  if (!IsStartCourseScriptEnabled()) {
    return;
  }

  LogScriptedInputOnce();
  const uint16_t scripted_buttons = ScriptedButtonsForElapsedMs(ScriptElapsedMillis());
  const auto merged_buttons = static_cast<uint16_t>(state.gamepad.buttons) | scripted_buttons;
  state.gamepad.buttons = merged_buttons;

  static std::atomic_uint32_t packet_number = 0;
  const uint32_t next_packet =
      std::max(++packet_number, static_cast<uint32_t>(state.packet_number) + 1);
  packet_number.store(next_packet);
  state.packet_number = next_packet;
}

void FillScriptedAKeystroke(rex::input::X_INPUT_KEYSTROKE& keystroke, uint16_t flags) {
  std::memset(&keystroke, 0, sizeof(keystroke));
  keystroke.virtual_key = kXInputPadA;
  keystroke.flags = flags;
  keystroke.user_index = 0;
}

bool TryConsumeScriptedKeystroke(rex::input::X_INPUT_KEYSTROKE& keystroke) {
  if (!IsStartCourseScriptEnabled()) {
    return false;
  }

  LogScriptedInputOnce();
  static std::atomic_bool key_down = false;
  const bool active =
      (ScriptedButtonsForElapsedMs(ScriptElapsedMillis()) & kXInputGamepadA) != 0;
  const bool was_down = key_down.load();

  if (active && !was_down) {
    key_down.store(true);
    FillScriptedAKeystroke(keystroke, rex::input::X_INPUT_KEYSTROKE_KEYDOWN);
    return true;
  }

  if (!active && was_down) {
    key_down.store(false);
    FillScriptedAKeystroke(keystroke, rex::input::X_INPUT_KEYSTROKE_KEYUP);
    return true;
  }

  return false;
}

void InstallOverride(const char* name, PPCFunc* func) {
  auto& registry = rex::GetPPCFuncRegistry();
  std::erase_if(registry, [name](const auto& entry) { return std::strcmp(entry.first, name) == 0; });
  registry.insert(registry.begin(), {name, func});
}

}  // namespace

extern "C" PPC_FUNC(doritos_XamInputGetState) {
  const uint32_t user_index = ctx.r3.u32;
  const uint32_t flags = ctx.r4.u32;
  const uint32_t state_ptr = ctx.r5.u32;

  if ((flags & 0xFFu) && (flags & kXInputFlagGamepad) == 0) {
    ctx.r3.u64 = kXErrorDeviceNotConnected;
    return;
  }

  auto* input_system = InputSystem();
  if (!input_system) {
    ctx.r3.u64 = kXErrorDeviceNotConnected;
    return;
  }

  auto* out_state = state_ptr != 0
                        ? reinterpret_cast<rex::input::X_INPUT_STATE*>(PPC_RAW_ADDR(state_ptr))
                        : nullptr;
  const uint32_t actual_user_index = ActualUserIndex(user_index, flags);
  const rex::X_RESULT result = input_system->GetState(actual_user_index, out_state);
  if (result == kXErrorSuccess && out_state && actual_user_index == 0) {
    ApplyScriptedInput(*out_state);
  }

  ctx.r3.u64 = result;
}

extern "C" PPC_FUNC(doritos_XamInputGetKeystroke) {
  const uint32_t user_index = ctx.r3.u32;
  const uint32_t flags = ctx.r4.u32;
  const uint32_t keystroke_ptr = ctx.r5.u32;

  if (!keystroke_ptr) {
    ctx.r3.u64 = kXErrorBadArguments;
    return;
  }

  if ((flags & 0xFFu) && (flags & kXInputFlagGamepad) == 0) {
    ctx.r3.u64 = kXErrorDeviceNotConnected;
    return;
  }

  auto* input_system = InputSystem();
  if (!input_system) {
    ctx.r3.u64 = kXErrorDeviceNotConnected;
    return;
  }

  auto* out_keystroke =
      reinterpret_cast<rex::input::X_INPUT_KEYSTROKE*>(PPC_RAW_ADDR(keystroke_ptr));
  const uint32_t actual_user_index = ActualUserIndex(user_index, flags);
  const rex::X_RESULT result = input_system->GetKeystroke(actual_user_index, flags, out_keystroke);
  if (result == kXErrorSuccess) {
    ctx.r3.u64 = result;
    return;
  }

  if (actual_user_index == 0 && TryConsumeScriptedKeystroke(*out_keystroke)) {
    ctx.r3.u64 = kXErrorSuccess;
    return;
  }

  ctx.r3.u64 = result;
}

extern "C" PPC_FUNC(doritos_XamInputGetKeystrokeEx) {
  const uint32_t user_index_ptr = ctx.r3.u32;
  const uint32_t flags = ctx.r4.u32;
  const uint32_t keystroke_ptr = ctx.r5.u32;

  if (!user_index_ptr || !keystroke_ptr) {
    ctx.r3.u64 = kXErrorBadArguments;
    return;
  }

  if ((flags & 0xFFu) && (flags & kXInputFlagGamepad) == 0) {
    ctx.r3.u64 = kXErrorDeviceNotConnected;
    return;
  }

  auto* input_system = InputSystem();
  if (!input_system) {
    ctx.r3.u64 = kXErrorDeviceNotConnected;
    return;
  }

  auto* user_index_out = reinterpret_cast<rex::be<uint32_t>*>(PPC_RAW_ADDR(user_index_ptr));
  auto* out_keystroke =
      reinterpret_cast<rex::input::X_INPUT_KEYSTROKE*>(PPC_RAW_ADDR(keystroke_ptr));
  const uint32_t actual_user_index = ActualUserIndex(static_cast<uint32_t>(*user_index_out), flags);
  const rex::X_RESULT result = input_system->GetKeystroke(actual_user_index, flags, out_keystroke);
  if (result == kXErrorSuccess) {
    *user_index_out = out_keystroke->user_index;
    ctx.r3.u64 = result;
    return;
  }

  if (actual_user_index == 0 && TryConsumeScriptedKeystroke(*out_keystroke)) {
    *user_index_out = out_keystroke->user_index;
    ctx.r3.u64 = kXErrorSuccess;
    return;
  }

  ctx.r3.u64 = result;
}

namespace doritos::runtime {

void InstallXamInputOverrides() {
  InstallOverride("__imp__XamInputGetState", &doritos_XamInputGetState);
  InstallOverride("__imp__XamInputGetKeystroke", &doritos_XamInputGetKeystroke);
  InstallOverride("__imp__XamInputGetKeystrokeEx", &doritos_XamInputGetKeystrokeEx);
}

}  // namespace doritos::runtime
