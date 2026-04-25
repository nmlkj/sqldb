Specification for a Storage Interface

## ---

**1\. Core Data Entities**

Before defining functions, the interface must define the types it handles:

* **TupleID (RID/TID):** A unique physical or logical pointer to a specific record (e.g., PageID \+ SlotOffset).  
* **Tuple:** The raw data container.  
* **ScanHandle:** A stateful cursor that keeps track of the current position during a search.  
* **Schema Object:** Metadata describing column types and offsets.

## ---

**2\. Functional Specification (The Methods)**

A robust storage interface typically breaks down into three categories: **Data Access**, **Index Management**, and **Transaction Control**.

### **A. Data Access (The CRUD Layer)**

These methods allow the Execution VM to move data between the Storage Engine and the Buffer Pool.

| Method | Parameters | Description |
| :---- | :---- | :---- |
| OpenScan | TableID, PredicateTree | Initializes a cursor. Can include "pushdown" filters to skip irrelevant data. |
| GetNext | ScanHandle | Returns the next tuple satisfying the scan criteria. Returns EOF when done. |
| InsertTuple | TableID, Tuple | Writes a new record. Returns the new TupleID. |
| UpdateTuple | TupleID, NewTuple | Replaces data at a specific location. Handles "In-place" vs "Append" logic. |
| DeleteTuple | TupleID | Marks a record as deleted (often a "tombstone" in LSM trees). |

### **B. Indexing Interface**

The VM doesn't want to know if the index is a B-Tree, Hash Map, or GiST. It just needs to navigate it.

* **Seek(IndexID, Key, ComparisonOp)**: Positions a cursor at the first entry matching the key (e.g., WHERE id \>= 100).  
* **InsertIndexEntry(IndexID, Key, TupleID)**: Updates the index when a new row is added to the table.  
* **BulkLoad(IndexID, Iterator)**: An optimized path for building an index from scratch.

### **C. Transactional Context**

The Execution VM must pass a **Transaction Object** to every call to ensure the Storage Engine knows which "version" of the truth to show.

* **Begin(IsolationLevel)**: Returns a TxnID.  
* **Prepare(TxnID)**: Used in 2-Phase Commit (2PC) to ensure the engine is ready to persist.  
* **Commit/Rollback(TxnID)**: Finalizes or undoes the changes.

## ---

**3\. The "Abstract" Architecture Flow**

The interaction between the VM and the Storage Engine follows a strict protocol to maintain separation of concerns:

1. **Handshake:** The VM requests a ScanHandle for Table\_Users.  
2. **Abstraction:** The Storage Interface decides whether to use a SequentialScan (reading the whole heap) or an IndexScan (using a B-Tree).  
3. **Iteration:** The VM calls GetNext() in a loop.  
4. **Buffer Interaction:** The Storage Interface talks to the **Buffer Manager** to pin pages in memory, then passes the memory pointer back to the VM.  
5. **Cleanup:** The VM calls CloseScan() to release locks and pins.

## ---

**4\. Performance Requirements (The "Non-Functional" Spec)**

* **Zero-Copy (where possible):** The interface should ideally return pointers to data in the Buffer Pool rather than copying strings/blobs into the VM's memory space.  
* **Thread Safety:** The interface must support concurrent calls from multiple VM worker threads.  
* **Idempotency:** Certain operations (like Delete) should be safe to retry in the event of a transient crash during a transaction recovery.