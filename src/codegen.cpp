#include "codegen.h"
#include <fstream>
#include <functional>
#include <iostream>

static int label_count = 0;
static std::string gen_label(const std::string &base) { return base + "_" + std::to_string(label_count++); }

std::string CodeGenContext::add_string(const std::string &s)
{
    if (string_labels.count(s) == 0)
        string_labels[s] = "str_" + std::to_string(string_labels.size());
    return string_labels[s];
}

std::string escape_string(const std::string &input)
{
    std::string output;
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] == '\\' && i + 1 < input.size())
        {
            char next = input[i + 1];
            if (next == 'n')
            {
                output += '\n'; // actual newline byte
                i++;
            }
            else if (next == 't')
            {
                output += '\t';
                i++;
            }
            else if (next == '\\')
            {
                output += '\\';
                i++;
            }
            else
            {
                // Unknown escape, output as is
                output += input[i];
            }
        }
        else
        {
            output += input[i];
        }
    }
    return output;
}

// ---------------- Data Section ----------------
static void write_data_section(std::ofstream &out, CodeGenContext &ctx)
{
    out << "section .data\n";
    for (auto &p : ctx.string_labels)
    {
        std::string processed = escape_string(p.first);
        out << p.second << ": db ";
        for (char c : processed)
        {
            out << (int)(unsigned char)c << ",";
        }
        out << "0\n"; // optional trailing newline and null terminator
    }

    out << "section .bss\n";
    out << "num_buf: resb 20\n";
}

// Forward declarations
void gen_expr(std::ofstream &out, const Expr *expr, CodeGenContext &ctx);
void gen_stmt(std::ofstream &out, const Stmt *stmt, CodeGenContext &ctx);

