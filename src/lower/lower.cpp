#include "lower.h"

#include "../lex/lexer.h"
#include "../sema/symbols.h"
#include "../support/strings.h"

#include <cctype>
#include <algorithm>
#include <map>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace cplus::lower {

namespace {

struct LocalLifetime {
    std::string name;
    std::string type_name;
    std::size_t token_index = 0;
    int scope_id = 0;
};

struct ReturnSite {
    std::size_t token_index = 0;
    std::size_t statement_end_index = 0;
    int scope_id = 0;
    std::optional<std::string> expression_text;
    std::vector<std::size_t> active_local_indexes;
};

bool is_scope_visible(const int local_scope, int current_scope, const std::unordered_map<int, int>& parents) {
    int scope = current_scope;
    while (true) {
        if (scope == local_scope) {
            return true;
        }
        const auto it = parents.find(scope);
        if (it == parents.end() || it->second == scope) {
            break;
        }
        scope = it->second;
    }
    return false;
}

} // namespace

CModule Lowerer::lower(const cplus::sema::AnalysisResult& analysis) const {
    CModule module;
    module.header_name = "generated.h";
    module.source_name = "generated.c";
    std::unordered_set<std::string> seen_maybe_types;
    std::unordered_map<std::string, std::string> class_name_map;
    std::unordered_set<std::string> enum_names;
    std::unordered_set<std::string> destructible_class_names;
    std::unordered_set<std::string> default_constructible_class_names;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> inject_bindings;

    for (const auto& decl : analysis.program.declarations) {
        if (const auto* klass = std::get_if<cplus::model::ClassDecl>(&decl)) {
            class_name_map.emplace(klass->name, cplus::sema::NameMangler::mangle(klass->namespace_path, klass->name));
            for (const auto& ctor : klass->constructors) {
                if (ctor.parameters.empty()) {
                    default_constructible_class_names.insert(klass->name);
                }
            }
            if (klass->has_destruct) {
                destructible_class_names.insert(klass->name);
            }
        } else if (const auto* enumeration = std::get_if<cplus::model::EnumDecl>(&decl)) {
            enum_names.insert(enumeration->name);
        } else if (const auto* bind = std::get_if<cplus::model::BindDecl>(&decl)) {
            inject_bindings[bind->owner_type][bind->slot_name] = bind->concrete_type;
        }
    }

    auto ensure_maybe = [&](const cplus::model::TypeRef& type) {
        if (!is_maybe_type(type.spelling)) {
            return;
        }
        const auto maybe_name = maybe_type_name(type.spelling);
        if (seen_maybe_types.insert(maybe_name).second) {
            module.maybe_types.push_back(lower_maybe(type.spelling, class_name_map));
        }
    };

    for (const auto& decl : analysis.program.declarations) {
        std::visit(
            [&](const auto& node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, cplus::model::ClassDecl>) {
                    std::unordered_set<std::string> method_names;
                    for (const auto& field : node.fields) {
                        ensure_maybe(field.type);
                    }
                    for (const auto& field : node.static_fields) {
                        ensure_maybe(field.type);
                    }
                    for (const auto& method : node.methods) {
                        method_names.insert(method.name);
                        ensure_maybe(method.return_type);
                        for (const auto& param : method.parameters) {
                            ensure_maybe(param.type);
                        }
                    }
                    for (const auto& ctor : node.constructors) {
                        method_names.insert(ctor.name);
                        ensure_maybe(ctor.return_type);
                        for (const auto& param : ctor.parameters) {
                            ensure_maybe(param.type);
                        }
                    }
                    const auto lowered_class = lower_class(node, class_name_map, inject_bindings);
                    module.structs.push_back(lowered_class);
                    for (const auto& global : lower_static_fields(node, class_name_map, inject_bindings)) {
                        module.globals.push_back(global);
                    }
                    for (const auto& method : node.methods) {
                        module.functions.push_back(lower_method(method, lowered_class.name, node.name, node.fields, method_names, class_name_map, enum_names, inject_bindings, default_constructible_class_names, destructible_class_names));
                    }
                    for (const auto& ctor : node.constructors) {
                        module.functions.push_back(lower_method(ctor, lowered_class.name, node.name, node.fields, method_names, class_name_map, enum_names, inject_bindings, default_constructible_class_names, destructible_class_names));
                    }
                } else if constexpr (std::is_same_v<T, cplus::model::EnumDecl>) {
                    module.enums.push_back(lower_enum(node));
                } else if constexpr (std::is_same_v<T, cplus::model::FunctionDecl>) {
                    ensure_maybe(node.signature.return_type);
                    for (const auto& param : node.signature.parameters) {
                        ensure_maybe(param.type);
                    }
                    module.functions.push_back(lower_function(node, class_name_map, enum_names, default_constructible_class_names, destructible_class_names));
                } else if constexpr (std::is_same_v<T, cplus::model::InterfaceDecl>) {
                    for (const auto& method : node.methods) {
                        ensure_maybe(method.return_type);
                        for (const auto& param : method.parameters) {
                            ensure_maybe(param.type);
                        }
                    }
                } else if constexpr (std::is_same_v<T, cplus::model::NamespaceDecl>) {
                    (void)node;
                } else if constexpr (std::is_same_v<T, cplus::model::RawCDecl>) {
                    const auto text = support::trim(node.text);
                    if (!text.empty()) {
                        module.source_prelude_lines.push_back(text);
                    }
                }
            },
            decl);
    }

    return module;
}

