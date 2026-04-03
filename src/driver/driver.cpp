#include "driver.h"

#include "emit/emit_c.h"
#include "lower/lower.h"
#include "lex/lexer.h"
#include "parse/parser.h"
#include "sema/analyze.h"
#include "support/files.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <unordered_set>
#include <utility>

namespace cplus::driver {

namespace {

cplus::model::SourceRange to_source_range(const cplus::ast::Span& span, const std::filesystem::path& file) {
    return {
        {file.string(), span.begin.line, span.begin.column},
        {file.string(), span.end.line, span.end.column},
    };
}

cplus::model::TypeRef to_model_type(const cplus::ast::TypeRef& type) {
    return {type.spelling};
}

cplus::model::Parameter to_model_parameter(const cplus::ast::Parameter& param, const std::filesystem::path& file) {
    return {param.name, to_model_type(param.type), to_source_range(param.span, file)};
}

cplus::model::MemberInitializer to_model_member_initializer(const cplus::ast::MemberInitializer& initializer, const std::filesystem::path& file) {
    return {initializer.field_name, initializer.arguments, to_source_range(initializer.span, file)};
}

cplus::model::FunctionSignature to_model_signature(const cplus::ast::MethodDecl& method, const std::filesystem::path& file) {
    cplus::model::FunctionSignature sig;
    sig.name = method.name;
    sig.return_type = to_model_type(method.return_type);
    sig.is_static = method.is_static;
    sig.is_implementation = method.is_implementation;
    sig.is_export_c = method.is_export_c;
    sig.is_private = method.is_private;
    for (const auto& param : method.parameters) {
        sig.parameters.push_back(to_model_parameter(param, file));
    }
    for (const auto& initializer : method.member_initializers) {
        sig.member_initializers.push_back(to_model_member_initializer(initializer, file));
    }
    sig.range = to_source_range(method.span, file);
    return sig;
}

cplus::model::FieldDecl to_model_field(const cplus::ast::FieldDecl& field, const std::filesystem::path& file) {
    return {
        field.name,
        to_model_type(field.type),
        field.is_static,
        field.is_inject,
        field.is_private_intent,
        to_source_range(field.span, file),
    };
}

bool signatures_match(const cplus::model::FunctionSignature& lhs, const cplus::model::FunctionSignature& rhs) {
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

cplus::model::ClassDecl* find_class_decl(
    cplus::model::Program& program,
    const std::vector<std::string>& namespace_path,
    const std::string& class_name) {
    for (auto& decl : program.declarations) {
        if (auto* klass = std::get_if<cplus::model::ClassDecl>(&decl)) {
            if (klass->namespace_path == namespace_path && klass->name == class_name) {
                return klass;
            }
        }
    }
    return nullptr;
}

cplus::model::FunctionDecl* find_free_function_decl(
    cplus::model::Program& program,
    const std::vector<std::string>& namespace_path,
    const cplus::model::FunctionSignature& signature) {
    for (auto& decl : program.declarations) {
        if (auto* function = std::get_if<cplus::model::FunctionDecl>(&decl)) {
            if (!function->owner_type_path.empty()) {
                continue;
            }
            if (function->namespace_path == namespace_path && signatures_match(function->signature, signature)) {
                return function;
            }
        }
    }
    return nullptr;
}

void merge_free_function_definition(cplus::model::Program& out, cplus::model::FunctionDecl model_decl) {
    if (auto* existing = find_free_function_decl(out, model_decl.namespace_path, model_decl.signature)) {
        if (existing->body_source.empty() && !model_decl.body_source.empty()) {
            existing->body_source = model_decl.body_source;
            existing->signature.body_source = model_decl.body_source;
            existing->range = model_decl.range;
        }
        return;
    }
    out.declarations.push_back(std::move(model_decl));
}

void merge_method_definition(
    cplus::model::Program& out,
    const std::vector<std::string>& namespace_path,
    const cplus::ast::FunctionDecl& node,
    const std::filesystem::path& source_path,
    support::Diagnostics& diagnostics) {
    if (node.name.parts.size() != 2) {
        diagnostics.error("qualified function definitions may only target a single owning type", node.span, source_path.string());
        return;
    }

    auto* klass = find_class_decl(out, namespace_path, node.name.parts.front());
    if (klass == nullptr) {
        diagnostics.error("out-of-class method definition has no matching class: " + node.name.parts.front(), node.span, source_path.string());
        return;
    }

    auto definition = to_model_signature(node.signature, source_path);
    definition.body_source = node.body_source;

    auto attach_body = [&](std::vector<cplus::model::FunctionSignature>& signatures) -> bool {
        for (auto& signature : signatures) {
            if (signatures_match(signature, definition)) {
                if (signature.body_source.empty()) {
                    signature.body_source = definition.body_source;
                    signature.member_initializers = definition.member_initializers;
                    signature.range = definition.range;
                }
                return true;
            }
        }
        return false;
    };

    if (attach_body(klass->methods) || attach_body(klass->constructors)) {
        return;
    }

    diagnostics.error(
        "out-of-class method definition has no matching declaration: " + node.name.parts.front() + "::" + node.name.parts.back(),
        node.span,
        source_path.string());
}

void ast_to_model_program(
    const std::vector<cplus::ast::Declaration>& declarations,
    const std::filesystem::path& source_path,
    cplus::model::Program& out,
    support::Diagnostics& diagnostics,
    std::vector<std::string> namespace_path = {},
    bool at_top_level = true) {
    for (const auto& decl : declarations) {
        std::visit(
            [&](const auto& node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, cplus::ast::NamespaceDecl>) {
                    auto nested_path = namespace_path;
                    nested_path.insert(nested_path.end(), node.name.parts.begin(), node.name.parts.end());
                    out.declarations.push_back(cplus::model::NamespaceDecl{nested_path, at_top_level, to_source_range(node.span, source_path)});
                    ast_to_model_program(node.declarations, source_path, out, diagnostics, nested_path, false);
                } else if constexpr (std::is_same_v<T, cplus::ast::EnumDecl>) {
                    cplus::model::EnumDecl model_decl;
                    model_decl.name = node.name.parts.empty() ? "" : node.name.parts.back();
                    model_decl.namespace_path = namespace_path;
                    for (const auto& member : node.members) {
                        model_decl.members.push_back(member.name);
                    }
                    model_decl.range = to_source_range(node.span, source_path);
                    out.declarations.push_back(std::move(model_decl));
                } else if constexpr (std::is_same_v<T, cplus::ast::InterfaceDecl>) {
                    cplus::model::InterfaceDecl model_decl;
                    model_decl.name = node.name.parts.empty() ? "" : node.name.parts.back();
                    model_decl.namespace_path = namespace_path;
                    model_decl.range = to_source_range(node.span, source_path);
                    for (const auto& method : node.methods) {
                        model_decl.methods.push_back(to_model_signature(method, source_path));
                    }
                    out.declarations.push_back(std::move(model_decl));
                } else if constexpr (std::is_same_v<T, cplus::ast::ClassDecl>) {
                    cplus::model::ClassDecl model_decl;
                    model_decl.name = node.name.parts.empty() ? "" : node.name.parts.back();
                    model_decl.namespace_path = namespace_path;
                    for (const auto& iface : node.implements) {
                        model_decl.implements.push_back(iface.parts.empty() ? "" : iface.parts.back());
                    }
                    model_decl.range = to_source_range(node.span, source_path);
                    for (const auto& field : node.fields) {
                        model_decl.fields.push_back(to_model_field(field, source_path));
                    }
                    for (const auto& field : node.static_fields) {
                        model_decl.static_fields.push_back(to_model_field(field, source_path));
                    }
                    for (const auto& method : node.methods) {
                        model_decl.methods.push_back(to_model_signature(method, source_path));
                    }
                    for (const auto& ctor : node.constructors) {
                        model_decl.constructors.push_back(to_model_signature(ctor, source_path));
                    }
                    model_decl.has_destroy = node.has_destroy;
                    out.declarations.push_back(std::move(model_decl));
                } else if constexpr (std::is_same_v<T, cplus::ast::FunctionDecl>) {
                    cplus::model::FunctionDecl model_decl;
                    if (node.name.parts.size() > 1) {
                        model_decl.owner_type_path.assign(node.name.parts.begin(), node.name.parts.end() - 1);
                    }
                    model_decl.namespace_path = namespace_path;
                    model_decl.range = to_source_range(node.span, source_path);
                    model_decl.signature = to_model_signature(node.signature, source_path);
                    model_decl.signature.body_source = node.body_source;
                    model_decl.body_source = node.body_source;
                    out.declarations.push_back(std::move(model_decl));
                } else if constexpr (std::is_same_v<T, cplus::ast::RawCDecl>) {
                    out.declarations.push_back(cplus::model::RawCDecl{node.text, to_source_range(node.span, source_path)});
                } else if constexpr (std::is_same_v<T, cplus::ast::BindDecl>) {
                    cplus::model::BindDecl model_decl;
                    model_decl.owner_type = node.owner_type.parts.empty() ? "" : node.owner_type.parts.back();
                    model_decl.slot_name = node.slot_name;
                    model_decl.concrete_type = node.concrete_type.parts.empty() ? "" : node.concrete_type.parts.back();
                    model_decl.namespace_path = namespace_path;
                    model_decl.range = to_source_range(node.span, source_path);
                    out.declarations.push_back(std::move(model_decl));
                }
            },
            decl);
    }
}

void ast_to_model_program_group(
    const std::vector<cplus::ast::Module>& modules,
    cplus::model::Program& out,
    support::Diagnostics& diagnostics) {
    std::optional<std::vector<std::string>> top_level_namespace;
    std::unordered_set<std::string> seen_namespace_paths;

    auto emit_declarations = [&](const auto& self,
                                 const std::vector<cplus::ast::Declaration>& declarations,
                                 const std::filesystem::path& source_path,
                                 std::vector<std::string> namespace_path,
                                 bool at_top_level) -> void {
        for (const auto& decl : declarations) {
            std::visit(
                [&](const auto& node) {
                    using T = std::decay_t<decltype(node)>;
                    if constexpr (std::is_same_v<T, cplus::ast::NamespaceDecl>) {
                        auto nested_path = namespace_path;
                        nested_path.insert(nested_path.end(), node.name.parts.begin(), node.name.parts.end());
                        const auto key = cplus::sema::NameMangler::join(nested_path);
                        if (at_top_level) {
                            if (!top_level_namespace.has_value()) {
                                top_level_namespace = nested_path;
                                if (seen_namespace_paths.insert(key).second) {
                                    out.declarations.push_back(cplus::model::NamespaceDecl{nested_path, true, to_source_range(node.span, source_path)});
                                }
                            } else if (*top_level_namespace != nested_path) {
                                diagnostics.error("all .hp/.cp files in a module must share the same top-level namespace", node.span, source_path.string());
                            } else if (seen_namespace_paths.insert(key).second) {
                                out.declarations.push_back(cplus::model::NamespaceDecl{nested_path, true, to_source_range(node.span, source_path)});
                            }
                        } else if (seen_namespace_paths.insert(key).second) {
                            out.declarations.push_back(cplus::model::NamespaceDecl{nested_path, false, to_source_range(node.span, source_path)});
                        }
                        self(self, node.declarations, source_path, nested_path, false);
                    } else if constexpr (std::is_same_v<T, cplus::ast::EnumDecl>) {
                        cplus::model::EnumDecl model_decl;
                        model_decl.name = node.name.parts.empty() ? "" : node.name.parts.back();
                        model_decl.namespace_path = namespace_path;
                        for (const auto& member : node.members) {
                            model_decl.members.push_back(member.name);
                        }
                        model_decl.range = to_source_range(node.span, source_path);
                        out.declarations.push_back(std::move(model_decl));
                    } else if constexpr (std::is_same_v<T, cplus::ast::InterfaceDecl>) {
                        cplus::model::InterfaceDecl model_decl;
                        model_decl.name = node.name.parts.empty() ? "" : node.name.parts.back();
                        model_decl.namespace_path = namespace_path;
                        model_decl.range = to_source_range(node.span, source_path);
                        for (const auto& method : node.methods) {
                            model_decl.methods.push_back(to_model_signature(method, source_path));
                        }
                        out.declarations.push_back(std::move(model_decl));
                    } else if constexpr (std::is_same_v<T, cplus::ast::ClassDecl>) {
                        cplus::model::ClassDecl model_decl;
                        model_decl.name = node.name.parts.empty() ? "" : node.name.parts.back();
                        model_decl.namespace_path = namespace_path;
                        for (const auto& iface : node.implements) {
                            model_decl.implements.push_back(iface.parts.empty() ? "" : iface.parts.back());
                        }
                        model_decl.range = to_source_range(node.span, source_path);
                        for (const auto& field : node.fields) {
                            model_decl.fields.push_back(to_model_field(field, source_path));
                        }
                        for (const auto& field : node.static_fields) {
                            model_decl.static_fields.push_back(to_model_field(field, source_path));
                        }
                        for (const auto& method : node.methods) {
                            model_decl.methods.push_back(to_model_signature(method, source_path));
                        }
                        for (const auto& ctor : node.constructors) {
                            model_decl.constructors.push_back(to_model_signature(ctor, source_path));
                        }
                        model_decl.has_destroy = node.has_destroy;
                        out.declarations.push_back(std::move(model_decl));
                    } else if constexpr (std::is_same_v<T, cplus::ast::FunctionDecl>) {
                        if (node.name.parts.size() > 1) {
                            merge_method_definition(out, namespace_path, node, source_path, diagnostics);
                        } else {
                            cplus::model::FunctionDecl model_decl;
                            model_decl.namespace_path = namespace_path;
                            model_decl.range = to_source_range(node.span, source_path);
                            model_decl.signature = to_model_signature(node.signature, source_path);
                            model_decl.signature.body_source = node.body_source;
                            model_decl.body_source = node.body_source;
                            merge_free_function_definition(out, std::move(model_decl));
                        }
                    } else if constexpr (std::is_same_v<T, cplus::ast::RawCDecl>) {
                        out.declarations.push_back(cplus::model::RawCDecl{node.text, to_source_range(node.span, source_path)});
                    } else if constexpr (std::is_same_v<T, cplus::ast::BindDecl>) {
                        cplus::model::BindDecl model_decl;
                        model_decl.owner_type = node.owner_type.parts.empty() ? "" : node.owner_type.parts.back();
                        model_decl.slot_name = node.slot_name;
                        model_decl.concrete_type = node.concrete_type.parts.empty() ? "" : node.concrete_type.parts.back();
                        model_decl.namespace_path = namespace_path;
                        model_decl.range = to_source_range(node.span, source_path);
                        out.declarations.push_back(std::move(model_decl));
                    }
                },
                decl);
        }
    };

    for (const auto& module : modules) {
        emit_declarations(emit_declarations, module.declarations, module.source_path, {}, true);
    }
}

void transfer_analysis_diagnostics(const cplus::sema::AnalysisResult& analysis, support::Diagnostics& diagnostics) {
    for (const auto& diag : analysis.diagnostics) {
        cplus::ast::Span span;
        span.begin.line = diag.range.begin.line;
        span.begin.column = diag.range.begin.column;
        span.end.line = diag.range.end.line;
        span.end.column = diag.range.end.column;
        switch (diag.severity) {
        case cplus::model::Severity::Error:
            diagnostics.error(diag.message, span, diag.range.begin.file);
            break;
        case cplus::model::Severity::Warning:
            diagnostics.warning(diag.message, span, diag.range.begin.file);
            break;
        case cplus::model::Severity::Note:
            diagnostics.note(diag.message, span, diag.range.begin.file);
            break;
        }
    }
}

void process_compilation_unit(const std::vector<std::filesystem::path>& files, const Options& options, support::Diagnostics& diagnostics) {
    auto ordered_files = files;
    std::sort(
        ordered_files.begin(),
        ordered_files.end(),
        [](const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
            const bool lhs_is_header = lhs.extension() == ".hp";
            const bool rhs_is_header = rhs.extension() == ".hp";
            if (lhs_is_header != rhs_is_header) {
                return lhs_is_header;
            }
            return lhs.string() < rhs.string();
        });

    std::vector<cplus::ast::Module> modules;
    modules.reserve(ordered_files.size());

    for (const auto& file : ordered_files) {
        std::string source;
        if (!support::read_text_file(file, source)) {
            diagnostics.error("failed to read file", {}, file.string());
            return;
        }

        auto module = parse::parse_module(std::move(source), file, diagnostics);
        if (options.verbose) {
            std::cout << file << ": " << module.declarations.size() << " top-level declarations\n";
        }
        modules.push_back(std::move(module));
    }

    cplus::model::Program program;
    ast_to_model_program_group(modules, program, diagnostics);

    cplus::sema::Analyzer analyzer;
    auto analysis = analyzer.analyze(program);
    transfer_analysis_diagnostics(analysis, diagnostics);
    if (diagnostics.has_errors()) {
        return;
    }

    cplus::lower::Lowerer lowerer;
    auto lowered = lowerer.lower(analysis);
    lowered.header_name = ordered_files.front().stem().string() + ".h";
    lowered.source_name = ordered_files.front().stem().string() + ".c";

    cplus::emit::CEmitter emitter;
    std::filesystem::create_directories(options.output_dir);
    if (!emitter.write_header(lowered, options.output_dir / lowered.header_name)) {
        diagnostics.error("failed to write generated header", {}, (options.output_dir / lowered.header_name).string());
    }
    if (!emitter.write_source(lowered, options.output_dir / lowered.source_name)) {
        diagnostics.error("failed to write generated source", {}, (options.output_dir / lowered.source_name).string());
    }
}

} // namespace

int run_transpiler(const Options& options, support::Diagnostics& diagnostics) {
    if (options.inputs.empty()) {
        diagnostics.error("no input files provided");
        return 1;
    }

    const auto files = support::discover_source_files(options.inputs);
    if (files.empty()) {
        diagnostics.error("no .cp or .hp files found");
        return 1;
    }

    std::map<std::filesystem::path, std::vector<std::filesystem::path>> grouped;
    for (const auto& file : files) {
        grouped[file.parent_path() / file.stem()].push_back(file);
    }

    for (const auto& [key, grouped_files] : grouped) {
        (void)key;
        process_compilation_unit(grouped_files, options, diagnostics);
    }

    return diagnostics.has_errors() ? 1 : 0;
}

} // namespace cplus::driver
