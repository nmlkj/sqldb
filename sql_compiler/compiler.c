#include "compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

CompilerContext ctx;

void compiler_init(CompilerContext* ctx) {
    ctx->code.capacity = 128;
    ctx->code.instructions = malloc(sizeof(Instruction) * ctx->code.capacity);
    ctx->code.count = 0;

    ctx->constants.capacity = 32;
    ctx->constants.constants = malloc(sizeof(RegisterValue) * ctx->constants.capacity);
    ctx->constants.count = 0;

    ctx->next_register = 0;
    ctx->current_tuple_register = 0;
}

uint32_t emit(CompilerContext* ctx, uint8_t opcode, uint8_t dest, uint8_t src1, uint8_t src2, int32_t immediate) {
    if (ctx->code.count >= ctx->code.capacity) {
        ctx->code.capacity *= 2;
        ctx->code.instructions = realloc(ctx->code.instructions, sizeof(Instruction) * ctx->code.capacity);
    }
    uint32_t idx = ctx->code.count++;
    ctx->code.instructions[idx].opcode = opcode;
    ctx->code.instructions[idx].dest = dest;
    ctx->code.instructions[idx].src1 = src1;
    ctx->code.instructions[idx].src2 = src2;
    ctx->code.instructions[idx].immediate = immediate;
    return idx;
}

void patch_jump(CompilerContext* ctx, uint32_t instruction_idx, int32_t target_idx) {
    ctx->code.instructions[instruction_idx].immediate = target_idx;
}

uint8_t allocate_register(CompilerContext* ctx) {
    if (ctx->next_register >= NUM_REGISTERS) {
        fprintf(stderr, "Error: Register limit exceeded\n");
        exit(1);
    }
    return ctx->next_register++;
}

void free_registers(CompilerContext* ctx, uint8_t start_reg) {
    ctx->next_register = start_reg;
}

uint32_t add_constant(CompilerContext* ctx, RegisterValue val) {
    if (ctx->constants.count >= ctx->constants.capacity) {
        ctx->constants.capacity *= 2;
        ctx->constants.constants = realloc(ctx->constants.constants, sizeof(RegisterValue) * ctx->constants.capacity);
    }
    uint32_t idx = ctx->constants.count++;
    ctx->constants.constants[idx] = val;
    return idx;
}

static VMContext execute_canned(Instruction* prog, RegisterValue* constants, uint32_t const_count) {
    VMContext vm_ctx;
    memset(&vm_ctx, 0, sizeof(vm_ctx));
    vm_ctx.program = prog;
    vm_ctx.constants = constants;
    vm_ctx.num_constants = const_count;
    for(int i = 0; i < NUM_REGISTERS; i++) vm_ctx.registers[i].i64 = -1;
    VM_Execute(&vm_ctx);
    return vm_ctx;
}

/* --- System Catalog Lookups --- */

static Instruction lookup_table_prog[] = {
    {OP_BEGIN, 0, 0, 0, 0},
    {OP_LOAD_CONST, 0, 0, 0, 0},        // r0 = target name
    {OP_LOAD_CONST, 5, 0, 0, 1},        // r5 = true
    {OP_OPEN_SCAN, 1, 0, 0, 0},         // r1 = scan table 0
    {OP_NEXT, 10, 1, 0, 11},            // next into r10, jump 11 if EOF
    {OP_COLUMN, 2, 10, 0, 1},           // r2 = col 1 (name)
    {OP_COMPARE, 3, 2, 0, 0},           // r3 = r2 cmp r0
    {OP_JUMP_IF, 3, 0, 0, 10},          // jump 10 if r3 != 0
    {OP_COLUMN, 4, 10, 0, 0},           // r4 = col 0 (id)
    {OP_HALT, 0, 0, 0, 0},
    {OP_JUMP_IF, 5, 0, 0, 4},           // jump back to 4
    {OP_HALT, 0, 0, 0, 0}
};

TableID lookup_table(const char* name) {
    RegisterValue constants[2];
    constants[0].s = (char*)name;
    constants[1].i64 = 1;
    VMContext res = execute_canned(lookup_table_prog, constants, 2);
    return (TableID)res.registers[4].i64;
}

/* 
 * lookup_all_columns_prog (Conceptual)
 * Similar to lookup_table, but runs SELECT col_name FROM all_columns WHERE table_id = ?
 * Since our VM doesn't have an internal result collector, we use a hybrid approach.
 */

IdentList lookup_all_columns(TableID tid) {
    IdentList list = {NULL, 0};
    FILE* f = fopen("table_1.csv", "r");
    if (!f) return list;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char* line_copy = strdup(line);
        char* token = strtok(line_copy, ",");
        if (!token) { free(line_copy); continue; }
        
        uint32_t table_id = atoi(token);
        if (table_id == tid) {
            strtok(NULL, ","); // skip col_idx
            char* name = strtok(NULL, ",");
            if (name) {
                list.count++;
                list.names = realloc(list.names, sizeof(char*) * list.count);
                list.names[list.count-1] = strdup(name);
            }
        }
        free(line_copy);
    }
    fclose(f);
    return list;
}

ColumnInfo lookup_column(TableID tid, const char* col_name) {
    ColumnInfo info = {-1, "UNKNOWN"};
    IdentList all = lookup_all_columns(tid);
    for (int i = 0; i < all.count; i++) {
        if (strcmp(all.names[i], col_name) == 0) {
            info.col_idx = i;
            info.data_type = strdup("TEXT");
            break;
        }
    }
    return info;
}

IndexID lookup_index(const char* name, const char* table_name) {
    return (IndexID)-1;
}
