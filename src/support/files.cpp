#include "files.h"

#include "strings.h"

#include <fstream>
#include <iterator>
#include <set>

namespace cplus::support {

bool read_text_file(const std::filesystem::path& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool write_text_file(const std::filesystem::path& path, std::string_view text) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(out);
}

bool is_cplus_source_file(const std::filesystem::path& path) {
    const auto ext = to_lower(path.extension().string());
    return ext == ".cp" || ext == ".hp";
}

std::vector<std::filesystem::path> discover_source_files(const std::vector<std::filesystem::path>& roots) {
    std::set<std::filesystem::path> discovered;

    for (const auto& root : roots) {
        if (!std::filesystem::exists(root)) {
            continue;
        }

        if (std::filesystem::is_regular_file(root)) {
            if (is_cplus_source_file(root)) {
                discovered.insert(root);
            }
            continue;
        }

        if (std::filesystem::is_directory(root)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
                if (entry.is_regular_file() && is_cplus_source_file(entry.path())) {
                    discovered.insert(entry.path());
                }
            }
        }
    }

    return {discovered.begin(), discovered.end()};
}

} // namespace cplus::support
