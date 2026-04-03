#include "parser.h"

#include "lex/lexer.h"
#include "support/strings.h"

#include <utility>

namespace cplus::parse {

namespace {

bool is_identifier_like(const lex::Token& token) {
    return token.kind == lex::TokenKind::Identifier;
}

bool is_operator_lexeme(std::string_view lexeme) {
    return lexeme == "=" || lexeme == "+" || lexeme == "-" || lexeme == "*" || lexeme == "/" || lexeme == "%" ||
        lexeme == "+=" || lexeme == "-=" || lexeme == "*=" || lexeme == "/=" || lexeme == "%=" ||
        lexeme == "==" || lexeme == "!=" || lexeme == "<" || lexeme == ">" || lexeme == "<=" || lexeme == ">=" ||
        lexeme == "&&" || lexeme == "||";
}

bool is_terminator_lexeme(std::string_view lexeme, const std::vector<std::string>& terminators) {
    for (const auto& term : terminators) {
        if (lexeme == term) {
            return true;
        }
    }
    return false;
}

} // namespace

Parser::Parser(std::string source, std::vector<lex::Token> tokens, std::filesystem::path source_path, support::Diagnostics& diagnostics)
    : source_(std::move(source)),
      tokens_(std::move(tokens)),
      source_path_(std::move(source_path)),
      diagnostics_(diagnostics) {}

const lex::Token& Parser::current() const {
    return tokens_[current_index_];
}

const lex::Token& Parser::previous() const {
    return tokens_[current_index_ - 1];
}

bool Parser::eof() const {
    return check(lex::TokenKind::EndOfFile);
}

bool Parser::check(lex::TokenKind kind) const {
    return current().kind == kind;
}

bool Parser::match(lex::TokenKind kind) {
    if (!check(kind)) {
        return false;
    }
    ++current_index_;
    return true;
}

const lex::Token& Parser::consume(lex::TokenKind kind, std::string message) {
    if (!check(kind)) {
        diagnostics_.error(std::move(message), current().span, source_path_.string());
        return current();
    }
    ++current_index_;
    return previous();
}

std::string Parser::slice(std::size_t begin_offset, std::size_t end_offset) const {
    if (begin_offset > end_offset || end_offset > source_.size()) {
        return {};
    }
    return source_.substr(begin_offset, end_offset - begin_offset);
}

ast::QualifiedName Parser::parse_qualified_name() {
    ast::QualifiedName name;
    if (!is_identifier_like(current())) {
        diagnostics_.error("expected identifier", current().span, source_path_.string());
        return name;
    }

    name.parts.push_back(current().lexeme);
    ++current_index_;
    while (check(lex::TokenKind::Punctuation) && current().lexeme == "::") {
        ++current_index_;
        if (!is_identifier_like(current())) {
            diagnostics_.error("expected identifier after ::", current().span, source_path_.string());
            break;
        }
        name.parts.push_back(current().lexeme);
        ++current_index_;
    }
    return name;
}

ast::TypeRef Parser::parse_type_ref_until(const std::vector<std::string>& terminators) {
    const std::size_t begin = current_index_;
    while (!eof()) {
        bool stop = false;
        for (const auto& term : terminators) {
            if (current().lexeme == term) {
                stop = true;
                break;
            }
        }
        if (stop) {
            break;
        }
        ++current_index_;
    }

    if (begin == current_index_) {
        diagnostics_.error("expected type", current().span, source_path_.string());
        return {"", current().span};
    }

    return {slice(tokens_[begin].span.begin.offset, tokens_[current_index_ - 1].span.end.offset), span_from(begin, current_index_ - 1)};
}

ast::Parameter Parser::parse_parameter() {
    const std::size_t begin = current_index_;
    std::vector<lex::Token> consumed;
    while (!eof() && current().lexeme != "," && current().lexeme != ")") {
        consumed.push_back(current());
        ++current_index_;
    }
    if (consumed.empty()) {
        diagnostics_.error("expected parameter", current().span, source_path_.string());
        return {"", {"", {}}, current().span};
    }

    const auto& last = consumed.back();
    std::string type_text = slice(consumed.front().span.begin.offset, last.span.begin.offset);
    std::string name_text = last.lexeme;
    while (!name_text.empty() && name_text.front() == '*') {
        type_text += "*";
        name_text.erase(name_text.begin());
    }

    return {name_text, {support::trim(type_text), span_from(begin, current_index_ - 1)}, span_from(begin, current_index_ - 1)};
}

std::vector<ast::Parameter> Parser::parse_parameter_list() {
    std::vector<ast::Parameter> params;
    consume(lex::TokenKind::Punctuation, "expected '(' before parameter list");
    while (!eof() && current().lexeme != ")") {
        params.push_back(parse_parameter());
        if (current().lexeme == ",") {
            ++current_index_;
        }
    }
    consume(lex::TokenKind::Punctuation, "expected ')' after parameter list");
    return params;
}

ast::MemberInitializer Parser::parse_member_initializer() {
    const std::size_t start = current_index_;
    ast::MemberInitializer initializer;
    if (!is_identifier_like(current())) {
        diagnostics_.error("expected field name in member initializer", current().span, source_path_.string());
        return initializer;
    }
    initializer.field_name = current().lexeme;
    ++current_index_;
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "(")) {
        diagnostics_.error("expected '(' after member initializer field name", current().span, source_path_.string());
        return initializer;
    }
    ++current_index_;
    while (!eof() && !(check(lex::TokenKind::Punctuation) && current().lexeme == ")")) {
        const std::size_t arg_begin = current_index_;
        while (!eof() &&
               !(check(lex::TokenKind::Punctuation) && (current().lexeme == "," || current().lexeme == ")"))) {
            ++current_index_;
        }
        if (arg_begin < current_index_) {
            initializer.arguments.push_back(support::trim(slice(tokens_[arg_begin].span.begin.offset, tokens_[current_index_ - 1].span.end.offset)));
        } else {
            initializer.arguments.push_back("");
        }
        if (check(lex::TokenKind::Punctuation) && current().lexeme == ",") {
            ++current_index_;
        }
    }
    consume(lex::TokenKind::Punctuation, "expected ')' after member initializer arguments");
    initializer.span = span_from(start, current_index_ == 0 ? start : current_index_ - 1);
    return initializer;
}

