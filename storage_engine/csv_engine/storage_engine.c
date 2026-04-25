#include "storage_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LINE_LENGTH 1024
#define TOMBSTONE_CHAR '*'

/* Internal Data Structures */

struct Transaction {
    IsolationLevel level;
    bool active;
};

struct PredicateTree {
    int column_index;
    ComparisonOp op;
    char* value;
};

struct ScanHandle {
    FILE* file;
    uint32_t current_line;
    uint16_t current_offset;
    TableID table_id;
    PredicateTree* predicate;
    Transaction* txn;
    char buffer[MAX_LINE_LENGTH];
};

struct Key {
    char* value;
};

/* Simple In-Memory Index Entry */
typedef struct IndexEntry {
    char* key;
    TupleID tid;
    struct IndexEntry* next;
} IndexEntry;

#define HASH_SIZE 1024
IndexEntry* index_hash_map[HASH_SIZE];

unsigned int hash(const char* str) {
    unsigned int h = 0;
    while (*str) h = h * 31 + *str++;
    return h % HASH_SIZE;
}

/* Global Mutex for File Access (Simulation of simple locking) */
pthread_mutex_t storage_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Helper: Get filename from TableID */
void get_table_filename(TableID table_id, char* filename) {
    sprintf(filename, "table_%u.csv", table_id);
}

/* --- Data Access Implementation --- */

ScanHandle* OpenScan(Transaction* txn, TableID table_id, PredicateTree* predicate) {
    char filename[256];
    get_table_filename(table_id, filename);

    FILE* file = fopen(filename, "r");
    if (!file) {
        // Create file if it doesn't exist for scanning
        file = fopen(filename, "a+");
        if (!file) return NULL;
        rewind(file);
    }

    ScanHandle* handle = (ScanHandle*)malloc(sizeof(ScanHandle));
    handle->file = file;
    handle->current_line = 0;
    handle->current_offset = 0;
    handle->table_id = table_id;
    handle->predicate = predicate;
    handle->txn = txn;
    return handle;
}

bool EvaluatePredicate(char* line, PredicateTree* predicate) {
    if (!predicate) return true;
    
    // Simple predicate evaluation: assuming comma-separated values
    char line_copy[MAX_LINE_LENGTH];
    strncpy(line_copy, line, MAX_LINE_LENGTH);
    
    // Remove newline if present
    size_t len = strlen(line_copy);
    if (len > 0 && line_copy[len-1] == '\n') {
        line_copy[len-1] = '\0';
    }
    
    char* token = strtok(line_copy, ",");
    int current_col = 0;
    while (token) {
        if (current_col == predicate->column_index) {
            int cmp = strcmp(token, predicate->value);
            switch (predicate->op) {
                case OP_EQ: return cmp == 0;
                case OP_GT: return cmp > 0;
                case OP_GTE: return cmp >= 0;
                case OP_LT: return cmp < 0;
                case OP_LTE: return cmp <= 0;
            }
        }
        token = strtok(NULL, ",");
        current_col++;
    }
    return false;
}

bool GetNext(ScanHandle* handle, Tuple** out_tuple) {
    if (!handle || !handle->file) return false;

    while (fgets(handle->buffer, MAX_LINE_LENGTH, handle->file)) {
        // Skip tombstones
        if (handle->buffer[0] == TOMBSTONE_CHAR) continue;

        // Apply predicate pushdown
        if (EvaluatePredicate(handle->buffer, handle->predicate)) {
            // Remove newline if present
            size_t len = strlen(handle->buffer);
            if (len > 0 && handle->buffer[len-1] == '\n') {
                handle->buffer[len-1] = '\0';
            }

            *out_tuple = (Tuple*)malloc(sizeof(Tuple));
            (*out_tuple)->data = handle->buffer; // Zero-copy: point to internal buffer
            return true;
        }
    }
    return false;
}

