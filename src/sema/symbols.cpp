#include "symbols.h"

#include <sstream>

namespace cplus::sema {

bool SymbolTable::add(Symbol symbol) {
    const auto it = index_.find(symbol.qualified_name);
    if (it != index_.end()) {
        return false;
    }

    index_.emplace(symbol.qualified_name, symbols_.size());
    symbols_.push_back(std::move(symbol));
    return true;
}

const Symbol* SymbolTable::find(std::string_view qualified_name) const {
    const auto it = index_.find(std::string(qualified_name));
    if (it == index_.end()) {
        return nullptr;
    }

    return &symbols_[it->second];
}

const std::vector<Symbol>& SymbolTable::all() const {
    return symbols_;
}

std::string NameMangler::join(const std::vector<std::string>& parts) {
    std::ostringstream out;
    bool first = true;
    for (const auto& part : parts) {
        if (!first) {
            out << "___";
        }
        first = false;
        out << part;
    }
    return out.str();
}

std::string NameMangler::mangle(const std::vector<std::string>& namespace_path, std::string_view leaf) {
    if (namespace_path.empty()) {
        return std::string(leaf);
    }

    std::ostringstream out;
    out << join(namespace_path) << "___" << leaf;
    return out.str();
}

} // namespace cplus::sema
