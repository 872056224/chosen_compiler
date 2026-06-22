#ifndef LL1_CODEGEN_SELECTIONDAG_ISDOPCODES_H
#define LL1_CODEGEN_SELECTIONDAG_ISDOPCODES_H

#include <cstdint>

namespace ll1 {
namespace ISD {

// ============================================================
// ISD::NodeType — Target-independent DAG node opcodes
//
// Positive values = target-independent (ISD) opcodes
// Negative values = target-specific machine opcodes
// ============================================================
enum NodeType : uint16_t {
    // Sentinel
    DELETED_NODE = 0,

    // === Control / Start ===
    EntryToken,       // Root of the DAG chain

    // === Terminators ===
    BR,               // Unconditional branch to MBB
    BR_CC,            // Conditional branch (condition, trueMBB, falseMBB)
    RET,              // Return (optional value operand)

    // === Copy ===
    CopyFromReg,      // Live-in: virtual reg → DAG value
    CopyToReg,        // Live-out: DAG value → virtual reg
    Register,         // Reference to a physical register

    // === Constants ===
    Constant,         // Integer constant value (int16_t)

    // === Memory ===
    LOAD,             // Load from memory (chain, ptr)
    STORE,            // Store to memory (chain, value, ptr)
    FrameIndex,       // Reference to a stack frame slot
    IndexedLoad,      // Indexed load: (chain, baseFI, index) — lea-like addressing
    IndexedStore,     // Indexed store: (chain, value, baseFI, index)

    // === Arithmetic (binary) ===
    ADD, SUB, MUL,
    SDIV, SREM,
    AND, OR, XOR,
    SHL, SHR,

    // === Unary ===
    SIGN_EXTEND,      // Sign-extend to larger type
    ZERO_EXTEND,      // Zero-extend to larger type
    TRUNCATE,         // Truncate to smaller type

    // === Comparison ===
    SETCC,            // Compare, produce boolean result

    // === Call ===
    CALL,             // Function call

    // === Chain / Merge ===
    TokenFactor,      // Merge multiple chains into one

    // === Type Assertions (for cast tracking) ===
    AssertSext,       // Records that value is sign-extended
    AssertZext,       // Records that value is zero-extended

    // === Stack ===
    ADJCALLSTACKDOWN, // Adjust stack before call
    ADJCALLSTACKUP,   // Adjust stack after call

    BUILTIN_OP_END    // Marker: end of target-independent opcodes
};

// Helper: check if an opcode is a target-specific machine opcode
inline bool isMachineOpcode(unsigned Opc) {
    return Opc >= BUILTIN_OP_END;
}

} // namespace ISD
} // namespace ll1

#endif
