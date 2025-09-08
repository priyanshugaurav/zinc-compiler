#include "parser.h"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token> &toks) : tokens(toks), idx(0) {}

const Token &Parser::peek() const {
    if (idx < tokens.size()) return tokens[idx];
    return tokens.back();
}
const Token &Parser::previous() const { return tokens[idx-1]; }
const Token &Parser::advance() {
    if (idx < tokens.size()) idx++;
    return previous();
}
bool Parser::isAtEnd() const { return peek().type == TokenType::Eof; }
bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}
bool Parser::check(TokenType t) const {
    if (isAtEnd()) return false;
    return peek().type == t;
}
void Parser::expect(TokenType t, const std::string &msg) {
    if (!check(t)) throw std::runtime_error("Parse error: expected " + msg + " at line " + std::to_string(peek().line));
    advance();
}

// Top-level
Program Parser::parseProgram() {
    Program prog;
    while (!isAtEnd()) {
        prog.push_back(parseDeclaration());
    }
    return prog;
}

Stmt::Ptr Parser::parseDeclaration() {
    if (match(TokenType::Fn)) return parseFunctionDecl();
    if (match(TokenType::Let)) return parseLetDecl();
    return parseStatement();
}

Stmt::Ptr Parser::parseFunctionDecl() {
    if (!check(TokenType::Identifier)) throw std::runtime_error("Expected function name after 'fn'");
    std::string name = peek().value; advance();

    expect(TokenType::LParen, " '(' after function name");
    std::vector<std::pair<std::string,std::string>> params;
    if (!check(TokenType::RParen)) {
        do {
            if (!check(TokenType::Identifier)) throw std::runtime_error("Expected parameter name");
            std::string pname = peek().value; advance();
            std::string ptype;
            if (match(TokenType::Colon)) {
                if (!check(TokenType::Identifier)) throw std::runtime_error("Expected type name for parameter");
                ptype = peek().value; advance();
            }
            params.push_back({pname, ptype});
        } while (match(TokenType::Comma));
    }
    expect(TokenType::RParen, "closing ')' after params");

    std::string retType;
    if (match(TokenType::Colon)) {
        if (!check(TokenType::Identifier)) throw std::runtime_error("Expected return type after ':'");
        retType = peek().value; advance();
    }

    auto body = parseBlock();
    return std::make_unique<FunctionDecl>(name, std::move(params), retType, std::move(body));
}

Stmt::Ptr Parser::parseLetDecl() {
    if (!check(TokenType::Identifier)) throw std::runtime_error("Expected identifier after 'let'");
    std::string name = peek().value; advance();
    std::string typeName;
    if (match(TokenType::Colon)) {
        if (!check(TokenType::Identifier)) throw std::runtime_error("Expected type name after ':'");
        typeName = peek().value; advance();
    }
    if (match(TokenType::Assign)) {
        Expr::Ptr init = parseExpression();
        if (match(TokenType::Semicolon)) { /* optional semicolon consumed */ }
        return std::make_unique<LetStmt>(name, typeName, std::move(init));
    } else {
        // allow let without initializer but require semicolon
        if (match(TokenType::Semicolon)) {
            return std::make_unique<LetStmt>(name, typeName, nullptr);
        }
        throw std::runtime_error("Expected '=' or ';' after let declaration");
    }
}

Stmt::Ptr Parser::parseStatement() {
    if (match(TokenType::Return)) return parseReturn();
    if (match(TokenType::If)) return parseIf();
    if (match(TokenType::While)) return parseWhile();
    if (match(TokenType::LBrace)) {
        auto blk = std::make_unique<BlockStmt>();
        while (!check(TokenType::RBrace) && !isAtEnd()) {
            blk->stmts.push_back(parseDeclaration());
        }
        expect(TokenType::RBrace, " '}' to close block");
        return blk;
    }

    // otherwise expression statement
    Expr::Ptr e = parseExpression();
    if (match(TokenType::Semicolon)) { /* consume optional semicolon */ }
    return std::make_unique<ExprStmt>(std::move(e));
}

Stmt::Ptr Parser::parseReturn() {
    Expr::Ptr val = nullptr;
    if (!check(TokenType::Semicolon)) val = parseExpression();
    if (match(TokenType::Semicolon)) {}
    return std::make_unique<ReturnStmt>(std::move(val));
}

