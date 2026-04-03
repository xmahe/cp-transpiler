#include "analyze.h"

#include "../support/strings.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <unordered_set>
#include <utility>

namespace cplus::sema {
namespace {

template <typename T>
void append_symbol(AnalysisResult& result, const std::string& name, SymbolKind kind, const T& node) {
    if (!result.symbols.add(Symbol{name, kind, node.range})) {
        result.diagnostics.push_back({
            model::Severity::Error,
            node.range,
            "duplicate symbol: " + name,
        });
    }
}

} // namespace

AnalysisResult Analyzer::analyze(const model::Program& program, const AnalysisOptions& options) const {
    AnalysisResult result;
    result.program = program;
    collect_symbols(program, result);
    validate_program(program, options, result);
    return result;
}

void Analyzer::collect_symbols(const model::Program& program, AnalysisResult& result) const {
    for (const auto& decl : program.declarations) {
        std::visit(
            [&](const auto& node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, model::NamespaceDecl>) {
                    append_symbol(result, NameMangler::join(node.path), SymbolKind::Namespace, node);
                } else if constexpr (std::is_same_v<T, model::EnumDecl>) {
                    append_symbol(result, NameMangler::mangle(node.namespace_path, node.name), SymbolKind::Enum, node);
                } else if constexpr (std::is_same_v<T, model::InterfaceDecl>) {
                    append_symbol(result, NameMangler::mangle(node.namespace_path, node.name), SymbolKind::Interface, node);
                } else if constexpr (std::is_same_v<T, model::ClassDecl>) {
                    append_symbol(result, NameMangler::mangle(node.namespace_path, node.name), SymbolKind::Class, node);
                } else if constexpr (std::is_same_v<T, model::FunctionDecl>) {
                    append_symbol(result, NameMangler::mangle(node.namespace_path, node.signature.name), SymbolKind::Function, node);
                } else if constexpr (std::is_same_v<T, model::RawCDecl>) {
                    (void)node;
                }
            },
            decl);
    }
}

void Analyzer::validate_program(const model::Program& program, const AnalysisOptions& options, AnalysisResult& result) const {
    std::size_t top_level_namespace_count = 0;
    for (const auto& decl : program.declarations) {
        if (const auto* ns = std::get_if<model::NamespaceDecl>(&decl); ns != nullptr && ns->is_top_level) {
            ++top_level_namespace_count;
        }
    }
    if (top_level_namespace_count != 1) {
        add_diagnostic(
            result,
            model::Severity::Error,
            {},
            "file must contain exactly one top-level namespace block");
    }

    for (const auto& decl : program.declarations) {
        validate_declaration(decl, options, result);
    }
    if (options.enforce_maybe_flow) {
        validate_maybe_flow(program, options, result);
    }
}

void Analyzer::validate_declaration(const model::Declaration& decl, const AnalysisOptions& options, AnalysisResult& result) const {
    std::visit(
        [&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, model::NamespaceDecl>) {
                for (const auto& part : node.path) {
                    validate_name_style(part, "CamelCase", node.range, result);
                }
            } else if constexpr (std::is_same_v<T, model::EnumDecl>) {
                validate_enum(node, result);
            } else if constexpr (std::is_same_v<T, model::InterfaceDecl>) {
                validate_interface(node, options, result);
            } else if constexpr (std::is_same_v<T, model::ClassDecl>) {
                validate_class(node, options, result);
            } else if constexpr (std::is_same_v<T, model::FunctionDecl>) {
                validate_name_style(node.signature.name, "CamelCase", node.range, result);
                validate_function_signature(node.signature, options, result);
                validate_function_body_restrictions(node, result);
            } else if constexpr (std::is_same_v<T, model::RawCDecl>) {
                (void)node;
            }
        },
        decl);
}

void Analyzer::validate_class(const model::ClassDecl& decl, const AnalysisOptions& options, AnalysisResult& result) const {
    validate_name_style(decl.name, "CamelCase", decl.range, result);

    std::set<std::string> method_names;
    for (const auto& field : decl.fields) {
        validate_name_style(field.name, field.is_private_intent ? "_snake_case" : "snake_case", field.range, result);
        validate_type(field.type, options, result);
    }

    for (const auto& field : decl.static_fields) {
        validate_name_style(field.name, field.is_private_intent ? "_snake_case" : "snake_case", field.range, result);
        validate_type(field.type, options, result);
    }

    for (const auto& method : decl.methods) {
        if (method.name != "Destroy") {
            validate_name_style(method.name, "CamelCase", method.range, result);
        }
        validate_function_signature(method, options, result);
        method_names.insert(method.name);
    }

    for (const auto& ctor : decl.constructors) {
        if (ctor.name != "Construct") {
            add_diagnostic(result, model::Severity::Error, ctor.range, "constructor must be named Construct");
        }
        validate_function_signature(ctor, options, result);
    }

    if (decl.has_destroy && method_names.find("Destroy") == method_names.end()) {
        add_diagnostic(result, model::Severity::Warning, decl.range, "class declares Destroy handling without Destroy method");
    }

    validate_interface_fulfillment(decl, result.program, result);
}