int InsertTuple(Transaction* txn, TableID table_id, Tuple* tuple, TupleID* out_tid) {
    pthread_mutex_lock(&storage_mutex);
    
    char filename[256];
    get_table_filename(table_id, filename);
    
    FILE* file = fopen(filename, "a+");
    if (!file) {
        pthread_mutex_unlock(&storage_mutex);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long offset = ftell(file);
    
    // Count existing lines to determine page_id (line number)
    rewind(file);
    uint32_t line_count = 0;
    char dummy[MAX_LINE_LENGTH];
    while (fgets(dummy, MAX_LINE_LENGTH, file)) line_count++;
    
    fseek(file, 0, SEEK_END);
    fprintf(file, "%s\n", tuple->data);
    
    out_tid->page_id = line_count + 1;
    out_tid->slot_offset = (uint16_t)offset;

    fclose(file);
    pthread_mutex_unlock(&storage_mutex);
    return 0;
}

int UpdateTuple(Transaction* txn, TupleID tid, Tuple* new_tuple) {
    // Simple update: Delete + Insert
    if (DeleteTuple(txn, tid) != 0) return -1;
    
    // We'd need TableID here, but storage_interface.h doesn't provide it in UpdateTuple.
    // In a real engine, tid might encode TableID or we'd have a mapping.
    // For this prototype, we'll assume we can resolve it or use a default.
    // This is a known limitation of the provided interface without TableID in Update.
    return 0; 
}

int DeleteTuple(Transaction* txn, TupleID tid) {
    pthread_mutex_lock(&storage_mutex);
    
    // How to find which file? Assuming a global context or encoded TableID.
    // Let's assume table 0 for now or a way to discover it.
    // In a full implementation, TupleID would likely contain a TableID or we'd track it.
    char filename[256];
    get_table_filename(0, filename); // Placeholder: TableID 0
    
    FILE* file = fopen(filename, "r+");
    if (!file) {
        pthread_mutex_unlock(&storage_mutex);
        return -1;
    }

    fseek(file, tid.slot_offset, SEEK_SET);
    fputc(TOMBSTONE_CHAR, file);
    
    fclose(file);
    pthread_mutex_unlock(&storage_mutex);
    return 0;
}

PredicateTree* CreatePredicate(int col, ComparisonOp op, char* value) {
    PredicateTree* pred = (PredicateTree*)malloc(sizeof(PredicateTree));
    pred->column_index = col;
    pred->op = op;
    pred->value = strdup(value);
    return pred;
}

void CloseScan(ScanHandle* handle) {
    if (handle) {
        if (handle->file) fclose(handle->file);
        free(handle);
    }
}

/* --- Indexing Implementation --- */

Key* CreateKey(char* value) {
    Key* key = (Key*)malloc(sizeof(Key));
    key->value = strdup(value);
    return key;
}

void DestroyKey(Key* key) {
    if (key) {
        free(key->value);
        free(key);
    }
}

ScanHandle* Seek(Transaction* txn, IndexID index_id, Key* key, ComparisonOp op) {
    // Only OP_EQ supported for this simple hash index
    if (op != OP_EQ) return NULL;

    unsigned int h = hash(key->value);
    IndexEntry* entry = index_hash_map[h];
    while (entry) {
        if (strcmp(entry->key, key->value) == 0) {
            // Found exact match. Return a ScanHandle positioned here.
            ScanHandle* handle = OpenScan(txn, 0, NULL); // Assuming TableID 0
            if (handle) {
                fseek(handle->file, entry->tid.slot_offset, SEEK_SET);
                // We might want to mark this handle to stop after one tuple for OP_EQ
            }
            return handle;
        }
        entry = entry->next;
    }
    return NULL;
}

int InsertIndexEntry(Transaction* txn, IndexID index_id, Key* key, TupleID tid) {
    unsigned int h = hash(key->value);
    IndexEntry* entry = (IndexEntry*)malloc(sizeof(IndexEntry));
    entry->key = strdup(key->value);
    entry->tid = tid;
    entry->next = index_hash_map[h];
    index_hash_map[h] = entry;
    return 0;
}

int BulkLoad(Transaction* txn, IndexID index_id, ScanHandle* iterator) {
    Tuple* tuple;
    while (GetNext(iterator, &tuple)) {
        // In a real bulk load, we'd extract the key from the tuple data
        // For now, it's a placeholder.
        free(tuple->data);
        free(tuple);
    }
    return 0;
}

/* --- Transactional Implementation --- */

Transaction* Begin(IsolationLevel level) {
    Transaction* txn = (Transaction*)malloc(sizeof(Transaction));
    txn->level = level;
    txn->active = true;
    return txn;
}

int Prepare(Transaction* txn) {
    return 0;
}

int Commit(Transaction* txn) {
    txn->active = false;
    free(txn);
    return 0;
}

int Rollback(Transaction* txn) {
    txn->active = false;
    free(txn);
    return 0;
}

char* GetColumnValue(Tuple* tuple, int col_idx) {
    #ifdef DEBUG
    printf("DEBUG: GetColumnValue %s:%d\n", __FILE__, __LINE__);
    printf("DEBUG: GetColumnValue tuple data=[%s] col_idx=%d\n", tuple ? tuple->data : "NULL", col_idx);
    #endif
    if (!tuple || !tuple->data) return NULL;
    
    char* line_copy = strdup(tuple->data);
    char* token = strtok(line_copy, ",");
    int current_col = 0;
    
    while (token) {
        if (current_col == col_idx) {
            char* result = strdup(token);
            free(line_copy);
            return result;
        }
        token = strtok(NULL, ",");
        current_col++;
    }
    
    free(line_copy);
    return NULL;
}

TableID CreateTable(const char* table_name, int col_count, char** col_names) {
    pthread_mutex_lock(&storage_mutex);
    
    // 1. Find next TableID by counting lines in table_0.csv
    FILE* t0 = fopen("table_0.csv", "r");
    uint32_t next_id = 0;
    if (t0) {
        char dummy[MAX_LINE_LENGTH];
        while (fgets(dummy, MAX_LINE_LENGTH, t0)) next_id++;
        fclose(t0);
    }
    
    // 2. Add to table_0.csv
    FILE* t0_append = fopen("table_0.csv", "a");
    if (!t0_append) {
        pthread_mutex_unlock(&storage_mutex);
        return (TableID)-1;
    }
    fprintf(t0_append, "%u,%s\n", next_id, table_name);
    fclose(t0_append);
    
    // 3. Add columns to table_1.csv
    FILE* t1_append = fopen("table_1.csv", "a");
    if (t1_append) {
        for (int i = 0; i < col_count; i++) {
            // all_columns schema: table_id, col_idx, col_name, data_type
            fprintf(t1_append, "%u,%d,%s,TEXT\n", next_id, i, col_names[i]);
        }
        fclose(t1_append);
    }
    
    // 4. Create the actual data file
    char filename[256];
    sprintf(filename, "table_%u.csv", next_id);
    FILE* new_table = fopen(filename, "w");
    if (new_table) {
        fclose(new_table);
    }
    
    pthread_mutex_unlock(&storage_mutex);
    return (TableID)next_id;
}
