#pragma once
#include <vector>
#include <string>
#include <memory>
#include "lexer.h"
#include "ast.h"

class Parser {
public:
    Parser(const std::vector<Token> &tokens);
    Program parseProgram();

private:
    const std::vector<Token> &tokens;
    size_t idx = 0;

    const Token &peek() const;
    const Token &previous() const;
    const Token &advance();
    bool match(TokenType t);
    bool check(TokenType t) const;
    void expect(TokenType t, const std::string &msg);

    // top-level
    Stmt::Ptr parseDeclaration();

    // statements
    Stmt::Ptr parseFunctionDecl();
    Stmt::Ptr parseLetDecl();
    Stmt::Ptr parseStatement();
    std::unique_ptr<BlockStmt> parseBlock();
    Stmt::Ptr parseIf();
    Stmt::Ptr parseWhile();
    Stmt::Ptr parseReturn();

    // expressions (precedence climbing)
    Expr::Ptr parseExpression();
    Expr::Ptr parseAssignment();
    Expr::Ptr parseOr();
    Expr::Ptr parseAnd();
    Expr::Ptr parseEquality();
    Expr::Ptr parseComparison();
    Expr::Ptr parseTerm();
    Expr::Ptr parseFactor();
    Expr::Ptr parseUnary();
    Expr::Ptr parseCall();
    Expr::Ptr parsePrimary();
    Expr::Ptr parseShift();
    Expr::Ptr parseBitAnd();
    Expr::Ptr parseBitXor();
    // parseBitXor
    Expr::Ptr parseBitOr();
    Expr::Ptr parseBlockExpression();

    // helpers
    bool isAtEnd() const;
};
