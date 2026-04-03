#include "cli.h"

#include "support/strings.h"

#include <iostream>

namespace cplus::driver {

void print_usage(std::ostream& out) {
    out << "usage: cplus [options] <inputs...>\n"
        << "  -o, --output <dir>   output directory\n"
        << "  -v, --verbose        verbose output\n"
        << "  -h, --help           show help\n";
}

bool parse_cli(int argc, char** argv, Options& options, std::string& error_message) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            options.help = true;
            continue;
        }
        if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
            continue;
        }
        if (arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) {
                error_message = "missing path after --output";
                return false;
            }
            options.output_dir = argv[++i];
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            error_message = "unknown option: " + arg;
            return false;
        }
        options.inputs.emplace_back(arg);
    }
    return true;
}

} // namespace cplus::driver