CStruct Lowerer::lower_class(
    const cplus::model::ClassDecl& decl,
    const std::unordered_map<std::string, std::string>& class_name_map,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings) const {
    CStruct result;
    result.name = cplus::sema::NameMangler::mangle(decl.namespace_path, decl.name);
    result.fields.reserve(decl.fields.size());
    for (const auto& field : decl.fields) {
        auto resolved_field = field;
        if (const auto resolved_type = resolve_field_class_name(field, decl.name, inject_bindings); !resolved_type.empty()) {
            resolved_field.type.spelling = resolved_type;
        }
        result.fields.push_back(CField{field.name, to_c_type(resolved_field.type, class_name_map), field.is_private_intent, false});
    }
    return result;
}

std::vector<CGlobal> Lowerer::lower_static_fields(
    const cplus::model::ClassDecl& decl,
    const std::unordered_map<std::string, std::string>& class_name_map,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings) const {
    std::vector<CGlobal> globals;
    globals.reserve(decl.static_fields.size());
    const auto class_name = cplus::sema::NameMangler::mangle(decl.namespace_path, decl.name);
    for (const auto& field : decl.static_fields) {
        auto resolved_field = field;
        if (const auto resolved_type = resolve_field_class_name(field, decl.name, inject_bindings); !resolved_type.empty()) {
            resolved_field.type.spelling = resolved_type;
        }
        globals.push_back(CGlobal{
            class_name + "___" + field.name,
            to_c_type(resolved_field.type, class_name_map),
            CLinkage::Internal,
            {},
        });
    }
    return globals;
}

CEnum Lowerer::lower_enum(const cplus::model::EnumDecl& decl) const {
    return CEnum{cplus::sema::NameMangler::mangle(decl.namespace_path, decl.name), decl.members};
}

CFunction Lowerer::lower_function(
    const cplus::model::FunctionDecl& decl,
    const std::unordered_map<std::string, std::string>& class_name_map,
    const std::unordered_set<std::string>& enum_names,
    const std::unordered_set<std::string>& default_constructible_class_names,
    const std::unordered_set<std::string>& destructible_class_names) const {
    CFunction result;
    result.name = decl.signature.is_export_c
        ? decl.signature.name
        : cplus::sema::NameMangler::mangle(decl.namespace_path, decl.signature.name);
    result.return_type = to_c_type(decl.signature.return_type, class_name_map);
    for (const auto& param : decl.signature.parameters) {
        result.parameters.push_back(CParameter{param.name, to_c_type(param.type, class_name_map)});
    }
    const auto constructor_body = rewrite_local_class_constructors(decl.body_source, class_name_map, default_constructible_class_names);
    const auto raii_body = rewrite_returns_with_raii(constructor_body, result.return_type, class_name_map, destructible_class_names);
    const auto local_names = collect_local_names(raii_body);
    const auto local_object_types = collect_local_object_types(raii_body, class_name_map);
    result.body_lines = lower_body_lines(rewrite_method_body(raii_body, "", {}, local_names, {}, enum_names, {}, local_object_types));
    result.linkage = decl.signature.is_static ? CLinkage::Internal : CLinkage::External;
    return result;
}

CFunction Lowerer::lower_method(
    const cplus::model::FunctionSignature& sig,
    std::string_view class_name,
    std::string_view source_class_name,
    const std::vector<cplus::model::FieldDecl>& instance_fields,
    const std::unordered_set<std::string>& method_names,
    const std::unordered_map<std::string, std::string>& class_name_map,
    const std::unordered_set<std::string>& enum_names,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings,
    const std::unordered_set<std::string>& default_constructible_class_names,
    const std::unordered_set<std::string>& destructible_class_names) const {
    CFunction result;
    result.name = std::string(class_name) + "___" + sig.name;
    result.return_type = to_c_type(sig.return_type, class_name_map);
    if (!sig.is_static) {
        result.parameters.push_back(CParameter{"self", CType{std::string(class_name) + "*"}});
    }
    for (const auto& param : sig.parameters) {
        result.parameters.push_back(CParameter{param.name, to_c_type(param.type, class_name_map)});
    }
    std::string method_body_source = sig.body_source;
    if (sig.name == "Construct") {
        std::vector<std::string> construct_lines;
        std::unordered_map<std::string, const cplus::model::MemberInitializer*> initializer_map;
        for (const auto& initializer : sig.member_initializers) {
            initializer_map[initializer.field_name] = &initializer;
        }
        for (const auto& field : instance_fields) {
            std::string field_type_name = resolve_field_class_name(field, source_class_name, inject_bindings);
            if (field_type_name.empty()) {
                field_type_name = field.type.spelling;
            }
            const auto mangled_it = class_name_map.find(field_type_name);
            if (mangled_it == class_name_map.end()) {
                continue;
            }

            const auto init_it = initializer_map.find(field.name);
            if (init_it != initializer_map.end()) {
                std::ostringstream line;
                line << mangled_it->second << "___Construct(&self->" << field.name;
                for (const auto& arg : init_it->second->arguments) {
                    if (!arg.empty()) {
                        line << ", " << arg;
                    }
                }
                line << ");";
                construct_lines.push_back(line.str());
                continue;
            }

            if (default_constructible_class_names.find(field_type_name) != default_constructible_class_names.end()) {
                construct_lines.push_back(mangled_it->second + "___Construct(&self->" + field.name + ");");
            }
        }
        method_body_source = prepend_lines_to_body(std::move(construct_lines), method_body_source);
    }
    method_body_source = rewrite_local_class_constructors(method_body_source, class_name_map, default_constructible_class_names);
    method_body_source = rewrite_returns_with_raii(
        method_body_source,
        result.return_type,
        class_name_map,
        destructible_class_names,
        sig.name == "Destruct"
            ? member_destruct_calls(instance_fields, source_class_name, class_name_map, inject_bindings, destructible_class_names)
            : std::vector<std::string>{});
    result.body_lines = lower_method_body_lines(
        method_body_source,
        sig,
        instance_fields,
        source_class_name,
        class_name,
        method_names,
        class_name_map,
        enum_names,
        inject_bindings);
    result.linkage = (sig.is_static || sig.is_private) ? CLinkage::Internal : CLinkage::External;
    return result;
}