std::vector<ast::MemberInitializer> Parser::parse_member_initializer_list() {
    std::vector<ast::MemberInitializer> initializers;
    consume(lex::TokenKind::Punctuation, "expected ':' before member initializer list");
    while (!eof()) {
        initializers.push_back(parse_member_initializer());
        if (!(check(lex::TokenKind::Punctuation) && current().lexeme == ",")) {
            break;
        }
        ++current_index_;
    }
    return initializers;
}

ast::MethodDecl Parser::parse_method_signature(bool allow_implementation) {
    const std::size_t start = current_index_;
    ast::MethodDecl decl;

    if (check(lex::TokenKind::KeywordStatic)) {
        decl.is_static = true;
        ++current_index_;
    }
    if (allow_implementation && check(lex::TokenKind::KeywordImplementation)) {
        decl.is_implementation = true;
        ++current_index_;
    }

    consume(lex::TokenKind::KeywordFn, "expected fn in method declaration");
    if (!is_identifier_like(current())) {
        diagnostics_.error("expected method name", current().span, source_path_.string());
        return decl;
    }
    decl.name = current().lexeme;
    ++current_index_;

    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "(")) {
        diagnostics_.error("expected '(' after method name", current().span, source_path_.string());
        return decl;
    }
    decl.parameters = parse_parameter_list();
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "->")) {
        diagnostics_.error("expected '->' after parameter list", current().span, source_path_.string());
        return decl;
    }
    ++current_index_;
    decl.return_type = parse_type_ref_until({";", "{", ":"});
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ":") {
        decl.member_initializers = parse_member_initializer_list();
    }
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
        ++current_index_;
    }
    decl.span = span_from(start, current_index_ == 0 ? start : current_index_ - 1);
    return decl;
}

std::pair<ast::QualifiedName, ast::MethodDecl> Parser::parse_function_signature() {
    const std::size_t start = current_index_;
    ast::MethodDecl decl;
    ast::QualifiedName name;

    if (check(lex::TokenKind::KeywordExportC)) {
        decl.is_export_c = true;
        ++current_index_;
    }

    if (check(lex::TokenKind::KeywordStatic)) {
        decl.is_static = true;
        ++current_index_;
    }

    consume(lex::TokenKind::KeywordFn, "expected fn in function declaration");
    name = parse_qualified_name();
    if (name.parts.empty()) {
        diagnostics_.error("expected function name", current().span, source_path_.string());
        return {name, decl};
    }
    decl.name = name.parts.back();

    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "(")) {
        diagnostics_.error("expected '(' after function name", current().span, source_path_.string());
        return {name, decl};
    }
    decl.parameters = parse_parameter_list();
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "->")) {
        diagnostics_.error("expected '->' after parameter list", current().span, source_path_.string());
        return {name, decl};
    }
    ++current_index_;
    decl.return_type = parse_type_ref_until({";", "{", ":"});
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ":") {
        decl.member_initializers = parse_member_initializer_list();
    }
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
        ++current_index_;
    }
    decl.span = span_from(start, current_index_ == 0 ? start : current_index_ - 1);
    return {std::move(name), std::move(decl)};
}

ast::FieldDecl Parser::parse_field(bool is_static, bool is_inject) {
    const std::size_t start = current_index_;
    ast::FieldDecl field;
    field.is_static = is_static;
    field.is_inject = is_inject;
    if (is_static && check(lex::TokenKind::KeywordStatic)) {
        ++current_index_;
    }
    if (is_inject && check(lex::TokenKind::KeywordInject)) {
        ++current_index_;
    }

    const auto type = parse_type_ref_until({";"});
    field.type = type;
    auto pieces = support::split(type.spelling, ' ');
    if (!pieces.empty()) {
        field.name = support::trim(pieces.back());
        pieces.pop_back();
        field.type.spelling = support::trim(support::join(pieces, " "));
        while (!field.name.empty() && field.name.front() == '*') {
            field.type.spelling += "*";
            field.name.erase(field.name.begin());
        }
    }
    field.is_private_intent = !field.name.empty() && field.name.front() == '_';
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
        ++current_index_;
    }
    field.span = span_from(start, current_index_ == 0 ? start : current_index_ - 1);
    return field;
}

