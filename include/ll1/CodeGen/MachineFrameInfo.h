#ifndef LL1_CODEGEN_MACHINEFRAMEINFO_H
#define LL1_CODEGEN_MACHINEFRAMEINFO_H

#include <vector>
#include <cstdint>

namespace ll1 {

// ============================================================
// MachineFrameInfo — Stack frame management
//
// Replaces the ad-hoc NextVarOffset in the old CodeGen.
// Each alloca becomes a StackObject with a FrameIndex.
// ============================================================
class MachineFrameInfo {
public:
    struct StackObject {
        uint16_t Size;           // Size in bytes
        uint16_t Alignment;      // Alignment (1 or 2)
        int16_t  Offset;         // Offset from BP (negative = below BP)
        bool     IsArray;        // Is this an array alloca?
        int      ArraySize;      // Number of elements (if array)
    };

    // Create a stack object, return its FrameIndex (0-based).
    int CreateStackObject(uint16_t Size, uint16_t Alignment = 2,
                          bool isArray = false, int arraySize = 0);

    int getNumObjects() const { return Objects.size(); }

    const StackObject& getObject(int FI) const { return Objects[FI]; }
    StackObject& getObject(int FI) { return Objects[FI]; }

    uint16_t getObjectSize(int FI) const { return Objects[FI].Size; }
    int16_t getObjectOffset(int FI) const { return Objects[FI].Offset; }

    // Total bytes of local variables (for sub sp, N in prologue).
    uint16_t getStackSize() const { return TotalSize; }

    // Whether this function has any stack objects.
    bool hasStackObjects() const { return !Objects.empty(); }

private:
    std::vector<StackObject> Objects;
    uint16_t TotalSize = 0;
};

} // namespace ll1
#endif
