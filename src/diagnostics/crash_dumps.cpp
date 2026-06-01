#include "diagnostics/crash_dumps.h"

#include "logging/log.h"

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#endif

#include <chrono>
#include <sstream>

namespace doritos::diagnostics {
namespace {

#if defined(_WIN32)
std::filesystem::path g_dump_dir;

LONG WINAPI DumpUnhandledException(EXCEPTION_POINTERS* exception_pointers) {
  std::filesystem::create_directories(g_dump_dir);
  auto name = g_dump_dir / "doritos-port-crash.dmp";
  HANDLE file = CreateFileW(name.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION info{};
    info.ThreadId = GetCurrentThreadId();
    info.ExceptionPointers = exception_pointers;
    info.ClientPointers = FALSE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithDataSegs,
                      &info, nullptr, nullptr);
    CloseHandle(file);
  }
  logging::Write(logging::Level::Error, "exceptions", "Unhandled exception; crash dump requested");
  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

}  // namespace

void InstallCrashDumpHandler(const std::filesystem::path& dump_dir) {
#if defined(_WIN32)
  g_dump_dir = dump_dir;
  SetUnhandledExceptionFilter(DumpUnhandledException);
  logging::Write(logging::Level::Info, "exceptions",
                 "Windows crash dump handler installed at " + dump_dir.string());
#else
  logging::Write(logging::Level::Info, "exceptions",
                 "Crash dump handler placeholder installed at " + dump_dir.string());
#endif
}

}  // namespace doritos::diagnostics