ast::BindDecl Parser::parse_bind() {
    const std::size_t start = current_index_;
    consume(lex::TokenKind::KeywordBind, "expected bind");
    ast::BindDecl decl;
    decl.owner_type = parse_qualified_name();
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == ".")) {
        diagnostics_.error("expected '.' after bind owner type", current().span, source_path_.string());
        return decl;
    }
    ++current_index_;
    if (!is_identifier_like(current())) {
        diagnostics_.error("expected slot name in bind declaration", current().span, source_path_.string());
        return decl;
    }
    decl.slot_name = current().lexeme;
    ++current_index_;
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "=")) {
        diagnostics_.error("expected '=' in bind declaration", current().span, source_path_.string());
        return decl;
    }
    ++current_index_;
    decl.concrete_type = parse_qualified_name();
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
        ++current_index_;
    }
    decl.span = span_from(start, current_index_ == 0 ? start : current_index_ - 1);
    return decl;
}

ast::ImportDecl Parser::parse_import() {
    const std::size_t start = current_index_;
    consume(lex::TokenKind::KeywordImport, "expected import");
    ast::ImportDecl decl;
    if (!check(lex::TokenKind::String)) {
        diagnostics_.error("expected string literal after import", current().span, source_path_.string());
        decl.span = tokens_[start].span;
        return decl;
    }
    decl.module_path = current().lexeme;
    if (decl.module_path.size() >= 2 && decl.module_path.front() == '"' && decl.module_path.back() == '"') {
        decl.module_path = decl.module_path.substr(1, decl.module_path.size() - 2);
    }
    ++current_index_;
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
        ++current_index_;
    }
    decl.span = span_from(start, current_index_ == 0 ? start : current_index_ - 1);
    return decl;
}

