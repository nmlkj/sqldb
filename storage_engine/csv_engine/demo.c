#include "storage_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper functions declared in storage_engine.c but not in interface */
PredicateTree* CreatePredicate(int col, ComparisonOp op, char* value);
Key* CreateKey(char* value);
void DestroyKey(Key* key);

int main() {
    printf("--- CSV Storage Engine Demo ---\n\n");

    // 1. Initialize Transaction
    Transaction* txn = Begin(READ_COMMITTED);
    TableID users_table = 0;
    IndexID id_index = 1;

    // 2. Insert Tuples
    printf("Step 1: Inserting records...\n");
    Tuple records[] = {
        {"101,Alice,Engineering"},
        {"102,Bob,Product"},
        {"103,Charlie,Engineering"},
        {"104,David,Sales"}
    };
    TupleID tids[4];

    for (int i = 0; i < 4; i++) {
        InsertTuple(txn, users_table, &records[i], &tids[i]);
        
        // Index the ID (column 0)
        char id_str[10];
        sprintf(id_str, "%d", 101 + i);
        Key* key = CreateKey(id_str);
        InsertIndexEntry(txn, id_index, key, tids[i]);
        DestroyKey(key);
    }
    printf("Successfully inserted and indexed 4 records.\n\n");

    // 3. Sequential Scan with Predicate Pushdown
    printf("Step 2: Scanning for users in 'Engineering' (Predicate Pushdown)...\n");
    PredicateTree* eng_pred = CreatePredicate(2, OP_EQ, "Engineering");
    ScanHandle* scan = OpenScan(txn, users_table, eng_pred);
    
    Tuple* tuple;
    while (GetNext(scan, &tuple)) {
        printf("  MATCH FOUND: %s\n", tuple->data);
        free(tuple);
    }
    CloseScan(scan);
    free(eng_pred);
    printf("\n");

    // 4. Index Seek
    printf("Step 3: Seeking user with ID '102' using Index...\n");
    Key* seek_key = CreateKey("102");
    ScanHandle* seek_handle = Seek(txn, id_index, seek_key, OP_EQ);
    
    if (seek_handle && GetNext(seek_handle, &tuple)) {
        printf("  INDEX HIT: %s\n", tuple->data);
        free(tuple);
        CloseScan(seek_handle);
    } else {
        printf("  INDEX MISS\n");
    }
    DestroyKey(seek_key);
    printf("\n");

    // 5. Deletion
    printf("Step 4: Deleting Bob (ID 102)...\n");
    DeleteTuple(txn, tids[1]); // Bob was the 2nd record
    
    printf("Step 5: Full scan after deletion (Verify tombstone)...\n");
    scan = OpenScan(txn, users_table, NULL);
    while (GetNext(scan, &tuple)) {
        printf("  RECORD: %s\n", tuple->data);
        free(tuple);
    }
    CloseScan(scan);
    printf("\n");

    // 6. Finalize
    Commit(txn);
    printf("Demo completed. Data persisted in table_0.csv\n");

    return 0;
}