void Analyzer::validate_interface(const model::InterfaceDecl& decl, const AnalysisOptions& options, AnalysisResult& result) const {
    validate_name_style(decl.name, "CamelCase", decl.range, result);
    for (const auto& method : decl.methods) {
        validate_name_style(method.name, "CamelCase", method.range, result);
        validate_function_signature(method, options, result);
    }
}

void Analyzer::validate_enum(const model::EnumDecl& decl, AnalysisResult& result) const {
    validate_name_style(decl.name, "CamelCase", decl.range, result);
    if (decl.members.empty()) {
        add_diagnostic(result, model::Severity::Error, decl.range, "enum must have at least one member");
        return;
    }

    for (const auto& member : decl.members) {
        if (!is_constant_enum_value(member)) {
            add_diagnostic(result, model::Severity::Error, decl.range, "invalid enum member style: " + member);
        }
    }

    if (!decl.members.empty()) {
        const auto& last = decl.members.back();
        if (last.empty() || last.back() != 'N') {
            add_diagnostic(result, model::Severity::Error, decl.range, "enum must end with trailing N member");
        }
    }
}

void Analyzer::validate_function_signature(const model::FunctionSignature& signature, const AnalysisOptions& options, AnalysisResult& result) const {
    validate_type(signature.return_type, options, result);
    for (const auto& param : signature.parameters) {
        validate_name_style(param.name, "snake_case", param.range, result);
        validate_type(param.type, options, result);
    }
}

void Analyzer::validate_type(const model::TypeRef& type, const AnalysisOptions& options, AnalysisResult& result) const {
    if (!options.warn_on_legacy_scalars) {
        return;
    }

    if (is_legacy_scalar(type.spelling)) {
        add_diagnostic(result, model::Severity::Warning, {}, "prefer fixed-width builtin types over legacy scalar: " + type.spelling);
    }

    if (is_maybe_type(type.spelling)) {
        if (type.spelling.empty() || type.spelling.back() != '>') {
            add_diagnostic(result, model::Severity::Error, {}, "invalid maybe<T> spelling: " + type.spelling);
        }
    } else if (type.spelling.find('<') != std::string::npos || type.spelling.find('>') != std::string::npos) {
        add_diagnostic(result, model::Severity::Error, {}, "templates are not supported except for maybe<T>: " + type.spelling);
    }
}

void Analyzer::validate_interface_fulfillment(const model::ClassDecl& decl, const model::Program& program, AnalysisResult& result) const {
    for (const auto& interface_name : decl.implements) {
        const model::InterfaceDecl* interface_decl = nullptr;
        for (const auto& candidate : program.declarations) {
            if (const auto* iface = std::get_if<model::InterfaceDecl>(&candidate)) {
                if (iface->name == interface_name) {
                    interface_decl = iface;
                    break;
                }
            }
        }

        if (interface_decl == nullptr) {
            add_diagnostic(result, model::Severity::Error, decl.range, "implements unknown interface: " + interface_name);
            continue;
        }

        for (const auto& required : interface_decl->methods) {
            const model::FunctionSignature* found = nullptr;
            for (const auto& method : decl.methods) {
                if (method.name == required.name && method.is_implementation) {
                    found = &method;
                    break;
                }
            }

            if (found == nullptr) {
                add_diagnostic(result, model::Severity::Error, decl.range, "class missing interface implementation: " + interface_name + "." + required.name);
                continue;
            }

            if (!signatures_match(*found, required)) {
                add_diagnostic(result, model::Severity::Error, found->range, "interface implementation signature mismatch for " + required.name);
            }
        }
    }

    for (const auto& method : decl.methods) {
        if (!method.is_implementation) {
            continue;
        }

        bool matched = false;
        for (const auto& interface_name : decl.implements) {
            for (const auto& candidate : program.declarations) {
                if (const auto* iface = std::get_if<model::InterfaceDecl>(&candidate)) {
                    if (iface->name != interface_name) {
                        continue;
                    }
                    for (const auto& required : iface->methods) {
                        if (required.name == method.name) {
                            matched = true;
                            break;
                        }
                    }
                }
                if (matched) {
                    break;
                }
            }
            if (matched) {
                break;
            }
        }

        if (!matched) {
            add_diagnostic(result, model::Severity::Error, method.range, "implementation method matches no declared interface requirement: " + method.name);
        }
    }
}

