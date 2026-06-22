#ifndef LL1_OPT_PASSMANAGER_H
#define LL1_OPT_PASSMANAGER_H

#include <ll1/IR/Module.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <cassert>
#include <functional>

namespace ll1 {

// ============================================================
// AnalysisKey — opaque, 8-byte aligned analysis identifier
// ============================================================
struct alignas(8) AnalysisKey {};
struct alignas(8) AnalysisSetKey {};

// ============================================================
// Pre-declared analysis set — preserved when CFG is unchanged
// ============================================================
class CFGAnalyses {
public:
    static AnalysisSetKey* ID() { return &SetKey; }
private:
    static AnalysisSetKey SetKey;
};

template<typename IRUnitT>
class AllAnalysesOn {
public:
    static AnalysisSetKey* ID() { return &SetKey; }
private:
    static AnalysisSetKey SetKey;
};

template<typename IRUnitT>
AnalysisSetKey AllAnalysesOn<IRUnitT>::SetKey;

// ============================================================
// PreservedAnalyses
// ============================================================
class PreservedAnalyses {
public:
    static PreservedAnalyses all() {
        PreservedAnalyses PA;
        PA.AllPreserved = true;
        return PA;
    }

    static PreservedAnalyses none() {
        return PreservedAnalyses();
    }

    template<typename AnalysisT>
    PreservedAnalyses& preserve() {
        return preserve(reinterpret_cast<AnalysisKey*>(AnalysisT::ID()));
    }

    template<typename AnalysisSetT>
    PreservedAnalyses& preserveSet() {
        return preserve(reinterpret_cast<AnalysisKey*>(AnalysisSetT::ID()));
    }

    template<typename AnalysisT>
    PreservedAnalyses& abandon() {
        return abandon(reinterpret_cast<AnalysisKey*>(AnalysisT::ID()));
    }

    template<typename AnalysisT>
    bool isPreserved() const {
        if (AllPreserved && NotPreservedIDs.empty()) return true;
        AnalysisKey *id = reinterpret_cast<AnalysisKey*>(AnalysisT::ID());
        if (NotPreservedIDs.count(id)) return false;
        if (AllPreserved) return true;
        return PreservedIDs.count(id) > 0;
    }

    bool areAllPreserved() const { return AllPreserved && NotPreservedIDs.empty(); }

    void intersect(const PreservedAnalyses &other) {
        if (other.areAllPreserved()) return;
        if (areAllPreserved()) {
            *this = other;
            return;
        }
        for (auto *id : other.NotPreservedIDs) {
            PreservedIDs.erase(id);
            NotPreservedIDs.insert(id);
        }
        for (auto it = PreservedIDs.begin(); it != PreservedIDs.end(); ) {
            if (!other.PreservedIDs.count(*it) && !other.AllPreserved)
                it = PreservedIDs.erase(it);
            else
                ++it;
        }
        AllPreserved = false;
    }

private:
    PreservedAnalyses& preserve(AnalysisKey *id) {
        NotPreservedIDs.erase(id);
        if (!AllPreserved) PreservedIDs.insert(id);
        return *this;
    }

    PreservedAnalyses& abandon(AnalysisKey *id) {
        PreservedIDs.erase(id);
        NotPreservedIDs.insert(id);
        return *this;
    }

    std::set<AnalysisKey*> PreservedIDs;
    std::set<AnalysisKey*> NotPreservedIDs;
    bool AllPreserved = false;
};

// ============================================================
// Forward declarations
// ============================================================
template<typename IRUnitT> class AnalysisManager;

using FunctionAnalysisManager = AnalysisManager<Function>;
using ModuleAnalysisManager   = AnalysisManager<Module>;

// ============================================================
// Pass base classes
// ============================================================
class FunctionPass {
public:
    virtual ~FunctionPass() = default;
    virtual PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) = 0;
    virtual const char* passName() const = 0;
};

class ModulePass {
public:
    virtual ~ModulePass() = default;
    virtual PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) = 0;
    virtual const char* passName() const = 0;
};

