#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>

// Forward Token (we only use TokenType names in parser; AST doesn't need Token)
#include "lexer.h"

// Base
struct Node {
    virtual ~Node() = default;
    virtual void pretty_print(int indent = 0) const = 0;
protected:
    static void indentPrint(int n) { for (int i=0;i<n;i++) std::cout << "  "; }
};

// Expressions
struct Expr : Node { using Ptr = std::unique_ptr<Expr>; };

struct Identifier : Expr {
    std::string name;
    Identifier(std::string n): name(std::move(n)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "Identifier(" << name << ")\n";
    }
};

struct NumberLiteral : Expr {
    std::string value;
    NumberLiteral(std::string v): value(std::move(v)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "Number(" << value << ")\n";
    }
};

struct StringLiteral : Expr {
    std::string value;
    StringLiteral(std::string v): value(std::move(v)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "String(\"" << value << "\")\n";
    }
};

struct BoolLiteral : Expr {
    bool value;
    BoolLiteral(bool v): value(v) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "Bool(" << (value ? "true" : "false") << ")\n";
    }
};

struct UnaryExpr : Expr {
    std::string op;
    Expr::Ptr right;
    UnaryExpr(std::string o, Expr::Ptr r): op(std::move(o)), right(std::move(r)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "Unary(" << op << ")\n";
        right->pretty_print(indent+1);
    }
};

struct BinaryExpr : Expr {
    std::string op;
    Expr::Ptr left;
    Expr::Ptr right;
    BinaryExpr(std::string o, Expr::Ptr l, Expr::Ptr r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "Binary(" << op << ")\n";
        left->pretty_print(indent+1);
        right->pretty_print(indent+1);
    }
};

struct CallExpr : Expr {
    Expr::Ptr callee;
    std::vector<Expr::Ptr> args;
    CallExpr(Expr::Ptr c, std::vector<Expr::Ptr> a): callee(std::move(c)), args(std::move(a)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "Call\n";
        callee->pretty_print(indent+1);
        indentPrint(indent+1); std::cout << "Args:\n";
        for (auto &a : args) a->pretty_print(indent+2);
    }
};

// Statements
struct Stmt : Node { using Ptr = std::unique_ptr<Stmt>; };

struct ExprStmt : Stmt {
    Expr::Ptr expr;
    ExprStmt(Expr::Ptr e): expr(std::move(e)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "ExprStmt\n";
        expr->pretty_print(indent+1);
    }
};

struct ReturnStmt : Stmt {
    Expr::Ptr value; // may be null
    ReturnStmt(Expr::Ptr v): value(std::move(v)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "Return\n";
        if (value) value->pretty_print(indent+1);
    }
};

struct LetStmt : Stmt {
    std::string name;
    std::string typeName; // optional
    Expr::Ptr init; // optional
    LetStmt(std::string n, std::string t, Expr::Ptr i) : name(std::move(n)), typeName(std::move(t)), init(std::move(i)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent);
        std::cout << "Let " << name;
        if (!typeName.empty()) std::cout << " : " << typeName;
        std::cout << "\n";
        if (init) init->pretty_print(indent+1);
    }
};

struct BlockStmt : Stmt {
    std::vector<Stmt::Ptr> stmts;
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "Block\n";
        for (auto &s : stmts) s->pretty_print(indent+1);
    }
};

struct IfStmt : Stmt {
    Expr::Ptr cond;
    std::unique_ptr<BlockStmt> thenBranch;
    std::unique_ptr<BlockStmt> elseBranch; // optional
    IfStmt(Expr::Ptr c, std::unique_ptr<BlockStmt> t, std::unique_ptr<BlockStmt> e)
        : cond(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "If\n";
        indentPrint(indent+1); std::cout << "Cond:\n"; cond->pretty_print(indent+2);
        indentPrint(indent+1); std::cout << "Then:\n"; thenBranch->pretty_print(indent+2);
        if (elseBranch) { indentPrint(indent+1); std::cout << "Else:\n"; elseBranch->pretty_print(indent+2); }
    }
};


struct IfExpr : Expr {
    Expr::Ptr cond;
    Expr::Ptr thenExpr;
    Expr::Ptr elseExpr;

    IfExpr(Expr::Ptr c, Expr::Ptr t, Expr::Ptr e)
        : cond(std::move(c)), thenExpr(std::move(t)), elseExpr(std::move(e)) {}

    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "IfExpr\n";
        indentPrint(indent+1); std::cout << "Cond:\n"; cond->pretty_print(indent+2);
        indentPrint(indent+1); std::cout << "Then:\n"; thenExpr->pretty_print(indent+2);
        indentPrint(indent+1); std::cout << "Else:\n"; elseExpr->pretty_print(indent+2);
    }
};



struct WhileStmt : Stmt {
    Expr::Ptr cond;
    std::unique_ptr<BlockStmt> body;
    WhileStmt(Expr::Ptr c, std::unique_ptr<BlockStmt> b): cond(std::move(c)), body(std::move(b)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "While\n";
        indentPrint(indent+1); std::cout << "Cond:\n"; cond->pretty_print(indent+2);
        indentPrint(indent+1); std::cout << "Body:\n"; body->pretty_print(indent+2);
    }
};

struct FunctionDecl : Stmt {
    std::string name;
    std::vector<std::pair<std::string,std::string>> params; // (name, typename optional)
    std::string returnType; // optional
    std::unique_ptr<BlockStmt> body;
    FunctionDecl(std::string n, std::vector<std::pair<std::string,std::string>> p, std::string r, std::unique_ptr<BlockStmt> b)
        : name(std::move(n)), params(std::move(p)), returnType(std::move(r)), body(std::move(b)) {}
    void pretty_print(int indent = 0) const override {
        indentPrint(indent); std::cout << "Function " << name;
        if (!returnType.empty()) std::cout << " : " << returnType;
        std::cout << "\n";
        indentPrint(indent+1); std::cout << "Params:\n";
        for (auto &pr : params) {
            indentPrint(indent+2); std::cout << pr.first;
            if (!pr.second.empty()) std::cout << " : " << pr.second;
            std::cout << "\n";
        }
        indentPrint(indent+1); std::cout << "Body:\n";
        body->pretty_print(indent+2);
    }
};

using Program = std::vector<Stmt::Ptr>;