CMaybeType Lowerer::lower_maybe(std::string_view spelling, const std::unordered_map<std::string, std::string>& class_name_map) const {
    return CMaybeType{maybe_type_name(spelling), to_c_type(cplus::model::TypeRef{maybe_inner_type(spelling)}, class_name_map)};
}

CType Lowerer::to_c_type(const cplus::model::TypeRef& type, const std::unordered_map<std::string, std::string>& class_name_map) {
    if (is_maybe_type(type.spelling)) {
        return CType{maybe_type_name(type.spelling)};
    }
    if (const auto it = class_name_map.find(type.spelling); it != class_name_map.end()) {
        return CType{it->second};
    }
    return CType{type.spelling.empty() ? "void" : type.spelling};
}

bool Lowerer::is_maybe_type(std::string_view spelling) {
    return spelling.rfind("maybe<", 0) == 0 && !spelling.empty() && spelling.back() == '>';
}

std::string Lowerer::maybe_type_name(std::string_view spelling) {
    std::string inner = maybe_inner_type(spelling);
    for (char& ch : inner) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
            ch = '_';
        }
    }
    return "__cplus_maybe_" + inner;
}

std::string Lowerer::maybe_inner_type(std::string_view spelling) {
    if (!is_maybe_type(spelling)) {
        return std::string(spelling);
    }
    return support::trim(spelling.substr(6, spelling.size() - 7));
}

std::string Lowerer::join_path(const std::vector<std::string>& path) {
    return cplus::sema::NameMangler::join(path);
}

std::vector<std::string> Lowerer::lower_body_lines(std::string_view body_source) {
    auto lines = support::split(body_source, '\n');
    std::size_t first = 0;
    std::size_t last = lines.size();

    while (first < last && support::trim(lines[first]).empty()) {
        ++first;
    }
    while (last > first && support::trim(lines[last - 1]).empty()) {
        --last;
    }

    std::size_t common_indent = std::string::npos;
    for (std::size_t i = first; i < last; ++i) {
        const auto trimmed = support::trim(lines[i]);
        if (trimmed.empty()) {
            continue;
        }
        std::size_t indent = 0;
        while (indent < lines[i].size() && (lines[i][indent] == ' ' || lines[i][indent] == '\t')) {
            ++indent;
        }
        common_indent = common_indent == std::string::npos ? indent : std::min(common_indent, indent);
    }

    std::vector<std::string> lowered;
    if (first == last) {
        return lowered;
    }

    for (std::size_t i = first; i < last; ++i) {
        const auto& line = lines[i];
        if (common_indent != std::string::npos && line.size() >= common_indent) {
            lowered.push_back(line.substr(common_indent));
        } else {
            lowered.push_back(line);
        }
    }

    return lowered;
}

std::vector<std::string> Lowerer::lower_method_body_lines(
    std::string_view body_source,
    const cplus::model::FunctionSignature& sig,
    const std::vector<cplus::model::FieldDecl>& instance_fields,
    std::string_view source_class_name,
    std::string_view class_name,
    const std::unordered_set<std::string>& method_names,
    const std::unordered_map<std::string, std::string>& class_name_map,
    const std::unordered_set<std::string>& enum_names,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings) {
    if (sig.is_static || body_source.empty()) {
        return lower_body_lines(body_source);
    }

    std::unordered_set<std::string> field_names;
    field_names.reserve(instance_fields.size());
    for (const auto& field : instance_fields) {
        field_names.insert(field.name);
    }
    std::unordered_map<std::string, std::string> field_object_types;
    for (const auto& field : instance_fields) {
        std::string resolved_type_name = resolve_field_class_name(field, source_class_name, inject_bindings);
        if (resolved_type_name.empty()) {
            resolved_type_name = field.type.spelling;
        }
        const auto it = class_name_map.find(resolved_type_name);
        if (it != class_name_map.end()) {
            field_object_types.emplace(field.name, it->second);
        }
    }

    std::unordered_set<std::string> local_names = collect_local_names(body_source);
    auto local_object_types = collect_local_object_types(body_source, class_name_map);
    for (const auto& param : sig.parameters) {
        local_names.insert(param.name);
    }
    local_names.insert("self");

    return lower_body_lines(rewrite_method_body(body_source, class_name, field_names, local_names, method_names, enum_names, field_object_types, local_object_types));
}

