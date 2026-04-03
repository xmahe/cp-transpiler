#pragma once

#include "model.h"
#include "symbols.h"

#include <string>
#include <string_view>
#include <unordered_map>

namespace cplus::sema {

struct AnalysisOptions {
    bool warn_on_legacy_scalars = true;
    bool enforce_maybe_flow = true;
};

struct MaybeFact {
    bool known_has_value = false;
};

struct FlowFacts {
    std::unordered_map<std::string, MaybeFact> maybe_values;
};

struct AnalysisResult {
    model::Program program;
    SymbolTable symbols;
    std::vector<model::Diagnostic> diagnostics;
};

class Analyzer {
public:
    AnalysisResult analyze(const model::Program& program, const AnalysisOptions& options = {}) const;

private:
    void collect_symbols(const model::Program& program, AnalysisResult& result) const;
    void validate_program(const model::Program& program, const AnalysisOptions& options, AnalysisResult& result) const;
    void validate_declaration(const model::Declaration& decl, const AnalysisOptions& options, AnalysisResult& result) const;
    void validate_class(const model::ClassDecl& decl, const AnalysisOptions& options, AnalysisResult& result) const;
    void validate_interface(const model::InterfaceDecl& decl, const AnalysisOptions& options, AnalysisResult& result) const;
    void validate_enum(const model::EnumDecl& decl, AnalysisResult& result) const;
    void validate_function_signature(const model::FunctionSignature& signature, const AnalysisOptions& options, AnalysisResult& result) const;
    void validate_type(const model::TypeRef& type, const AnalysisOptions& options, AnalysisResult& result) const;
    void validate_bindings(const model::Program& program, AnalysisResult& result) const;
    void validate_interface_fulfillment(const model::ClassDecl& decl, const model::Program& program, AnalysisResult& result) const;
    void validate_maybe_flow(const model::Program& program, const AnalysisOptions& options, AnalysisResult& result) const;
    void validate_missing_free_function_definitions(const model::Program& program, AnalysisResult& result) const;
    void validate_function_body_restrictions(const model::FunctionDecl& decl, AnalysisResult& result) const;
    void validate_name_style(const std::string& name, const std::string& expected, const model::SourceRange& range, AnalysisResult& result) const;
    void add_diagnostic(AnalysisResult& result, model::Severity severity, const model::SourceRange& range, std::string message) const;
    static bool signatures_match(const model::FunctionSignature& lhs, const model::FunctionSignature& rhs);
    static bool is_maybe_type(std::string_view spelling);
    static std::string maybe_inner_type(std::string_view spelling);
    static bool is_camel_case(std::string_view name);
    static bool is_snake_case(std::string_view name);
    static bool is_constant_enum_value(std::string_view name);
    static bool is_legacy_scalar(std::string_view spelling);
};

} // namespace cplus::sema
