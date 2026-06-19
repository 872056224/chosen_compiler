#include <ll1/IR/Type.h>
#include <cassert>
#include <iostream>

int main() {
    auto *i1 = ll1::Type::getInt1Ty();
    auto *i8 = ll1::Type::getInt8Ty();
    auto *i16 = ll1::Type::getInt16Ty();
    auto *v = ll1::Type::getVoidTy();

    assert(i1->getKind() == ll1::Type::TypeKind::Int1);
    assert(i8->getKind() == ll1::Type::TypeKind::Int8);
    assert(i16->getKind() == ll1::Type::TypeKind::Int16);
    assert(v->getKind() == ll1::Type::TypeKind::Void);

    // Singletons: same pointer on repeated calls
    assert(ll1::Type::getInt16Ty() == i16);
    assert(ll1::Type::getVoidTy() == v);

    std::cout << "Type tests passed.\n";
    return 0;
}
