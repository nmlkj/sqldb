#ifndef SHELL_H
#define SHELL_H

#include "../sql_compiler/compiler.h"
#include "../sql_vm/vm.h"

typedef struct {
    int interactive_mode;
    char *history_file;
    VMContext *vm_ctx;
    CompilerContext *compiler_ctx;
} ShellState;

void shell_init(ShellState *state);
void shell_loop(ShellState *state);
void shell_execute_line(ShellState *state, char *line);
void shell_print_results(VMContext *ctx);
void shell_handle_dot_command(ShellState *state, char *line);

#endif // SHELL_H
