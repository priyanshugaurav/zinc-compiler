#pragma once
#include "ast.h"   // Use your existing AST definitions
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <fstream>

struct CodeGenContext {
    std::map<std::string,std::string> string_labels;  // string -> label
    std::map<std::string,int> locals;                // variable -> stack offset
    int stack_offset = 0;

    std::string add_string(const std::string &s);
    std::string escape_string(const std::string &s);
};

// Forward declarations
void gen_expr(std::ofstream &out, const Expr *expr, CodeGenContext &ctx);
void gen_stmt(std::ofstream &out, const Stmt *stmt, CodeGenContext &ctx);
void gen_program(std::ofstream &out, const std::vector<Stmt::Ptr> &program);
