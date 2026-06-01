#include "runtime/import_trap.h"

#include <sstream>

#include "logging/log.h"

namespace doritos::runtime {

uint32_t LogUnimplementedImport(const ImportCall& call) {
  std::ostringstream message;
  message << "unimplemented import " << call.subsystem << "::" << call.name
          << " caller=0x" << std::hex << call.guest_caller << std::dec
          << " count=" << call.call_count << " returning X_STATUS_NOT_IMPLEMENTED";
  logging::Write(logging::Level::Error, "imports", message.str());
  return 0xC0000002u;
}

}  // namespace doritos::runtime



