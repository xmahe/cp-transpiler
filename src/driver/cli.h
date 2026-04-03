#pragma once

#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

namespace cplus::driver {

struct Options {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path output_dir = ".";
    bool verbose = false;
    bool help = false;
};

bool parse_cli(int argc, char** argv, Options& options, std::string& error_message);
void print_usage(std::ostream& out);

} // namespace cplus::driver
