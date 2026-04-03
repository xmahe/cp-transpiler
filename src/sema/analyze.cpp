#include "analyze.h"

#include "../support/strings.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <unordered_set>
#include <utility>

namespace cplus::sema {
namespace {

const model::ClassDecl* find_class_decl(const model::Program& program, std::string_view name) {
    for (const auto& decl : program.declarations) {
        if (const auto* klass = std::get_if<model::ClassDecl>(&decl)) {
            if (klass->name == name) {
                return klass;
            }
        }
    }
    return nullptr;
}

const model::InterfaceDecl* find_interface_decl(const model::Program& program, std::string_view name) {
    for (const auto& decl : program.declarations) {
        if (const auto* iface = std::get_if<model::InterfaceDecl>(&decl)) {
            if (iface->name == name) {
                return iface;
            }
        }
    }
    return nullptr;
}

const model::BindDecl* find_bind_decl(const model::Program& program, std::string_view owner_type, std::string_view slot_name) {
    for (const auto& decl : program.declarations) {
        if (const auto* bind = std::get_if<model::BindDecl>(&decl)) {
            if (bind->owner_type == owner_type && bind->slot_name == slot_name) {
                return bind;
            }
        }
    }
    return nullptr;
}

bool is_integer_like_type(std::string_view type) {
    return type == "u8" || type == "u16" || type == "u32" || type == "u64" ||
        type == "i8" || type == "i16" || type == "i32" || type == "i64";
}

bool is_float_like_type(std::string_view type) {
    return type == "f32" || type == "f64";
}

bool is_integer_literal(std::string_view text) {
    if (text.empty()) {
        return false;
    }
    for (const char ch : text) {
        if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '_')) {
            return false;
        }
    }
    return true;
}

bool is_float_literal(std::string_view text) {
    bool saw_dot = false;
    bool saw_digit = false;
    for (const char ch : text) {
        if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '_') {
            saw_digit = true;
            continue;
        }
        if (ch == '.' && !saw_dot) {
            saw_dot = true;
            continue;
        }
        return false;
    }
    return saw_dot && saw_digit;
}

std::optional<std::string> infer_initializer_argument_type(
    std::string_view text,
    const model::FunctionSignature& constructor,
    const model::ClassDecl& owner_class) {
    const auto trimmed = support::trim(text);
    for (const auto& param : constructor.parameters) {
        if (param.name == trimmed) {
            return param.type.spelling;
        }
    }
    for (const auto& field : owner_class.fields) {
        if (field.name == trimmed) {
            return field.type.spelling;
        }
    }
    if (is_integer_literal(trimmed)) {
        return std::string("u32");
    }
    if (is_float_literal(trimmed)) {
        return std::string("f32");
    }
    if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
        return std::string("u8*");
    }
    if (trimmed.size() >= 2 && trimmed.front() == '\'' && trimmed.back() == '\'') {
        return std::string("u8");
    }
    return std::nullopt;
}

bool types_compatible(std::string_view argument_type, std::string_view parameter_type) {
    if (argument_type == parameter_type) {
        return true;
    }
    if (is_integer_like_type(argument_type) && is_integer_like_type(parameter_type)) {
        return true;
    }
    if (is_float_like_type(argument_type) && is_float_like_type(parameter_type)) {
        return true;
    }
    return false;
}