std::string Lowerer::rewrite_method_body(
    std::string_view body_source,
    std::string_view class_name,
    const std::unordered_set<std::string>& field_names,
    const std::unordered_set<std::string>& local_names,
    const std::unordered_set<std::string>& method_names,
    const std::unordered_set<std::string>& enum_names,
    const std::unordered_map<std::string, std::string>& field_object_types,
    const std::unordered_map<std::string, std::string>& local_object_types) {
    if (body_source.empty()) {
        return std::string(body_source);
    }

    const auto tokens = cplus::lex::lex_source(std::string(body_source), "<method-body>");
    std::string rewritten;
    std::size_t cursor = 0;
    bool pending_rewritten_call = false;
    std::string pending_rewritten_call_prefix;
    bool at_statement_start = true;

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.kind == cplus::lex::TokenKind::EndOfFile) {
            break;
        }

        if (token.span.begin.offset > cursor) {
            rewritten.append(body_source.substr(cursor, token.span.begin.offset - cursor));
        }

        if (at_statement_start &&
            token.kind == cplus::lex::TokenKind::Identifier &&
            i + 1 < tokens.size() &&
            tokens[i + 1].kind == cplus::lex::TokenKind::Identifier) {
            auto local_type_it = local_object_types.find(tokens[i + 1].lexeme);
            if (local_type_it != local_object_types.end()) {
                rewritten.append(local_type_it->second);
                cursor = token.span.end.offset;
                at_statement_start = false;
                continue;
            }
        }

        if (token.kind == cplus::lex::TokenKind::Identifier &&
            enum_names.find(token.lexeme) != enum_names.end() &&
            local_names.find(token.lexeme) == local_names.end() &&
            i + 2 < tokens.size() &&
            tokens[i + 1].kind == cplus::lex::TokenKind::Punctuation && tokens[i + 1].lexeme == "::" &&
            tokens[i + 2].kind == cplus::lex::TokenKind::Identifier) {
            rewritten.append(tokens[i + 2].lexeme);
            cursor = tokens[i + 2].span.end.offset;
            i += 2;
            at_statement_start = false;
            continue;
        }

        if (token.kind == cplus::lex::TokenKind::Identifier &&
            i + 3 < tokens.size() &&
            tokens[i + 1].kind == cplus::lex::TokenKind::Punctuation && tokens[i + 1].lexeme == "." &&
            tokens[i + 2].kind == cplus::lex::TokenKind::Identifier &&
            tokens[i + 3].kind == cplus::lex::TokenKind::Punctuation && tokens[i + 3].lexeme == "(") {
            auto field_it = field_object_types.find(token.lexeme);
            auto local_it = local_object_types.find(token.lexeme);
            if (field_it != field_object_types.end() && local_names.find(token.lexeme) == local_names.end()) {
                rewritten.append(field_it->second);
                rewritten.append("___");
                rewritten.append(tokens[i + 2].lexeme);
                pending_rewritten_call = true;
                pending_rewritten_call_prefix = "&self->" + token.lexeme;
                cursor = tokens[i + 2].span.end.offset;
                i += 2;
                continue;
            }
            if (local_it != local_object_types.end()) {
                rewritten.append(local_it->second);
                rewritten.append("___");
                rewritten.append(tokens[i + 2].lexeme);
                pending_rewritten_call = true;
                pending_rewritten_call_prefix = "&" + token.lexeme;
                cursor = tokens[i + 2].span.end.offset;
                i += 2;
                continue;
            }
        }

        bool rewrite_field = false;
        bool rewrite_call = false;
        if (token.kind == cplus::lex::TokenKind::Identifier &&
            field_names.find(token.lexeme) != field_names.end() &&
            local_names.find(token.lexeme) == local_names.end()) {
            const bool preceded_by_member =
                i > 0 && tokens[i - 1].kind == cplus::lex::TokenKind::Punctuation &&
                (tokens[i - 1].lexeme == "." || tokens[i - 1].lexeme == "->" || tokens[i - 1].lexeme == "::");
            const bool followed_by_scope =
                i + 1 < tokens.size() && tokens[i + 1].kind == cplus::lex::TokenKind::Punctuation &&
                tokens[i + 1].lexeme == "::";
            rewrite_field = !preceded_by_member && !followed_by_scope;
        }

        if (token.kind == cplus::lex::TokenKind::Identifier &&
            method_names.find(token.lexeme) != method_names.end() &&
            local_names.find(token.lexeme) == local_names.end()) {
            const bool preceded_by_member =
                i > 0 && tokens[i - 1].kind == cplus::lex::TokenKind::Punctuation &&
                (tokens[i - 1].lexeme == "." || tokens[i - 1].lexeme == "->" || tokens[i - 1].lexeme == "::");
            const bool followed_by_call =
                i + 1 < tokens.size() && tokens[i + 1].kind == cplus::lex::TokenKind::Punctuation &&
                tokens[i + 1].lexeme == "(";
            rewrite_call = !preceded_by_member && followed_by_call;
        }

        if (rewrite_call) {
            rewritten.append(class_name);
            rewritten.append("___");
            rewritten.append(token.lexeme);
            pending_rewritten_call = true;
            pending_rewritten_call_prefix = "self";
            cursor = token.span.end.offset;
            continue;
        }

        if (pending_rewritten_call &&
            token.kind == cplus::lex::TokenKind::Punctuation &&
            token.lexeme == "(") {
            rewritten.append("(");
            rewritten.append(pending_rewritten_call_prefix);
            if (i + 1 < tokens.size() &&
                !(tokens[i + 1].kind == cplus::lex::TokenKind::Punctuation && tokens[i + 1].lexeme == ")")) {
                rewritten.append(", ");
            }
            cursor = token.span.end.offset;
            pending_rewritten_call = false;
            pending_rewritten_call_prefix.clear();
            continue;
        }

        if (rewrite_field) {
            rewritten.append("self->");
        }
        rewritten.append(token.lexeme);
        cursor = token.span.end.offset;
        pending_rewritten_call = false;
        pending_rewritten_call_prefix.clear();
        if (token.kind == cplus::lex::TokenKind::Punctuation &&
            (token.lexeme == ";" || token.lexeme == "{" || token.lexeme == "}")) {
            at_statement_start = true;
        } else {
            at_statement_start = false;
        }
    }

    if (cursor < body_source.size()) {
        rewritten.append(body_source.substr(cursor));
    }

    return rewritten;
}

