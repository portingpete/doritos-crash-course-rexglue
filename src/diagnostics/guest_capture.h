#pragma once

#include <atomic>
#include <thread>

namespace rex {
class Runtime;
}

namespace doritos::diagnostics {

void StartGuestOutputCapture(rex::Runtime* runtime, std::atomic_bool& stop_requested,
                             std::thread& worker);

}  // namespace doritos::diagnostics
