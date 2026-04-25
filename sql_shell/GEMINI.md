Write SQL shell by following instructions below.

## ---

**1\. Define the Shell Core (Main Loop)**

The shell’s primary job is a **REPL** (Read-Eval-Print Loop). You need to create a main.c within ./sql\_shell that handles the following lifecycle:

* **Read:** Capture user input until a semicolon ; is reached.  
* **Evaluate:** Pass the raw string to the **SQL Compiler** (./sql\_compiler).  
* **Execute:** Pass the resulting bytecode/instruction set to the **VM** (./sql\_vm).  
* **Print:** Format the results returned from the **Storage Engine** (./storage\_engine).

## ---

**2\. Integration with the Compiler**

Since you are using lex and yacc (as seen in scanner.l and parser.y), your shell must interface with the generated parser.

* **Linking:** Include parser.tab.h and compiler.h in your shell source.

* **Input Buffering:** Use yy\_scan\_string() or redirect yyin to handle the strings captured from the shell prompt.  
* **Error Handling:** Ensure your shell catches yyerror so a single syntax mistake doesn't crash the entire session.

## ---

**3\. Wiring the Virtual Machine (VM)**

The shell needs to initialize the VM state. Your vm.h likely defines the instruction set and the register/stack state.

* **Initialization:** Call an init function (e.g., vm\_init()) at shell startup to prepare memory and storage handles.

* **Execution Pipe:**  
  1. The Compiler produces an Abstract Syntax Tree (AST) or OpCodes.  
  2. The Shell passes these to vm\_execute(opcode\_list).  
  3. The VM interacts with the storage\_interface.h to fetch or mutate data.

## ---

**4\. Interfacing with the Storage Engine**

Your shell will likely be the primary "viewer" of the .csv tables.

* **Metadata Loading:** Upon startup, the shell should verify the existence of table\_0.csv, table\_1.csv, etc., via the csv\_engine.

* **Formatting Output:** When a SELECT query is executed, the VM will return a result set. You must implement a formatter in the shell to print ASCII tables:**Note:** Use storage\_interface.h to determine column widths and types to ensure the shell output is readable.

## ---

**5\. Implementation Steps for the Engineer**

### **Phase A: The Makefile**

Update the Makefile in the root or ./sql\_shell to link the disparate objects. You will need to link:

* ../sql\_compiler/lex.yy.o  
* ../sql\_compiler/parser.tab.o  
* ../sql\_vm/vm.o  
* ../storage\_engine/csv\_engine/storage\_engine.o

### **Phase B: Command Handling**

Implement "Dot Commands" for the shell that bypass the SQL compiler (similar to SQLite):

* .exit or .quit: Gracefully close file pointers in the CSV engine and exit.  
* .tables: List all CSV files in the storage directory.  
* .help: Print supported SQL syntax.

### **Phase C: The Execution Flow**

1. **Prompt:** Display sql\> to the user.  
2. **Lex/Parse:** Pass string to yyparse().  
3. **Generate:** Convert AST to VM instructions (if not already done by the compiler).  
4. **Run:** Execute via vm\_run().  
5. **Flush:** Ensure any INSERT or UPDATE commands trigger a write-back through the storage\_interface.h to the CSV files.

### ---

**Suggested Directory Addition**

To keep the shell clean, I recommend adding a shell.h to define the REPL state:

C

// Example structure for sql\_shell/shell.h  
typedef struct {  
    int interactive\_mode;  
    char \*history\_file;  
    // Link to VM state  
    VMContext \*vm\_ctx;  
} ShellState;

Would you like me to draft a basic main.c for the shell that specifically hooks into your lex.yy.c and vm.h files?