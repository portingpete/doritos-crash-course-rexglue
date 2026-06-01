#include "runtime/xam_profile_overrides.h"

#include <atomic>
#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xtypes.h>

#include "logging/log.h"

namespace {

constexpr uint32_t kLiveSigninState = 2;
constexpr uint32_t kSyntheticSigninInfoFlags = 0x00000002u;
constexpr uint64_t kSyntheticXuid = 0xB13EBABEBABEBABEull;
constexpr uint32_t kXErrorSuccess = 0x00000000u;
constexpr uint32_t kXErrorNoSuchUser = 0x00000525u;
constexpr uint32_t kXEInvalidArg = 0x80070057u;
constexpr uint32_t kXENoSuchUser = 0x80070525u;
constexpr uint32_t kXESuccess = 0x00000000u;
constexpr std::string_view kSyntheticProfileName = "User";

bool IsPrimaryUser(uint32_t user_index) {
  return user_index == 0;
}

void WriteSyntheticSigninInfo(uint8_t* base, uint32_t info_ptr) {
  auto* info = PPC_RAW_ADDR(info_ptr);
  std::memset(info, 0, 40);
  PPC_STORE_U64(info_ptr, kSyntheticXuid);
  PPC_STORE_U32(info_ptr + 8, kSyntheticSigninInfoFlags);
  PPC_STORE_U32(info_ptr + 12, kLiveSigninState);

  auto* name = reinterpret_cast<char*>(PPC_RAW_ADDR(info_ptr + 24));
  const size_t bytes = std::min(kSyntheticProfileName.size(), size_t{15});
  std::memcpy(name, kSyntheticProfileName.data(), bytes);
}

void InstallOverride(const char* name, PPCFunc* func) {
  auto& registry = rex::GetPPCFuncRegistry();
  std::erase_if(registry, [name](const auto& entry) { return std::strcmp(entry.first, name) == 0; });
  registry.insert(registry.begin(), {name, func});
}

void LogOnce(std::atomic_bool& logged, const char* message) {
  if (!logged.exchange(true)) {
    REXKRNL_INFO("Doritos profile override: {}", message);
    const std::string log_message = std::string("Doritos profile override: ") + message;
    doritos::logging::Write(doritos::logging::Level::Info, "profile",
                           log_message);
  }
}

}  // namespace

extern "C" PPC_FUNC(doritos_XamUserGetSigninState) {
  static std::atomic_bool logged = false;
  LogOnce(logged, "XamUserGetSigninState -> Live signed in");
  const uint32_t user_index = ctx.r3.u32;
  ctx.r3.u64 = IsPrimaryUser(user_index) ? kLiveSigninState : 0;
}

extern "C" PPC_FUNC(doritos_XamUserGetSigninInfo) {
  static std::atomic_bool logged = false;
  LogOnce(logged, "XamUserGetSigninInfo -> synthetic profile");
  const uint32_t user_index = ctx.r3.u32;
  const uint32_t info_ptr = ctx.r5.u32;

  if (info_ptr == 0) {
    ctx.r3.u64 = kXEInvalidArg;
    return;
  }
  if (!IsPrimaryUser(user_index)) {
    ctx.r3.u64 = kXENoSuchUser;
    return;
  }

  WriteSyntheticSigninInfo(base, info_ptr);
  ctx.r3.u64 = kXESuccess;
}

extern "C" PPC_FUNC(doritos_XamUserCheckPrivilege) {
  static std::atomic_bool logged = false;
  LogOnce(logged, "XamUserCheckPrivilege -> allowed");
  const uint32_t user_index = ctx.r3.u32;
  if (user_index != 0xFF && !IsPrimaryUser(user_index)) {
    ctx.r3.u64 = kXErrorNoSuchUser;
    return;
  }

  if (ctx.r5.u32 != 0) {
    PPC_STORE_U32(ctx.r5.u32, 1);
  }
  ctx.r3.u64 = kXErrorSuccess;
}

extern "C" PPC_FUNC(doritos_XamShowSigninUI) {
  static std::atomic_bool logged = false;
  LogOnce(logged, "XamShowSigninUI -> no modal");
  if (auto* kernel_state = REX_KERNEL_STATE()) {
    kernel_state->BroadcastNotification(0x0000000A, 1);
    kernel_state->BroadcastNotification(0x00000009, 0);
  }
  ctx.r3.u64 = kXErrorSuccess;
}

namespace doritos::runtime {

void InstallXamProfileOverrides() {
  InstallOverride("__imp__XamUserGetSigninState", &doritos_XamUserGetSigninState);
  InstallOverride("__imp__XamUserGetSigninInfo", &doritos_XamUserGetSigninInfo);
  InstallOverride("__imp__XamUserCheckPrivilege", &doritos_XamUserCheckPrivilege);
  InstallOverride("__imp__XamShowSigninUI", &doritos_XamShowSigninUI);
  REXKRNL_INFO("Doritos profile override installed: user 0 reports Live sign-in and privileges");
  doritos::logging::Write(doritos::logging::Level::Info, "profile",
                         "Doritos profile override installed: user 0 reports Live sign-in and privileges");
}

}  // namespace doritos::runtime
