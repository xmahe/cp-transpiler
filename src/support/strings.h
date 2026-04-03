#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace cplus::support {

std::string trim(std::string_view text);
bool starts_with(std::string_view text, std::string_view prefix);
bool ends_with(std::string_view text, std::string_view suffix);
std::string to_lower(std::string_view text);
std::vector<std::string> split(std::string_view text, char delimiter);
std::string join(const std::vector<std::string>& parts, std::string_view delimiter);

} // namespace cplus::support
