#include "runtime/xam_profile_overrides.h"

#include <atomic>
#include <algorithm>
#include <cstdio>
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
// XUSER_INFO_FLAG_LIVE_ENABLED; keep the XUSER_INFO_FLAG_GUEST bit clear.
constexpr uint32_t kSyntheticSigninInfoFlags = 0x00000001u;
constexpr uint64_t kSyntheticXuid = 0xB13EBABEBABEBABEull;
constexpr uint32_t kXErrorSuccess = 0x00000000u;
constexpr uint32_t kXErrorNoSuchUser = 0x00000525u;
constexpr uint32_t kXEInvalidArg = 0x80070057u;
constexpr uint32_t kXENoSuchUser = 0x80070525u;
constexpr uint32_t kXESuccess = 0x00000000u;
constexpr std::string_view kSyntheticProfileName = "User";
constexpr uint32_t kMaxTraceCallsPerImport = 96;

bool IsPrimaryUser(uint32_t user_index) {
  return user_index == 0;
}

bool ShouldAllowPrivilege(uint32_t privilege_mask) {
  // Doritos' start-course gate asks for 0xFE. Other masks observed after the
  // profile bypass activate optional online/social paths that ReXGlue does not
  // currently emulate faithfully.
  return privilege_mask == 0x000000FEu;
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

bool ShouldTrace(std::atomic<uint32_t>& count) {
  return count.fetch_add(1) < kMaxTraceCallsPerImport;
}

template <typename... Args>
void Tracef(const char* format, Args... args) {
  char buffer[1024]{};
  const int written = std::snprintf(buffer, sizeof(buffer), format, args...);
  if (written <= 0) {
    return;
  }
  doritos::logging::Write(doritos::logging::Level::Info, "xam",
                         std::string_view(buffer, std::min<size_t>(written, sizeof(buffer) - 1)));
}

std::string ReadGuestCString(uint8_t* base, uint32_t ptr, size_t max_len = 64) {
  if (ptr == 0) {
    return "<null>";
  }

  const auto* value = reinterpret_cast<const char*>(PPC_RAW_ADDR(ptr));
  std::string output;
  output.reserve(max_len);
  for (size_t i = 0; i < max_len && value[i] != '\0'; ++i) {
    const unsigned char ch = static_cast<unsigned char>(value[i]);
    output.push_back(ch >= 0x20 && ch < 0x7F ? static_cast<char>(ch) : '.');
  }
  return output;
}

std::string ReadGuestFixedCString(uint8_t* base, uint32_t ptr, size_t max_len) {
  if (ptr == 0) {
    return "<null>";
  }

  const auto* value = reinterpret_cast<const char*>(PPC_RAW_ADDR(ptr));
  std::string output;
  output.reserve(max_len);
  for (size_t i = 0; i < max_len && value[i] != '\0'; ++i) {
    const unsigned char ch = static_cast<unsigned char>(value[i]);
    output.push_back(ch >= 0x20 && ch < 0x7F ? static_cast<char>(ch) : '.');
  }
  return output;
}

void AppendHex(std::string& output, uint32_t value) {
  char temp[16]{};
  std::snprintf(temp, sizeof(temp), "%08X", value);
  output.append(temp);
}

std::string DescribeSettingIds(uint8_t* base, uint32_t setting_count, uint32_t setting_ids_ptr) {
  if (setting_count == 0 || setting_ids_ptr == 0) {
    return "[]";
  }

  std::string output = "[";
  const uint32_t count = std::min(setting_count, 8u);
  for (uint32_t i = 0; i < count; ++i) {
    if (i != 0) {
      output += ",";
    }
    AppendHex(output, PPC_LOAD_U32(setting_ids_ptr + i * 4));
  }
  if (setting_count > count) {
    output += ",...";
  }
  output += "]";
  return output;
}

std::string DescribeWrittenSettings(uint8_t* base, uint32_t setting_count, uint32_t settings_ptr) {
  if (setting_count == 0 || settings_ptr == 0) {
    return "[]";
  }

  std::string output = "[";
  const uint32_t count = std::min(setting_count, 8u);
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t ptr = settings_ptr + i * 40;
    const uint32_t setting_id = PPC_LOAD_U32(ptr + 16);
    const uint8_t data_type = *reinterpret_cast<const uint8_t*>(PPC_RAW_ADDR(ptr + 24));
    const uint32_t data_size = PPC_LOAD_U32(ptr + 32);
    const uint32_t data_ptr = PPC_LOAD_U32(ptr + 36);

    char temp[96]{};
    std::snprintf(temp, sizeof(temp), "%s%08X:type=%u:size=%u:ptr=%08X", i == 0 ? "" : ",",
                  setting_id, uint32_t{data_type}, data_size, data_ptr);
    output += temp;
  }
  if (setting_count > count) {
    output += ",...";
  }
  output += "]";
  return output;
}

