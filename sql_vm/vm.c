#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"

static void cleanup_vm(VMContext* ctx) {
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (ctx->scan_active[i] && ctx->scans[i]) {
            CloseScan(ctx->scans[i]);
            ctx->scan_active[i] = false;
            ctx->scans[i] = NULL;
        }
    }
    // We don't rollback/commit here, that should be done by instructions or caller
}

VMResult VM_Execute(VMContext* ctx) {
    VMResult result = {VM_STATUS_SUCCESS, 0, NULL};
    ctx->pc = 0;
    ctx->error_flag = false;

    while (true) {
        Instruction inst = ctx->program[ctx->pc];
        uint8_t opcode = inst.opcode;
        uint8_t rd = inst.dest;
        uint8_t rs1 = inst.src1;
        uint8_t rs2 = inst.src2;
        int32_t imm = inst.immediate;

        switch (opcode) {
            case OP_HALT:
                cleanup_vm(ctx);
                return result;

            case OP_OPEN_SCAN: { 
                TableID tid = (TableID)imm;
                // Note: PredicateTree is NULL for now as per simple OPEN_SCAN
                ScanHandle* handle = OpenScan(ctx->active_txn, tid, NULL);
                if (!handle) {
                    ctx->error_flag = true;
                    ctx->last_error = -1; // Generic error
                    result.status = VM_STATUS_ERROR;
                    result.message = "Failed to open scan";
                    cleanup_vm(ctx);
                    return result;
                }
                ctx->registers[rd].scan_handle = handle;
                ctx->scans[rd] = handle;
                ctx->scan_active[rd] = true;
                break;
            }

            case OP_SEEK: {
                IndexID idxid = (IndexID)imm;
                Key* key = ctx->registers[rs1].key;
                ComparisonOp op = (ComparisonOp)rs2;
                ScanHandle* handle = Seek(ctx->active_txn, idxid, key, op);
                if (!handle) {
                    ctx->registers[rd].scan_handle = NULL; // Or handle as error?
                    // Spec says: Stores ScanHandle in Rd. Seek returns NULL if not found.
                }
                ctx->registers[rd].scan_handle = handle;
                if (handle) {
                    ctx->scans[rd] = handle;
                    ctx->scan_active[rd] = true;
                }
                break;
            }

            case OP_NEXT: { 
                ScanHandle* handle = ctx->registers[rs1].scan_handle;
                Tuple* tuple = NULL;
                bool found = GetNext(handle, &tuple);
                if (found) {
                    ctx->registers[rd].tuple = tuple;
                } else {
                    ctx->pc = (uint32_t)imm - 1; // Jump to Label (pc will be incremented)
                }
                break;
            }

            case OP_CLOSE: {
                if (ctx->scan_active[rd]) {
                    CloseScan(ctx->scans[rd]);
                    ctx->scan_active[rd] = false;
                    ctx->scans[rd] = NULL;
                }
                break;
            }

            case OP_INSERT: {
                TableID tid = (TableID)imm;
                Tuple* tuple = ctx->registers[rs1].tuple;
                TupleID new_tid;
                int err = InsertTuple(ctx->active_txn, tid, tuple, &new_tid);
                if (err != 0) {
                    ctx->error_flag = true;
                    ctx->last_error = err;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Insert failed";
                    cleanup_vm(ctx);
                    return result;
                }
                // Where to store new_tid? Spec doesn't say for INSERT, but maybe it's useful.
                break;
            }

            case OP_UPDATE: {
                TupleID tid = ctx->registers[rd].tid;
                Tuple* tuple = ctx->registers[rs1].tuple;
                int err = UpdateTuple(ctx->active_txn, tid, tuple);
                if (err != 0) {
                    ctx->error_flag = true;
                    ctx->last_error = err;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Update failed";
                    cleanup_vm(ctx);
                    return result;
                }
                break;
            }

            case OP_DELETE: {
                TupleID tid = ctx->registers[rd].tid;
                int err = DeleteTuple(ctx->active_txn, tid);
                if (err != 0) {
                    ctx->error_flag = true;
                    ctx->last_error = err;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Delete failed";
                    cleanup_vm(ctx);
                    return result;
                }
                break;
            }

            case OP_IDX_INSERT: {
                IndexID idxid = (IndexID)imm;
                Key* key = ctx->registers[rs1].key;
                TupleID tid = ctx->registers[rs2].tid;
                int err = InsertIndexEntry(ctx->active_txn, idxid, key, tid);
                if (err != 0) {
                    ctx->error_flag = true;
                    ctx->last_error = err;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Index insert failed";
                    cleanup_vm(ctx);
                    return result;
                }
                break;
            }

            case OP_BULK_LOAD: {
                IndexID idxid = (IndexID)imm;
                ScanHandle* iter = ctx->registers[rs1].scan_handle;
                int err = BulkLoad(ctx->active_txn, idxid, iter);
                if (err != 0) {
                    ctx->error_flag = true;
                    ctx->last_error = err;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Bulk load failed";
                    cleanup_vm(ctx);
                    return result;
                }
                break;
            }

            case OP_BEGIN: {
                IsolationLevel level = (IsolationLevel)imm;
                ctx->active_txn = Begin(level);
                if (!ctx->active_txn) {
                    ctx->error_flag = true;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Failed to begin transaction";
                    cleanup_vm(ctx);
                    return result;
                }
                break;
            }

            case OP_PREPARE: {
                int err = Prepare(ctx->active_txn);
                if (err != 0) {
                    ctx->error_flag = true;
                    ctx->last_error = err;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Prepare failed";
                    cleanup_vm(ctx);
                    return result;
                }
                break;
            }

            case OP_COMMIT: {
                int err = Commit(ctx->active_txn);
                ctx->active_txn = NULL;
                if (err != 0) {
                    ctx->error_flag = true;
                    ctx->last_error = err;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Commit failed";
                    cleanup_vm(ctx);
                    return result;
                }
                break;
            }

            case OP_ROLLBACK: {
                int err = Rollback(ctx->active_txn);
                ctx->active_txn = NULL;
                if (err != 0) {
                    ctx->error_flag = true;
                    ctx->last_error = err;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Rollback failed";
                    cleanup_vm(ctx);
                    return result;
                }
                break;
            }

            case OP_COLUMN: {
                Tuple* tuple = ctx->registers[rs1].tuple;
                int col_idx = imm;
                char* val = GetColumnValue(tuple, col_idx); 
#ifdef DEBUG
                printf("DEBUG: OP_COLUMN rd=%d col=%d val=[%s]\n", rd, col_idx, val ? val : "NULL");
#endif
                if (val) {
                    char* endptr;
                    long long i_val = strtoll(val, &endptr, 10);
                    if (*endptr == '\0') {
                        ctx->registers[rd].i64 = i_val;
                        free(val); // Don't need the string if it's an int
                    } else {
                        ctx->registers[rd].s = val;
                    }
                } else {
                    ctx->registers[rd].s = NULL;
                }
                break;
            }

            case OP_LOAD_CONST: {
                ctx->registers[rd] = ctx->constants[imm];
                break;
            }

            case OP_ADD:
                ctx->registers[rd].i64 = ctx->registers[rs1].i64 + ctx->registers[rs2].i64;
                break;
            case OP_SUB:
                ctx->registers[rd].i64 = ctx->registers[rs1].i64 - ctx->registers[rs2].i64;
                break;
            case OP_MUL:
                ctx->registers[rd].i64 = ctx->registers[rs1].i64 * ctx->registers[rs2].i64;
                break;
            case OP_DIV:
                if (ctx->registers[rs2].i64 == 0) {
                    ctx->error_flag = true;
                    result.status = VM_STATUS_ERROR;
                    result.message = "Division by zero";
                    cleanup_vm(ctx);
                    return result;
                }
                ctx->registers[rd].i64 = ctx->registers[rs1].i64 / ctx->registers[rs2].i64;
                break;

                                    case OP_COMPARE: {
                RegisterValue rv1 = ctx->registers[rs1];
                RegisterValue rv2 = ctx->registers[rs2];
                if (rv1.i64 > 4096 && rv2.i64 > 4096 && rv1.s && rv2.s) {
                    ctx->registers[rd].i64 = strcmp(rv1.s, rv2.s);
#ifdef DEBUG
                printf("DEBUG: COMPARE RESULT=%ld\n", ctx->registers[rd].i64);
#endif
                } else {
                    int64_t v1 = rv1.i64;
                    int64_t v2 = rv2.i64;
                    if (v1 < v2) ctx->registers[rd].i64 = -1;
                    else if (v1 > v2) ctx->registers[rd].i64 = 1;
                    else ctx->registers[rd].i64 = 0;
                }
                break;
            }

            case OP_JUMP_IF: {
                if (ctx->registers[rd].i64 != 0) {
                    ctx->pc = (uint32_t)imm - 1; // pc will be incremented
                }
                break;
            }

            case OP_RESULT_ROW: {
                int count = imm;
                int start_reg = rs1;
                for (int i = 0; i < count; i++) {
                    RegisterValue rv = ctx->registers[start_reg + i];
                    if (rv.i64 > 4096 || rv.i64 < -4096) {
                        printf("| %-15s ", rv.s ? rv.s : "NULL");
                    } else {
                        printf("| %-15ld ", rv.i64);
                    }
                }
                printf("|\n");
                break;
            }


            case OP_CREATE_TABLE: { 
                char* table_name = ctx->constants[imm].s; 
                int col_count = rs2; 
                char** col_names = malloc(sizeof(char*) * col_count); 
                for (int i = 0; i < col_count; i++) { 
                    col_names[i] = ctx->registers[rs1 + i].s; 
                } 
                TableID new_tid = CreateTable(table_name, col_count, col_names); 
                free(col_names); 
                ctx->registers[rd].i64 = (int64_t)new_tid; 
                break; 
            }
            case OP_MAKE_RECORD: { 
                int start_reg = rs1; 
                int count = rs2; 
                char buffer[4096] = {0}; 
                for (int i = 0; i < count; i++) { 
                    RegisterValue rv = ctx->registers[start_reg + i]; 
                    char val_str[1024]; 
                    if (rv.i64 > 4096 || rv.i64 < -4096) { 
                        sprintf(val_str, "%s", rv.s ? rv.s : ""); 
                    } else { 
                        sprintf(val_str, "%ld", rv.i64); 
                    } 
                    strcat(buffer, val_str); 
                    if (i < count - 1) strcat(buffer, ","); 
                } 
                Tuple* tuple = malloc(sizeof(Tuple)); 
                tuple->data = strdup(buffer); 
                ctx->registers[rd].tuple = tuple; 
                break; 
            }
            default:
                ctx->error_flag = true;
                result.status = VM_STATUS_ERROR;
                result.message = "Unknown opcode";
                cleanup_vm(ctx);
                return result;
        }

        ctx->pc++;
    }
}