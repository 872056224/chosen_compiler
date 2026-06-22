#include <ll1/Driver/Compiler.h>
#include <cassert>
#include <iostream>
#include <string>

using namespace ll1;

// Helper: check if text contains a substring
static bool contains(const std::string &text, const std::string &sub) {
    return text.find(sub) != std::string::npos;
}

int main() {
    ll1::Compiler compiler;

    // ============================================================
    // Test 1: func_call — basic two-function program
    // ============================================================
    {
        std::string src = R"(
            fn int add(int a, int b) {
                return a + b;
            }
            fn int main() {
                int x = add(10, 20);
                print(x);
                return 0;
            }
        )";
        std::string output = compiler.compileFileNew(""); // compile from src...
        // Use compileNew directly
        output = compiler.compileNew(src);

        if (compiler.hasError()) {
            std::cerr << "[FAIL] test1: " << compiler.getErrorMsg() << std::endl;
            return 1;
        }

        // Verify key patterns in output
        assert(contains(output, "; Function: add"));
        assert(contains(output, "add:"));
        assert(contains(output, "push bp"));
        assert(contains(output, "ret"));
        assert(contains(output, "; Function: main"));
        assert(contains(output, "call add"));
        assert(contains(output, "call __print"));

        std::cout << "[PASS] test_new_pipeline: func_call" << std::endl;
    }

    // ============================================================
    // Test 2: basic — if/else
    // ============================================================
    {
        std::string src = R"(
            fn int main() {
                int a = 10;
                int b = 20;
                if (a < b) {
                    return b;
                }
                return a;
            }
        )";
        std::string output = compiler.compileNew(src);

        if (compiler.hasError()) {
            std::cerr << "[FAIL] test2: " << compiler.getErrorMsg() << std::endl;
            return 1;
        }

        assert(contains(output, "main:"));
        assert(contains(output, "push bp"));
        assert(contains(output, "jl "));
        assert(contains(output, "jmp "));
        assert(contains(output, "ret"));

        std::cout << "[PASS] test_new_pipeline: basic" << std::endl;
    }

    // ============================================================
    // Test 3: for loop — verify BB labels and conditional jump
    // ============================================================
    {
        std::string src = R"(
            fn int main() {
                int sum = 0;
                int i = 0;
                for (i = 0; i < 10; i = i + 1) {
                    sum = sum + i;
                }
                return sum;
            }
        )";
        std::string output = compiler.compileNew(src);

        if (compiler.hasError()) {
            std::cerr << "[FAIL] test3: " << compiler.getErrorMsg() << std::endl;
            return 1;
        }

        assert(contains(output, "for_cond"));
        assert(contains(output, "for_body"));
        assert(contains(output, "for_exit"));
        assert(contains(output, "cmp "));
        assert(contains(output, "jl "));

        std::cout << "[PASS] test_new_pipeline: for_test" << std::endl;
    }

    // ============================================================
    // Test 4: Simple return — minimal function
    // ============================================================
    {
        std::string src = R"(
            fn int main() {
                return 42;
            }
        )";
        std::string output = compiler.compileNew(src);

        if (compiler.hasError()) {
            std::cerr << "[FAIL] test4: " << compiler.getErrorMsg() << std::endl;
            return 1;
        }

        assert(contains(output, "main:"));
        assert(contains(output, "ret"));

        std::cout << "[PASS] test_new_pipeline: simple_return" << std::endl;
    }

    // ============================================================
    // Test 5: print only — void-like
    // ============================================================
    {
        std::string src = R"(
            fn int main() {
                print(100);
                return 0;
            }
        )";
        std::string output = compiler.compileNew(src);

        if (compiler.hasError()) {
            std::cerr << "[FAIL] test5: " << compiler.getErrorMsg() << std::endl;
            return 1;
        }

        assert(contains(output, "call __print"));
        assert(contains(output, "push "));
        assert(contains(output, "ret"));

        std::cout << "[PASS] test_new_pipeline: print" << std::endl;
    }

    std::cout << "\n=== New pipeline: 5/5 tests PASSED ===" << std::endl;
    return 0;
}