std::unique_ptr<ast::Expression> Parser::parse_primary_expression() {
    if (eof()) {
        return nullptr;
    }

    const auto start = current_index_;
    const auto token = current();

    if (token.kind == lex::TokenKind::Identifier) {
        auto name = parse_qualified_name();
        auto expr = std::make_unique<ast::Expression>();
        expr->span = token.span;
        expr->node = ast::IdentifierExpr{name, token.span};
        return expr;
    }

    if (token.kind == lex::TokenKind::Number || token.kind == lex::TokenKind::String || token.kind == lex::TokenKind::Character) {
        ++current_index_;
        auto expr = std::make_unique<ast::Expression>();
        expr->span = token.span;
        expr->node = ast::LiteralExpr{token.lexeme, token.span};
        return expr;
    }

    if (token.lexeme == "(") {
        ++current_index_;
        auto inner = parse_expression_until({")"});
        if (check(lex::TokenKind::Punctuation) && current().lexeme == ")") {
            ++current_index_;
        }
        return inner;
    }

    ++current_index_;
    auto expr = std::make_unique<ast::Expression>();
    expr->span = token.span;
    expr->node = ast::RawExpr{token.lexeme, token.span};
    (void)start;
    return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_postfix_expression(std::unique_ptr<ast::Expression> expr) {
    while (!eof()) {
        if (current().lexeme == ".") {
            ++current_index_;
            if (!is_identifier_like(current())) {
                diagnostics_.error("expected identifier after '.'", current().span, source_path_.string());
                break;
            }
            const auto member = current().lexeme;
            const auto member_span = current().span;
            ++current_index_;
            auto wrapped = std::make_unique<ast::Expression>();
            wrapped->span.begin = expr->span.begin;
            wrapped->span.end = member_span.end;
            wrapped->node = ast::MemberExpr{std::move(expr), member, wrapped->span};
            expr = std::move(wrapped);
            continue;
        }

        if (current().lexeme == "(") {
            ++current_index_;
            auto call = std::make_unique<ast::CallExpr>();
            call->callee = std::move(expr);
            while (!eof() && current().lexeme != ")") {
                call->arguments.push_back(parse_expression_until({",", ")"}));
                if (current().lexeme == ",") {
                    ++current_index_;
                }
            }
            const auto end_span = current().span;
            if (current().lexeme == ")") {
                ++current_index_;
            }
            auto wrapped = std::make_unique<ast::Expression>();
            wrapped->span.begin = call->callee ? call->callee->span.begin : end_span.begin;
            wrapped->span.end = end_span.end;
            wrapped->node = ast::CallExpr{std::move(call->callee), std::move(call->arguments), wrapped->span};
            expr = std::move(wrapped);
            continue;
        }

        break;
    }
    return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_expression_until(const std::vector<std::string>& terminators) {
    if (eof() || is_terminator_lexeme(current().lexeme, terminators)) {
        auto expr = std::make_unique<ast::Expression>();
        expr->span = current().span;
        expr->node = ast::RawExpr{"", current().span};
        return expr;
    }

    const std::size_t begin_index = current_index_;
    auto lhs = parse_postfix_expression(parse_primary_expression());
    while (!eof() && !is_terminator_lexeme(current().lexeme, terminators)) {
        const auto op = current().lexeme;
        if (!is_operator_lexeme(op)) {
            break;
        }
        ++current_index_;
        auto rhs = parse_postfix_expression(parse_primary_expression());
        auto wrapped = std::make_unique<ast::Expression>();
        wrapped->span.begin = lhs ? lhs->span.begin : tokens_[begin_index].span.begin;
        wrapped->span.end = rhs ? rhs->span.end : current().span.end;
        if (op == "=" || op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=") {
            wrapped->node = ast::AssignmentExpr{std::move(lhs), std::move(rhs), wrapped->span};
        } else {
            wrapped->node = ast::BinaryExpr{op, std::move(lhs), std::move(rhs), wrapped->span};
        }
        lhs = std::move(wrapped);
    }

    if (lhs == nullptr) {
        auto expr = std::make_unique<ast::Expression>();
        expr->span = tokens_[begin_index].span;
        expr->node = ast::RawExpr{slice(tokens_[begin_index].span.begin.offset, tokens_[current_index_ == 0 ? begin_index : current_index_ - 1].span.end.offset), tokens_[begin_index].span};
        return expr;
    }
    return lhs;
}

bool Parser::is_declaration_statement() const {
    if (eof()) {
        return false;
    }
    if (!(current().kind == lex::TokenKind::Identifier || current().kind == lex::TokenKind::KeywordStatic)) {
        return false;
    }

    std::size_t i = current_index_;
    std::size_t identifier_count = 0;
    while (i < tokens_.size()) {
        const auto& token = tokens_[i];
        if (token.lexeme == ";") {
            return identifier_count >= 2;
        }
        if (token.lexeme == "=" || token.lexeme == "+=" || token.lexeme == "-=" || token.lexeme == "*=" || token.lexeme == "/=" || token.lexeme == "%=") {
            return identifier_count >= 2;
        }
        if (token.lexeme == "(" || token.lexeme == "[" || token.lexeme == "]" || token.lexeme == "{") {
            return false;
        }
        if (token.lexeme == "." || token.lexeme == "->") {
            return false;
        }
        if (token.kind == lex::TokenKind::Identifier) {
            ++identifier_count;
        }
        ++i;
    }

    return false;
}

std::unique_ptr<ast::Statement> Parser::parse_expression_statement() {
    const std::size_t start = current_index_;
    auto stmt = std::make_unique<ast::Statement>();
    stmt->span.begin = current().span.begin;
    stmt->node = ast::ExprStmt{parse_expression_until({";"}), current().span};
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
        stmt->span.end = current().span.end;
        ++current_index_;
    }
    if (stmt->span.end.offset == 0 && current_index_ > 0) {
        stmt->span.end = tokens_[current_index_ - 1].span.end;
    }
    (void)start;
    return stmt;
}

std::unique_ptr<ast::Statement> Parser::parse_return_statement() {
    auto stmt = std::make_unique<ast::Statement>();
    const auto start_span = current().span;
    ++current_index_;
    stmt->span.begin = start_span.begin;
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
        stmt->node = ast::ReturnStmt{nullptr, start_span};
        stmt->span.end = current().span.end;
        ++current_index_;
        return stmt;
    }
    auto value = parse_expression_until({";"});
    ast::Span span = start_span;
    if (value) {
        span.end = value->span.end;
    }
    stmt->node = ast::ReturnStmt{std::move(value), span};
    if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
        stmt->span.end = current().span.end;
        ++current_index_;
    } else {
        stmt->span.end = span.end;
    }
    return stmt;
}

std::unique_ptr<ast::Statement> Parser::parse_if_statement() {
    auto stmt = std::make_unique<ast::Statement>();
    const auto start_span = current().span;
    ++current_index_;
    stmt->span.begin = start_span.begin;

    consume(lex::TokenKind::Punctuation, "expected '(' after if");
    auto condition = parse_expression_until({")"});
    consume(lex::TokenKind::Punctuation, "expected ')' after if condition");

    auto then_branch = parse_statement();
    std::unique_ptr<ast::Statement> else_branch;
    if (check(lex::TokenKind::KeywordElse)) {
        ++current_index_;
        else_branch = parse_statement();
    }

    ast::Span span = start_span;
    if (else_branch != nullptr) {
        span.end = else_branch->span.end;
    } else if (then_branch != nullptr) {
        span.end = then_branch->span.end;
    } else if (condition != nullptr) {
        span.end = condition->span.end;
    }

    stmt->span = span;
    stmt->node = ast::IfStmt{std::move(condition), std::move(then_branch), std::move(else_branch), span};
    return stmt;
}

