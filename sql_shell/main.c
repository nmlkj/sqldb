#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "../sql_compiler/parser.tab.h"

// Forward declarations for flex/bison
extern int yyparse();
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern void yylex_destroy();

// Global compiler context as defined in compiler.c
extern CompilerContext ctx;

void yyerror(const char *s) {
    fprintf(stderr, "SQL Error: %s\n", s);
}

void shell_init(ShellState *state) {
    state->interactive_mode = 1;
    state->history_file = ".sql_shell_history";
    
    state->compiler_ctx = &ctx;
    compiler_init(state->compiler_ctx);
    
    state->vm_ctx = malloc(sizeof(VMContext));
    memset(state->vm_ctx, 0, sizeof(VMContext));
}

void shell_handle_dot_command(ShellState *state, char *line) {
    if (strncmp(line, ".exit", 5) == 0 || strncmp(line, ".quit", 5) == 0) {
        printf("Goodbye!\n");
        exit(0);
    } else if (strncmp(line, ".tables", 7) == 0) {
        printf("Listing tables (CSV files):\n");
        system("ls ../storage_engine/csv_engine/*.csv");
    } else if (strncmp(line, ".help", 5) == 0) {
        printf("Supported SQL syntax:\n");
        printf("  SELECT * FROM table;\n");
        printf("  SELECT col1, col2 FROM table WHERE col1 = val;\n");
        printf("  INSERT INTO table (col1, col2) VALUES (val1, val2);\n");
        printf("Dot Commands:\n");
        printf("  .exit, .quit - Exit the shell\n");
        printf("  .tables      - List all tables\n");
        printf("  .help        - Show this help\n");
    } else {
        printf("Unknown dot command: %s. Type .help for assistance.\n", line);
    }
}

void shell_execute_line(ShellState *state, char *line) {
    if (line[0] == '.') {
        shell_handle_dot_command(state, line);
        return;
    }

    // Reset compiler context for new statement
    // Note: We might need a proper reset function in compiler.c
    // For now, let's just re-init if needed or assume compiler_init handles it
    compiler_init(state->compiler_ctx);

    YY_BUFFER_STATE buffer = yy_scan_string(line);
    if (yyparse() == 0) {
        printf("Executing %u instructions...\n", state->compiler_ctx->code.count);
        for (uint32_t i = 0; i < state->compiler_ctx->code.count; i++) {
            Instruction instr = state->compiler_ctx->code.instructions[i];
            printf("[%03u] Op:%u Rd:%u Rs1:%u Rs2:%u Imm:%d\n", i, instr.opcode, instr.dest, instr.src1, instr.src2, instr.immediate);
        }
        // Successful parse and code generation
        state->vm_ctx->program = state->compiler_ctx->code.instructions;
        state->vm_ctx->constants = state->compiler_ctx->constants.constants;
        state->vm_ctx->num_constants = state->compiler_ctx->constants.count;
        
        VMResult res = VM_Execute(state->vm_ctx);
        if (res.status == VM_STATUS_ERROR) {
            fprintf(stderr, "VM Error: %s\n", res.message);
        }
    }
    
    yy_delete_buffer(buffer);
    // yylex_destroy(); // Clean up lexer state
}

void shell_loop(ShellState *state) {
    char line[1024];
    printf("SQL Shell v0.1\n");
    printf("Enter SQL statements ending with ';' or '.help' for commands.\n");
    
    while (1) {
        printf("sql> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0) continue;
        
        // Check if it ends with semicolon or is a dot command
        if (line[0] == '.' || line[strlen(line)-1] == ';') {
            shell_execute_line(state, line);
        } else {
            // Support multi-line input in a real shell, but keep it simple for now
            printf("Error: Statements must end with ';'\n");
        }
    }
}

int main(int argc, char **argv) {
    ShellState state;
    shell_init(&state);
    shell_loop(&state);
    return 0;
}
