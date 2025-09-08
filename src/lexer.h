#pragma once
#include <string>
#include <vector>

enum class TokenType
{
    // End
    Eof,

    // Keywords
    Let,
    Fn,
    If,
    Else,
    While,
    Return,
    Print,
    Scan, 
    True,
    False,

    // Literals & identifiers
    Identifier,
    Number,
    String,

    // Punctuation
    LParen,
    RParen,
    LBrace,
    RBrace,
    Colon,
    Semicolon,
    Comma,

    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    MOD,
    Bang,   // !
    Assign, // =
    Equal,
    NotEqual, // == !=
    Less,
    LessEqual, // < <=
    Greater,
    GreaterEqual, // > >=
    AndAnd,
    OrOr, // && ||
    ShiftLeft,
    ShiftRight, // << >>
    Pipe,       // for |> if you want pipe later
    BitAnd,
    BitOr,
    BitXor,
};

struct Token
{
    TokenType type;
    std::string value; // textual value for identifiers / literals / operator lexeme optionally
    int line;
    int col;
};