std::unique_ptr<ast::Statement> Parser::parse_while_statement() {
    auto stmt = std::make_unique<ast::Statement>();
    const auto start_span = current().span;
    ++current_index_;
    stmt->span.begin = start_span.begin;

    consume(lex::TokenKind::Punctuation, "expected '(' after while");
    auto condition = parse_expression_until({")"});
    consume(lex::TokenKind::Punctuation, "expected ')' after while condition");
    auto body = parse_statement();

    ast::Span span = start_span;
    if (body != nullptr) {
        span.end = body->span.end;
    } else if (condition != nullptr) {
        span.end = condition->span.end;
    }

    stmt->span = span;
    stmt->node = ast::WhileStmt{std::move(condition), std::move(body), span};
    return stmt;
}

std::unique_ptr<ast::Statement> Parser::parse_for_statement() {
    auto stmt = std::make_unique<ast::Statement>();
    const auto start_span = current().span;
    ++current_index_;
    stmt->span.begin = start_span.begin;

    consume(lex::TokenKind::Punctuation, "expected '(' after for");

    std::unique_ptr<ast::Statement> init;
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == ";")) {
        auto init_expr = parse_expression_until({";"});
        init = std::make_unique<ast::Statement>();
        init->span = init_expr != nullptr ? init_expr->span : start_span;
        init->node = ast::ExprStmt{std::move(init_expr), init->span};
    }
    consume(lex::TokenKind::Punctuation, "expected ';' after for initializer");

    std::unique_ptr<ast::Expression> condition;
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == ";")) {
        condition = parse_expression_until({";"});
    }
    consume(lex::TokenKind::Punctuation, "expected ';' after for condition");

    std::unique_ptr<ast::Expression> step;
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == ")")) {
        step = parse_expression_until({")"});
    }
    consume(lex::TokenKind::Punctuation, "expected ')' after for header");

    auto body = parse_statement();

    ast::Span span = start_span;
    if (body != nullptr) {
        span.end = body->span.end;
    } else if (step != nullptr) {
        span.end = step->span.end;
    } else if (condition != nullptr) {
        span.end = condition->span.end;
    } else if (init != nullptr) {
        span.end = init->span.end;
    }

    stmt->span = span;
    stmt->node = ast::ForStmt{std::move(init), std::move(condition), std::move(step), std::move(body), span};
    return stmt;
}

std::unique_ptr<ast::Statement> Parser::parse_declaration_statement() {
    const std::size_t start = current_index_;
    bool is_static = false;
    if (check(lex::TokenKind::KeywordStatic)) {
        is_static = true;
        ++current_index_;
    }
    std::size_t semicolon_index = current_index_;
    while (semicolon_index < tokens_.size() && tokens_[semicolon_index].lexeme != ";") {
        ++semicolon_index;
    }

    std::size_t eq_index = current_index_;
    while (eq_index < semicolon_index && tokens_[eq_index].lexeme != "=") {
        ++eq_index;
    }

    const std::size_t left_end_index = eq_index == semicolon_index ? semicolon_index - 1 : eq_index - 1;
    std::string left_text = slice(tokens_[current_index_].span.begin.offset, tokens_[left_end_index].span.end.offset);
    std::string name_text;
    std::string type_text;
    const auto last_space = left_text.find_last_of(" \t");
    if (last_space != std::string::npos) {
        type_text = support::trim(left_text.substr(0, last_space));
        name_text = support::trim(left_text.substr(last_space + 1));
        while (!name_text.empty() && name_text.front() == '*') {
            type_text += "*";
            name_text.erase(name_text.begin());
        }
    } else {
        name_text = support::trim(left_text);
    }

    auto stmt = std::make_unique<ast::Statement>();
    stmt->span.begin = tokens_[start].span.begin;
    ast::DeclStmt decl;
    decl.type = {support::trim(type_text.empty() ? left_text : type_text), tokens_[start].span};
    decl.name = name_text;
    decl.is_static = is_static;
    decl.span.begin = tokens_[start].span.begin;

    current_index_ = eq_index;
    if (current_index_ < semicolon_index && check(lex::TokenKind::Punctuation) && current().lexeme == "=") {
        ++current_index_;
        decl.initializer = parse_expression_until({";"});
    }

    if (current_index_ < tokens_.size() && check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
        decl.span.end = current().span.end;
        stmt->span.end = current().span.end;
        ++current_index_;
    } else if (semicolon_index < tokens_.size()) {
        decl.span.end = tokens_[semicolon_index].span.end;
        stmt->span.end = tokens_[semicolon_index].span.end;
        current_index_ = semicolon_index + 1;
    }

    stmt->node = std::move(decl);
    return stmt;
}

