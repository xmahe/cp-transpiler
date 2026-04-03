#include "lexer.h"

#include "support/strings.h"

#include <cctype>
#include <unordered_map>

namespace cplus::lex {

namespace {

bool is_identifier_start(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool is_identifier_continue(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

} // namespace

bool is_keyword(std::string_view lexeme, TokenKind& kind_out) {
    static const std::unordered_map<std::string_view, TokenKind> keywords = {
        {"fn", TokenKind::KeywordFn},
        {"namespace", TokenKind::KeywordNamespace},
        {"class", TokenKind::KeywordClass},
        {"interface", TokenKind::KeywordInterface},
        {"enum", TokenKind::KeywordEnum},
        {"implements", TokenKind::KeywordImplements},
        {"implementation", TokenKind::KeywordImplementation},
        {"public", TokenKind::KeywordPublic},
        {"private", TokenKind::KeywordPrivate},
        {"static", TokenKind::KeywordStatic},
        {"if", TokenKind::KeywordIf},
        {"else", TokenKind::KeywordElse},
        {"switch", TokenKind::KeywordSwitch},
        {"while", TokenKind::KeywordWhile},
        {"for", TokenKind::KeywordFor},
        {"break", TokenKind::KeywordBreak},
        {"continue", TokenKind::KeywordContinue},
        {"return", TokenKind::KeywordReturn},
    };

    const auto it = keywords.find(lexeme);
    if (it == keywords.end()) {
        return false;
    }
    kind_out = it->second;
    return true;
}

Lexer::Lexer(std::string source, std::string file_name)
    : source_(std::move(source)), file_name_(std::move(file_name)) {}

const std::vector<Token>& Lexer::tokens() const {
    return tokens_;
}

bool Lexer::eof() const {
    return index_ >= source_.size();
}

char Lexer::peek(std::size_t lookahead) const {
    const std::size_t pos = index_ + lookahead;
    if (pos >= source_.size()) {
        return '\0';
    }
    return source_[pos];
}

char Lexer::advance() {
    if (eof()) {
        return '\0';
    }
    const char ch = source_[index_++];
    ++location_.offset;
    if (ch == '\n') {
        ++location_.line;
        location_.column = 1;
    } else {
        ++location_.column;
    }
    return ch;
}

bool Lexer::match(char expected) {
    if (peek() != expected) {
        return false;
    }
    advance();
    return true;
}

Token Lexer::make_token(TokenKind kind, std::size_t begin, std::size_t end, const ast::Location& start, const ast::Location& finish) const {
    return Token{kind, source_.substr(begin, end - begin), ast::Span{start, finish}};
}

void Lexer::skip_whitespace() {
    while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
        advance();
    }
}

void Lexer::skip_line_comment() {
    while (!eof() && peek() != '\n') {
        advance();
    }
}

void Lexer::skip_block_comment() {
    while (!eof()) {
        if (peek() == '*' && peek(1) == '/') {
            advance();
            advance();
            return;
        }
        advance();
    }
}

Token Lexer::lex_identifier() {
    const ast::Location start = location_;
    const std::size_t begin = index_;
    advance();
    while (!eof() && is_identifier_continue(peek())) {
        advance();
    }
    const std::size_t end = index_;
    TokenKind kind = TokenKind::Identifier;
    is_keyword(source_.substr(begin, end - begin), kind);
    return make_token(kind, begin, end, start, location_);
}

Token Lexer::lex_number() {
    const ast::Location start = location_;
    const std::size_t begin = index_;
    advance();
    while (!eof() && (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }
    if (!eof() && peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        advance();
        while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }
    return make_token(TokenKind::Number, begin, index_, start, location_);
}

Token Lexer::lex_string() {
    const ast::Location start = location_;
    const std::size_t begin = index_;
    advance();
    while (!eof()) {
        if (peek() == '\\') {
            advance();
            if (!eof()) {
                advance();
            }
            continue;
        }
        if (peek() == '"') {
            advance();
            break;
        }
        advance();
    }
    return make_token(TokenKind::String, begin, index_, start, location_);
}

Token Lexer::lex_character() {
    const ast::Location start = location_;
    const std::size_t begin = index_;
    advance();
    while (!eof()) {
        if (peek() == '\\') {
            advance();
            if (!eof()) {
                advance();
            }
            continue;
        }
        if (peek() == '\'') {
            advance();
            break;
        }
        advance();
    }
    return make_token(TokenKind::Character, begin, index_, start, location_);
}

Token Lexer::lex_punctuation() {
    const ast::Location start = location_;
    const std::size_t begin = index_;
    const char ch = advance();
    if (ch == ':' && peek() == ':') {
        advance();
    } else if (ch == '-' && peek() == '>') {
        advance();
    } else if ((ch == '=' || ch == '!' || ch == '<' || ch == '>') && peek() == '=') {
        advance();
    } else if ((ch == '&' || ch == '|') && peek() == ch) {
        advance();
    }
    return make_token(TokenKind::Punctuation, begin, index_, start, location_);
}

std::vector<Token> Lexer::tokenize() {
    tokens_.clear();
    index_ = 0;
    location_ = {};

    while (!eof()) {
        skip_whitespace();
        if (eof()) {
            break;
        }

        if (peek() == '/' && peek(1) == '/') {
            skip_line_comment();
            continue;
        }
        if (peek() == '/' && peek(1) == '*') {
            advance();
            advance();
            skip_block_comment();
            continue;
        }
        if (is_identifier_start(peek())) {
            tokens_.push_back(lex_identifier());
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(peek()))) {
            tokens_.push_back(lex_number());
            continue;
        }
        if (peek() == '"') {
            tokens_.push_back(lex_string());
            continue;
        }
        if (peek() == '\'') {
            tokens_.push_back(lex_character());
            continue;
        }
        tokens_.push_back(lex_punctuation());
    }

    tokens_.push_back(Token{TokenKind::EndOfFile, {}, ast::Span{location_, location_}});
    return tokens_;
}

std::vector<Token> lex_source(std::string source, std::string file_name) {
    Lexer lexer(std::move(source), std::move(file_name));
    return lexer.tokenize();
}

} // namespace cplus::lex
