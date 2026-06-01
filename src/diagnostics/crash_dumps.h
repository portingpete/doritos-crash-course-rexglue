#pragma once

#include <filesystem>

namespace doritos::diagnostics {

void InstallCrashDumpHandler(const std::filesystem::path& dump_dir);

}  // namespace doritos::diagnostics