int score_initializer_match(
    const model::MemberInitializer& initializer,
    const model::FunctionSignature& target_ctor,
    const model::FunctionSignature& owner_ctor,
    const model::ClassDecl& owner_class) {
    if (target_ctor.parameters.size() != initializer.arguments.size()) {
        return -1;
    }

    int score = 0;
    for (std::size_t index = 0; index < initializer.arguments.size(); ++index) {
        const auto inferred_type = infer_initializer_argument_type(initializer.arguments[index], owner_ctor, owner_class);
        if (!inferred_type.has_value()) {
            continue;
        }
        if (!types_compatible(*inferred_type, target_ctor.parameters[index].type.spelling)) {
            return -1;
        }
        if (*inferred_type == target_ctor.parameters[index].type.spelling) {
            score += 2;
        } else {
            score += 1;
        }
    }
    return score;
}

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
    validate_bindings(program, result);
    validate_missing_free_function_definitions(program, result);
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
            } else if constexpr (std::is_same_v<T, model::BindDecl>) {
                (void)node;
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
        if (method.name != "Destruct") {
            validate_name_style(method.name, "CamelCase", method.range, result);
        }
        validate_function_signature(method, options, result);
        if (!method.member_initializers.empty()) {
            add_diagnostic(result, model::Severity::Error, method.range, "member initializer list is only allowed on Construct");
        }
        method_names.insert(method.name);
    }

    for (const auto& ctor : decl.constructors) {
        if (ctor.name != "Construct") {
            add_diagnostic(result, model::Severity::Error, ctor.range, "constructor must be named Construct");
        }
        validate_function_signature(ctor, options, result);

        std::unordered_set<std::string> initialized_fields;
        for (const auto& initializer : ctor.member_initializers) {
            if (!initialized_fields.insert(initializer.field_name).second) {
                add_diagnostic(result, model::Severity::Error, initializer.range, "duplicate member initializer: " + initializer.field_name);
                continue;
            }

            const model::FieldDecl* field_decl = nullptr;
            for (const auto& field : decl.fields) {
                if (field.name == initializer.field_name) {
                    field_decl = &field;
                    break;
                }
            }
            if (field_decl == nullptr) {
                add_diagnostic(result, model::Severity::Error, initializer.range, "unknown field in member initializer: " + initializer.field_name);
                continue;
            }

            std::string target_type = field_decl->type.spelling;
            if (field_decl->is_inject) {
                const auto* bind = find_bind_decl(result.program, decl.name, field_decl->name);
                if (bind == nullptr) {
                    continue;
                }
                target_type = bind->concrete_type;
            }

            const auto* target_class = find_class_decl(result.program, target_type);
            if (target_class == nullptr) {
                add_diagnostic(result, model::Severity::Error, initializer.range, "member initializer target is not a class type: " + initializer.field_name);
                continue;
            }

            int best_score = -1;
            std::size_t best_count = 0;
            for (const auto& target_ctor : target_class->constructors) {
                const int score = score_initializer_match(initializer, target_ctor, ctor, decl);
                if (score < 0) {
                    continue;
                }
                if (score > best_score) {
                    best_score = score;
                    best_count = 1;
                } else if (score == best_score) {
                    ++best_count;
                }
            }
            if (best_score < 0) {
                add_diagnostic(result, model::Severity::Error, initializer.range, "no matching Construct overload for member initializer: " + initializer.field_name);
            } else if (best_count > 1) {
                add_diagnostic(result, model::Severity::Error, initializer.range, "ambiguous Construct overload for member initializer: " + initializer.field_name);
            }
        }
    }

    if (decl.has_destruct && method_names.find("Destruct") == method_names.end()) {
        add_diagnostic(result, model::Severity::Warning, decl.range, "class declares Destruct handling without Destruct method");
    }

    for (const auto& field : decl.fields) {
        std::string field_type = field.type.spelling;
        if (field.is_inject) {
            const auto* bind = find_bind_decl(result.program, decl.name, field.name);
            if (bind == nullptr) {
                continue;
            }
            field_type = bind->concrete_type;
        }
        const auto* field_class = find_class_decl(result.program, field_type);
        if (field_class == nullptr) {
            continue;
        }

        bool has_zero_arg_construct = false;
        for (const auto& ctor : field_class->constructors) {
            if (ctor.parameters.empty()) {
                has_zero_arg_construct = true;
                break;
            }
        }

        if (!field_class->constructors.empty() && !has_zero_arg_construct) {
            bool explicitly_initialized_somewhere = false;
            for (const auto& ctor : decl.constructors) {
                for (const auto& initializer : ctor.member_initializers) {
                    if (initializer.field_name == field.name) {
                        explicitly_initialized_somewhere = true;
                        break;
                    }
                }
                if (explicitly_initialized_somewhere) {
                    break;
                }
            }
            if (!explicitly_initialized_somewhere) {
                add_diagnostic(
                    result,
                    model::Severity::Error,
                    field.range,
                    "class field requires zero-argument Construct or explicit member initializer: " + field_type);
            }
        }
    }

    validate_interface_fulfillment(decl, result.program, result);
}

