#include <ll1/Opt/PassManager.h>
#include <ll1/Opt/Passes/Dominators.h>
#include <ll1/Opt/Passes/Mem2Reg.h>
#include <ll1/Opt/Passes/PhiElim.h>
#include <ll1/Opt/Passes/InstCombine.h>
#include <ll1/Opt/Passes/Reassociate.h>
#include <ll1/Opt/Passes/GVN.h>
#include <ll1/Opt/Passes/SimplifyCFG.h>
#include <ll1/Opt/Passes/DCE.h>
#include <ll1/Opt/Passes/LoopUnroll.h>
#include <ll1/IR/LLVMContext.h>
#include <ll1/IR/IRPrinter.h>
#include <fstream>
#include <iostream>

namespace ll1 {

static std::string s_passDumpDir;
static int s_passDumpCounter = 0;

void setPassDumpDir(const std::string &dir) {
    s_passDumpDir = dir;
    s_passDumpCounter = 0;
}

// ============================================================
// Static member definitions
// ============================================================
AnalysisSetKey CFGAnalyses::SetKey;

// ============================================================
// FunctionPassManager
// ============================================================
PreservedAnalyses FunctionPassManager::run(Function &F, FunctionAnalysisManager &AM) {
    PreservedAnalyses PA = PreservedAnalyses::all();

    for (auto &pass : Passes) {
        std::cerr << "[opt] Running function pass: " << pass->passName()
                  << " on " << F.getName() << std::endl;
        PreservedAnalyses passPA = pass->run(F, AM);
        PA.intersect(passPA);
        AM.invalidate(F, passPA);

        // --dump-pass-ir: write IR after each pass
        if (!s_passDumpDir.empty() && F.getName() != "__print" && F.getName() != "__read") {
            s_passDumpCounter++;
            char buf[256];
            snprintf(buf, sizeof(buf), "%s/%02d_after_%s_%s.ll",
                     s_passDumpDir.c_str(), s_passDumpCounter,
                     pass->passName(), F.getName().c_str());
            std::ofstream of(buf);
            IRPrinter::PrintState ps;
            of << "; After " << pass->passName() << " on " << F.getName() << "\n";
            of << IRPrinter::printFunction(F, ps);
        }
    }
    return PA;
}

// ============================================================
// ModulePassManager
// ============================================================
PreservedAnalyses ModulePassManager::run(Module &M, ModuleAnalysisManager &AM) {
    PreservedAnalyses PA = PreservedAnalyses::all();

    for (auto &pass : Passes) {
        std::cerr << "[opt] Running module pass: " << pass->passName() << std::endl;
        PreservedAnalyses passPA = pass->run(M, AM);
        PA.intersect(passPA);
        AM.invalidate(M, passPA);
    }

    return PA;
}

// ============================================================
// ModuleToFunctionPassAdaptor
// ============================================================
PreservedAnalyses ModuleToFunctionPassAdaptor::run(Module &M, ModuleAnalysisManager &/*MAM*/) {
    if (FPM.isEmpty()) return PreservedAnalyses::all();

    for (auto &F : M.getFunctionList()) {
        FunctionAnalysisManager FAM;
        if (registerAnalyses) {
            registerAnalyses(FAM);
        }
        FPM.run(*F, FAM);
    }

    return PreservedAnalyses::none();
}

// ============================================================
// PassBuilder
// ============================================================
ModulePassManager PassBuilder::buildDefaultPipeline() {
    FunctionPassManager FPM;
    FPM.addPass(Mem2Reg());
    FPM.addPass(PhiElimination());  // Lower phi nodes for CodeGen
    FPM.addPass(InstCombine());
    FPM.addPass(Reassociate());
    FPM.addPass(GVN());
    FPM.addPass(SimplifyCFG());
    FPM.addPass(LoopUnroll(8));  // unroll loops with trip-count <= 8
    FPM.addPass(DCSPass());

    ModuleToFunctionPassAdaptor adaptor(std::move(FPM));
    adaptor.registerAnalyses = [](FunctionAnalysisManager &FAM) {
        FAM.registerPass<DominatorTreeAnalysis>([]() {
            return DominatorTreeAnalysis();
        });
    };

    ModulePassManager MPM;
    MPM.addPass(std::move(adaptor));
    return MPM;
}

} // namespace ll1
