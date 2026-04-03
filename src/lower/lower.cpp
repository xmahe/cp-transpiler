#include "lower.h"

#include "../lex/lexer.h"
#include "../sema/symbols.h"
#include "../support/strings.h"

#include <cctype>
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace cplus::lower {

CModule Lowerer::lower(const cplus::sema::AnalysisResult& analysis) const {
    CModule module;
    module.header_name = "generated.h";
    module.source_name = "generated.c";
    module.includes.push_back(module.header_name);
    std::unordered_set<std::string> seen_maybe_types;
    std::unordered_map<std::string, std::string> class_name_map;

    for (const auto& decl : analysis.program.declarations) {
        if (const auto* klass = std::get_if<cplus::model::ClassDecl>(&decl)) {
            class_name_map.emplace(klass->name, cplus::sema::NameMangler::mangle(klass->namespace_path, klass->name));
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
                    const auto lowered_class = lower_class(node, class_name_map);
                    module.structs.push_back(lowered_class);
                    for (const auto& global : lower_static_fields(node, class_name_map)) {
                        module.globals.push_back(global);
                    }
                    for (const auto& method : node.methods) {
                        module.functions.push_back(lower_method(method, lowered_class.name, node.fields, method_names, class_name_map));
                    }
                    for (const auto& ctor : node.constructors) {
                        module.functions.push_back(lower_method(ctor, lowered_class.name, node.fields, method_names, class_name_map));
                    }
                } else if constexpr (std::is_same_v<T, cplus::model::EnumDecl>) {
                    module.enums.push_back(lower_enum(node));
                } else if constexpr (std::is_same_v<T, cplus::model::FunctionDecl>) {
                    ensure_maybe(node.signature.return_type);
                    for (const auto& param : node.signature.parameters) {
                        ensure_maybe(param.type);
                    }
                    module.functions.push_back(lower_function(node, class_name_map));
                } else if constexpr (std::is_same_v<T, cplus::model::InterfaceDecl>) {
                    for (const auto& method : node.methods) {
                        ensure_maybe(method.return_type);
                        for (const auto& param : method.parameters) {
                            ensure_maybe(param.type);
                        }
                    }
                    for (const auto& method : node.methods) {
                        module.functions.push_back(
                            lower_method(
                                method,
                                cplus::sema::NameMangler::mangle(node.namespace_path, node.name),
                                {},
                                {},
                                class_name_map));
                    }
                } else if constexpr (std::is_same_v<T, cplus::model::NamespaceDecl>) {
                    (void)node;
                } else if constexpr (std::is_same_v<T, cplus::model::RawCDecl>) {
                    (void)node;
                }
            },
            decl);
    }

    return module;
}

CStruct Lowerer::lower_class(const cplus::model::ClassDecl& decl, const std::unordered_map<std::string, std::string>& class_name_map) const {
    CStruct result;
    result.name = cplus::sema::NameMangler::mangle(decl.namespace_path, decl.name);
    result.fields.reserve(decl.fields.size());
    for (const auto& field : decl.fields) {
        result.fields.push_back(CField{field.name, to_c_type(field.type, class_name_map), field.is_private_intent, false});
    }
    return result;
}

std::vector<CGlobal> Lowerer::lower_static_fields(const cplus::model::ClassDecl& decl, const std::unordered_map<std::string, std::string>& class_name_map) const {
    std::vector<CGlobal> globals;
    globals.reserve(decl.static_fields.size());
    const auto class_name = cplus::sema::NameMangler::mangle(decl.namespace_path, decl.name);
    for (const auto& field : decl.static_fields) {
        globals.push_back(CGlobal{
            class_name + "___" + field.name,
            to_c_type(field.type, class_name_map),
            CLinkage::Internal,
            {},
        });
    }
    return globals;
}

CEnum Lowerer::lower_enum(const cplus::model::EnumDecl& decl) const {
    return CEnum{cplus::sema::NameMangler::mangle(decl.namespace_path, decl.name), decl.members};
}

CFunction Lowerer::lower_function(const cplus::model::FunctionDecl& decl, const std::unordered_map<std::string, std::string>& class_name_map) const {
    CFunction result;
    result.name = cplus::sema::NameMangler::mangle(decl.namespace_path, decl.signature.name);
    result.return_type = to_c_type(decl.signature.return_type, class_name_map);
    for (const auto& param : decl.signature.parameters) {
        result.parameters.push_back(CParameter{param.name, to_c_type(param.type, class_name_map)});
    }
    result.body_lines = lower_body_lines(decl.body_source);
    result.linkage = decl.signature.is_static ? CLinkage::Internal : CLinkage::External;
    return result;
}

CFunction Lowerer::lower_method(
    const cplus::model::FunctionSignature& sig,
    std::string_view class_name,
    const std::vector<cplus::model::FieldDecl>& instance_fields,
    const std::unordered_set<std::string>& method_names,
    const std::unordered_map<std::string, std::string>& class_name_map) const {
    CFunction result;
    result.name = std::string(class_name) + "___" + sig.name;
    result.return_type = to_c_type(sig.return_type, class_name_map);
    if (!sig.is_static) {
        result.parameters.push_back(CParameter{"self", CType{std::string(class_name) + "*"}});
    }
    for (const auto& param : sig.parameters) {
        result.parameters.push_back(CParameter{param.name, to_c_type(param.type, class_name_map)});
    }
    result.body_lines = lower_method_body_lines(sig.body_source, sig, instance_fields, class_name, method_names, class_name_map);
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
    std::string_view class_name,
    const std::unordered_set<std::string>& method_names,
    const std::unordered_map<std::string, std::string>& class_name_map) {
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
        const auto it = class_name_map.find(field.type.spelling);
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

    return lower_body_lines(rewrite_method_body(body_source, class_name, field_names, local_names, method_names, field_object_types, local_object_types));
}

std::string Lowerer::rewrite_method_body(
    std::string_view body_source,
    std::string_view class_name,
    const std::unordered_set<std::string>& field_names,
    const std::unordered_set<std::string>& local_names,
    const std::unordered_set<std::string>& method_names,
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

} // namespace cplus::lower