// ============================================================
// FunctionPassManager
// ============================================================
// Per-pass IR dump control (--dump-pass-ir=<dir>)
void setPassDumpDir(const std::string &dir);

class FunctionPassManager {
public:
    template<typename PassT>
    void addPass(PassT p) {
        Passes.push_back(std::make_unique<PassT>(std::move(p)));
    }

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    bool isEmpty() const { return Passes.empty(); }

private:
    std::vector<std::unique_ptr<FunctionPass>> Passes;
};

// ============================================================
// ModulePassManager
// ============================================================
class ModulePassManager {
public:
    template<typename PassT>
    void addPass(PassT p) {
        Passes.push_back(std::make_unique<PassT>(std::move(p)));
    }

    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    bool isEmpty() const { return Passes.empty(); }

private:
    std::vector<std::unique_ptr<ModulePass>> Passes;
};

// ============================================================
// ModuleToFunctionPassAdaptor
// ============================================================
class ModuleToFunctionPassAdaptor : public ModulePass {
public:
    explicit ModuleToFunctionPassAdaptor(FunctionPassManager fpm)
        : FPM(std::move(fpm)) {}

    PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) override;
    const char* passName() const override { return "ModuleToFunctionPassAdaptor"; }

    // Callback set by PassBuilder to register analyses into each FAM
    std::function<void(FunctionAnalysisManager&)> registerAnalyses;

private:
    FunctionPassManager FPM;
};

// ============================================================
// AnalysisManager internals (detail namespace)
// ============================================================
namespace detail {

// Type-erased result storage
class AnalysisResultConcept {
public:
    virtual ~AnalysisResultConcept() = default;
};

template<typename ResultT>
class AnalysisResultModel : public AnalysisResultConcept {
public:
    template<typename... Args>
    explicit AnalysisResultModel(Args&&... args)
        : Result(std::forward<Args>(args)...) {}

    ResultT Result;
};

// Type-erased pass storage (knows how to run and produce a result)
class AnalysisPassConcept {
public:
    virtual ~AnalysisPassConcept() = default;
    virtual std::unique_ptr<AnalysisResultConcept> run(
        void *IR, void *AM) = 0;
};

template<typename IRUnitT, typename PassT>
class AnalysisPassModel : public AnalysisPassConcept {
public:
    explicit AnalysisPassModel(PassT pass) : Pass(std::move(pass)) {}

    std::unique_ptr<AnalysisResultConcept> run(void *IR, void *AM) override {
        auto *ir = static_cast<IRUnitT*>(IR);
        auto *am = static_cast<AnalysisManager<IRUnitT>*>(AM);
        auto result = Pass.run(*ir, *am);
        return std::make_unique<AnalysisResultModel<typename PassT::Result>>(
            std::move(result));
    }

private:
    PassT Pass;
};

} // namespace detail

// ============================================================
// AnalysisManager
// ============================================================
template<typename IRUnitT>
class AnalysisManager {
public:
    AnalysisManager() = default;

    AnalysisManager(AnalysisManager &&other) noexcept
        : AnalysisPasses(std::move(other.AnalysisPasses))
        , ResultCache(std::move(other.ResultCache)) {}

    AnalysisManager& operator=(AnalysisManager &&other) noexcept {
        AnalysisPasses = std::move(other.AnalysisPasses);
        ResultCache = std::move(other.ResultCache);
        return *this;
    }

    // Non-copyable
    AnalysisManager(const AnalysisManager&) = delete;
    AnalysisManager& operator=(const AnalysisManager&) = delete;

    // Register an analysis pass
    template<typename PassT, typename PassBuilderT>
    bool registerPass(PassBuilderT &&builder) {
        auto *id = reinterpret_cast<AnalysisKey*>(PassT::ID());
        if (AnalysisPasses.count(id)) {
            return false;  // Already registered
        }
        using ModelT = detail::AnalysisPassModel<IRUnitT, PassT>;
        auto pass = builder();
        AnalysisPasses[id] = std::make_unique<ModelT>(std::move(pass));
        return true;
    }

