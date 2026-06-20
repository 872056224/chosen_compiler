#include <ll1/Driver/Compiler.h>
#include <ll1/Lex/Lexer.h>
#include <ll1/Parse/Parser.h>
#include <ll1/Sema/Sema.h>
#include <ll1/IRGen/IRGen.h>
#include <ll1/IR/IRPrinter.h>
#include <ll1/CodeGen/CodeGen.h>
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

std::string Compiler::compile(const std::string &source) {
    HasError = false;
    ErrorMsg.clear();

    auto mod = runFrontend(source);
    if (!mod) return "";

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

} // namespace ll1
