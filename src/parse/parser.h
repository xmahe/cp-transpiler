#pragma once

#include "ast/ast.h"
#include "lex/token.h"
#include "support/diagnostics.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace cplus::parse {

class Parser {
public:
    Parser(std::string source, std::vector<lex::Token> tokens, std::filesystem::path source_path, support::Diagnostics& diagnostics);

    ast::Module parse_module();

private:
    const lex::Token& current() const;
    const lex::Token& previous() const;
    bool eof() const;
    bool check(lex::TokenKind kind) const;
    bool match(lex::TokenKind kind);
    const lex::Token& consume(lex::TokenKind kind, std::string message);
    std::string slice(std::size_t begin_offset, std::size_t end_offset) const;
    ast::QualifiedName parse_qualified_name();
    ast::TypeRef parse_type_ref_until(const std::vector<std::string>& terminators);
    ast::Parameter parse_parameter();
    std::vector<ast::Parameter> parse_parameter_list();
    ast::MethodDecl parse_method_signature(bool allow_implementation);
    std::pair<ast::QualifiedName, ast::MethodDecl> parse_function_signature();
    ast::FieldDecl parse_field(bool is_static);
    std::unique_ptr<ast::Expression> parse_expression_until(const std::vector<std::string>& terminators);
    std::unique_ptr<ast::Expression> parse_primary_expression();
    std::unique_ptr<ast::Expression> parse_postfix_expression(std::unique_ptr<ast::Expression> expr);
    bool is_declaration_statement() const;
    std::unique_ptr<ast::Statement> parse_statement();
    std::unique_ptr<ast::Statement> parse_block_statement();
    std::unique_ptr<ast::Statement> parse_return_statement();
    std::unique_ptr<ast::Statement> parse_expression_statement();
    std::unique_ptr<ast::Statement> parse_declaration_statement();
    std::vector<ast::Declaration> parse_block_declarations();
    void parse_class_body(ast::ClassDecl& decl);
    void parse_interface_body(ast::InterfaceDecl& decl);
    std::unique_ptr<ast::BlockStmt> parse_function_body();
    std::size_t find_matching_brace(std::size_t open_index) const;
    ast::Span span_from(std::size_t begin_index, std::size_t end_index) const;
    ast::Declaration parse_namespace();
    ast::Declaration parse_class();
    ast::Declaration parse_interface();
    ast::Declaration parse_enum();
    ast::Declaration parse_function();
    ast::Declaration parse_raw_c();
    std::vector<ast::EnumMember> parse_enum_members(std::size_t open_index, std::size_t close_index);

    std::string source_;
    std::vector<lex::Token> tokens_;
    std::filesystem::path source_path_;
    support::Diagnostics& diagnostics_;
    std::size_t current_index_ = 0;
};

ast::Module parse_module(std::string source, std::filesystem::path source_path, support::Diagnostics& diagnostics);

} // namespace cplus::parse
