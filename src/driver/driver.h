#pragma once

#include "driver/cli.h"
#include "support/diagnostics.h"

namespace cplus::driver {

int run_transpiler(const Options& options, support::Diagnostics& diagnostics);

} // namespace cplus::driver
