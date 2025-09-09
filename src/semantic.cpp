#pragma once
#include "ast.h"
#include "environment.h"
#include <memory>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// A tiny semantic analyzer that:
//  - builds the Environment (variables + functions)
//  - pushes/pops scopes for blocks / functions
//  - performs simple type checking for arithmetic, logic, comparisons, assignment
//  - records expression types in exprTypes map (useful later)
//
// Supported types: "int", "string", "bool", "void", "unknown"
// Number literals -> "int", string literals -> "string", true/false -> "bool"

class SemanticAnalyzer {
public:
    SemanticAnalyzer() : env(std::make_shared<Environment>()) {}

    // Analyze top-level program; on success, env contains all top-level declarations.
    void analyze(const Program &program) {
        // start with a fresh global environment
        env = std::make_shared<Environment>();

        // You could predefine builtin functions (print, scan) here:
        auto printSym = std::make_shared<Symbol>();
        printSym->name = "print";
        printSym->kind = SymbolKind::Function;
        printSym->paramTypes = { "string" }; // simple varargs not implemented here
        printSym->returnType = "int";
        env->define("print", printSym);

        // analyze each top-level stmt
        for (auto &stmt : program) analyzeStmt(stmt.get());
    }

    std::shared_ptr<Environment> getEnvironment() const { return env; }

    // get inferred type for an expression (after analyze)
    // throws if not found
    std::string getExprType(const Expr *e) const {
        auto it = exprTypes.find(e);
        if (it == exprTypes.end()) throw std::runtime_error("No type recorded for expression");
        return it->second;
    }

private:
    std::shared_ptr<Environment> env;
    std::unordered_map<const Expr*, std::string> exprTypes;

    // keep track of current function return type for 'return' checks
    std::vector<std::string> functionReturnStack;

    // Helpers
    void analyzeStmt(const Stmt *s) {
        if (!s) return;

        if (auto f = dynamic_cast<const FunctionDecl*>(s)) {
            // declare function in current scope
            if (env->lookupCurrent(f->name)) {
                throw std::runtime_error("Function already defined in this scope: " + f->name);
            }
            auto sym = std::make_shared<Symbol>();
            sym->name = f->name;
            sym->kind = SymbolKind::Function;
            sym->returnType = f->returnType.empty() ? "void" : f->returnType;
            // param types might be empty strings -> "unknown"
            for (auto &p : f->params) sym->paramTypes.push_back(p.second.empty() ? "unknown" : p.second);
            env->define(f->name, sym);

            // analyze function body in a new scope
            env->push();
            // define params as variables in the function scope
            for (size_t i = 0; i < f->params.size(); ++i) {
                auto &pp = f->params[i];
                auto psym = std::make_shared<Symbol>();
                psym->name = pp.first;
                psym->kind = SymbolKind::Var;
                psym->type = pp.second.empty() ? "unknown" : pp.second;
                if (!env->define(pp.first, psym))
                    throw std::runtime_error("Parameter name conflict: " + pp.first);
            }

            // track current function return type
            functionReturnStack.push_back(sym->returnType);

            // analyze body
            analyzeStmt(f->body.get());

            functionReturnStack.pop_back();
            env->pop();
        }
        else if (auto let = dynamic_cast<const LetStmt*>(s)) {
            // must have either type or initializer
            if (let->typeName.empty() && !let->init) {
                throw std::runtime_error("let '" + let->name + "' must have a type or an initializer");
            }

            if (env->lookupCurrent(let->name)) {
                throw std::runtime_error("Variable already defined in current scope: " + let->name);
            }

            std::string varType = let->typeName;
            if (let->init) {
                std::string initType = analyzeExpr(let->init.get());
                if (varType.empty()) {
                    // infer from initializer
                    varType = initType;
                } else {
                    // check type matches
                    if (initType != "unknown" && initType != varType) {
                        throw std::runtime_error("Type mismatch in initializer for '" + let->name + "' : init is " + initType + " but variable declared " + varType);
                    }
                }
            }

            auto sym = std::make_shared<Symbol>();
            sym->name = let->name;
            sym->kind = SymbolKind::Var;
            sym->type = varType.empty() ? "unknown" : varType;

            env->define(let->name, sym);
        }
        else if (auto block = dynamic_cast<const BlockStmt*>(s)) {
            env->push();
            for (auto &st : block->stmts) analyzeStmt(st.get());
            env->pop();
        }
        else if (auto ret = dynamic_cast<const ReturnStmt*>(s)) {
            if (functionReturnStack.empty()) {
                throw std::runtime_error("Return used outside of function");
            }
            if (ret->value) {
                std::string rtype = analyzeExpr(ret->value.get());
                std::string expected = functionReturnStack.back();
                if (expected != "void" && rtype != "unknown" && rtype != expected) {
                    throw std::runtime_error("Return type mismatch: expected " + expected + " got " + rtype);
                }
            } else {
                // returning nothing
                std::string expected = functionReturnStack.back();
                if (expected != "void") {
                    throw std::runtime_error("Return missing value for function with return type " + expected);
                }
            }
        }
        else if (auto exprs = dynamic_cast<const ExprStmt*>(s)) {
            analyzeExpr(exprs->expr.get());
        }
        else if (auto ifs = dynamic_cast<const IfStmt*>(s)) {
            std::string condt = analyzeExpr(ifs->cond.get());
            if (condt != "bool" && condt != "unknown") {
                throw std::runtime_error("If condition must be bool (found " + condt + ")");
            }
            env->push();
            analyzeStmt(ifs->thenBranch.get());
            env->pop();
            if (ifs->elseBranch) {
                env->push();
                analyzeStmt(ifs->elseBranch.get());
                env->pop();
            }
        }
        else if (auto wh = dynamic_cast<const WhileStmt*>(s)) {
            std::string condt = analyzeExpr(wh->cond.get());
            if (condt != "bool" && condt != "unknown") {
                throw std::runtime_error("While condition must be bool (found " + condt + ")");
            }
            env->push();
            analyzeStmt(wh->body.get());
            env->pop();
        }
        else {
            // fallback: nothing
        }
    }