std::unique_ptr<ast::Statement> Parser::parse_statement() {
    if (eof()) {
        return nullptr;
    }

    if (check(lex::TokenKind::Punctuation) && current().lexeme == "{") {
        return parse_block_statement();
    }
    if (check(lex::TokenKind::KeywordIf)) {
        return parse_if_statement();
    }
    if (check(lex::TokenKind::KeywordWhile)) {
        return parse_while_statement();
    }
    if (check(lex::TokenKind::KeywordFor)) {
        return parse_for_statement();
    }
    if (check(lex::TokenKind::KeywordReturn)) {
        return parse_return_statement();
    }
    if (check(lex::TokenKind::KeywordBreak)) {
        auto stmt = std::make_unique<ast::Statement>();
        stmt->span.begin = current().span.begin;
        stmt->span.end = current().span.end;
        stmt->node = ast::BreakStmt{current().span};
        ++current_index_;
        if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
            stmt->span.end = current().span.end;
            ++current_index_;
        }
        return stmt;
    }
    if (check(lex::TokenKind::KeywordContinue)) {
        auto stmt = std::make_unique<ast::Statement>();
        stmt->span.begin = current().span.begin;
        stmt->span.end = current().span.end;
        stmt->node = ast::ContinueStmt{current().span};
        ++current_index_;
        if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
            stmt->span.end = current().span.end;
            ++current_index_;
        }
        return stmt;
    }
    if (is_declaration_statement()) {
        return parse_declaration_statement();
    }
    return parse_expression_statement();
}

std::unique_ptr<ast::Statement> Parser::parse_block_statement() {
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "{")) {
        return nullptr;
    }
    const std::size_t start = current_index_;
    ++current_index_;
    auto block = std::make_unique<ast::BlockStmt>();
    block->span.begin = tokens_[start].span.begin;
    while (!eof() && !(check(lex::TokenKind::Punctuation) && current().lexeme == "}")) {
        auto stmt = parse_statement();
        if (stmt != nullptr) {
            block->statements.push_back(std::move(stmt));
        } else {
            break;
        }
    }
    if (check(lex::TokenKind::Punctuation) && current().lexeme == "}") {
        block->span.end = current().span.end;
        ++current_index_;
    }
    auto stmt = std::make_unique<ast::Statement>();
    stmt->span = block->span;
    stmt->node = std::move(*block);
    return stmt;
}

std::unique_ptr<ast::BlockStmt> Parser::parse_function_body() {
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "{")) {
        return nullptr;
    }
    auto stmt = parse_block_statement();
    if (stmt == nullptr) {
        return nullptr;
    }
    if (auto* block = std::get_if<ast::BlockStmt>(&stmt->node)) {
        auto body = std::make_unique<ast::BlockStmt>(std::move(*block));
        return body;
    }
    return nullptr;
}

std::vector<ast::Declaration> Parser::parse_block_declarations() {
    std::vector<ast::Declaration> decls;
    while (!eof() && !(check(lex::TokenKind::Punctuation) && current().lexeme == "}")) {
        if (check(lex::TokenKind::KeywordNamespace)) {
            decls.push_back(parse_namespace());
        } else if (check(lex::TokenKind::KeywordClass)) {
            decls.push_back(parse_class());
        } else if (check(lex::TokenKind::KeywordStruct)) {
            decls.push_back(parse_struct());
        } else if (check(lex::TokenKind::KeywordInterface)) {
            decls.push_back(parse_interface());
        } else if (check(lex::TokenKind::KeywordEnum)) {
            decls.push_back(parse_enum());
        } else if (check(lex::TokenKind::KeywordFn) || check(lex::TokenKind::KeywordExportC)) {
            decls.push_back(parse_function());
        } else if (check(lex::TokenKind::KeywordBind)) {
            decls.push_back(parse_bind_declaration());
        } else if (check(lex::TokenKind::KeywordImport)) {
            decls.push_back(parse_import_declaration());
        } else {
            decls.push_back(parse_raw_c());
        }
    }
    return decls;
}

void Parser::parse_class_body(ast::ClassDecl& decl) {
    bool in_public = false;
    bool in_private = false;

    while (!eof() && !(check(lex::TokenKind::Punctuation) && current().lexeme == "}")) {
        if (check(lex::TokenKind::KeywordPublic)) {
            ++current_index_;
            consume(lex::TokenKind::Punctuation, "expected ':' after public");
            in_public = true;
            in_private = false;
            continue;
        }
        if (check(lex::TokenKind::KeywordPrivate)) {
            ++current_index_;
            consume(lex::TokenKind::Punctuation, "expected ':' after private");
            in_public = false;
            in_private = true;
            continue;
        }

        const bool is_static = check(lex::TokenKind::KeywordStatic);
        const bool is_inject = check(lex::TokenKind::KeywordInject);
        const bool is_method = is_static
            ? (current_index_ + 1 < tokens_.size() && (tokens_[current_index_ + 1].kind == lex::TokenKind::KeywordFn || tokens_[current_index_ + 1].kind == lex::TokenKind::KeywordImplementation))
            : (check(lex::TokenKind::KeywordFn) || check(lex::TokenKind::KeywordImplementation));

        if (is_method) {
            auto method = parse_method_signature(true);
            method.is_private = in_private;
            if (method.name == "Construct") {
                decl.constructors.push_back(method);
            } else {
                if (method.name == "Destruct") {
                    decl.has_destruct = true;
                }
                decl.methods.push_back(method);
            }
            continue;
        }

        auto field = parse_field(is_static, is_inject);
        if (field.is_static) {
            decl.static_fields.push_back(field);
        } else {
            decl.fields.push_back(field);
        }
        (void)in_public;
        (void)in_private;
    }
}

