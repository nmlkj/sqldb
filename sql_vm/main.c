#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("--- Test 1: Scan and Column Extraction ---\n");
    {
        RegisterValue constants[] = {
            {.i64 = 1}
        };

        Instruction program[] = {
            {OP_BEGIN, 0, 0, 0, READ_COMMITTED},
            {OP_LOAD_CONST, 0, 0, 0, 0},   // R0 = Const[0] = 1
            {OP_OPEN_SCAN, 1, 0, 0, 101},  // R1 = OpenScan(Table 101)
            // PC 3: Loop Start
            {OP_NEXT, 2, 1, 0, 7},         // R2 = Next(R1). If EOF, jump to PC 7
            {OP_COLUMN, 3, 2, 0, 1},       // R3 = Column(R2, 1) - Alice, Bob, Charlie
            {OP_RESULT_ROW, 0, 3, 0, 1},   // Emit row [R3]
            {OP_JUMP_IF, 0, 0, 0, 3},      // If R0, jump to PC 3
            // PC 7
            {OP_HALT, 0, 0, 0, 0}
        };

        VMContext ctx = {0};
        ctx.program = program;
        ctx.constants = constants;
        ctx.num_constants = 1;
        
        VMResult res = VM_Execute(&ctx);
        if (res.status == VM_STATUS_ERROR) {
            printf("VM Error: %s (code: %d)\n", res.message, res.error_code);
        } else {
            printf("Test 1 Finished Successfully\n");
        }
    }

    printf("\n--- Test 3: Filtering (Age > 30) ---\n");
    {
        RegisterValue constants[] = {
            {.i64 = 30},
            {.i64 = 1}
        };

        Instruction program[] = {
            {OP_BEGIN, 0, 0, 0, READ_COMMITTED},
            {OP_OPEN_SCAN, 1, 0, 0, 101},  // R1 = Scan
            {OP_LOAD_CONST, 4, 0, 0, 0},   // R4 = 30
            {OP_LOAD_CONST, 5, 0, 0, 1},   // R5 = 1
            // PC 4: Loop Start
            {OP_NEXT, 2, 1, 0, 15},        // R2 = Next(R1). If EOF, jump to PC 15 (HALT)
            {OP_COLUMN, 3, 2, 0, 2},       // R3 = Age (int)
            {OP_COMPARE, 6, 3, 4, 0},      // R6 = Compare(Age, 30) -> 1, 0, -1
            {OP_ADD, 7, 6, 5, 0},          // R7 = R6 + 1 -> 2, 1, 0
            {OP_SUB, 8, 7, 5, 0},          // R8 = R7 - 1 -> 1, 0, -1
            {OP_MUL, 9, 8, 7, 0},          // R9 = R8 * R7 -> 2, 0, 0
            {OP_JUMP_IF, 0, 9, 0, 12},     // If R9 != 0 (Age > 30), jump to PC 12
            {OP_JUMP_IF, 0, 5, 0, 4},      // Unconditional jump to PC 4
            // PC 12: RESULT_ROW
            {OP_COLUMN, 10, 2, 0, 1},      // R10 = Name
            {OP_RESULT_ROW, 0, 10, 0, 1},
            {OP_JUMP_IF, 0, 5, 0, 4},      // Jump back to PC 4
            // PC 15
            {OP_HALT, 0, 0, 0, 0}
        };

        VMContext ctx = {0};
        ctx.program = program;
        ctx.constants = constants;
        ctx.num_constants = 2;
        
        VMResult res = VM_Execute(&ctx);
        if (res.status == VM_STATUS_ERROR) {
            printf("VM Error: %s\n", res.message);
        } else {
            printf("Test 3 Finished Successfully\n");
        }
    }

    return 0;
}
