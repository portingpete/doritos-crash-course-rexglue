#include "diagnostics/exception_trace.h"

#include <atomic>
#include <cstdio>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "logging/log.h"

namespace {

std::atomic<void*> g_handler{};
std::atomic_uint32_t g_logged_exceptions{};
std::atomic_flag g_logging = ATOMIC_FLAG_INIT;

LONG CALLBACK FirstChanceExceptionHandler(EXCEPTION_POINTERS* pointers) {
  if (!pointers || !pointers->ExceptionRecord) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  const auto* record = pointers->ExceptionRecord;
  if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION &&
      record->ExceptionCode != EXCEPTION_IN_PAGE_ERROR) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  const uint32_t index = g_logged_exceptions.fetch_add(1);
  if (index >= 64 || g_logging.test_and_set()) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  const ULONG_PTR access_type =
      record->NumberParameters > 0 ? record->ExceptionInformation[0] : 0;
  const ULONG_PTR access_address =
      record->NumberParameters > 1 ? record->ExceptionInformation[1] : 0;

  char buffer[384]{};
  std::snprintf(buffer, sizeof(buffer),
                "first-chance exception code=%08lX ip=%p access_type=%llu access_address=%p",
                record->ExceptionCode, record->ExceptionAddress,
                static_cast<unsigned long long>(access_type),
                reinterpret_cast<void*>(access_address));
  doritos::logging::Write(doritos::logging::Level::Error, "exceptions",
                          std::string_view(buffer));

  g_logging.clear();
  return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

namespace doritos::diagnostics {

void InstallFirstChanceExceptionTrace() {
  void* expected = nullptr;
  void* handler = AddVectoredExceptionHandler(/*First=*/1, FirstChanceExceptionHandler);
  if (!g_handler.compare_exchange_strong(expected, handler) && handler) {
    RemoveVectoredExceptionHandler(handler);
    return;
  }

  if (handler) {
    doritos::logging::Write(doritos::logging::Level::Info, "exceptions",
                            "first-chance exception trace installed");
  } else {
    doritos::logging::Write(doritos::logging::Level::Warning, "exceptions",
                            "first-chance exception trace install failed");
  }
}

}  // namespace doritos::diagnostics