Stmt::Ptr Parser::parseIf() {
    Expr::Ptr cond = parseExpression();
    auto thenBlock = parseBlock();
    std::unique_ptr<BlockStmt> elseBlock = nullptr;
    if (match(TokenType::Else)) {
        // else can be block or another if (we choose block-only for simplicity)
        if (check(TokenType::LBrace)) elseBlock = parseBlock();
        else {
            // allow "else if ..." by wrapping single stmt in a block
            auto tmpBlk = std::make_unique<BlockStmt>();
            tmpBlk->stmts.push_back(parseDeclaration());
            elseBlock = std::move(tmpBlk);
        }
    }
    return std::make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
}

Stmt::Ptr Parser::parseWhile() {
    Expr::Ptr cond = parseExpression();
    auto body = parseBlock();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    expect(TokenType::LBrace, " '{' to start block");
    auto blk = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        blk->stmts.push_back(parseDeclaration());
    }
    expect(TokenType::RBrace, " '}' to close block");
    return blk;
}



// Expressions (precedence climbing / cascading)
Expr::Ptr Parser::parseExpression() {
    
    return parseAssignment();
}

Expr::Ptr Parser::parseAssignment() {
    Expr::Ptr left = parseOr();
    if (match(TokenType::Assign)) {
        Identifier *id = dynamic_cast<Identifier*>(left.get());
        if (!id) throw std::runtime_error("Invalid assignment target at line " + std::to_string(previous().line));
        Expr::Ptr right = parseAssignment();
        return std::make_unique<BinaryExpr>("=", std::move(left), std::move(right));
    }
    return left;
}

Expr::Ptr Parser::parseOr() {
    Expr::Ptr left = parseAnd();
    while (match(TokenType::OrOr)) {
        Expr::Ptr right = parseAnd();
        left = std::make_unique<BinaryExpr>("||", std::move(left), std::move(right));
    }
    return left;
}

Expr::Ptr Parser::parseAnd() {
    Expr::Ptr left = parseBitOr();   // <<== CHANGE: call parseBitOr instead of parseEquality
    while (match(TokenType::AndAnd)) {
        Expr::Ptr right = parseBitOr();
        left = std::make_unique<BinaryExpr>("&&", std::move(left), std::move(right));
    }
    return left;
}

Expr::Ptr Parser::parseBitOr() {
    Expr::Ptr left = parseBitXor();
    while (match(TokenType::BitOr)) {
        Expr::Ptr right = parseBitXor();
        left = std::make_unique<BinaryExpr>("|", std::move(left), std::move(right));
    }
    return left;
}

Expr::Ptr Parser::parseBitXor() {
    Expr::Ptr left = parseBitAnd();
    while (match(TokenType::BitXor)) {
        Expr::Ptr right = parseBitAnd();
        left = std::make_unique<BinaryExpr>("^", std::move(left), std::move(right));
    }
    return left;
}

Expr::Ptr Parser::parseBitAnd() {
    Expr::Ptr left = parseEquality();
    while (match(TokenType::BitAnd)) {
        Expr::Ptr right = parseEquality();
        left = std::make_unique<BinaryExpr>("&", std::move(left), std::move(right));
    }
    return left;
}

Expr::Ptr Parser::parseShift() {
    Expr::Ptr left = parseTerm();
    while (true) {
        if (match(TokenType::ShiftLeft)) {
            Expr::Ptr right = parseTerm();
            left = std::make_unique<BinaryExpr>("<<", std::move(left), std::move(right));
            continue;
        }
        if (match(TokenType::ShiftRight)) {
            Expr::Ptr right = parseTerm();
            left = std::make_unique<BinaryExpr>(">>", std::move(left), std::move(right));
            continue;
        }
        break;
    }
    return left;
}


Expr::Ptr Parser::parseEquality() {
    Expr::Ptr left = parseComparison();
    while (true) {
        if (match(TokenType::Equal)) {
            Expr::Ptr right = parseComparison();
            left = std::make_unique<BinaryExpr>("==", std::move(left), std::move(right));
            continue;
        }
        if (match(TokenType::NotEqual)) {
            Expr::Ptr right = parseComparison();
            left = std::make_unique<BinaryExpr>("!=", std::move(left), std::move(right));
            continue;
        }
        break;
    }
    return left;
}

