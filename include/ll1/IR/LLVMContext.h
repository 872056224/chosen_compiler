#ifndef LL1_IR_LLVMCONTEXT_H
#define LL1_IR_LLVMCONTEXT_H

#include <ll1/IR/Type.h>

namespace ll1 {

class LLVMContext {
public:
    Type *getVoidTy()  { return Type::getVoidTy(); }
    Type *getInt1Ty()  { return Type::getInt1Ty(); }
    Type *getInt8Ty()  { return Type::getInt8Ty(); }
    Type *getInt16Ty() { return Type::getInt16Ty(); }
};

} // namespace ll1
#endif