std::string DescribeContentData(uint8_t* base, uint32_t content_data_ptr) {
  if (content_data_ptr == 0) {
    return "<null>";
  }

  const uint32_t device_id = PPC_LOAD_U32(content_data_ptr);
  const uint32_t content_type = PPC_LOAD_U32(content_data_ptr + 4);
  const std::string file_name = ReadGuestFixedCString(base, content_data_ptr + 264, 42);

  char prefix[96]{};
  std::snprintf(prefix, sizeof(prefix), "device=%08X type=%08X file=", device_id, content_type);
  return std::string(prefix) + file_name;
}

}  // namespace

extern "C" PPC_FUNC(__imp__XamContentCreateEx);
extern "C" PPC_FUNC(__imp__XamContentClose);
extern "C" PPC_FUNC(__imp__XamContentGetLicenseMask);
extern "C" PPC_FUNC(__imp__XamContentCreateEnumerator);
extern "C" PPC_FUNC(__imp__XamContentGetDeviceData);
extern "C" PPC_FUNC(__imp__XamUserGetName);
extern "C" PPC_FUNC(__imp__XamUserGetXUID);
extern "C" PPC_FUNC(__imp__XamUserGetDeviceContext);
extern "C" PPC_FUNC(__imp__XamUserReadProfileSettings);
extern "C" PPC_FUNC(__imp__XamUserWriteProfileSettings);

extern "C" PPC_FUNC(doritos_XamUserGetSigninState) {
  static std::atomic_bool logged = false;
  LogOnce(logged, "XamUserGetSigninState -> Live signed in");
  const uint32_t user_index = ctx.r3.u32;
  ctx.r3.u64 = IsPrimaryUser(user_index) ? kLiveSigninState : 0;
}

extern "C" PPC_FUNC(doritos_XamUserGetSigninInfo) {
  static std::atomic_bool logged = false;
  LogOnce(logged, "XamUserGetSigninInfo -> synthetic profile");
  static std::atomic<uint32_t> trace_count = 0;
  const uint32_t user_index = ctx.r3.u32;
  const uint32_t flags = ctx.r4.u32;
  const uint32_t info_ptr = ctx.r5.u32;
  const bool trace = ShouldTrace(trace_count);
  if (trace) {
    Tracef("XamUserGetSigninInfo user=%u flags=%08X info=%08X", user_index, flags, info_ptr);
  }

  if (info_ptr == 0) {
    ctx.r3.u64 = kXEInvalidArg;
    if (trace) {
      Tracef("XamUserGetSigninInfo -> %08X", ctx.r3.u32);
    }
    return;
  }
  if (!IsPrimaryUser(user_index)) {
    ctx.r3.u64 = kXENoSuchUser;
    if (trace) {
      Tracef("XamUserGetSigninInfo -> %08X", ctx.r3.u32);
    }
    return;
  }

  WriteSyntheticSigninInfo(base, info_ptr);
  ctx.r3.u64 = kXESuccess;
  if (trace) {
    Tracef("XamUserGetSigninInfo -> %08X flags_word=%08X state=%08X", ctx.r3.u32,
           PPC_LOAD_U32(info_ptr + 8), PPC_LOAD_U32(info_ptr + 12));
  }
}