std::unordered_set<std::string> Lowerer::collect_local_names(std::string_view body_source) {
    std::unordered_set<std::string> names;
    const auto tokens = cplus::lex::lex_source(std::string(body_source), "<method-body>");

    bool at_statement_start = true;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.kind == cplus::lex::TokenKind::EndOfFile) {
            break;
        }

        if (token.kind == cplus::lex::TokenKind::Punctuation && (token.lexeme == ";" || token.lexeme == "{")) {
            at_statement_start = true;
            continue;
        }
        if (token.kind == cplus::lex::TokenKind::Punctuation && token.lexeme == "}") {
            at_statement_start = true;
            continue;
        }

        if (!at_statement_start) {
            continue;
        }

        std::size_t start = i;
        if (tokens[start].kind == cplus::lex::TokenKind::KeywordStatic) {
            ++start;
        }
        if (start >= tokens.size() || tokens[start].kind != cplus::lex::TokenKind::Identifier) {
            at_statement_start = false;
            continue;
        }

        std::size_t end = start;
        bool saw_open_paren = false;
        bool saw_member = false;
        std::size_t identifier_count = 0;
        while (end < tokens.size() && !(tokens[end].kind == cplus::lex::TokenKind::Punctuation && tokens[end].lexeme == ";")) {
            if (tokens[end].kind == cplus::lex::TokenKind::Identifier) {
                ++identifier_count;
            }
            if (tokens[end].kind == cplus::lex::TokenKind::Punctuation &&
                (tokens[end].lexeme == "(" || tokens[end].lexeme == "[" || tokens[end].lexeme == "{")) {
                saw_open_paren = true;
            }
            if (tokens[end].kind == cplus::lex::TokenKind::Punctuation &&
                (tokens[end].lexeme == "." || tokens[end].lexeme == "->" || tokens[end].lexeme == "::")) {
                saw_member = true;
            }
            if (tokens[end].kind == cplus::lex::TokenKind::Punctuation && tokens[end].lexeme == "=") {
                break;
            }
            ++end;
        }

        if (!saw_open_paren && !saw_member && identifier_count >= 2) {
            for (std::size_t j = end; j > start; --j) {
                if (tokens[j - 1].kind == cplus::lex::TokenKind::Identifier) {
                    names.insert(tokens[j - 1].lexeme);
                    break;
                }
            }
        }

        at_statement_start = false;
    }

    return names;
}

std::unordered_map<std::string, std::string> Lowerer::collect_local_object_types(
    std::string_view body_source,
    const std::unordered_map<std::string, std::string>& class_name_map) {
    std::unordered_map<std::string, std::string> local_types;
    const auto tokens = cplus::lex::lex_source(std::string(body_source), "<method-body>");

    bool at_statement_start = true;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.kind == cplus::lex::TokenKind::EndOfFile) {
            break;
        }
        if (token.kind == cplus::lex::TokenKind::Punctuation && (token.lexeme == ";" || token.lexeme == "{" || token.lexeme == "}")) {
            at_statement_start = true;
            continue;
        }
        if (!at_statement_start) {
            continue;
        }

        std::size_t start = i;
        if (tokens[start].kind == cplus::lex::TokenKind::KeywordStatic) {
            ++start;
        }
        if (start + 1 >= tokens.size() || tokens[start].kind != cplus::lex::TokenKind::Identifier || tokens[start + 1].kind != cplus::lex::TokenKind::Identifier) {
            at_statement_start = false;
            continue;
        }

        if (class_name_map.find(tokens[start].lexeme) != class_name_map.end()) {
            local_types.emplace(tokens[start + 1].lexeme, class_name_map.at(tokens[start].lexeme));
        }
        at_statement_start = false;
    }

    return local_types;
}