// ---------------- Expression Generation ----------------
void gen_expr(std::ofstream &out, const Expr *expr, CodeGenContext &ctx)
{
    if (auto n = dynamic_cast<const NumberLiteral *>(expr))
    {
        out << "    mov rax," << n->value << "\n";
    }
    else if (auto id = dynamic_cast<const Identifier *>(expr))
    {
        // Prefer codegen env lookup; fall back to semantic symbol's stackOffset
        try
        {
            int off = ctx.lookupLocal(id->name);
            out << "    mov rax,[rbp-" << off << "]\n";
        }
        catch (...)
        {
            if (ctx.semEnv)
            {
                auto s = ctx.semEnv->lookup(id->name);
                if (s && s->stackOffset)
                {
                    out << "    mov rax,[rbp-" << s->stackOffset << "]\n";
                }
            }
        }
    }

    else if (auto bin = dynamic_cast<const BinaryExpr *>(expr))
    {
        gen_expr(out, bin->left.get(), ctx);
        out << "    push rax\n";
        gen_expr(out, bin->right.get(), ctx);
        out << "    mov rbx,rax\n    pop rax\n";
        if (bin->op == "+")
            out << "    add rax,rbx\n";
        else if (bin->op == "-")
            out << "    sub rax,rbx\n";
        else if (bin->op == "*")
            out << "    imul rax,rbx\n";
        else if (bin->op == "%")
            out << "    cqo\n    idiv rbx\n    mov rax,rdx\n";
        else if (bin->op == "/")
            out << "    cqo\n    idiv rbx\n"; // result in RAX

        else if (bin->op == "=")
        {
            if (auto idl = dynamic_cast<const Identifier *>(bin->left.get()))
            {
                out << "    mov [rbp-" << ctx.locals[idl->name] << "],rbx\n";
                out << "    mov rax,rbx\n";
            }
        }
        // Comparison (sets 0 or 1 in rax)
        else if (bin->op == "==")
        {
            out << "    cmp rax,rbx\n";
            out << "    sete al\n";
            out << "    movzx rax,al\n";
        }
        else if (bin->op == "!=")
        {
            out << "    cmp rax,rbx\n";
            out << "    setne al\n";
            out << "    movzx rax,al\n";
        }
        else if (bin->op == "<")
        {
            out << "    cmp rax,rbx\n";
            out << "    setl al\n";
            out << "    movzx rax,al\n";
        }
        else if (bin->op == "<=")
        {
            out << "    cmp rax,rbx\n";
            out << "    setle al\n";
            out << "    movzx rax,al\n";
        }
        else if (bin->op == ">")
        {
            out << "    cmp rax,rbx\n";
            out << "    setg al\n";
            out << "    movzx rax,al\n";
        }
        else if (bin->op == ">=")
        {
            out << "    cmp rax,rbx\n";
            out << "    setge al\n";
            out << "    movzx rax,al\n";
        }

        // Logical (assume non-zero = true)
        else if (bin->op == "&&")
        {
            std::string label_false = gen_label("and_false");
            std::string label_end = gen_label("and_end");
            out << "    cmp rax,0\n";
            out << "    je " << label_false << "\n";
            out << "    cmp rbx,0\n";
            out << "    je " << label_false << "\n";
            out << "    mov rax,1\n";
            out << "    jmp " << label_end << "\n";
            out << label_false << ":\n";
            out << "    xor rax,rax\n";
            out << label_end << ":\n";
        }
        else if (bin->op == "||")
        {
            std::string label_true = gen_label("or_true");
            std::string label_end = gen_label("or_end");
            out << "    cmp rax,0\n";
            out << "    jne " << label_true << "\n";
            out << "    cmp rbx,0\n";
            out << "    jne " << label_true << "\n";
            out << "    xor rax,rax\n";
            out << "    jmp " << label_end << "\n";
            out << label_true << ":\n";
            out << "    mov rax,1\n";
            out << label_end << ":\n";
        }

        // Bitwise
        else if (bin->op == "&")
            out << "    and rax,rbx\n";
        else if (bin->op == "|")
            out << "    or rax,rbx\n";
        else if (bin->op == "^")
            out << "    xor rax,rbx\n";
        else if (bin->op == "<<")
        {
            out << "    mov cl, bl\n"; // Move lower 8 bits of rbx into cl (shift count)
            out << "    shl rax, cl\n";
        }
        else if (bin->op == ">>")
        {
            out << "    mov cl, bl\n";
            out << "    shr rax, cl\n";
        }
    }
    else if (auto ife = dynamic_cast<const IfExpr *>(expr))
    {
        std::string elseLabel = gen_label("else");
        std::string endLabel = gen_label("ifend");

        gen_expr(out, ife->cond.get(), ctx); // result in rax
        out << "    cmp rax, 0\n";
        out << "    je ." << elseLabel << "\n";

        gen_expr(out, ife->thenExpr.get(), ctx); // result in rax
        out << "    jmp ." << endLabel << "\n";

        out << "." << elseLabel << ":\n";
        gen_expr(out, ife->elseExpr.get(), ctx); // result in rax

        out << "." << endLabel << ":\n";
    }

    else if (auto c = dynamic_cast<const CallExpr *>(expr))
    {
        if (auto idc = dynamic_cast<const Identifier *>(c->callee.get()))
        {
            if (idc->name == "print")
            {
                out << "    push r12\n";
                out << "    xor r12, r12\n"; // total length

                for (auto &arg : c->args)
                {
                    if (auto sl = dynamic_cast<const StringLiteral *>(arg.get()))
                    {
                        std::string lbl = ctx.add_string(sl->value);
                        out << "    mov rax, 1\n";
                        out << "    mov rdi, 1\n";
                        out << "    lea rsi, [rel " << lbl << "]\n";
                        out << "    mov rdx, " << sl->value.size() << "\n";
                        out << "    syscall\n";

                        out << "    add r12, rdx\n";
                    }
                    else
                    {
                        gen_expr(out, arg.get(), ctx); // result in rax
                        out << "    mov rbx, rax\n";
                        out << "    lea rdi, [rel num_buf+19]\n";

                        std::string label = gen_label("conv");

                        // Special case for zero
                        out << "    cmp rbx, 0\n";
                        out << "    jne ." << label << "_start\n";
                        out << "    dec rdi\n";
                        out << "    mov byte [rdi], '0'\n";
                        out << "    mov r8, rdi\n";
                        out << "    jmp ." << label << "_done\n";

                        out << "." << label << "_start:\n";
                        out << "    mov r8, rdi\n";
                        out << "." << label << "_loop:\n";
                        out << "    xor rdx, rdx\n";
                        out << "    mov rax, rbx\n";
                        out << "    mov rcx, 10\n";
                        out << "    div rcx\n";
                        out << "    add dl, '0'\n";
                        out << "    dec rdi\n";
                        out << "    mov [rdi], dl\n";
                        out << "    mov rbx, rax\n";
                        out << "    test rax, rax\n";
                        out << "    jnz ." << label << "_loop\n";

                        out << "." << label << "_done:\n";
                        out << "    mov rsi, rdi\n";
                        out << "    mov rdx, r8\n";
                        out << "    sub rdx, rdi\n";
                        out << "    mov rax, 1\n";
                        out << "    mov rdi, 1\n";
                        out << "    syscall\n";

                        out << "    add r12, rdx\n";
                    }
                }

                out << "    mov rax, r12\n"; // total printed
                out << "    pop r12\n";
            }
            else if (idc->name == "scan")
            {
                out << "    mov rax, 0\n";
                out << "    mov rdi, 0\n";
                out << "    lea rsi, [rel input_buf]\n";
                out << "    mov rdx, 32\n";
                out << "    syscall\n";

                out << "    mov rcx, 0\n";         // accumulator
                out << "    mov rsi, input_buf\n"; // pointer to buffer
                out << "    mov rdx, rax\n";       // bytes read

                std::string loopLabel = gen_label("scan_loop");
                std::string doneLabel = gen_label("scan_done");

                out << loopLabel << ":\n";
                out << "    cmp rdx, 0\n";
                out << "    je " << doneLabel << "\n";
                out << "    mov al, byte [rsi]\n";
                out << "    cmp al, '0'\n";
                out << "    jl skip_char_" << loopLabel << "\n";
                out << "    cmp al, '9'\n";
                out << "    jg skip_char_" << loopLabel << "\n";
                out << "    sub al, '0'\n";
                out << "    imul rcx, rcx, 10\n";
                out << "    add rcx, rax\n";
                out << "skip_char_" << loopLabel << ":\n";
                out << "    inc rsi\n";
                out << "    dec rdx\n";
                out << "    jmp " << loopLabel << "\n";
                out << doneLabel << ":\n";
                out << "    mov rax, rcx\n"; // integer return in rax
            }

            else
            {
                static const std::string regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                for (size_t i = 0; i < c->args.size(); ++i)
                {
                    gen_expr(out, c->args[i].get(), ctx);
                    out << "    mov " << regs[i] << ",rax\n";
                }
                out << "    call " << idc->name << "\n";
            }
        }
    }
}

