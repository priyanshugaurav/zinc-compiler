#pragma once
#include "ast.h"   // Use your existing AST definitions
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <unordered_map>
#include "environment.h"

struct CodeGenContext {
    std::map<std::string, int> locals;
    std::unordered_map<std::string,std::string> string_labels;
    std::shared_ptr<Environment> semEnv;          // set by caller (from SemanticAnalyzer)
    std::vector<std::unordered_map<std::string,int>> envStack; // codegen scopes (name -> offset)
    int stack_offset = 0; // total bytes allocated for this function so far

    CodeGenContext() { envStack.emplace_back(); }

    void pushScope() { envStack.emplace_back(); }
    void popScope()  { if (envStack.size()>1) envStack.pop_back(); }

    // allocate local in current codegen scope (8 bytes)
    int allocateLocal(const std::string &name) {
        stack_offset += 8;
        envStack.back()[name] = stack_offset;
        // also update semantic symbol if available
        if (semEnv) {
            auto s = semEnv->lookup(name);
            if (s) s->stackOffset = stack_offset;
        }
        return stack_offset;
    }

    // lookup local offset going outward from inner->outer codegen scopes
    int lookupLocal(const std::string &name) const {
        for (auto it = envStack.rbegin(); it != envStack.rend(); ++it) {
            auto f = it->find(name);
            if (f != it->end()) return f->second;
        }
        throw std::runtime_error("lookupLocal: not found " + name);
    }

    std::string add_string(const std::string &s);
};

// Forward declarations
void gen_expr(std::ofstream &out, const Expr *expr, CodeGenContext &ctx);
void gen_stmt(std::ofstream &out, const Stmt *stmt, CodeGenContext &ctx);
void gen_program(std::ofstream &out, const std::vector<Stmt::Ptr> &program);