void Parser::parse_interface_body(ast::InterfaceDecl& decl) {
    while (!eof() && !(check(lex::TokenKind::Punctuation) && current().lexeme == "}")) {
        if (check(lex::TokenKind::KeywordPublic) || check(lex::TokenKind::KeywordPrivate)) {
            ++current_index_;
            consume(lex::TokenKind::Punctuation, "expected ':' after visibility label");
            continue;
        }
        auto method = parse_method_signature(false);
        method.is_private = false;
        decl.methods.push_back(method);
    }
}

std::size_t Parser::find_matching_brace(std::size_t open_index) const {
    if (open_index >= tokens_.size() || tokens_[open_index].lexeme != "{") {
        return open_index;
    }

    std::size_t depth = 0;
    for (std::size_t i = open_index; i < tokens_.size(); ++i) {
        if (tokens_[i].lexeme == "{") {
            ++depth;
        } else if (tokens_[i].lexeme == "}") {
            if (depth == 0) {
                break;
            }
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    return open_index;
}

ast::Span Parser::span_from(std::size_t begin_index, std::size_t end_index) const {
    return ast::Span{tokens_[begin_index].span.begin, tokens_[end_index].span.end};
}

ast::Declaration Parser::parse_namespace() {
    const std::size_t start = current_index_;
    ++current_index_;
    ast::QualifiedName name = parse_qualified_name();
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "{")) {
        diagnostics_.error("expected '{' after namespace name", current().span, source_path_.string());
        if (!eof()) {
            ++current_index_;
        }
        return ast::RawCDecl{"", tokens_[start].span};
    }
    ++current_index_;
    ast::NamespaceDecl decl;
    decl.name = name;
    decl.declarations = parse_block_declarations();
    const std::size_t close_index = current_index_;
    consume(lex::TokenKind::Punctuation, "expected '}' after namespace body");
    decl.span = span_from(start, close_index);
    return decl;
}

ast::Declaration Parser::parse_class() {
    const std::size_t start = current_index_;
    ++current_index_;
    ast::QualifiedName name = parse_qualified_name();

    std::vector<ast::QualifiedName> implements;
    if (check(lex::TokenKind::KeywordImplements)) {
        ++current_index_;
        implements.push_back(parse_qualified_name());
        while (check(lex::TokenKind::Punctuation) && current().lexeme == ",") {
            ++current_index_;
            implements.push_back(parse_qualified_name());
        }
    }

    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "{")) {
        diagnostics_.error("expected '{' after class header", current().span, source_path_.string());
        if (!eof()) {
            ++current_index_;
        }
        return ast::RawCDecl{"", tokens_[start].span};
    }
    ++current_index_;
    ast::ClassDecl decl;
    decl.name = name;
    decl.implements = implements;
    parse_class_body(decl);
    const std::size_t close_index = current_index_;
    consume(lex::TokenKind::Punctuation, "expected '}' after class body");
    decl.span = span_from(start, close_index);
    return decl;
}

ast::Declaration Parser::parse_struct() {
    const std::size_t start = current_index_;
    ++current_index_;
    ast::QualifiedName name = parse_qualified_name();
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "{")) {
        diagnostics_.error("expected '{' after struct name", current().span, source_path_.string());
        if (!eof()) {
            ++current_index_;
        }
        return ast::RawCDecl{"", tokens_[start].span};
    }
    ++current_index_;
    ast::ClassDecl decl;
    decl.name = name;
    decl.is_struct = true;
    while (!eof() && !(check(lex::TokenKind::Punctuation) && current().lexeme == "}")) {
        if (check(lex::TokenKind::KeywordPublic) || check(lex::TokenKind::KeywordPrivate)) {
            diagnostics_.error("struct does not support visibility sections", current().span, source_path_.string());
            ++current_index_;
            if (check(lex::TokenKind::Punctuation) && current().lexeme == ":") {
                ++current_index_;
            }
            continue;
        }
        if (check(lex::TokenKind::KeywordFn) || check(lex::TokenKind::KeywordImplementation) || check(lex::TokenKind::KeywordStatic) || check(lex::TokenKind::KeywordInject)) {
            diagnostics_.error("struct may only contain fields", current().span, source_path_.string());
            while (!eof() && !(check(lex::TokenKind::Punctuation) && (current().lexeme == ";" || current().lexeme == "}"))) {
                ++current_index_;
            }
            if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
                ++current_index_;
            }
            continue;
        }
        decl.fields.push_back(parse_field(false, false));
    }
    const std::size_t close_index = current_index_;
    consume(lex::TokenKind::Punctuation, "expected '}' after struct body");
    decl.span = span_from(start, close_index);
    return decl;
}

ast::Declaration Parser::parse_interface() {
    const std::size_t start = current_index_;
    ++current_index_;
    ast::QualifiedName name = parse_qualified_name();
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "{")) {
        diagnostics_.error("expected '{' after interface name", current().span, source_path_.string());
        if (!eof()) {
            ++current_index_;
        }
        return ast::RawCDecl{"", tokens_[start].span};
    }
    ++current_index_;
    ast::InterfaceDecl decl;
    decl.name = name;
    parse_interface_body(decl);
    const std::size_t close_index = current_index_;
    consume(lex::TokenKind::Punctuation, "expected '}' after interface body");
    decl.span = span_from(start, close_index);
    return decl;
}