// ---------------- Statement Generation ----------------
void gen_stmt(std::ofstream &out, const Stmt *stmt, CodeGenContext &ctx)
{
    if (auto f = dynamic_cast<const FunctionDecl *>(stmt))
    {
        // reset context for new function
        ctx.envStack.clear();
        ctx.envStack.emplace_back();
        ctx.stack_offset = 0;

        // allocate params first (so params are at lower offsets)
        static const std::string regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        for (size_t i = 0; i < f->params.size(); ++i)
        {
            int off = ctx.allocateLocal(f->params[i].first);
            // we don't emit mov regs->stack until after prologue below
        }

        // Pre-scan function body for let-decls (recursively) and allocate offsets
        std::function<void(const Stmt *)> preScan = [&](const Stmt *s)
        {
            if (!s)
                return;
            if (auto b = dynamic_cast<const BlockStmt *>(s))
            {
                for (auto &st : b->stmts)
                    preScan(st.get());
            }
            else if (auto l = dynamic_cast<const LetStmt *>(s))
            {
                ctx.allocateLocal(l->name);
            }
            else if (auto iff = dynamic_cast<const IfStmt *>(s))
            {
                preScan(iff->thenBranch.get());
                if (iff->elseBranch)
                    preScan(iff->elseBranch.get());
            }
            else if (auto wh = dynamic_cast<const WhileStmt *>(s))
            {
                preScan(wh->body.get());
            }
            else if (auto fn = dynamic_cast<const FunctionDecl *>(s))
            {
                // nested function: don't include its locals in parent function frame
            }
        };

        if (auto b = dynamic_cast<const BlockStmt *>(f->body.get()))
            preScan(b);

        // emit label / prologue
        out << f->name << ":\n";
        out << "    push rbp\n    mov rbp,rsp\n";
        out << "    sub rsp," << ctx.stack_offset << "\n";

        // now move parameters to their allocated stack slots
        for (size_t i = 0; i < f->params.size(); ++i)
        {
            int off;
            try
            {
                off = ctx.lookupLocal(f->params[i].first);
            }
            catch (...)
            {
                continue;
            }
            out << "    mov [rbp-" << off << "]," << regs[i] << "\n";
        }

        // generate body
        gen_stmt(out, f->body.get(), ctx);

        out << "    leave\n    ret\n";
    }

    else if (auto b = dynamic_cast<const BlockStmt *>(stmt))
        for (auto &s : b->stmts)
            gen_stmt(out, s.get(), ctx);
    else if (auto l = dynamic_cast<const LetStmt *>(stmt))
    {
        if (l->init)
        {
            gen_expr(out, l->init.get(), ctx);
            int off = ctx.lookupLocal(l->name);
            out << "    mov [rbp-" << off << "],rax\n";
        }
    }

    else if (auto r = dynamic_cast<const ReturnStmt *>(stmt))
    {
        if (r->value)
            gen_expr(out, r->value.get(), ctx);
        out << "    leave\n    ret\n";
    }
    else if (auto e = dynamic_cast<const ExprStmt *>(stmt))
        gen_expr(out, e->expr.get(), ctx);

    else if (auto i = dynamic_cast<const IfStmt *>(stmt))
    {
        std::string label_else = gen_label("else");
        std::string label_end = gen_label("ifend");

        // Evaluate the if condition
        gen_expr(out, i->cond.get(), ctx);
        out << "    cmp rax, 0\n";
        out << "    je " << label_else << "\n";
        gen_stmt(out, i->thenBranch.get(), ctx);
        out << "    jmp " << label_end << "\n";

        out << label_else << ":\n";
        if (i->elseBranch)
        {
            // Else-if can come here if represented similarly in AST
            gen_stmt(out, i->elseBranch.get(), ctx);
        }
        out << label_end << ":\n";
    }

    else if (auto w = dynamic_cast<const WhileStmt *>(stmt))
    {
        std::string label_start = gen_label("while_start");
        std::string label_end = gen_label("while_end");

        out << label_start << ":\n";

        // Evaluate condition
        gen_expr(out, w->cond.get(), ctx);
        out << "    cmp rax, 0\n";
        out << "    je " << label_end << "\n";

        // Generate loop body
        gen_stmt(out, w->body.get(), ctx);

        // Jump back to start
        out << "    jmp " << label_start << "\n";
        out << label_end << ":\n";
    }
}