extern "C" PPC_FUNC(doritos_XamUserCheckPrivilege) {
  static std::atomic_bool logged = false;
  LogOnce(logged, "XamUserCheckPrivilege -> limited Live privilege policy");
  static std::atomic<uint32_t> trace_count = 0;
  const uint32_t user_index = ctx.r3.u32;
  const uint32_t mask = ctx.r4.u32;
  const uint32_t out_ptr = ctx.r5.u32;
  const bool trace = ShouldTrace(trace_count);
  if (trace) {
    Tracef("XamUserCheckPrivilege user=%u mask=%08X out=%08X", user_index, mask, out_ptr);
  }
  if (user_index != 0xFF && !IsPrimaryUser(user_index)) {
    ctx.r3.u64 = kXErrorNoSuchUser;
    if (trace) {
      Tracef("XamUserCheckPrivilege -> %08X", ctx.r3.u32);
    }
    return;
  }

  const uint32_t allowed = ShouldAllowPrivilege(mask) ? 1u : 0u;
  if (ctx.r5.u32 != 0) {
    PPC_STORE_U32(ctx.r5.u32, allowed);
  }
  ctx.r3.u64 = kXErrorSuccess;
  if (trace) {
    Tracef("XamUserCheckPrivilege -> %08X out_value=%08X", ctx.r3.u32,
           out_ptr != 0 ? PPC_LOAD_U32(out_ptr) : 0);
  }
}

extern "C" PPC_FUNC(doritos_XamShowSigninUI) {
  static std::atomic_bool logged = false;
  LogOnce(logged, "XamShowSigninUI -> no modal");
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  if (trace) {
    Tracef("XamShowSigninUI arg0=%08X arg1=%08X", ctx.r3.u32, ctx.r4.u32);
  }
  if (auto* kernel_state = REX_KERNEL_STATE()) {
    kernel_state->BroadcastNotification(0x0000000A, 1);
    kernel_state->BroadcastNotification(0x00000009, 0);
  }
  ctx.r3.u64 = kXErrorSuccess;
  if (trace) {
    Tracef("XamShowSigninUI -> %08X", ctx.r3.u32);
  }
}

extern "C" PPC_FUNC(doritos_XamUserGetName) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const uint32_t user_index = ctx.r3.u32;
  const uint32_t buffer_ptr = ctx.r4.u32;
  const uint32_t buffer_len = ctx.r5.u32;
  if (trace) {
    Tracef("XamUserGetName user=%u buffer=%08X len=%u", user_index, buffer_ptr, buffer_len);
  }
  __imp__XamUserGetName(ctx, base);
  if (trace) {
    Tracef("XamUserGetName -> %08X name=%s", ctx.r3.u32,
           buffer_ptr != 0 ? ReadGuestCString(base, buffer_ptr, 16).c_str() : "<null>");
  }
}

extern "C" PPC_FUNC(doritos_XamUserGetXUID) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const uint32_t user_index = ctx.r3.u32;
  const uint32_t type_mask = ctx.r4.u32;
  const uint32_t xuid_ptr = ctx.r5.u32;
  if (trace) {
    Tracef("XamUserGetXUID user=%u type_mask=%08X out=%08X", user_index, type_mask, xuid_ptr);
  }
  __imp__XamUserGetXUID(ctx, base);
  if (trace) {
    Tracef("XamUserGetXUID -> %08X xuid=%016llX", ctx.r3.u32,
           xuid_ptr != 0 ? static_cast<unsigned long long>(PPC_LOAD_U64(xuid_ptr)) : 0ull);
  }
}

extern "C" PPC_FUNC(doritos_XamUserGetDeviceContext) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const uint32_t user_index = ctx.r3.u32;
  const uint32_t context_kind = ctx.r4.u32;
  const uint32_t out_ptr = ctx.r5.u32;
  if (trace) {
    Tracef("XamUserGetDeviceContext user=%u kind=%08X out=%08X", user_index, context_kind, out_ptr);
  }
  __imp__XamUserGetDeviceContext(ctx, base);
  if (trace) {
    Tracef("XamUserGetDeviceContext -> %08X context=%08X", ctx.r3.u32,
           out_ptr != 0 ? PPC_LOAD_U32(out_ptr) : 0);
  }
}