std::vector<ast::EnumMember> Parser::parse_enum_members(std::size_t open_index, std::size_t close_index) {
    std::vector<ast::EnumMember> members;
    for (std::size_t i = open_index + 1; i < close_index; ++i) {
        if (tokens_[i].kind != lex::TokenKind::Identifier) {
            continue;
        }
        members.push_back(ast::EnumMember{tokens_[i].lexeme, tokens_[i].span});
    }
    return members;
}

ast::Declaration Parser::parse_enum() {
    const std::size_t start = current_index_;
    ++current_index_;
    ast::QualifiedName name = parse_qualified_name();
    if (!(check(lex::TokenKind::Punctuation) && current().lexeme == "{")) {
        diagnostics_.error("expected '{' after enum name", current().span, source_path_.string());
        if (!eof()) {
            ++current_index_;
        }
        return ast::RawCDecl{"", tokens_[start].span};
    }
    ++current_index_;
    const std::size_t open_index = current_index_ - 1;
    const std::size_t close_index = find_matching_brace(open_index);
    auto members = parse_enum_members(open_index, close_index);
    current_index_ = close_index + 1;
    return ast::EnumDecl{name, std::move(members), span_from(start, close_index)};
}

ast::Declaration Parser::parse_function() {
    const std::size_t start = current_index_;
    auto [name, signature] = parse_function_signature();
    std::string body;
    std::unique_ptr<ast::BlockStmt> body_ast;

    if (signature.is_export_c && name.parts.size() > 1) {
        diagnostics_.error("export_c is only allowed on free functions", signature.span, source_path_.string());
    }

    if (check(lex::TokenKind::Punctuation) && current().lexeme == "{") {
        const std::size_t open_index = current_index_;
        body_ast = parse_function_body();
        const std::size_t close_index = current_index_ == 0 ? open_index : current_index_ - 1;
        body = slice(tokens_[open_index].span.end.offset, tokens_[close_index].span.begin.offset);
        return ast::FunctionDecl{name, std::move(signature), std::move(body), std::move(body_ast), span_from(start, close_index)};
    }

    return ast::FunctionDecl{name, std::move(signature), std::move(body), nullptr, tokens_[start].span};
}

ast::Declaration Parser::parse_bind_declaration() {
    return parse_bind();
}

ast::Declaration Parser::parse_import_declaration() {
    return parse_import();
}

ast::Declaration Parser::parse_raw_c() {
    const std::size_t start = current_index_;
    if (check(lex::TokenKind::Punctuation) && current().lexeme == "#") {
        const auto line = current().span.begin.line;
        while (!eof() && current().span.begin.line == line) {
            ++current_index_;
        }
    } else {
        while (!eof() && !(check(lex::TokenKind::Punctuation) && current().lexeme == ";")) {
            ++current_index_;
        }
        if (check(lex::TokenKind::Punctuation) && current().lexeme == ";") {
            ++current_index_;
        }
    }
    const std::size_t end_index = current_index_ == 0 ? 0 : current_index_ - 1;
    const std::string text = slice(tokens_[start].span.begin.offset, tokens_[end_index].span.end.offset);
    return ast::RawCDecl{text, tokens_[start].span};
}

ast::Module Parser::parse_module() {
    ast::Module module;
    module.source_path = source_path_;
    module.source_text = source_;

    while (!eof()) {
        if (check(lex::TokenKind::KeywordNamespace)) {
            module.declarations.push_back(parse_namespace());
            continue;
        }
        if (check(lex::TokenKind::KeywordClass)) {
            module.declarations.push_back(parse_class());
            continue;
        }
        if (check(lex::TokenKind::KeywordStruct)) {
            module.declarations.push_back(parse_struct());
            continue;
        }
        if (check(lex::TokenKind::KeywordInterface)) {
            module.declarations.push_back(parse_interface());
            continue;
        }
        if (check(lex::TokenKind::KeywordEnum)) {
            module.declarations.push_back(parse_enum());
            continue;
        }
        if (check(lex::TokenKind::KeywordFn) || check(lex::TokenKind::KeywordExportC)) {
            module.declarations.push_back(parse_function());
            continue;
        }
        if (check(lex::TokenKind::KeywordBind)) {
            module.declarations.push_back(parse_bind_declaration());
            continue;
        }
        if (check(lex::TokenKind::KeywordImport)) {
            module.declarations.push_back(parse_import_declaration());
            continue;
        }
        if (check(lex::TokenKind::EndOfFile)) {
            break;
        }
        module.declarations.push_back(parse_raw_c());
    }

    return module;
}

ast::Module parse_module(std::string source, std::filesystem::path source_path, support::Diagnostics& diagnostics) {
    auto tokens = lex::lex_source(source, source_path.string());
    Parser parser(std::move(source), std::move(tokens), std::move(source_path), diagnostics);
    return parser.parse_module();
}

} // namespace cplus::parse