void Analyzer::validate_bindings(const model::Program& program, AnalysisResult& result) const {
    std::unordered_map<std::string, const model::BindDecl*> bindings;

    for (const auto& decl : program.declarations) {
        const auto* bind = std::get_if<model::BindDecl>(&decl);
        if (bind == nullptr) {
            continue;
        }

        const std::string slot_key = bind->owner_type + "." + bind->slot_name;
        if (!bindings.emplace(slot_key, bind).second) {
            add_diagnostic(result, model::Severity::Error, bind->range, "duplicate bind for slot: " + slot_key);
            continue;
        }

        const auto* owner_class = find_class_decl(program, bind->owner_type);
        if (owner_class == nullptr) {
            add_diagnostic(result, model::Severity::Error, bind->range, "bind references unknown owner class: " + bind->owner_type);
            continue;
        }

        const model::FieldDecl* slot_field = nullptr;
        for (const auto& field : owner_class->fields) {
            if (field.name == bind->slot_name) {
                slot_field = &field;
                break;
            }
        }
        if (slot_field == nullptr) {
            add_diagnostic(result, model::Severity::Error, bind->range, "bind references unknown slot: " + slot_key);
            continue;
        }
        if (!slot_field->is_inject) {
            add_diagnostic(result, model::Severity::Error, bind->range, "bind target is not an inject slot: " + slot_key);
            continue;
        }

        const auto* interface_decl = find_interface_decl(program, slot_field->type.spelling);
        if (interface_decl == nullptr) {
            add_diagnostic(result, model::Severity::Error, slot_field->range, "inject slot must use an interface type: " + slot_field->type.spelling);
            continue;
        }

        const auto* concrete_class = find_class_decl(program, bind->concrete_type);
        if (concrete_class == nullptr) {
            add_diagnostic(result, model::Severity::Error, bind->range, "bind references unknown concrete class: " + bind->concrete_type);
            continue;
        }

        bool implements_interface = false;
        for (const auto& implemented : concrete_class->implements) {
            if (implemented == interface_decl->name) {
                implements_interface = true;
                break;
            }
        }
        if (!implements_interface) {
            add_diagnostic(result, model::Severity::Error, bind->range, "bound class does not implement interface " + interface_decl->name + ": " + bind->concrete_type);
        }
    }

    for (const auto& decl : program.declarations) {
        const auto* klass = std::get_if<model::ClassDecl>(&decl);
        if (klass == nullptr) {
            continue;
        }
        for (const auto& field : klass->fields) {
            if (!field.is_inject) {
                continue;
            }
            const std::string slot_key = klass->name + "." + field.name;
            if (bindings.find(slot_key) == bindings.end()) {
                add_diagnostic(result, model::Severity::Error, field.range, "missing bind for inject slot: " + slot_key);
            }
        }
    }
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

void Analyzer::validate_missing_free_function_definitions(const model::Program& program, AnalysisResult& result) const {
    for (const auto& decl : program.declarations) {
        const auto* fn = std::get_if<model::FunctionDecl>(&decl);
        if (fn == nullptr || !fn->owner_type_path.empty() || !fn->body_source.empty()) {
            continue;
        }

        bool found_definition = false;
        for (const auto& candidate_decl : program.declarations) {
            const auto* candidate = std::get_if<model::FunctionDecl>(&candidate_decl);
            if (candidate == nullptr || !candidate->owner_type_path.empty() || candidate->body_source.empty()) {
                continue;
            }
            if (candidate->namespace_path == fn->namespace_path && signatures_match(candidate->signature, fn->signature)) {
                found_definition = true;
                break;
            }
        }

        if (!found_definition) {
            add_diagnostic(result, model::Severity::Error, fn->range, "missing function definition: " + fn->signature.name);
        }
    }
}

void Analyzer::validate_function_body_restrictions(const model::FunctionDecl& decl, AnalysisResult& result) const {
    if (decl.signature.is_export_c) {
        if (!decl.owner_type_path.empty()) {
            add_diagnostic(result, model::Severity::Error, decl.range, "export_c is only allowed on free functions");
        }
        if (decl.signature.is_static) {
            add_diagnostic(result, model::Severity::Error, decl.range, "export_c cannot be combined with static");
        }
    }

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
    if (expected == "CamelCase" && (name == "Construct" || name == "Destruct")) {
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
