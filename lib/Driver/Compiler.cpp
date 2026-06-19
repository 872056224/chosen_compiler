#include <ll1/Driver/Compiler.h>
#include <ll1/Lex/Lexer.h>
#include <ll1/Parse/Parser.h>
#include <ll1/Sema/Sema.h>
#include <ll1/IRGen/IRGen.h>
#include <ll1/CodeGen/CodeGen.h>
#include <sstream>
#include <fstream>
#include <iostream>

namespace ll1 {

Compiler::Compiler() {}

std::string Compiler::compile(const std::string &source) {
    HasError = false;
    ErrorMsg.clear();

    // Phase 1: Lex
    std::istringstream in(source);
    Lexer lexer(in);

    // Phase 2: Parse
    Parser parser(lexer);
    auto prog = parser.parseProgram();
    if (parser.hasError()) {
        HasError = true;
        ErrorMsg = parser.getErrorMsg();
        return "";
    }

    // Phase 3: Semantic analysis
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
        return "";
    }

    // Phase 4: IR Generation
    IRGen irgen;
    auto mod = irgen.generate(*prog);

    // Phase 5: Code Generation (8086)
    CodeGen cg;
    auto mm = cg.generate(*mod);
    std::string asmOutput = cg.emitAssembly(mm);

    return asmOutput;
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

} // namespace ll1
