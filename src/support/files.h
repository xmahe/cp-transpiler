#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cplus::support {

bool read_text_file(const std::filesystem::path& path, std::string& out);
bool write_text_file(const std::filesystem::path& path, std::string_view text);
bool is_cplus_source_file(const std::filesystem::path& path);
std::vector<std::filesystem::path> discover_source_files(const std::vector<std::filesystem::path>& roots);

} // namespace cplus::support
