#ifndef LL1_DRIVER_COMPILER_H
#define LL1_DRIVER_COMPILER_H

#include <string>
#include <memory>

namespace ll1 {

class Module;

class Compiler {
public:
    Compiler();

    // Compile source string, return assembly output
    std::string compile(const std::string &source);

    // Compile source string, return LLVM IR (.ll format)
    std::string compileIR(const std::string &source);

    // Compile from file
    std::string compileFile(const std::string &filename);
    std::string compileFileIR(const std::string &filename);

    bool hasError() const { return HasError; }
    const std::string &getErrorMsg() const { return ErrorMsg; }

private:
    std::unique_ptr<Module> runFrontend(const std::string &source);

    bool HasError = false;
    std::string ErrorMsg;
};

} // namespace ll1
#endif
