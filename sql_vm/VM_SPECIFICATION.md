# SQL Query Execution Virtual Machine (SQL-VM) Technical Specification

## 1. Introduction
The SQL-VM is a middle-tier execution engine designed to bridge high-level SQL statements with a low-level data persistence layer. It executes compiled bytecode generated from SQL queries by interfacing with the `storage_interface.h` API.

## 2. Execution Model
The SQL-VM utilizes a **Register-based Architecture**. This design was chosen for its efficiency in reducing instruction count and its natural mapping to the relational operations and expression evaluations required by the SQL grammar.

### 2.1 State Management
The VM state is maintained in a `VMContext` structure:
- **Program Counter (PC):** Pointer to the next instruction to be executed.
- **Registers (R0 - R255):** A bank of 256 general-purpose registers capable of holding:
    - Integer/Floating point values
    - String pointers
    - Opaque handles (`ScanHandle*`, `Tuple*`, `Transaction*`, `TableID`, `IndexID`)
- **Transaction Context:** A pointer to the active `Transaction` object.
- **Scan Registry:** A table tracking active `ScanHandle` pointers to ensure resource cleanup.
- **Constant Pool:** A read-only section containing literal values (strings/numbers) referenced by instructions.

### 2.2 Execution Loop
1. **Fetch:** Retrieve the instruction at the current PC.
2. **Decode:** Identify the opcode and its operands.
3. **Execute:** Perform the operation (e.g., call a storage function, perform arithmetic).
4. **Advance:** Increment the PC or jump to a new address.
5. **Repeat:** Continue until a `HALT` instruction is encountered or an error occurs.

---

## 3. Instruction Set Architecture (ISA)

Instructions are 64-bit words, typically formatted as: `[Opcode: 8][Dest: 8][Src1: 8][Src2: 8][Immediate: 32]`.

### 3.1 Data Access & Cursor Instructions
| Opcode | Operands | Description | Storage API Mapping |
| :--- | :--- | :--- | :--- |
| `OPEN_SCAN` | `Rd, TableID` | Opens a full scan on `TableID`. Stores `ScanHandle` in `Rd`. | `OpenScan()` |
| `SEEK` | `Rd, IdxID, Rk, Op` | Opens an index scan on `IdxID` using key in `Rk`. Stores `ScanHandle` in `Rd`. | `Seek()` |
| `NEXT` | `Rd, Rs, Label` | Fetches next tuple from scan `Rs` into `Rd`. Jumps to `Label` if EOF. | `GetNext()` |
| `CLOSE` | `Rs` | Closes the scan handle in `Rs`. | `CloseScan()` |

### 3.2 DML & DDL Instructions
| Opcode | Operands | Description | Storage API Mapping |
| :--- | :--- | :--- | :--- |
| `INSERT` | `TableID, Rt` | Inserts the tuple in `Rt` into `TableID`. | `InsertTuple()` |
| `UPDATE` | `TidR, Rt` | Updates record at `TidR` with data from `Rt`. | `UpdateTuple()` |
| `DELETE` | `TidR` | Deletes record at `TidR`. | `DeleteTuple()` |
| `IDX_INSERT`| `IdxID, Rk, TidR` | Inserts index entry for key `Rk` and `TidR`. | `InsertIndexEntry()` |
| `BULK_LOAD` | `IdxID, Rs` | Bulk loads index `IdxID` from scan `Rs`. | `BulkLoad()` |

### 3.3 Transaction Control Instructions
| Opcode | Operands | Description | Storage API Mapping |
| :--- | :--- | :--- | :--- |
| `BEGIN` | `IsoLevel` | Starts a transaction with `IsoLevel`. | `Begin()` |
| `PREPARE` | - | Prepares current transaction. | `Prepare()` |
| `COMMIT` | - | Commits current transaction. | `Commit()` |
| `ROLLBACK` | - | Rolls back current transaction. | `Rollback()` |

### 3.4 Expression & Logic Instructions
| Opcode | Operands | Description |
| :--- | :--- | :--- |
| `COLUMN` | `Rd, Rt, ColIdx` | Extracts value of column `ColIdx` from tuple `Rt` into `Rd`. |
| `LOAD_CONST` | `Rd, ConstIdx` | Loads a constant from the pool into `Rd`. |
| `ADD/SUB/MUL/DIV`| `Rd, Rs1, Rs2` | Standard arithmetic: `Rd = Rs1 op Rs2`. |
| `COMPARE` | `Rd, Rs1, Rs2` | Compares `Rs1` and `Rs2`, sets `Rd` to -1, 0, or 1. |
| `JUMP_IF` | `Rs, Label` | Jumps to `Label` if `Rs` evaluates to true (non-zero). |
| `HALT` | - | Terminates execution. |
| `RESULT_ROW` | `R_start, Count` | Emits a row to the caller using `Count` registers starting at `R_start`. |

---

## 4. Interface Integration

The VM acts as an orchestrator for `storage_interface.h`.

### 4.1 Query Lifecycle Example (SELECT)
1. **Transaction Initialization:** `Begin()` is called to create a `Transaction*`.
2. **Opening Scan:** `OPEN_SCAN` calls `OpenScan(txn, table_id, NULL)`.
3. **Fetching Loop:** 
    - `NEXT` calls `GetNext(handle, &tuple)`.
    - If successful, `COLUMN` instructions extract data into registers.
    - `WHERE` clause logic (e.g., `COMPARE`, `JUMP_IF`) filters the row.
    - `RESULT_ROW` packages registers into a result set.
4. **Cleanup:** `CLOSE` calls `CloseScan(handle)`.

### 4.2 Handling Predicates
For simple predicates, the VM can compile the logic into `PredicateTree` structures and pass them to `OpenScan` for storage-layer optimization (pushdown). More complex logic is handled by the VM's internal filtering loop (`COMPARE` + `JUMP_IF`).

---

## 5. Error Handling

The VM uses a dual-layered error propagation protocol:

1. **Storage Errors:** Return codes from `storage_interface.h` (e.g., `InsertTuple` returning non-zero) are captured. The VM immediately sets an internal `ERROR_FLAG`, stores the error code in a dedicated `ERR` register, and jumps to an error-handling block or halts.
2. **VM Runtime Errors:** Errors such as Division by Zero, Invalid Register Access, or Null Scan Handles trigger a `VM_EXCEPTION`.
3. **Propagation:** The calling environment receives a `VMResult` structure containing:
    - `status`: Success, EOF, or Error.
    - `error_code`: Specific code from the storage engine or VM.
    - `message`: Human-readable diagnostics.

---

## 6. Resource Management
To prevent memory leaks:
- The VM maintains a **Handle Tracker**. Any `ScanHandle` or `Transaction` allocated during execution must be registered.
- The `HALT` instruction or an unrecovered error triggers an automatic cleanup phase where all registered handles are closed via `CloseScan` or `Rollback`.