    // Get analysis result (runs the analysis if not cached)
    template<typename PassT>
    typename PassT::Result& getResult(IRUnitT &IR) {
        auto *id = reinterpret_cast<AnalysisKey*>(PassT::ID());
        assert(AnalysisPasses.count(id) &&
               "Analysis pass not registered before query!");

        auto cacheKey = std::make_pair(id, static_cast<void*>(&IR));
        auto it = ResultCache.find(cacheKey);
        if (it != ResultCache.end()) {
            using ResultModelT = detail::AnalysisResultModel<typename PassT::Result>;
            return static_cast<ResultModelT*>(it->second.get())->Result;
        }

        // Run the analysis
        auto &passConcept = *AnalysisPasses[id];
        auto resultConcept = passConcept.run(&IR, this);
        auto *rawPtr = resultConcept.get();
        ResultCache[cacheKey] = std::move(resultConcept);

        using ResultModelT = detail::AnalysisResultModel<typename PassT::Result>;
        return static_cast<ResultModelT*>(rawPtr)->Result;
    }

    // Get cached result (returns nullptr if not cached)
    template<typename PassT>
    typename PassT::Result* getCachedResult(IRUnitT &IR) {
        auto *id = reinterpret_cast<AnalysisKey*>(PassT::ID());
        auto cacheKey = std::make_pair(id, static_cast<void*>(&IR));
        auto it = ResultCache.find(cacheKey);
        if (it == ResultCache.end()) return nullptr;

        using ResultModelT = detail::AnalysisResultModel<typename PassT::Result>;
        return &static_cast<ResultModelT*>(it->second.get())->Result;
    }

    // Check if a pass is registered
    template<typename PassT>
    bool isPassRegistered() const {
        auto *id = reinterpret_cast<AnalysisKey*>(PassT::ID());
        return AnalysisPasses.count(id) > 0;
    }

    // Invalidate cached analyses based on PreservedAnalyses
    void invalidate(IRUnitT &IR, const PreservedAnalyses &PA) {
        if (PA.areAllPreserved()) return;

        auto *irPtr = static_cast<void*>(&IR);
        for (auto it = ResultCache.begin(); it != ResultCache.end(); ) {
            if (it->first.second == irPtr) {
                auto *id = reinterpret_cast<AnalysisKey*>(it->first.first);
                // Check if this specific analysis is preserved
                // We use a simple check: the analysis is preserved if PA marks it so
                // For now, check the PreservedAnalyses manually
                // (This is a simplification — LLVM uses AnalysisKey/Set matching)
                it = ResultCache.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Clear all cached results for an IR unit
    void clear(IRUnitT &IR) {
        auto *irPtr = static_cast<void*>(&IR);
        for (auto it = ResultCache.begin(); it != ResultCache.end(); ) {
            if (it->first.second == irPtr)
                it = ResultCache.erase(it);
            else
                ++it;
        }
    }

    // Check if empty
    bool empty() const { return ResultCache.empty(); }

private:
    // Registered analysis passes
    std::unordered_map<AnalysisKey*, std::unique_ptr<detail::AnalysisPassConcept>> AnalysisPasses;

    // Result cache: (AnalysisKey*, IRUnitT*) → unique_ptr<ResultConcept>
    using CacheKey = std::pair<AnalysisKey*, void*>;
    struct PairHash {
        size_t operator()(const CacheKey &p) const {
            return std::hash<AnalysisKey*>{}(p.first) ^
                   (std::hash<void*>{}(p.second) << 1);
        }
    };
    using CacheMap = std::unordered_map<CacheKey, std::unique_ptr<detail::AnalysisResultConcept>, PairHash>;

    CacheMap ResultCache;
};

// ============================================================
// PassBuilder
// ============================================================
class PassBuilder {
public:
    ModulePassManager buildDefaultPipeline();
};

} // namespace ll1

#endif
