#pragma once

#include "model.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cplus::sema {

enum class SymbolKind {
    Namespace,
    Enum,
    Interface,
    Class,
    Function,
    Method,
    Field,
    Constructor,
};

struct Symbol {
    std::string qualified_name;
    SymbolKind kind = SymbolKind::Function;
    model::SourceRange range{};
};

class SymbolTable {
public:
    bool add(Symbol symbol);
    const Symbol* find(std::string_view qualified_name) const;
    const std::vector<Symbol>& all() const;

private:
    std::vector<Symbol> symbols_;
    std::unordered_map<std::string, std::size_t> index_;
};

struct NameMangler {
    static std::string join(const std::vector<std::string>& parts);
    static std::string mangle(const std::vector<std::string>& namespace_path, std::string_view leaf);
};

} // namespace cplus::sema
