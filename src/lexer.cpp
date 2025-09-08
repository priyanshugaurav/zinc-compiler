#include "lexer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <stdexcept>

class Lexer
{
    std::string src;
    size_t pos = 0;
    int line = 1;
    int col = 1;

    std::unordered_map<std::string, TokenType> keywords = {
        {"let", TokenType::Let},
        {"fn", TokenType::Fn},
        {"if", TokenType::If},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"return", TokenType::Return},
        {"true", TokenType::True},
        {"false", TokenType::False},
    };

    char peek() const { return pos < src.size() ? src[pos] : '\0'; }
    char peekNext() const { return (pos + 1) < src.size() ? src[pos + 1] : '\0'; }

    char advance()
    {
        char c = peek();
        if (c == '\n')
        {
            line++;
            col = 1;
        }
        else
            col++;
        pos++;
        return c;
    }

    bool match(char expected)
    {
        if (peek() == expected)
        {
            advance();
            return true;
        }
        return false;
    }

    void skipWhitespaceAndComments()
    {
        while (true)
        {
            char c = peek();
            if (isspace(static_cast<unsigned char>(c)))
            {
                advance();
                continue;
            }
            if (c == '/' && peekNext() == '/')
            {
                // single-line comment
                advance();
                advance();
                while (peek() != '\n' && peek() != '\0')
                    advance();
                continue;
            }
            if (c == '/' && peekNext() == '*')
            {
                // block comment
                advance();
                advance();
                while (!(peek() == '*' && peekNext() == '/'))
                {
                    if (peek() == '\0')
                        throw std::runtime_error("Unterminated block comment");
                    advance();
                }
                advance();
                advance(); // consume */
                continue;
            }

            break;
        }
    }

    Token makeToken(TokenType t, const std::string &val = "")
    {
        return Token{t, val, line, col};
    }

    Token identifier()
    {
        size_t start = pos;
        while (isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
            advance();
        std::string txt = src.substr(start, pos - start);
        auto it = keywords.find(txt);
        if (it != keywords.end())
            return makeToken(it->second, txt);
        return makeToken(TokenType::Identifier, txt);
    }

    Token number()
    {
        size_t start = pos;
        while (isdigit(static_cast<unsigned char>(peek())))
            advance();
        // no floats for now
        std::string txt = src.substr(start, pos - start);
        return makeToken(TokenType::Number, txt);
    }

    Token stringLiteral()
    {
        // assume opening " already consumed by caller
        size_t start = pos;
        while (peek() != '"' && peek() != '\0')
        {
            if (peek() == '\\')
            {
                // skip escape char and next char
                advance();
                if (peek() != '\0')
                    advance();
            }
            else
                advance();
        }
        if (peek() != '"')
            throw std::runtime_error("Unterminated string literal");
        std::string txt = src.substr(start, pos - start);
        advance(); // consume closing "
        return makeToken(TokenType::String, txt);
    }

public:
    Lexer(const std::string &s) : src(s) {}

    std::vector<Token> tokenize()
    {
        std::vector<Token> out;
        while (true)
        {
            skipWhitespaceAndComments();
            char c = peek();
            if (c == '\0')
            {
                out.push_back(makeToken(TokenType::Eof, ""));
                break;
            }

            // single-char tokens and multi-char operators
            if (isalpha(static_cast<unsigned char>(c)) || c == '_')
            {
                advance();
                // rollback one char because identifier() expects start at current pos
                pos--;
                col--;
                out.push_back(identifier());
                continue;
            }

            if (isdigit(static_cast<unsigned char>(c)))
            {
                advance();
                pos--;
                col--;
                out.push_back(number());
                continue;
            }

            // punctuation and operators
            switch (c)
            {
            case '(':
                advance();
                out.push_back(makeToken(TokenType::LParen, "("));
                break;
            case ')':
                advance();
                out.push_back(makeToken(TokenType::RParen, ")"));
                break;
            case '{':
                advance();
                out.push_back(makeToken(TokenType::LBrace, "{"));
                break;
            case '}':
                advance();
                out.push_back(makeToken(TokenType::RBrace, "}"));
                break;
            case ':':
                advance();
                out.push_back(makeToken(TokenType::Colon, ":"));
                break;
            case ';':
                advance();
                out.push_back(makeToken(TokenType::Semicolon, ";"));
                break;
            case ',':
                advance();
                out.push_back(makeToken(TokenType::Comma, ","));
                break;
            case '+':
                advance();
                out.push_back(makeToken(TokenType::Plus, "+"));
                break;
            case '-':
                advance();
                out.push_back(makeToken(TokenType::Minus, "-"));
                break;
            case '*':
                advance();
                out.push_back(makeToken(TokenType::Star, "*"));
                break;
            case '/':
                advance();
                out.push_back(makeToken(TokenType::Slash, "/"));
                break;
            case '%':
                advance();
                out.push_back(makeToken(TokenType::MOD, "%"));
                break;
            case '!':
                advance();
                if (match('='))
                    out.push_back(makeToken(TokenType::NotEqual, "!="));
                else
                    out.push_back(makeToken(TokenType::Bang, "!"));
                break;
            case '=':
                advance();
                if (match('='))
                    out.push_back(makeToken(TokenType::Equal, "=="));
                else
                    out.push_back(makeToken(TokenType::Assign, "="));
                break;
            case '&':
                advance();
                if (match('&'))
                    out.push_back(makeToken(TokenType::AndAnd, "&&"));
                else
                    out.push_back(makeToken(TokenType::BitAnd, "&"));
                break;
            case '|':
                advance();
                if (match('|'))
                    out.push_back(makeToken(TokenType::OrOr, "||"));
                else
                    out.push_back(makeToken(TokenType::BitOr, "|"));
                break;
            case '^':
                advance();
                out.push_back(makeToken(TokenType::BitXor, "^"));
                break;
            case '<':
                advance();
                if (match('<'))
                    out.push_back(makeToken(TokenType::ShiftLeft, "<<"));
                else if (match('='))
                    out.push_back(makeToken(TokenType::LessEqual, "<="));
                else
                    out.push_back(makeToken(TokenType::Less, "<"));
                break;
            case '>':
                advance();
                if (match('>'))
                    out.push_back(makeToken(TokenType::ShiftRight, ">>"));
                else if (match('='))
                    out.push_back(makeToken(TokenType::GreaterEqual, ">="));
                else
                    out.push_back(makeToken(TokenType::Greater, ">"));
                break;

            case '"':
                advance(); // consume opening quote
                out.push_back(stringLiteral());
                break;
            default:
                throw std::runtime_error(std::string("Unexpected character in input: '") + c + "' at line " + std::to_string(line));
            }
        }

        return out;
    }
};

// Convenience function to use from main.cpp
std::vector<Token> lexString(const std::string &s)
{
    Lexer lx(s);
    return lx.tokenize();
}
