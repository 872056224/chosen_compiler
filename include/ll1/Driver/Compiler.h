#ifndef LL1_DRIVER_COMPILER_H
#define LL1_DRIVER_COMPILER_H

#include <string>
#include <memory>

namespace ll1 {

class Compiler {
public:
    Compiler();

    // Compile source string, return assembly output
    // Returns empty string on error
    std::string compile(const std::string &source);

    // Compile from file
    std::string compileFile(const std::string &filename);

    bool hasError() const { return HasError; }
    const std::string &getErrorMsg() const { return ErrorMsg; }

private:
    bool HasError = false;
    std::string ErrorMsg;
};

} // namespace ll1
#endif