extern "C" PPC_FUNC(doritos_XamUserReadProfileSettings) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const uint32_t title_id = ctx.r3.u32;
  const uint32_t user_index = ctx.r4.u32;
  const uint32_t xuid_count = ctx.r5.u32;
  const uint32_t xuids_ptr = ctx.r6.u32;
  const uint32_t setting_count = ctx.r7.u32;
  const uint32_t setting_ids_ptr = ctx.r8.u32;
  const uint32_t buffer_size_ptr = ctx.r9.u32;
  const uint32_t buffer_ptr = ctx.r10.u32;
  const uint32_t buffer_size_before = buffer_size_ptr != 0 ? PPC_LOAD_U32(buffer_size_ptr) : 0;
  const std::string setting_ids = DescribeSettingIds(base, setting_count, setting_ids_ptr);
  if (trace) {
    Tracef("XamUserReadProfileSettings title=%08X user=%u xuid_count=%u xuids=%08X count=%u ids=%s buffer_size_ptr=%08X buffer_size=%u buffer=%08X",
           title_id, user_index, xuid_count, xuids_ptr, setting_count, setting_ids.c_str(),
           buffer_size_ptr, buffer_size_before, buffer_ptr);
  }
  __imp__XamUserReadProfileSettings(ctx, base);
  if (trace) {
    Tracef("XamUserReadProfileSettings -> %08X buffer_size=%u", ctx.r3.u32,
           buffer_size_ptr != 0 ? PPC_LOAD_U32(buffer_size_ptr) : 0);
  }
}

extern "C" PPC_FUNC(doritos_XamUserWriteProfileSettings) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const uint32_t title_id = ctx.r3.u32;
  const uint32_t user_index = ctx.r4.u32;
  const uint32_t setting_count = ctx.r5.u32;
  const uint32_t settings_ptr = ctx.r6.u32;
  const uint32_t overlapped_ptr = ctx.r7.u32;
  const std::string settings = DescribeWrittenSettings(base, setting_count, settings_ptr);
  if (trace) {
    Tracef("XamUserWriteProfileSettings title=%08X user=%u count=%u settings=%08X values=%s overlapped=%08X",
           title_id, user_index, setting_count, settings_ptr, settings.c_str(), overlapped_ptr);
  }
  __imp__XamUserWriteProfileSettings(ctx, base);
  if (trace) {
    Tracef("XamUserWriteProfileSettings -> %08X", ctx.r3.u32);
  }
}

extern "C" PPC_FUNC(doritos_XamContentCreateEx) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const uint32_t user_index = ctx.r3.u32;
  const uint32_t root_name_ptr = ctx.r4.u32;
  const uint32_t content_data_ptr = ctx.r5.u32;
  const uint32_t flags = ctx.r6.u32;
  const uint32_t disposition_ptr = ctx.r7.u32;
  const uint32_t license_mask_ptr = ctx.r8.u32;
  const uint32_t cache_size = ctx.r9.u32;
  const std::string root_name = ReadGuestCString(base, root_name_ptr, 64);
  const std::string content_data = DescribeContentData(base, content_data_ptr);
  if (trace) {
    Tracef("XamContentCreateEx user=%u root=%s content={%s} flags=%08X disposition=%08X license=%08X cache=%u",
           user_index, root_name.c_str(), content_data.c_str(), flags, disposition_ptr,
           license_mask_ptr, cache_size);
  }
  __imp__XamContentCreateEx(ctx, base);
  if (trace) {
    Tracef("XamContentCreateEx -> %08X disposition=%08X license=%08X", ctx.r3.u32,
           disposition_ptr != 0 ? PPC_LOAD_U32(disposition_ptr) : 0,
           license_mask_ptr != 0 ? PPC_LOAD_U32(license_mask_ptr) : 0);
  }
}

extern "C" PPC_FUNC(doritos_XamContentClose) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const std::string root_name = ReadGuestCString(base, ctx.r3.u32, 64);
  if (trace) {
    Tracef("XamContentClose root=%s overlapped=%08X", root_name.c_str(), ctx.r4.u32);
  }
  __imp__XamContentClose(ctx, base);
  if (trace) {
    Tracef("XamContentClose -> %08X", ctx.r3.u32);
  }
}

