#ifndef LL1_IR_MODULE_H
#define LL1_IR_MODULE_H

#include <vector>
#include <memory>
#include <string>

namespace ll1 {

class Function;
class Type;

class Module {
public:
    explicit Module(const std::string &name = "");

    const std::string &getModuleName() const { return Name; }

    const std::vector<std::unique_ptr<Function>> &getFunctionList() const { return Functions; }
    Function *getFunction(const std::string &name);
    Function *createFunction(Type *returnTy, const std::string &name);
    void addFunction(std::unique_ptr<Function> fn);

private:
    std::string Name;
    std::vector<std::unique_ptr<Function>> Functions;
};

} // namespace ll1
#endif
