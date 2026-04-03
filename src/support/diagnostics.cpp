#include "diagnostics.h"

#include <iostream>
#include <utility>

namespace cplus::support {

void Diagnostics::add(Severity severity, std::string message, ast::Span span, std::string file) {
    diagnostics_.push_back(Diagnostic{severity, std::move(message), span, std::move(file)});
}

void Diagnostics::error(std::string message, ast::Span span, std::string file) {
    add(Severity::Error, std::move(message), span, std::move(file));
}

void Diagnostics::warning(std::string message, ast::Span span, std::string file) {
    add(Severity::Warning, std::move(message), span, std::move(file));
}

void Diagnostics::note(std::string message, ast::Span span, std::string file) {
    add(Severity::Note, std::move(message), span, std::move(file));
}

const std::vector<Diagnostic>& Diagnostics::items() const {
    return diagnostics_;
}

bool Diagnostics::has_errors() const {
    for (const auto& diagnostic : diagnostics_) {
        if (diagnostic.severity == Severity::Error) {
            return true;
        }
    }
    return false;
}

void Diagnostics::print(std::ostream& out) const {
    for (const auto& diagnostic : diagnostics_) {
        out << to_string(diagnostic.severity);
        if (!diagnostic.file.empty()) {
            out << ": " << diagnostic.file;
            if (diagnostic.span.begin.line > 0) {
                out << ":" << diagnostic.span.begin.line << ":" << diagnostic.span.begin.column;
            }
        }
        out << ": " << diagnostic.message << '\n';
    }
}

std::string to_string(Severity severity) {
    switch (severity) {
    case Severity::Error:
        return "error";
    case Severity::Warning:
        return "warning";
    case Severity::Note:
        return "note";
    }
    return "unknown";
}

} // namespace cplus::support