void Analyzer::validate_maybe_flow(const model::Program& program, const AnalysisOptions& options, AnalysisResult& result) const {
    (void)options;
    for (const auto& decl : program.declarations) {
        const auto* fn = std::get_if<model::FunctionDecl>(&decl);
        if (fn == nullptr) {
            continue;
        }
        std::unordered_set<std::string> maybe_names;
        for (const auto& param : fn->signature.parameters) {
            if (is_maybe_type(param.type.spelling)) {
                maybe_names.insert(param.name);
            }
        }

        if (maybe_names.empty()) {
            continue;
        }

        for (const auto& maybe_name : maybe_names) {
            const std::string has_value_probe = "if (" + maybe_name + ".has_value())";
            const std::string value_probe = maybe_name + ".value()";
            const auto value_pos = fn->body_source.find(value_probe);
            if (value_pos == std::string::npos) {
                continue;
            }
            const auto has_pos = fn->body_source.find(has_value_probe);
            if (has_pos == std::string::npos || has_pos > value_pos) {
                add_diagnostic(result, model::Severity::Error, fn->range, "unsafe maybe<T>.value() access without proven has_value(): " + maybe_name);
            }
        }
    }
}

void Analyzer::validate_function_body_restrictions(const model::FunctionDecl& decl, AnalysisResult& result) const {
    const auto& body = decl.body_source;
    if (body.empty()) {
        return;
    }

    auto reject_if_contains = [&](std::string_view needle, std::string message) {
        if (body.find(needle) != std::string::npos) {
            add_diagnostic(result, model::Severity::Error, decl.range, std::move(message));
        }
    };

    reject_if_contains("goto", "goto is forbidden");
    reject_if_contains("continue", "continue is forbidden");
    reject_if_contains("++", "++ is forbidden");
    reject_if_contains("--", "-- is forbidden");
    reject_if_contains("?", "ternary operator is forbidden");

    if (body.find("switch") != std::string::npos && body.find("default:") == std::string::npos) {
        add_diagnostic(result, model::Severity::Error, decl.range, "switch must include default");
    }
}

void Analyzer::validate_name_style(const std::string& name, const std::string& expected, const model::SourceRange& range, AnalysisResult& result) const {
    if (expected == "CamelCase" && (name == "Construct" || name == "Destroy")) {
        return;
    }
    const bool ok = (expected == "CamelCase") ? is_camel_case(name) : is_snake_case(name);
    if (!ok) {
        add_diagnostic(result, model::Severity::Error, range, "identifier does not match expected style " + expected + ": " + name);
    }
}

void Analyzer::add_diagnostic(AnalysisResult& result, model::Severity severity, const model::SourceRange& range, std::string message) const {
    result.diagnostics.push_back(model::Diagnostic{severity, range, std::move(message)});
}

bool Analyzer::signatures_match(const model::FunctionSignature& lhs, const model::FunctionSignature& rhs) {
    if (lhs.name != rhs.name || lhs.return_type.spelling != rhs.return_type.spelling || lhs.parameters.size() != rhs.parameters.size()) {
        return false;
    }
    for (std::size_t index = 0; index < lhs.parameters.size(); ++index) {
        if (lhs.parameters[index].type.spelling != rhs.parameters[index].type.spelling) {
            return false;
        }
    }
    return true;
}

bool Analyzer::is_maybe_type(std::string_view spelling) {
    return spelling.rfind("maybe<", 0) == 0;
}

std::string Analyzer::maybe_inner_type(std::string_view spelling) {
    if (!is_maybe_type(spelling) || spelling.size() < 8 || spelling.back() != '>') {
        return {};
    }
    return std::string(spelling.substr(6, spelling.size() - 7));
}

bool Analyzer::is_camel_case(std::string_view name) {
    if (name.empty()) {
        return false;
    }

    if (!std::isupper(static_cast<unsigned char>(name.front()))) {
        return false;
    }

    for (const char ch : name) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
            return false;
        }
    }

    return true;
}

bool Analyzer::is_snake_case(std::string_view name) {
    if (name.empty()) {
        return false;
    }

    if (!std::islower(static_cast<unsigned char>(name.front())) && name.front() != '_') {
        return false;
    }

    for (const char ch : name) {
        if (!(std::islower(static_cast<unsigned char>(ch)) || std::isdigit(static_cast<unsigned char>(ch)) || ch == '_')) {
            return false;
        }
    }

    return true;
}

bool Analyzer::is_constant_enum_value(std::string_view name) {
    return name.rfind("k", 0) == 0;
}

bool Analyzer::is_legacy_scalar(std::string_view spelling) {
    return spelling == "char" || spelling == "short" || spelling == "int" || spelling == "long" || spelling == "float" || spelling == "double";
}

} // namespace cplus::sema
