#ifndef STORAGE_INTERFACE_H
#define STORAGE_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file storage_interface.h
 * @brief Specification for the Storage Engine Interface (SEI).
 *
 * This header defines the core data entities and functional methods for
 * Data Access, Index Management, and Transaction Control in the RDBMS.
 */

/* --- Core Data Entities --- */

/**
 * @brief A unique physical or logical pointer to a specific record.
 * Typically composed of a PageID and a SlotOffset.
 */
typedef struct {
    uint32_t page_id;
    uint16_t slot_offset;
} TupleID;

/**
 * @brief Opaque handle for a Tuple (raw data container).
 */
typedef struct Tuple {
    char* data; // CSV line
} Tuple;

/**
 * @brief Opaque handle for a Scan cursor.
 */
typedef struct ScanHandle ScanHandle;

/**
 * @brief Opaque handle for Schema metadata.
 */
typedef struct Schema Schema;

/**
 * @brief Opaque handle for a Transaction object.
 */
typedef struct Transaction Transaction;

/**
 * @brief Identifier for a Table.
 */
typedef uint32_t TableID;

/**
 * @brief Identifier for an Index.
 */
typedef uint32_t IndexID;

/**
 * @brief Opaque handle for an Index Key.
 */
typedef struct Key Key;

/**
 * @brief Opaque handle for a Predicate Tree used in scans.
 */
typedef struct PredicateTree PredicateTree;

/**
 * @brief Transaction Isolation Levels.
 */
typedef enum {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
} IsolationLevel;

/**
 * @brief Comparison Operators for Index Seek.
 */
typedef enum {
    OP_EQ,
    OP_GT,
    OP_GTE,
    OP_LT,
    OP_LTE
} ComparisonOp;

/* --- Functional Specification --- */

/* A. Data Access (The CRUD Layer) */

/**
 * @brief Initializes a cursor for scanning a table.
 * @param txn The transaction context.
 * @param table_id The ID of the table to scan.
 * @param predicate Optional pushdown filter to skip irrelevant data.
 * @return A new ScanHandle, or NULL on error.
 */
ScanHandle* OpenScan(Transaction* txn, TableID table_id, PredicateTree* predicate);

/**
 * @brief Returns the next tuple satisfying the scan criteria.
 * @param handle The active scan handle.
 * @param out_tuple Pointer to store the resulting tuple.
 * @return true if a tuple was found, false on EOF or error.
 */
bool GetNext(ScanHandle* handle, Tuple** out_tuple);

/**
 * @brief Writes a new record to a table.
 * @param txn The transaction context.
 * @param table_id The ID of the target table.
 * @param tuple The tuple data to insert.
 * @param out_tid Pointer to store the new TupleID.
 * @return 0 on success, error code otherwise.
 */
int InsertTuple(Transaction* txn, TableID table_id, Tuple* tuple, TupleID* out_tid);

/**
 * @brief Replaces data at a specific location.
 * @param txn The transaction context.
 * @param tid The ID of the tuple to update.
 * @param new_tuple The new data.
 * @return 0 on success, error code otherwise.
 */
int UpdateTuple(Transaction* txn, TupleID tid, Tuple* new_tuple);

/**
 * @brief Marks a record as deleted.
 * @param txn The transaction context.
 * @param tid The ID of the tuple to delete.
 * @return 0 on success, error code otherwise.
 */
int DeleteTuple(Transaction* txn, TupleID tid);

TableID CreateTable(const char* table_name, int col_count, char** col_names);

/**
 * @brief Releases locks and resources associated with a scan.
 * @param handle The scan handle to close.
 */
void CloseScan(ScanHandle* handle);

/**
 * @brief Extracts the value of a specific column from a tuple.
 * @param tuple The source tuple.
 * @param col_idx The zero-based index of the column to extract.
 * @return A string representation of the column value, or NULL if col_idx is out of bounds.
 */
char* GetColumnValue(Tuple* tuple, int col_idx);


/* B. Indexing Interface */

/**
 * @brief Positions a cursor at the first entry matching the key.
 * @param txn The transaction context.
 * @param index_id The ID of the index to seek.
 * @param key The key to search for.
 * @param op The comparison operator.
 * @return A ScanHandle positioned at the match, or NULL if not found.
 */
ScanHandle* Seek(Transaction* txn, IndexID index_id, Key* key, ComparisonOp op);

/**
 * @brief Updates the index when a new row is added.
 * @param txn The transaction context.
 * @param index_id The ID of the index.
 * @param key The key to insert.
 * @param tid The associated TupleID.
 * @return 0 on success, error code otherwise.
 */
int InsertIndexEntry(Transaction* txn, IndexID index_id, Key* key, TupleID tid);

/**
 * @brief Optimized path for building an index from scratch.
 * @param txn The transaction context.
 * @param index_id The ID of the index.
 * @param iterator A source of key-tid pairs (simplified as ScanHandle here).
 * @return 0 on success, error code otherwise.
 */
int BulkLoad(Transaction* txn, IndexID index_id, ScanHandle* iterator);


/* C. Transactional Context */

/**
 * @brief Starts a new transaction.
 * @param level The desired isolation level.
 * @return A new Transaction object.
 */
Transaction* Begin(IsolationLevel level);

/**
 * @brief Prepares a transaction for commit (2PC).
 * @param txn The transaction to prepare.
 * @return 0 if ready, error code otherwise.
 */
int Prepare(Transaction* txn);

/**
 * @brief Finalizes the transaction changes.
 * @param txn The transaction to commit.
 * @return 0 on success, error code otherwise.
 */
int Commit(Transaction* txn);

/**
 * @brief Undoes the transaction changes.
 * @param txn The transaction to rollback.
 * @return 0 on success, error code otherwise.
 */
int Rollback(Transaction* txn);

#endif /* STORAGE_INTERFACE_H */
