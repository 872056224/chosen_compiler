#include <ll1/IR/Type.h>

namespace ll1 {

Type Type::VoidTy(TypeKind::Void);
Type Type::Int1Ty(TypeKind::Int1);
Type Type::Int8Ty(TypeKind::Int8);
Type Type::Int16Ty(TypeKind::Int16);

Type *Type::getVoidTy() { return &VoidTy; }
Type *Type::getInt1Ty() { return &Int1Ty; }
Type *Type::getInt8Ty() { return &Int8Ty; }
Type *Type::getInt16Ty() { return &Int16Ty; }

} // namespace ll1
