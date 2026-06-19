#ifndef LL1_SEMA_SCOPE_H
#define LL1_SEMA_SCOPE_H

#include <string>
#include <map>
#include <vector>

namespace ll1 {

class ASTNode;
class FuncDecl;
enum class BuiltinType;

struct Symbol {
    std::string Name;
    BuiltinType Ty;
    const ASTNode *Node = nullptr;  // Points to VarDecl or FuncDecl
    bool IsFunction = false;
    bool IsParameter = false;
};

class Scope {
public:
    enum class ScopeKind { Global, Function, Block };

    explicit Scope(ScopeKind kind, Scope *parent = nullptr);

    ScopeKind getKind() const { return Kind; }
    Scope *getParent() const { return Parent; }

    bool insert(const Symbol &sym);
    const Symbol *lookup(const std::string &name) const;
    const Symbol *lookupLocal(const std::string &name) const;

private:
    ScopeKind Kind;
    Scope *Parent;
    std::map<std::string, Symbol> Symbols;
};

class SymbolTable {
public:
    SymbolTable();

    void enterScope(Scope::ScopeKind kind);
    void exitScope();
    Scope *currentScope() { return CurrentScope; }

    bool declare(const Symbol &sym);
    const Symbol *lookup(const std::string &name) const;

private:
    Scope *CurrentScope; // Top of scope stack
};

} // namespace ll1
#endif
