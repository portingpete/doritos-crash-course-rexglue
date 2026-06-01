#pragma once

#include <cstdint>
#include <string>

namespace doritos::runtime {

struct ImportCall {
  std::string subsystem;
  std::string name;
  uint32_t guest_caller = 0;
  uint64_t call_count = 0;
};

uint32_t LogUnimplementedImport(const ImportCall& call);

}  // namespace doritos::runtime