    // analyze expression and return its type string
    std::string analyzeExpr(const Expr *e) {
        if (!e) return "unknown";

        if (auto n = dynamic_cast<const NumberLiteral*>(e)) {
            exprTypes[e] = "int";
            return "int";
        }
        if (auto s = dynamic_cast<const StringLiteral*>(e)) {
            exprTypes[e] = "string";
            return "string";
        }
        if (auto b = dynamic_cast<const BoolLiteral*>(e)) {
            exprTypes[e] = "bool";
            return "bool";
        }
        if (auto id = dynamic_cast<const Identifier*>(e)) {
            auto sym = env->lookup(id->name);
            if (!sym) throw std::runtime_error("Undefined identifier: " + id->name);
            exprTypes[e] = sym->type;
            return sym->type;
        }
        if (auto u = dynamic_cast<const UnaryExpr*>(e)) {
            std::string rt = analyzeExpr(u->right.get());
            if (u->op == "-" ) {
                if (rt != "int" && rt != "unknown") throw std::runtime_error("Unary '-' requires int");
                exprTypes[e] = "int";
                return "int";
            }
            if (u->op == "!") {
                if (rt != "bool" && rt != "unknown") throw std::runtime_error("Unary '!' requires bool");
                exprTypes[e] = "bool";
                return "bool";
            }
        }
        if (auto bin = dynamic_cast<const BinaryExpr*>(e)) {
            std::string L = analyzeExpr(bin->left.get());
            std::string R = analyzeExpr(bin->right.get());
            const auto &op = bin->op;

            // assignment
            if (op == "=") {
                // left must be ident
                auto idl = dynamic_cast<const Identifier*>(bin->left.get());
                if (!idl) throw std::runtime_error("Left-hand side of assignment must be a variable");
                auto sym = env->lookup(idl->name);
                if (!sym) throw std::runtime_error("Assign to undefined variable: " + idl->name);

                if (sym->type == "unknown" && R != "unknown") {
                    // infer variable type
                    sym->type = R;
                } else if (sym->type != "unknown" && R != "unknown" && sym->type != R) {
                    throw std::runtime_error("Type mismatch in assignment to '" + idl->name + "': " + sym->type + " <- " + R);
                }
                exprTypes[e] = sym->type;
                return sym->type;
            }

            // arithmetic
            if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
                if (op == "+" && L == "string" && R == "string") {
                    exprTypes[e] = "string"; // string concat
                    return "string";
                }
                if ((L == "int" || L == "unknown") && (R == "int" || R == "unknown")) {
                    exprTypes[e] = "int";
                    return "int";
                }
                throw std::runtime_error("Arithmetic operator '" + op + "' requires integer operands");
            }

            // comparisons
            if (op == "==" || op == "!=") {
                if (L != R && L != "unknown" && R != "unknown")
                    throw std::runtime_error("Comparing different types with '" + op + "': " + L + " vs " + R);
                exprTypes[e] = "bool";
                return "bool";
            }
            if (op == "<" || op == "<=" || op == ">" || op == ">=") {
                if (L == "int" || L == "unknown") {
                    exprTypes[e] = "bool";
                    return "bool";
                }
                throw std::runtime_error("Relational operator '" + op + "' requires integer operands");
            }

            // logical
            if (op == "&&" || op == "||") {
                if ((L == "bool" || L == "unknown") && (R == "bool" || R == "unknown")) {
                    exprTypes[e] = "bool";
                    return "bool";
                }
                throw std::runtime_error("Logical operator '" + op + "' requires bool operands");
            }

            // bitwise
            if (op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>") {
                if ((L == "int" || L == "unknown") && (R == "int" || R == "unknown")) {
                    exprTypes[e] = "int";
                    return "int";
                }
                throw std::runtime_error("Bitwise operator '" + op + "' requires integer operands");
            }

            // fallback
            exprTypes[e] = "unknown";
            return "unknown";
        }
        if (auto call = dynamic_cast<const CallExpr*>(e)) {
            // callee should be identifier (function name) or another expression returning a function (not implemented)
            auto id = dynamic_cast<const Identifier*>(call->callee.get());
            if (!id) throw std::runtime_error("Call target must be a function identifier");
            auto sym = env->lookup(id->name);
            if (!sym) throw std::runtime_error("Call to undefined function: " + id->name);
            if (sym->kind != SymbolKind::Function) throw std::runtime_error("Identifier is not a function: " + id->name);

            // check args
            if (call->args.size() != sym->paramTypes.size()) {
                // allow mismatch if declared types are "unknown"? For now enforce exact count
                throw std::runtime_error("Argument count mismatch in call to " + id->name);
            }
            for (size_t i = 0; i < call->args.size(); ++i) {
                std::string argt = analyzeExpr(call->args[i].get());
                std::string expected = sym->paramTypes[i];
                if (expected != "unknown" && argt != "unknown" && expected != argt) {
                    throw std::runtime_error("Argument type mismatch for parameter " + std::to_string(i) + " in call to " + id->name);
                }
            }
            exprTypes[e] = sym->returnType;
            return sym->returnType;
        }
        if (auto ife = dynamic_cast<const IfExpr*>(e)) {
            std::string condt = analyzeExpr(ife->cond.get());
            if (condt != "bool" && condt != "unknown") throw std::runtime_error("If condition must be bool");
            std::string thenT = analyzeExpr(ife->thenExpr.get());
            std::string elseT = analyzeExpr(ife->elseExpr.get());
            if (thenT != elseT && thenT != "unknown" && elseT != "unknown") {
                throw std::runtime_error("If-expression branches must return same type: then=" + thenT + " else=" + elseT);
            }
            std::string resultType = (thenT != "unknown" ? thenT : elseT);
            exprTypes[e] = resultType;
            return resultType;
        }

        // unknown fallback
        exprTypes[e] = "unknown";
        return "unknown";
    }
};