Expr::Ptr Parser::parseComparison() {
    Expr::Ptr left = parseShift(); // instead of parseTerm()
    while (true) {
        if (match(TokenType::Less)) {
            Expr::Ptr right = parseShift();
            left = std::make_unique<BinaryExpr>("<", std::move(left), std::move(right));
            continue;
        }
        if (match(TokenType::LessEqual)) {
            Expr::Ptr right = parseShift();
            left = std::make_unique<BinaryExpr>("<=", std::move(left), std::move(right));
            continue;
        }
        if (match(TokenType::Greater)) {
            Expr::Ptr right = parseShift();
            left = std::make_unique<BinaryExpr>(">", std::move(left), std::move(right));
            continue;
        }
        if (match(TokenType::GreaterEqual)) {
            Expr::Ptr right = parseShift();
            left = std::make_unique<BinaryExpr>(">=", std::move(left), std::move(right));
            continue;
        }
        break;
    }
    return left;
}

Expr::Ptr Parser::parseTerm() {
    Expr::Ptr left = parseFactor();
    while (true) {
        if (match(TokenType::Plus)) {
            Expr::Ptr right = parseFactor();
            left = std::make_unique<BinaryExpr>("+", std::move(left), std::move(right));
            continue;
        }
        if (match(TokenType::Minus)) {
            Expr::Ptr right = parseFactor();
            left = std::make_unique<BinaryExpr>("-", std::move(left), std::move(right));
            continue;
        }
        break;
    }
    return left;
}

Expr::Ptr Parser::parseFactor() {
    Expr::Ptr left = parseUnary();
    while (true) {
        if (match(TokenType::Star)) {
            Expr::Ptr right = parseUnary();
            left = std::make_unique<BinaryExpr>("*", std::move(left), std::move(right));
            continue;
        }
        if (match(TokenType::Slash)) {
            Expr::Ptr right = parseUnary();
            left = std::make_unique<BinaryExpr>("/", std::move(left), std::move(right));
            continue;
        }
        if (match(TokenType::MOD)) {
            Expr::Ptr right = parseUnary();
            left = std::make_unique<BinaryExpr>("%", std::move(left), std::move(right));
            continue;
        }
        break;
    }
    return left;
}

Expr::Ptr Parser::parseUnary() {
    if (match(TokenType::Bang)) {
        Expr::Ptr right = parseUnary();
        return std::make_unique<UnaryExpr>("!", std::move(right));
    }
    if (match(TokenType::Minus)) {
        Expr::Ptr right = parseUnary();
        return std::make_unique<UnaryExpr>("-", std::move(right));
    }
    return parseCall();
}

Expr::Ptr Parser::parseCall() {
    Expr::Ptr expr = parsePrimary();
    while (true) {
        if (match(TokenType::LParen)) {
            std::vector<Expr::Ptr> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::Comma));
            }
            expect(TokenType::RParen, "closing ')' in call");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
            continue;
        }
        break;
    }
    return expr;
}

Expr::Ptr Parser::parseBlockExpression() {
    expect(TokenType::LBrace, " '{' to start expression block");

    // Support a single expression inside block
    Expr::Ptr expr = parseExpression();

    if (match(TokenType::Semicolon)) {
        throw std::runtime_error("Expression block cannot end in semicolon (must return a value)");
    }

    expect(TokenType::RBrace, " '}' to close expression block");

    return expr;
}


Expr::Ptr Parser::parsePrimary() {
    if (match(TokenType::If)) {
    Expr::Ptr cond = parseExpression();
    Expr::Ptr thenExpr = nullptr;
    Expr::Ptr elseExpr = nullptr;

    thenExpr = parseBlockExpression(); // returns single expr in block
    if (match(TokenType::Else)) {
        elseExpr = parseBlockExpression();
    } else {
        throw std::runtime_error("if-expression must have an else branch");
    }

    return std::make_unique<IfExpr>(std::move(cond), std::move(thenExpr), std::move(elseExpr));
}


    // Existing expression parsing
    if (match(TokenType::Number)) {
        return std::make_unique<NumberLiteral>(previous().value);
    }
    if (match(TokenType::String)) {
        return std::make_unique<StringLiteral>(previous().value);
    }
    if (match(TokenType::True)) {
        return std::make_unique<BoolLiteral>(true);
    }
    if (match(TokenType::False)) {
        return std::make_unique<BoolLiteral>(false);
    }
    if (match(TokenType::Identifier)) {
        return std::make_unique<Identifier>(previous().value);
    }
    if (match(TokenType::LParen)) {
        Expr::Ptr e = parseExpression();
        expect(TokenType::RParen, "closing ')'");
        return e;
    }

    throw std::runtime_error("Unexpected token in expression at line " + std::to_string(peek().line));
}
