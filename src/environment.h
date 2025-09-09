#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <iostream>

// Simple symbol kind
enum class SymbolKind { Var, Function };

struct Symbol {
    std::string name;
    SymbolKind kind = SymbolKind::Var;

    // For variables:
    std::string type = "unknown"; // e.g. "int", "string", "bool", "void", or "unknown"

    // For functions:
    std::vector<std::string> paramTypes; // param types (may contain "unknown")
    std::string returnType = "unknown";

    // optional: codegen may fill this
    int stackOffset = 0;
};

struct EnvNode {
    std::unordered_map<std::string, std::shared_ptr<Symbol>> table;
    std::shared_ptr<EnvNode> parent;
    EnvNode(std::shared_ptr<EnvNode> p = nullptr) : parent(p) {}
};

class Environment {
public:
    Environment() : current(std::make_shared<EnvNode>(nullptr)) {}
    Environment(std::shared_ptr<EnvNode> node) : current(node) {}

    // create child scope and return pointer to new current node
    std::shared_ptr<EnvNode> push() {
        current = std::make_shared<EnvNode>(current);
        return current;
    }

    // pop scope (go to parent). If already global, keep global.
    void pop() {
        if (current->parent) current = current->parent;
    }

    // define a symbol in the *current* scope.
    // returns false if name already exists in current scope
    bool define(const std::string &name, std::shared_ptr<Symbol> sym) {
        if (current->table.find(name) != current->table.end()) return false;
        current->table[name] = sym;
        return true;
    }

    // look up a symbol in current scope chain (current -> parent -> ... -> global)
    std::shared_ptr<Symbol> lookup(const std::string &name) const {
        for (auto node = current; node; node = node->parent) {
            auto it = node->table.find(name);
            if (it != node->table.end()) return it->second;
        }
        return nullptr;
    }

    // lookup only in the current scope
    std::shared_ptr<Symbol> lookupCurrent(const std::string &name) const {
        auto it = current->table.find(name);
        if (it != current->table.end()) return it->second;
        return nullptr;
    }

    // debug dump (prints scopes from current up to global)
    void dump(std::ostream &os = std::cout) const {
        int depth = 0;
        for (auto node = current; node; node = node->parent) {
            os << "Scope depth " << depth++ << ":\n";
            for (auto &p : node->table) {
                auto s = p.second;
                os << "  " << p.first << " : ";
                if (s->kind == SymbolKind::Var) {
                    os << "var " << s->type;
                } else {
                    os << "fn (";
                    for (size_t i = 0; i < s->paramTypes.size(); ++i) {
                        if (i) os << ", ";
                        os << s->paramTypes[i];
                    }
                    os << ") -> " << s->returnType;
                }
                os << "  [stackOffset=" << s->stackOffset << "]\n";
            }
        }
    }

    std::shared_ptr<EnvNode> getCurrentNode() const { return current; }

private:
    std::shared_ptr<EnvNode> current;
};