std::string Lowerer::rewrite_local_class_constructors(
    std::string_view body_source,
    const std::unordered_map<std::string, std::string>& class_name_map,
    const std::unordered_set<std::string>& default_constructible_class_names) {
    if (body_source.empty()) {
        return std::string(body_source);
    }

    const auto tokens = cplus::lex::lex_source(std::string(body_source), "<construct-body>");
    std::string rewritten;
    std::size_t cursor = 0;
    bool at_statement_start = true;

    auto append_original = [&](std::size_t begin, std::size_t end) {
        if (end > begin) {
            rewritten.append(body_source.substr(begin, end - begin));
        }
    };

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.kind == cplus::lex::TokenKind::EndOfFile) {
            break;
        }

        if (!at_statement_start) {
            if (token.kind == cplus::lex::TokenKind::Punctuation &&
                (token.lexeme == ";" || token.lexeme == "{" || token.lexeme == "}")) {
                at_statement_start = true;
            }
            continue;
        }

        std::size_t start = i;
        if (tokens[start].kind == cplus::lex::TokenKind::KeywordStatic) {
            at_statement_start = false;
            continue;
        }
        if (start + 1 >= tokens.size() ||
            tokens[start].kind != cplus::lex::TokenKind::Identifier ||
            tokens[start + 1].kind != cplus::lex::TokenKind::Identifier) {
            at_statement_start = false;
            continue;
        }

        const auto& type_name = tokens[start].lexeme;
        const auto mangled_it = class_name_map.find(type_name);
        if (mangled_it == class_name_map.end()) {
            at_statement_start = false;
            continue;
        }

        const auto& local_name = tokens[start + 1].lexeme;
        std::size_t statement_end = start + 2;
        int paren_depth = 0;
        while (statement_end < tokens.size()) {
            const auto& current = tokens[statement_end];
            if (current.kind == cplus::lex::TokenKind::Punctuation) {
                if (current.lexeme == "(") {
                    ++paren_depth;
                } else if (current.lexeme == ")") {
                    --paren_depth;
                } else if (current.lexeme == ";" && paren_depth == 0) {
                    break;
                } else if ((current.lexeme == "=" || current.lexeme == "{" || current.lexeme == "[") && paren_depth == 0) {
                    statement_end = tokens.size();
                    break;
                }
            }
            ++statement_end;
        }
        if (statement_end >= tokens.size() || tokens[statement_end].lexeme != ";") {
            at_statement_start = false;
            continue;
        }

        std::vector<std::string> args;
        bool has_constructor_sugar = false;
        if (start + 2 < statement_end &&
            tokens[start + 2].kind == cplus::lex::TokenKind::Punctuation &&
            tokens[start + 2].lexeme == "(") {
            has_constructor_sugar = true;
            std::size_t arg_begin = start + 3;
            paren_depth = 0;
            for (std::size_t j = start + 3; j < statement_end; ++j) {
                if (tokens[j].kind == cplus::lex::TokenKind::Punctuation) {
                    if (tokens[j].lexeme == "(") {
                        ++paren_depth;
                    } else if (tokens[j].lexeme == ")") {
                        if (paren_depth == 0) {
                            if (arg_begin < j) {
                                args.push_back(support::trim(std::string(body_source.substr(
                                    tokens[arg_begin].span.begin.offset,
                                    tokens[j - 1].span.end.offset - tokens[arg_begin].span.begin.offset))));
                            }
                            break;
                        }
                        --paren_depth;
                    } else if (tokens[j].lexeme == "," && paren_depth == 0) {
                        if (arg_begin < j) {
                            args.push_back(support::trim(std::string(body_source.substr(
                                tokens[arg_begin].span.begin.offset,
                                tokens[j - 1].span.end.offset - tokens[arg_begin].span.begin.offset))));
                        } else {
                            args.push_back("");
                        }
                        arg_begin = j + 1;
                    }
                }
            }
        }

        if (!has_constructor_sugar && default_constructible_class_names.find(type_name) == default_constructible_class_names.end()) {
            at_statement_start = false;
            continue;
        }

        append_original(cursor, tokens[start].span.begin.offset);
        rewritten.append(type_name);
        rewritten.push_back(' ');
        rewritten.append(local_name);
        rewritten.append(";");
        rewritten.push_back('\n');
        rewritten.append(mangled_it->second);
        rewritten.append("___Construct(&");
        rewritten.append(local_name);
        for (const auto& arg : args) {
            if (!arg.empty()) {
                rewritten.append(", ");
                rewritten.append(arg);
            }
        }
        rewritten.append(");");
        cursor = tokens[statement_end].span.end.offset;
        i = statement_end;
        at_statement_start = true;
    }

    append_original(cursor, body_source.size());
    return rewritten;
}

std::vector<std::string> Lowerer::member_construct_calls(
    const std::vector<cplus::model::FieldDecl>& instance_fields,
    std::string_view source_class_name,
    const std::unordered_map<std::string, std::string>& class_name_map,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings,
    const std::unordered_set<std::string>& default_constructible_class_names) {
    std::vector<std::string> calls;
    for (const auto& field : instance_fields) {
        std::string field_type_name = resolve_field_class_name(field, source_class_name, inject_bindings);
        if (field_type_name.empty()) {
            field_type_name = field.type.spelling;
        }
        if (default_constructible_class_names.find(field_type_name) == default_constructible_class_names.end()) {
            continue;
        }
        const auto mangled_it = class_name_map.find(field_type_name);
        if (mangled_it == class_name_map.end()) {
            continue;
        }
        calls.push_back(mangled_it->second + "___Construct(&self->" + field.name + ");");
    }
    return calls;
}

std::vector<std::string> Lowerer::member_destruct_calls(
    const std::vector<cplus::model::FieldDecl>& instance_fields,
    std::string_view source_class_name,
    const std::unordered_map<std::string, std::string>& class_name_map,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings,
    const std::unordered_set<std::string>& destructible_class_names) {
    std::vector<std::string> calls;
    for (auto it = instance_fields.rbegin(); it != instance_fields.rend(); ++it) {
        std::string field_type_name = resolve_field_class_name(*it, source_class_name, inject_bindings);
        if (field_type_name.empty()) {
            field_type_name = it->type.spelling;
        }
        if (destructible_class_names.find(field_type_name) == destructible_class_names.end()) {
            continue;
        }
        const auto mangled_it = class_name_map.find(field_type_name);
        if (mangled_it == class_name_map.end()) {
            continue;
        }
        calls.push_back(mangled_it->second + "___Destruct(&self->" + it->name + ");");
    }
    return calls;
}

std::string Lowerer::prepend_lines_to_body(std::vector<std::string> lines, std::string_view body_source) {
    if (lines.empty()) {
        return std::string(body_source);
    }
    std::string rewritten;
    for (const auto& line : lines) {
        rewritten.append(line);
        rewritten.push_back('\n');
    }
    rewritten.append(body_source);
    return rewritten;
}

std::string Lowerer::resolve_field_class_name(
    const cplus::model::FieldDecl& field,
    std::string_view source_class_name,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings) {
    if (!field.is_inject) {
        return {};
    }
    const auto owner_it = inject_bindings.find(std::string(source_class_name));
    if (owner_it == inject_bindings.end()) {
        return {};
    }
    const auto slot_it = owner_it->second.find(field.name);
    if (slot_it == owner_it->second.end()) {
        return {};
    }
    return slot_it->second;
}

