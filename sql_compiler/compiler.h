#ifndef COMPILER_H
#define COMPILER_H

#include "vm.h"

typedef struct {
    Instruction* instructions;
    uint32_t capacity;
    uint32_t count;
} CodeBuffer;

typedef struct {
    RegisterValue* constants;
    uint32_t capacity;
    uint32_t count;
} ConstantPool;

typedef struct {
    char** names;
    int count;
} IdentList;

typedef struct {
    int col_idx;
    char* data_type;
} ColumnInfo;

typedef struct {
    CodeBuffer code;
    ConstantPool constants;
    uint8_t next_register;
    uint8_t current_tuple_register;
} CompilerContext;

void compiler_init(CompilerContext* ctx);
uint32_t emit(CompilerContext* ctx, uint8_t opcode, uint8_t dest, uint8_t src1, uint8_t src2, int32_t immediate);
void patch_jump(CompilerContext* ctx, uint32_t instruction_idx, int32_t target_idx);
uint8_t allocate_register(CompilerContext* ctx);
void free_registers(CompilerContext* ctx, uint8_t start_reg);
uint32_t add_constant(CompilerContext* ctx, RegisterValue val);

// System Catalog lookups using internal VM execution
TableID lookup_table(const char* name);
ColumnInfo lookup_column(TableID tid, const char* col_name);
IdentList lookup_all_columns(TableID tid);

#endif // COMPILER_H
