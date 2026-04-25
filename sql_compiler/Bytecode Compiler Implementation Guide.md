Project: Bytecode Compiler Implementation
I need to develop a bytecode compiler that targets a specific virtual machine. Please provide the implementation based on the following specifications:

VM specification is in @VM_SPECIFICATION.md
## **1\. Lexical Analysis (scanner.l)**

Your Flex scanner must translate raw text into tokens that match the terminal symbols in @grammar.txt.

* **Keywords:** Map SQL keywords to unique token IDs (e.g., SELECT, INSERT, CREATE, TABLE, PRIMARY, KEY, NOT, NULL, AND, OR).  
* **Identifiers:** Match \<letter\> \[ \<alphanumeric\_sequence\> \]. Store the string value in yylval so the parser can identify table and column names.

* **Literals:**  
  * **Numeric:** Match \<digit\>+. Convert to int64\_t or double.

  * **String:** Match characters between single quotes ('...').

* **Operators:** Return tokens for \=, \<\>, \<, \>, \<=, \>=, \+, \-, \*, /, ||.

## ---

**2\. Parser Implementation (parser.y)**

The Bison parser will implement the recursive descent logic defined in the grammar. You should generate bytecode either during the reduction phase or by traversing an AST.

### **A. Data Definition (DDL) Mapping**

* **CREATE TABLE:** Map \<column\_definitions\> to a schema object. While the VM doesn't have a direct OP\_CREATE\_TABLE, the compiler must interface with the storage engine to register the new TableID before any instructions can reference it.

* **Data Types:** Map INT/INTEGER, TEXT, REAL, etc., to the internal type system used by the VM registers.

### **B. Data Manipulation (DML) Mapping**

* **SELECT:**  
  1. Emit OP\_BEGIN to start the transaction.  
  2. Emit OP\_OPEN\_SCAN for the target table\_id.  
  3. **Loop Label:** Emit OP\_NEXT. If EOF, jump to the end.  
  4. **Filter:** For \<where\_clause\>, emit OP\_COLUMN, OP\_LOAD\_CONST, and OP\_COMPARE. Use OP\_JUMP\_IF to skip rows that don't match.  
  5. **Output:** Emit OP\_COLUMN for each requested field, then OP\_RESULT\_ROW.  
  6. Emit OP\_CLOSE and OP\_HALT.  
* **INSERT:**  
  1. Evaluate \<expr\_list\> into registers using arithmetic opcodes (OP\_ADD, etc.).  
  2. Emit OP\_INSERT using the populated registers and the target TableID.  
  3. If an index exists, emit OP\_IDX\_INSERT.

## ---

**3\. Register & Resource Management**

Since the VM has a fixed set of 256 registers (R0 \- R255), the compiler must include a simple **Register Allocator**.

* **Temporary Values:** Use a counter to track the next available register during expression evaluation (e.g., a \+ b \* c requires intermediate registers for the result of b \* c).

* **Register Spilling:** For complex queries, ensure you do not exceed the 255-register limit.  
* **Constant Pool:** When the parser encounters a literal, add it to the VMContext-\>constants pool and emit OP\_LOAD\_CONST with the corresponding index.

## ---

**4\. Logical Expression Compilation**

The \<boolean\_expr\> grammar uses a hierarchical structure to enforce operator precedence (**NOT** \> **AND** \> **OR**).

| Logic Level | Implementation Strategy |
| :---- | :---- |
| **Predicate** | Emit OP\_COMPARE followed by a conditional jump. |
| **AND** | Use "short-circuit" logic: if the first factor is false, jump immediately to the next row/statement.  |
| **OR** | If the first term is true, jump directly to the "Success" block of the filter.  |

## ---

**5\. Build System (Makefile)**

Your Makefile should handle the specific dependencies of the generator tools:

1. **Flex:** lex.yy.c from scanner.l.  
2. **Bison:** parser.tab.c and parser.tab.h from parser.y (use \-d flag).  
3. **Compilation:** Link lex.yy.o, parser.tab.o, and your main driver against the VM implementation.  
4. **Clean:** Remove generated .c, .h, and binary files.

**Instruction Format Hint:** When emitting instructions, ensure they follow the 64-bit word format: \[Opcode:8\]\[Dest:8\]\[Src1:8\]\[Src2:8\]\[Immediate:32\].