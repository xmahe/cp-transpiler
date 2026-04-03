#pragma once

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace cplus::model {

struct SourceLocation {
    std::string file;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct SourceRange {
    SourceLocation begin;
    SourceLocation end;
};

enum class Severity {
    Note,
    Warning,
    Error,
};

struct Diagnostic {
    Severity severity = Severity::Note;
    SourceRange range{};
    std::string message;
};

struct TypeRef {
    std::string spelling;
};

struct Parameter {
    std::string name;
    TypeRef type;
    SourceRange range{};
};

struct MemberInitializer {
    std::string field_name;
    std::vector<std::string> arguments;
    SourceRange range{};
};

struct FunctionSignature {
    std::string name;
    TypeRef return_type;
    std::vector<Parameter> parameters;
    std::vector<MemberInitializer> member_initializers;
    std::string body_source;
    bool is_static = false;
    bool is_implementation = false;
    bool is_export_c = false;
    bool is_private = false;
    SourceRange range{};
};

struct FieldDecl {
    std::string name;
    TypeRef type;
    bool is_static = false;
    bool is_inject = false;
    bool is_private_intent = false;
    SourceRange range{};
};

struct BindDecl {
    std::string owner_type;
    std::string slot_name;
    std::string concrete_type;
    std::vector<std::string> namespace_path;
    SourceRange range{};
};

struct EnumDecl {
    std::string name;
    std::vector<std::string> namespace_path;
    std::vector<std::string> members;
    SourceRange range{};
};

struct InterfaceDecl {
    std::string name;
    std::vector<std::string> namespace_path;
    std::vector<FunctionSignature> methods;
    SourceRange range{};
};

struct ClassDecl {
    std::string name;
    bool is_struct = false;
    std::vector<std::string> namespace_path;
    std::vector<std::string> implements;
    std::vector<FieldDecl> fields;
    std::vector<FieldDecl> static_fields;
    std::vector<FunctionSignature> methods;
    std::vector<FunctionSignature> constructors;
    bool has_destruct = false;
    SourceRange range{};
};

struct FunctionDecl {
    std::vector<std::string> owner_type_path;
    FunctionSignature signature;
    std::string body_source;
    std::vector<std::string> namespace_path;
    SourceRange range{};
};

struct RawCDecl {
    std::string text;
    SourceRange range{};
};

struct NamespaceDecl {
    std::vector<std::string> path;
    bool is_top_level = false;
    SourceRange range{};
};

using Declaration = std::variant<NamespaceDecl, EnumDecl, InterfaceDecl, ClassDecl, FunctionDecl, RawCDecl, BindDecl>;

struct Program {
    std::vector<Declaration> declarations;
};

struct Module {
    std::string stem;
    bool has_header = false;
    bool has_source = false;
    Program header;
    Program source;
};

} // namespace cplus::model
