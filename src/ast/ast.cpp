#include "ast.h"

#include <sstream>

namespace cplus::ast {

bool QualifiedName::empty() const {
    return parts.empty();
}

std::string QualifiedName::str(std::string_view separator) const {
    std::ostringstream out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            out << separator;
        }
        out << parts[i];
    }
    return out.str();
}

} // namespace cplus::ast
