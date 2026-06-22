#include <ll1/Driver/Compiler.h>
#include <ll1/Lex/Lexer.h>
#include <ll1/Parse/Parser.h>
#include <ll1/Sema/Sema.h>
#include <ll1/IRGen/IRGen.h>
#include <ll1/IR/IRPrinter.h>
#include <ll1/CodeGen/CodeGen.h>
#include <ll1/CodeGen/Target8086/Target8086.h>
#include <ll1/Opt/PassManager.h>
#include <sstream>
#include <fstream>
#include <iostream>

namespace ll1 {

Compiler::Compiler() {}

// Shared: run frontend + IR generation, return Module (nullptr on error)
std::unique_ptr<Module> Compiler::runFrontend(const std::string &source) {
    std::istringstream in(source);
    Lexer lexer(in);

    Parser parser(lexer);
    auto prog = parser.parseProgram();
    if (parser.hasError()) {
        HasError = true;
        ErrorMsg = parser.getErrorMsg();
        return nullptr;
    }

    DiagnosticEngine diag;
    Sema sema(diag);
    bool ok = sema.analyze(*prog);
    if (!ok || diag.hasErrors()) {
        HasError = true;
        std::ostringstream ss;
        for (auto &d : diag.getDiagnostics()) {
            if (d.Lvl == Diagnostic::Error) {
                ss << "line " << d.Loc.Line << ": error: " << d.Message << "\n";
            }
        }
        ErrorMsg = ss.str();
        return nullptr;
    }

    IRGen irgen;
    return irgen.generate(*prog);
}

std::unique_ptr<Module> Compiler::optimize(std::unique_ptr<Module> module) {
    if (!module) return nullptr;

    PassBuilder pb;
    auto mpm = pb.buildDefaultPipeline();

    ModuleAnalysisManager MAM;
    mpm.run(*module, MAM);
    return module;
}

std::string Compiler::compileOptDump(const std::string &source, const std::string &dumpDir) {
    HasError = false;
    ErrorMsg.clear();

    auto mod = runFrontend(source);
    if (!mod) return "";

    // Enable per-pass IR dump
    setPassDumpDir(dumpDir);

    mod = optimize(std::move(mod));

    CodeGen cg;
    auto mm = cg.generate(*mod);
    return cg.emitAssembly(mm);
}

std::string Compiler::compileFileOptDump(const std::string &filename, const std::string &dumpDir) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        HasError = true;
        ErrorMsg = "cannot open file: " + filename;
        return "";
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return compileOptDump(source, dumpDir);
}

std::string Compiler::compile(const std::string &source) {
    HasError = false;
    ErrorMsg.clear();

    auto mod = runFrontend(source);
    if (!mod) return "";

    CodeGen cg;
    auto mm = cg.generate(*mod);
    return cg.emitAssembly(mm);
}

std::string Compiler::compileOpt(const std::string &source) {
    HasError = false;
    ErrorMsg.clear();

    auto mod = runFrontend(source);
    if (!mod) return "";

    mod = optimize(std::move(mod));

    CodeGen cg;
    auto mm = cg.generate(*mod);
    return cg.emitAssembly(mm);
}

std::string Compiler::compileIR(const std::string &source) {
    HasError = false;
    ErrorMsg.clear();

    auto mod = runFrontend(source);
    if (!mod) return "";

    return IRPrinter::print(*mod);
}

std::string Compiler::compileFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        HasError = true;
        ErrorMsg = "cannot open file: " + filename;
        return "";
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return compile(source);
}

std::string Compiler::compileFileOpt(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        HasError = true;
        ErrorMsg = "cannot open file: " + filename;
        return "";
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return compileOpt(source);
}

std::string Compiler::compileFileIR(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        HasError = true;
        ErrorMsg = "cannot open file: " + filename;
        return "";
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return compileIR(source);
}

std::string Compiler::compileNew(const std::string &source) {
    HasError = false;
    ErrorMsg.clear();

    auto mod = runFrontend(source);
    if (!mod) return "";

    Target8086Machine TM;
    CodeGen cg(TM);
    return cg.generateNew(*mod);
}

std::string Compiler::compileNewOpt(const std::string &source) {
    HasError = false;
    ErrorMsg.clear();

    auto mod = runFrontend(source);
    if (!mod) return "";

    mod = optimize(std::move(mod));

    Target8086Machine TM;
    CodeGen cg(TM);
    return cg.generateNew(*mod);
}

std::string Compiler::compileFileNew(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        HasError = true;
        ErrorMsg = "cannot open file: " + filename;
        return "";
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return compileNew(source);
}

std::string Compiler::compileFileNewOpt(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        HasError = true;
        ErrorMsg = "cannot open file: " + filename;
        return "";
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return compileNewOpt(source);
}

} // namespace ll1
