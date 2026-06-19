#include <ll1/Sema/Scope.h>

namespace ll1 {

Scope::Scope(ScopeKind kind, Scope *parent)
    : Kind(kind), Parent(parent) {}

bool Scope::insert(const Symbol &sym) {
    if (Symbols.count(sym.Name)) {
        return false; // Already exists in this scope
    }
    Symbols[sym.Name] = sym;
    return true;
}

const Symbol *Scope::lookup(const std::string &name) const {
    const Scope *s = this;
    while (s) {
        auto it = s->Symbols.find(name);
        if (it != s->Symbols.end()) {
            return &it->second;
        }
        s = s->Parent;
    }
    return nullptr;
}

const Symbol *Scope::lookupLocal(const std::string &name) const {
    auto it = Symbols.find(name);
    return (it != Symbols.end()) ? &it->second : nullptr;
}

SymbolTable::SymbolTable() {
    CurrentScope = new Scope(Scope::ScopeKind::Global, nullptr);
}

void SymbolTable::enterScope(Scope::ScopeKind kind) {
    CurrentScope = new Scope(kind, CurrentScope);
}

void SymbolTable::exitScope() {
    if (CurrentScope && CurrentScope->getParent()) {
        Scope *parent = CurrentScope->getParent();
        delete CurrentScope;
        CurrentScope = parent;
    }
}

bool SymbolTable::declare(const Symbol &sym) {
    return CurrentScope->insert(sym);
}

const Symbol *SymbolTable::lookup(const std::string &name) const {
    return CurrentScope->lookup(name);
}

} // namespace ll1
