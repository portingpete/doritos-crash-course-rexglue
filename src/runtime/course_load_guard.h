#pragma once

namespace rex::runtime {
class FunctionDispatcher;
}

namespace doritos::runtime {

void InstallCourseLoadGuard(rex::runtime::FunctionDispatcher* dispatcher);

}  // namespace doritos::runtime
