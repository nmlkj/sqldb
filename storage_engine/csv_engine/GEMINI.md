Role: You are a systems engineer specializing in database internals and C development.

Task: Create a CSV-based Storage Engine implementation in C. This engine must strictly adhere to the Storage Interface Specification2.md and implement the function signatures defined in storage_interface.h.

Core Requirements:

Persistence: Data must be stored in standard CSV files. Each table should correspond to a unique .csv file.

Tuple Management: * Implement TupleID using the page_id as a line number and slot_offset as the byte offset within the file.

Support Insert, Update, and Delete operations. For Delete, implement a "tombstone" mechanism or mark lines as invalid to avoid expensive file rewrites.

Scan Logic: * Implement OpenScan and GetNext to allow sequential iteration through the CSV file.

Include a basic implementation of Predicate Pushdown within GetNext to filter rows before returning them to the VM.

Transaction Support: * Provide a skeletal implementation of the Transaction methods (Begin, Commit, Rollback).

For this version, use file-level locking or simple shadow paging to ensure atomicity during writes.

Performance: * Follow the Zero-Copy principle where possible by returning pointers to a buffer rather than creating new string allocations for every column.

Ensure basic thread safety for concurrent GetNext calls.

Files to Reference:

storage_interface.h: Use this for all struct definitions and function prototypes.

Storage Interface Specification2.md: Use this to understand the architectural flow and performance requirements.

Output:
Provide the complete storage_engine.c source code and a brief explanation of how you handled the file I/O and transaction isolation.

Key Architectural Components to Implement
The Scan Handle: Must maintain an open FILE* pointer and the current byte offset to track progress.

Buffer Interaction: Simulate a simple buffer that reads chunks of the CSV into memory to minimize disk hits.

Indexing: Implement a simple in-memory Hash Map for the IndexID to satisfy the Seek and InsertIndexEntry methods.