// ---------------- Program Generation ----------------
// Forward declaration
void collect_strings_expr(const Expr *expr, CodeGenContext &ctx);

void collect_strings(const Stmt *stmt, CodeGenContext &ctx)
{
    if (auto f = dynamic_cast<const FunctionDecl *>(stmt))
        collect_strings(f->body.get(), ctx);
    else if (auto b = dynamic_cast<const BlockStmt *>(stmt))
        for (auto &s : b->stmts)
            collect_strings(s.get(), ctx);
    else if (auto e = dynamic_cast<const ExprStmt *>(stmt))
        collect_strings_expr(e->expr.get(), ctx);
    else if (auto i = dynamic_cast<const IfStmt *>(stmt))
    {
        collect_strings_expr(i->cond.get(), ctx);
        collect_strings(i->thenBranch.get(), ctx);
        if (i->elseBranch)
            collect_strings(i->elseBranch.get(), ctx);
    }
    else if (auto w = dynamic_cast<const WhileStmt *>(stmt))
    {
        collect_strings_expr(w->cond.get(), ctx);
        collect_strings(w->body.get(), ctx);
    }
    else if (auto l = dynamic_cast<const LetStmt *>(stmt))
    {
        if (l->init)
            collect_strings_expr(l->init.get(), ctx);
    }

    // etc
}

void collect_strings_expr(const Expr *expr, CodeGenContext &ctx)
{
    if (!expr)
        return;

    if (auto c = dynamic_cast<const CallExpr *>(expr))
    {
        for (auto &arg : c->args)
        {
            if (auto sl = dynamic_cast<const StringLiteral *>(arg.get()))
            {
                ctx.add_string(sl->value);
            }
            else
            {
                collect_strings_expr(arg.get(), ctx);
            }
        }
    }
    else if (auto bin = dynamic_cast<const BinaryExpr *>(expr))
    {
        collect_strings_expr(bin->left.get(), ctx);
        collect_strings_expr(bin->right.get(), ctx);
    }
    else if (auto id = dynamic_cast<const Identifier *>(expr))
    {
        // No strings to collect
    }
    else if (auto n = dynamic_cast<const NumberLiteral *>(expr))
    {
        // No strings to collect
    }
    // ✅ Add this block:
    else if (auto sl = dynamic_cast<const StringLiteral *>(expr))
    {
        ctx.add_string(sl->value);
    }
}

void gen_program(std::ofstream &out, const std::vector<Stmt::Ptr> &program)
{
    CodeGenContext ctx;

    // Collect all strings from all statements and their expressions, recursively
    for (auto &stmt : program)
        collect_strings(stmt.get(), ctx);

    write_data_section(out, ctx);

    out << "section .text\n";
    out << "global _start\n";

    out << "_start:\n";
    out << "    call main\n";
    out << "    mov rax,60\n    xor rdi,rdi\n    syscall\n";

    for (auto &stmt : program)
        gen_stmt(out, stmt.get(), ctx);
}