std::string Lowerer::rewrite_returns_with_raii(
    std::string_view body_source,
    const CType& return_type,
    const std::unordered_map<std::string, std::string>& class_name_map,
    const std::unordered_set<std::string>& destructible_class_names,
    std::vector<std::string> extra_cleanup_lines) {
    if (body_source.empty() || (destructible_class_names.empty() && extra_cleanup_lines.empty())) {
        return std::string(body_source);
    }

    const auto tokens = cplus::lex::lex_source(std::string(body_source), "<raii-body>");
    std::vector<LocalLifetime> locals;
    std::vector<ReturnSite> returns;
    std::unordered_map<int, int> scope_parent;
    std::unordered_map<int, std::size_t> scope_close_token_index;
    std::vector<int> scope_stack{0};
    scope_parent.emplace(0, 0);
    int next_scope_id = 1;

    bool at_statement_start = true;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.kind == cplus::lex::TokenKind::EndOfFile) {
            break;
        }

        if (token.kind == cplus::lex::TokenKind::Punctuation && token.lexeme == "{") {
            const int parent = scope_stack.back();
            scope_stack.push_back(next_scope_id);
            scope_parent.emplace(next_scope_id, parent);
            ++next_scope_id;
            at_statement_start = true;
            continue;
        }
        if (token.kind == cplus::lex::TokenKind::Punctuation && token.lexeme == "}") {
            scope_close_token_index[scope_stack.back()] = i;
            if (scope_stack.size() > 1) {
                scope_stack.pop_back();
            }
            at_statement_start = true;
            continue;
        }
        if (token.kind == cplus::lex::TokenKind::Punctuation && token.lexeme == ";") {
            at_statement_start = true;
            continue;
        }

        if (token.kind == cplus::lex::TokenKind::KeywordReturn) {
            std::size_t end = i + 1;
            while (end < tokens.size() && !(tokens[end].kind == cplus::lex::TokenKind::Punctuation && tokens[end].lexeme == ";")) {
                ++end;
            }
            ReturnSite site;
            site.token_index = i;
            site.statement_end_index = end;
            site.scope_id = scope_stack.back();
            if (i + 1 < end) {
                site.expression_text = std::string(body_source.substr(tokens[i + 1].span.begin.offset, tokens[end - 1].span.end.offset - tokens[i + 1].span.begin.offset));
            }
            for (std::size_t local_index = 0; local_index < locals.size(); ++local_index) {
                const auto& local = locals[local_index];
                if (local.token_index < i && is_scope_visible(local.scope_id, site.scope_id, scope_parent)) {
                    site.active_local_indexes.push_back(local_index);
                }
            }
            returns.push_back(std::move(site));
            at_statement_start = false;
            continue;
        }

        if (at_statement_start) {
            std::size_t start = i;
            if (tokens[start].kind == cplus::lex::TokenKind::KeywordStatic) {
                ++start;
            }
            if (start + 1 < tokens.size() &&
                tokens[start].kind == cplus::lex::TokenKind::Identifier &&
                tokens[start + 1].kind == cplus::lex::TokenKind::Identifier &&
                destructible_class_names.find(tokens[start].lexeme) != destructible_class_names.end()) {
                const auto it = class_name_map.find(tokens[start].lexeme);
                if (it != class_name_map.end()) {
                    locals.push_back(LocalLifetime{tokens[start + 1].lexeme, it->second, i, scope_stack.back()});
                }
            }
        }

        at_statement_start = false;
    }

    bool needs_cleanup = !locals.empty() || !extra_cleanup_lines.empty();
    for (const auto& site : returns) {
        if (!site.active_local_indexes.empty()) {
            needs_cleanup = true;
            break;
        }
    }
    if (!needs_cleanup) {
        return std::string(body_source);
    }

    scope_close_token_index[0] = tokens.size() - 1;

    auto is_tail_return_for_scope = [&](const ReturnSite& site, int scope_id) {
        const auto close_it = scope_close_token_index.find(scope_id);
        if (close_it == scope_close_token_index.end()) {
            return false;
        }
        for (std::size_t token_index = site.statement_end_index + 1; token_index < close_it->second; ++token_index) {
            const auto& trailing = tokens[token_index];
            if (trailing.kind == cplus::lex::TokenKind::EndOfFile) {
                break;
            }
            if (!(trailing.kind == cplus::lex::TokenKind::Punctuation && trailing.lexeme == "}")) {
                return false;
            }
        }
        return true;
    };

    std::unordered_set<int> scopes_terminated_by_tail_return;
    for (const auto& site : returns) {
        int scope_id = site.scope_id;
        while (true) {
            if (is_tail_return_for_scope(site, scope_id)) {
                scopes_terminated_by_tail_return.insert(scope_id);
            } else {
                break;
            }
            const auto parent_it = scope_parent.find(scope_id);
            if (parent_it == scope_parent.end() || parent_it->second == scope_id) {
                break;
            }
            scope_id = parent_it->second;
        }
    }

    std::map<std::vector<std::size_t>, std::string> label_for_set;
    std::unordered_map<int, std::vector<std::size_t>> locals_by_scope;
    int label_counter = 0;
    for (std::size_t local_index = 0; local_index < locals.size(); ++local_index) {
        locals_by_scope[locals[local_index].scope_id].push_back(local_index);
    }
    for (const auto& site : returns) {
        if (site.active_local_indexes.empty()) {
            continue;
        }
        if (label_for_set.find(site.active_local_indexes) == label_for_set.end()) {
            label_for_set.emplace(site.active_local_indexes, "__cleanup_" + std::to_string(label_counter++));
        }
    }

    std::string rewritten;
    if (return_type.spelling != "void") {
        rewritten.append(return_type.spelling);
        rewritten.append(" __cplus_ret;\n");
    }

    std::size_t cursor = 0;
    std::size_t return_site_index = 0;
    std::vector<int> rewrite_scope_stack{0};
    int rewrite_next_scope_id = 1;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.kind == cplus::lex::TokenKind::EndOfFile) {
            break;
        }

        if (return_site_index < returns.size() && returns[return_site_index].token_index == i) {
            const auto& site = returns[return_site_index];
            if (token.span.begin.offset > cursor) {
                rewritten.append(body_source.substr(cursor, token.span.begin.offset - cursor));
            }

            if (site.active_local_indexes.empty()) {
                rewritten.append("return");
                if (site.expression_text.has_value()) {
                    rewritten.push_back(' ');
                    rewritten.append(*site.expression_text);
                }
                rewritten.append(";");
            } else {
                if (return_type.spelling != "void" && site.expression_text.has_value()) {
                    rewritten.append("__cplus_ret = ");
                    rewritten.append(*site.expression_text);
                    rewritten.append(";\n");
                }
                rewritten.append("goto ");
                rewritten.append(label_for_set.at(site.active_local_indexes));
                rewritten.append(";");
            }

            cursor = tokens[site.statement_end_index].span.end.offset;
            i = site.statement_end_index;
            ++return_site_index;
            continue;
        }

        if (token.kind == cplus::lex::TokenKind::Punctuation && token.lexeme == "{") {
            rewrite_scope_stack.push_back(rewrite_next_scope_id++);
            continue;
        }

        if (token.kind == cplus::lex::TokenKind::Punctuation && token.lexeme == "}") {
            if (token.span.begin.offset > cursor) {
                rewritten.append(body_source.substr(cursor, token.span.begin.offset - cursor));
            }
            const int closing_scope = rewrite_scope_stack.back();
            if (scopes_terminated_by_tail_return.find(closing_scope) == scopes_terminated_by_tail_return.end()) {
                if (const auto it = locals_by_scope.find(closing_scope); it != locals_by_scope.end()) {
                    for (auto local_it = it->second.rbegin(); local_it != it->second.rend(); ++local_it) {
                        const auto& local = locals[*local_it];
                        rewritten.append(local.type_name);
                        rewritten.append("___Destruct(&");
                        rewritten.append(local.name);
                        rewritten.append(");\n");
                    }
                }
            }
            rewritten.append(token.lexeme);
            cursor = token.span.end.offset;
            if (rewrite_scope_stack.size() > 1) {
                rewrite_scope_stack.pop_back();
            }
            continue;
        }
    }

    if (cursor < body_source.size()) {
        rewritten.append(body_source.substr(cursor));
    }

    if (const auto it = locals_by_scope.find(0); it != locals_by_scope.end() || !extra_cleanup_lines.empty()) {
        const auto trimmed_rewritten = support::trim(rewritten);
        std::vector<std::size_t> outer_active_set;
        if (it != locals_by_scope.end()) {
            outer_active_set = it->second;
        }
        std::string outer_cleanup_label;
        if (!outer_active_set.empty()) {
            if (const auto label_it = label_for_set.find(outer_active_set); label_it != label_for_set.end()) {
                outer_cleanup_label = label_it->second;
            }
        }

        if (!rewritten.empty() && rewritten.back() != '\n') {
            rewritten.push_back('\n');
        }
        if (!outer_cleanup_label.empty() &&
            !support::ends_with(trimmed_rewritten, "goto " + outer_cleanup_label + ";") &&
            !support::ends_with(trimmed_rewritten, "return;") &&
            !support::ends_with(trimmed_rewritten, "return __cplus_ret;") &&
            scopes_terminated_by_tail_return.find(0) == scopes_terminated_by_tail_return.end()) {
            rewritten.append("goto ");
            rewritten.append(outer_cleanup_label);
            rewritten.append(";\n");
        } else if (outer_cleanup_label.empty() &&
                   !support::ends_with(trimmed_rewritten, "return;") &&
                   !support::ends_with(trimmed_rewritten, "return __cplus_ret;") &&
                   scopes_terminated_by_tail_return.find(0) == scopes_terminated_by_tail_return.end()) {
            if (it != locals_by_scope.end()) {
                for (auto local_it = it->second.rbegin(); local_it != it->second.rend(); ++local_it) {
                    const auto& local = locals[*local_it];
                    rewritten.append(local.type_name);
                    rewritten.append("___Destruct(&");
                    rewritten.append(local.name);
                    rewritten.append(");\n");
                }
            }
            for (const auto& line : extra_cleanup_lines) {
                rewritten.append(line);
                rewritten.push_back('\n');
            }
        }
    }

    if (!rewritten.empty() && rewritten.back() != '\n') {
        rewritten.push_back('\n');
    }

    for (const auto& [active_set, label] : label_for_set) {
        rewritten.append(label);
        rewritten.append(":\n");
        for (auto it = active_set.rbegin(); it != active_set.rend(); ++it) {
            const auto& local = locals[*it];
            rewritten.append(local.type_name);
            rewritten.append("___Destruct(&");
            rewritten.append(local.name);
            rewritten.append(");\n");
        }
        for (const auto& line : extra_cleanup_lines) {
            rewritten.append(line);
            rewritten.push_back('\n');
        }
        if (return_type.spelling == "void") {
            rewritten.append("return;\n");
        } else {
            rewritten.append("return __cplus_ret;\n");
        }
    }

    return rewritten;
}

} // namespace cplus::lower
