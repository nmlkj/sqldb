#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdbool.h>
#include "storage_interface.h"

#define NUM_REGISTERS 256

typedef union {
    int64_t i64;
    double f64;
    char* s;
    ScanHandle* scan_handle;
    Tuple* tuple;
    Transaction* txn;
    TableID table_id;
    IndexID index_id;
    TupleID tid;
    Key* key;
    void* ptr;
} RegisterValue;

typedef enum {
    OP_HALT = 0,
    OP_OPEN_SCAN,
    OP_SEEK,
    OP_NEXT,
    OP_CLOSE,
    OP_INSERT,
    OP_UPDATE,
    OP_DELETE,
    OP_IDX_INSERT,
    OP_BULK_LOAD,
    OP_BEGIN,
    OP_PREPARE,
    OP_COMMIT,
    OP_ROLLBACK,
    OP_COLUMN,
    OP_LOAD_CONST,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_COMPARE,
    OP_JUMP_IF,
    OP_RESULT_ROW,
    OP_CREATE_TABLE,
    OP_MAKE_RECORD
} Opcode;

typedef struct {
    uint8_t opcode;
    uint8_t dest;
    uint8_t src1;
    uint8_t src2;
    int32_t immediate;
} Instruction;

typedef enum {
    VM_STATUS_SUCCESS = 0,
    VM_STATUS_EOF,
    VM_STATUS_ERROR
} VMStatus;

typedef struct {
    VMStatus status;
    int error_code;
    const char* message;
} VMResult;

typedef struct {
    Instruction* program;
    uint32_t pc;
    RegisterValue registers[NUM_REGISTERS];
    Transaction* active_txn;
    
    // Constant Pool
    RegisterValue* constants;
    uint32_t num_constants;

    // Scan Registry for cleanup
    ScanHandle* scans[NUM_REGISTERS];
    bool scan_active[NUM_REGISTERS];

    bool error_flag;
    int last_error;
} VMContext;

VMResult VM_Execute(VMContext* ctx);

#endif // VM_H
