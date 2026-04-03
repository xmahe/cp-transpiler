#pragma once

#include "token.h"

#include <string>
#include <vector>

namespace cplus::lex {

class Lexer {
public:
    Lexer(std::string source, std::string file_name);

    const std::vector<Token>& tokens() const;
    std::vector<Token> tokenize();

private:
    Token make_token(TokenKind kind, std::size_t begin, std::size_t end, const ast::Location& start, const ast::Location& finish) const;
    void skip_whitespace();
    void skip_line_comment();
    void skip_block_comment();
    bool eof() const;
    char peek(std::size_t lookahead = 0) const;
    char advance();
    bool match(char expected);
    Token lex_identifier();
    Token lex_number();
    Token lex_string();
    Token lex_character();
    Token lex_punctuation();

    std::string source_;
    std::string file_name_;
    std::size_t index_ = 0;
    ast::Location location_{};
    std::vector<Token> tokens_;
};

std::vector<Token> lex_source(std::string source, std::string file_name);

} // namespace cplus::lex