extern "C" PPC_FUNC(doritos_XamContentGetLicenseMask) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const uint32_t mask_ptr = ctx.r3.u32;
  if (trace) {
    Tracef("XamContentGetLicenseMask mask=%08X overlapped=%08X", mask_ptr, ctx.r4.u32);
  }
  __imp__XamContentGetLicenseMask(ctx, base);
  if (trace) {
    Tracef("XamContentGetLicenseMask -> %08X mask_value=%08X", ctx.r3.u32,
           mask_ptr != 0 ? PPC_LOAD_U32(mask_ptr) : 0);
  }
}

extern "C" PPC_FUNC(doritos_XamContentCreateEnumerator) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const uint32_t buffer_size_ptr = ctx.r8.u32;
  const uint32_t handle_out = ctx.r9.u32;
  if (trace) {
    Tracef("XamContentCreateEnumerator user=%u device=%08X type=%08X flags=%08X items=%u buffer_size_ptr=%08X handle_out=%08X",
           ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32, buffer_size_ptr,
           handle_out);
  }
  __imp__XamContentCreateEnumerator(ctx, base);
  if (trace) {
    Tracef("XamContentCreateEnumerator -> %08X buffer_size=%u handle=%08X", ctx.r3.u32,
           buffer_size_ptr != 0 ? PPC_LOAD_U32(buffer_size_ptr) : 0,
           handle_out != 0 ? PPC_LOAD_U32(handle_out) : 0);
  }
}

extern "C" PPC_FUNC(doritos_XamContentGetDeviceData) {
  static std::atomic<uint32_t> trace_count = 0;
  const bool trace = ShouldTrace(trace_count);
  const uint32_t device_id = ctx.r3.u32;
  const uint32_t data_ptr = ctx.r4.u32;
  if (trace) {
    Tracef("XamContentGetDeviceData device=%08X data=%08X", device_id, data_ptr);
  }
  __imp__XamContentGetDeviceData(ctx, base);
  if (trace) {
    Tracef("XamContentGetDeviceData -> %08X", ctx.r3.u32);
  }
}

namespace doritos::runtime {

void InstallXamProfileOverrides() {
  InstallOverride("__imp__XamUserGetSigninState", &doritos_XamUserGetSigninState);
  InstallOverride("__imp__XamUserGetSigninInfo", &doritos_XamUserGetSigninInfo);
  InstallOverride("__imp__XamUserCheckPrivilege", &doritos_XamUserCheckPrivilege);
  InstallOverride("__imp__XamShowSigninUI", &doritos_XamShowSigninUI);
  InstallOverride("__imp__XamUserGetName", &doritos_XamUserGetName);
  InstallOverride("__imp__XamUserGetXUID", &doritos_XamUserGetXUID);
  InstallOverride("__imp__XamUserGetDeviceContext", &doritos_XamUserGetDeviceContext);
  InstallOverride("__imp__XamUserReadProfileSettings", &doritos_XamUserReadProfileSettings);
  InstallOverride("__imp__XamUserWriteProfileSettings", &doritos_XamUserWriteProfileSettings);
  InstallOverride("__imp__XamContentCreateEx", &doritos_XamContentCreateEx);
  InstallOverride("__imp__XamContentClose", &doritos_XamContentClose);
  InstallOverride("__imp__XamContentGetLicenseMask", &doritos_XamContentGetLicenseMask);
  InstallOverride("__imp__XamContentCreateEnumerator", &doritos_XamContentCreateEnumerator);
  InstallOverride("__imp__XamContentGetDeviceData", &doritos_XamContentGetDeviceData);
  REXKRNL_INFO("Doritos profile override installed: user 0 reports Live sign-in with limited privileges");
  doritos::logging::Write(doritos::logging::Level::Info, "profile",
                         "Doritos profile override installed: user 0 reports Live sign-in with limited privileges");
}

}  // namespace doritos::runtime
