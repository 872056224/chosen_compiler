#include <ll1/Driver/Compiler.h>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: ll1c <source.ll1> [-o output] [--emit-llvm]\n";
        std::cerr << "  -o <file>     output file (default: output.asm)\n";
        std::cerr << "  --emit-llvm   emit LLVM IR instead of 8086 assembly\n";
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = "output.asm";
    bool emitLLVM = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "--emit-llvm") {
            emitLLVM = true;
            if (outputFile == "output.asm") outputFile = "output.ll";
        }
    }

    ll1::Compiler compiler;
    std::string output;

    if (emitLLVM) {
        output = compiler.compileFileIR(inputFile);
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
