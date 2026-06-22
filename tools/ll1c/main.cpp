#include <ll1/Driver/Compiler.h>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: ll1c <source.ll1> [-o output] [--emit-llvm] [--opt] [--new-codegen] [--dump-pass-ir=<dir>]\n";
        std::cerr << "  -o <file>         output file (default: output.asm)\n";
        std::cerr << "  --emit-llvm       emit LLVM IR instead of 8086 assembly\n";
        std::cerr << "  --opt             run optimization pipeline (old CodeGen)\n";
        std::cerr << "  --new-codegen     use new Machine IR pipeline (SDAG→ISel→RA→Print)\n";
        std::cerr << "  --dump-pass-ir=<dir>  dump IR after each pass into <dir>\n";
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = "output.asm";
    bool emitLLVM = false;
    bool enableOpt = false;
    bool newCodegen = false;
    std::string dumpPassDir;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "--emit-llvm") {
            emitLLVM = true;
            if (outputFile == "output.asm") outputFile = "output.ll";
        } else if (arg == "--opt") {
            enableOpt = true;
        } else if (arg == "--new-codegen") {
            newCodegen = true;
        } else if (arg.rfind("--dump-pass-ir=", 0) == 0) {
            dumpPassDir = arg.substr(15);  // "--dump-pass-ir=" is 15 chars
            enableOpt = true;  // implied
        }
    }

    ll1::Compiler compiler;
    std::string output;

    if (emitLLVM) {
        output = compiler.compileFileIR(inputFile);
    } else if (!dumpPassDir.empty()) {
        output = compiler.compileFileOptDump(inputFile, dumpPassDir);
    } else if (newCodegen && enableOpt) {
        output = compiler.compileFileNewOpt(inputFile);
    } else if (newCodegen) {
        output = compiler.compileFileNew(inputFile);
    } else if (enableOpt) {
        output = compiler.compileFileOpt(inputFile);
    } else {
        output = compiler.compileFile(inputFile);
    }

    if (compiler.hasError()) {
        std::cerr << compiler.getErrorMsg();
        return 1;
    }

    std::ofstream out(outputFile);
    if (!out.is_open()) {
        std::cerr << "Error: cannot write to " << outputFile << "\n";
        return 1;
    }
    out << output;
    out.close();

    std::cout << "Compiled " << inputFile << " -> " << outputFile << "\n";
    return 0;
}
