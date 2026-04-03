#include "driver/cli.h"
#include "driver/driver.h"
#include "support/diagnostics.h"

#include <iostream>

int main(int argc, char** argv) {
    cplus::driver::Options options;
    std::string error_message;
    if (!cplus::driver::parse_cli(argc, argv, options, error_message)) {
        std::cerr << error_message << "\n";
        cplus::driver::print_usage(std::cerr);
        return 1;
    }

    if (options.help) {
        cplus::driver::print_usage(std::cout);
        return 0;
    }

    cplus::support::Diagnostics diagnostics;
    const int exit_code = cplus::driver::run_transpiler(options, diagnostics);
    diagnostics.print(std::cerr);
    return exit_code;
}
