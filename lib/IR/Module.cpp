#include <ll1/IR/Module.h>
#include <ll1/IR/Function.h>

namespace ll1 {

Module::Module(const std::string &name) : Name(name) {}

Function *Module::getFunction(const std::string &name) {
    for (auto &fn : Functions) {
        if (fn->getName() == name) return fn.get();
    }
    return nullptr;
}

Function *Module::createFunction(Type *returnTy, const std::string &name) {
    auto fn = std::make_unique<Function>(returnTy, name);
    auto *raw = fn.get();
    Functions.push_back(std::move(fn));
    return raw;
}

void Module::addFunction(std::unique_ptr<Function> fn) {
    Functions.push_back(std::move(fn));
}

} // namespace ll1
