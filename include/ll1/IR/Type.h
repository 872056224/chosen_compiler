#ifndef LL1_IR_TYPE_H
#define LL1_IR_TYPE_H

namespace ll1 {

class Type {
public:
    enum class TypeKind { Void, Int1, Int8, Int16 };

    TypeKind getKind() const { return Kind; }

    static Type *getVoidTy();
    static Type *getInt1Ty();
    static Type *getInt8Ty();
    static Type *getInt16Ty();

private:
    Type(TypeKind k) : Kind(k) {}
    TypeKind Kind;

    static Type VoidTy;
    static Type Int1Ty;
    static Type Int8Ty;
    static Type Int16Ty;
};

} // namespace ll1

#endif
