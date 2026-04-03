#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace cplus::ast {

struct Location {
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct Span {
    Location begin;
    Location end;
};

struct QualifiedName {
    std::vector<std::string> parts;

    bool empty() const;
    std::string str(std::string_view separator = "::") const;
};

struct EnumMember {
    std::string name;
    Span span{};
};

struct TypeRef {
    std::string spelling;
    Span span{};
};

struct Parameter {
    std::string name;
    TypeRef type;
    Span span{};
};

struct MemberInitializer {
    std::string field_name;
    std::vector<std::string> arguments;
    Span span{};
};

struct MethodDecl {
    std::string name;
    TypeRef return_type;
    std::vector<Parameter> parameters;
    std::vector<MemberInitializer> member_initializers;
    bool is_static = false;
    bool is_implementation = false;
    bool is_export_c = false;
    bool is_private = false;
    Span span{};
};

struct FieldDecl {
    std::string name;
    TypeRef type;
    bool is_static = false;
    bool is_inject = false;
    bool is_private_intent = false;
    Span span{};
};

struct BindDecl {
    QualifiedName owner_type;
    std::string slot_name;
    QualifiedName concrete_type;
    Span span{};
};

struct Expression;
struct Statement;

using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;

struct IdentifierExpr {
    QualifiedName name;
    Span span{};
};

struct LiteralExpr {
    std::string value;
    Span span{};
};

struct RawExpr {
    std::string text;
    Span span{};
};

struct MemberExpr {
    ExpressionPtr object;
    std::string member;
    Span span{};
};

struct CallExpr {
    ExpressionPtr callee;
    std::vector<ExpressionPtr> arguments;
    Span span{};
};

struct AssignmentExpr {
    ExpressionPtr target;
    ExpressionPtr value;
    Span span{};
};

struct BinaryExpr {
    std::string op;
    ExpressionPtr lhs;
    ExpressionPtr rhs;
    Span span{};
};

struct UnaryExpr {
    std::string op;
    ExpressionPtr operand;
    Span span{};
};

using ExpressionNode = std::variant<IdentifierExpr, LiteralExpr, MemberExpr, CallExpr, AssignmentExpr, BinaryExpr, UnaryExpr, RawExpr>;

struct Expression {
    ExpressionNode node;
    Span span{};
};

struct BlockStmt {
    std::vector<StatementPtr> statements;
    Span span{};
};

struct ReturnStmt {
    ExpressionPtr value;
    Span span{};
};

struct ExprStmt {
    ExpressionPtr expression;
    Span span{};
};

struct DeclStmt {
    TypeRef type;
    std::string name;
    ExpressionPtr initializer;
    bool is_static = false;
    Span span{};
};

struct IfStmt {
    ExpressionPtr condition;
    StatementPtr then_branch;
    StatementPtr else_branch;
    Span span{};
};

struct WhileStmt {
    ExpressionPtr condition;
    StatementPtr body;
    Span span{};
};

struct ForStmt {
    StatementPtr init;
    ExpressionPtr condition;
    ExpressionPtr step;
    StatementPtr body;
    Span span{};
};

struct BreakStmt {
    Span span{};
};

struct ContinueStmt {
    Span span{};
};

struct RawStmt {
    std::string text;
    Span span{};
};

using StatementNode = std::variant<BlockStmt, ReturnStmt, ExprStmt, DeclStmt, IfStmt, WhileStmt, ForStmt, BreakStmt, ContinueStmt, RawStmt>;

struct Statement {
    StatementNode node;
    Span span{};
};

struct NamespaceDecl {
    QualifiedName name;
    std::vector<std::variant<NamespaceDecl, struct InterfaceDecl, struct ClassDecl, struct EnumDecl, struct FunctionDecl, struct RawCDecl, BindDecl>> declarations;
    Span span{};
};

struct InterfaceDecl {
    QualifiedName name;
    std::vector<MethodDecl> methods;
    Span span{};
};

struct ClassDecl {
    QualifiedName name;
    std::vector<QualifiedName> implements;
    std::vector<FieldDecl> fields;
    std::vector<FieldDecl> static_fields;
    std::vector<MethodDecl> methods;
    std::vector<MethodDecl> constructors;
    bool has_destruct = false;
    Span span{};
};

struct EnumDecl {
    QualifiedName name;
    std::vector<EnumMember> members;
    Span span{};
};

struct FunctionDecl {
    QualifiedName name;
    MethodDecl signature;
    std::string body_source;
    std::unique_ptr<BlockStmt> body;
    Span span{};
};

struct RawCDecl {
    std::string text;
    Span span{};
};

using Declaration = std::variant<NamespaceDecl, InterfaceDecl, ClassDecl, EnumDecl, FunctionDecl, RawCDecl, BindDecl>;

struct Module {
    std::filesystem::path source_path;
    std::string source_text;
    std::vector<Declaration> declarations;
};

} // namespace cplus::ast
