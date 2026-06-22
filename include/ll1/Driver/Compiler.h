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

    // Compile source string, return assembly output (with optimizations)
    std::string compileOpt(const std::string &source);

    // Compile source string, return LLVM IR (.ll format)
    std::string compileIR(const std::string &source);

    // Compile using new Machine IR pipeline (SDAG→ISel→RegAlloc→Print)
    std::string compileNew(const std::string &source);
    std::string compileNewOpt(const std::string &source);
    std::string compileFileNew(const std::string &filename);
    std::string compileFileNewOpt(const std::string &filename);

    // Compile from file
    std::string compileFile(const std::string &filename);
    std::string compileFileOpt(const std::string &filename);
    std::string compileFileIR(const std::string &filename);

    // Compile with per-pass IR dump (--dump-pass-ir=<dir>)
    std::string compileOptDump(const std::string &source, const std::string &dumpDir);
    std::string compileFileOptDump(const std::string &filename, const std::string &dumpDir);

    bool hasError() const { return HasError; }
    const std::string &getErrorMsg() const { return ErrorMsg; }

private:
    std::unique_ptr<Module> runFrontend(const std::string &source);
    std::unique_ptr<Module> optimize(std::unique_ptr<Module> module);

    bool HasError = false;
    std::string ErrorMsg;
};

} // namespace ll1
#endif
