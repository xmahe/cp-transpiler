#pragma once

#include "ast/ast.h"

#include <ostream>
#include <string>
#include <vector>

namespace cplus::support {

enum class Severity {
    Error,
    Warning,
    Note,
};

struct Diagnostic {
    Severity severity = Severity::Error;
    std::string message;
    ast::Span span{};
    std::string file;
};

class Diagnostics {
public:
    void add(Severity severity, std::string message, ast::Span span = {}, std::string file = {});
    void error(std::string message, ast::Span span = {}, std::string file = {});
    void warning(std::string message, ast::Span span = {}, std::string file = {});
    void note(std::string message, ast::Span span = {}, std::string file = {});

    const std::vector<Diagnostic>& items() const;
    bool has_errors() const;
    void print(std::ostream& out) const;

private:
    std::vector<Diagnostic> diagnostics_;
};

std::string to_string(Severity severity);

} // namespace cplus::support
