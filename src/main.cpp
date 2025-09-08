// src/main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "codegen.h" // ✅ include codegen
#include <cstdlib>   // For system()

// prototype of lexString defined in lexer.cpp
std::vector<Token> lexString(const std::string &s);

static bool ends_with(const std::string &s, const std::string &suffix)
{
    if (s.size() < suffix.size())
        return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: zinc <source-file.zinc>\n";
        return 1;
    }

    std::string path = argv[1];
    if (!ends_with(path, ".zinc"))
    {
        std::cerr << "Error: input file must have a .zinc extension.\n";
        return 1;
    }

    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in)
    {
        std::cerr << "Error: cannot open file '" << path << "'\n";
        return 1;
    }

    std::ostringstream sb;
    sb << in.rdbuf();
    std::string code = sb.str();

    try
    {
        auto tokens = lexString(code);

        // std::cout << "Tokens:\n";
        for (auto &tk : tokens)
        {
            // std::cout << "  [" << tk.line << ":" << tk.col << "] ";
            // std::cout << (int)tk.type << " '" << tk.value << "'\n";
        }

        Parser parser(tokens);
        Program program = parser.parseProgram();

        // std::cout << "\n=== AST ===\n";
        // for (auto &s : program)
        // {
        //     // s->pretty_print(0);
        //     // std::cout << "----------------\n";
        // }

        // ✅ Generate NASM code
        // std::cout << "\n=== Generating Assembly ===\n";

        std::ofstream out("out.asm");
        gen_program(out, program);
        out.close();
        // std::cout << "Assembly written to out.asm\n";
        // std::cout << "Assembling with NASM...\n";
        if (system("nasm -f elf64 out.asm -o out.o") != 0)
        {
            std::cerr << "Error: NASM failed.\n";
            return 1;
        }

        // ✅ Link with ld
        // std::cout << "Linking with ld...\n";
        if (system("ld out.o -o test") != 0)
        {
            std::cerr << "Error: ld failed.\n";
            return 1;
        }

        // std::cout << "Running zinc_out...\n";
        if (system("./test") != 0)
        {
            std::cerr << "Error: execution failed.\n";
            return 1;
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
