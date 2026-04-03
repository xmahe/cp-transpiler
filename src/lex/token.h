#pragma once

#include "ast/ast.h"

#include <string>

namespace cplus::lex {

enum class TokenKind {
    EndOfFile,
    Identifier,
    Number,
    String,
    Character,
    KeywordFn,
    KeywordNamespace,
    KeywordClass,
    KeywordStruct,
    KeywordInterface,
    KeywordEnum,
    KeywordImplements,
    KeywordImplementation,
    KeywordExportC,
    KeywordInject,
    KeywordBind,
    KeywordPublic,
    KeywordPrivate,
    KeywordStatic,
    KeywordIf,
    KeywordElse,
    KeywordSwitch,
    KeywordWhile,
    KeywordFor,
    KeywordBreak,
    KeywordContinue,
    KeywordReturn,
    Punctuation,
    Unknown,
};

struct Token {
    TokenKind kind = TokenKind::Unknown;
    std::string lexeme;
    ast::Span span{};
};

bool is_keyword(std::string_view lexeme, TokenKind& kind_out);

} // namespace cplus::lex
