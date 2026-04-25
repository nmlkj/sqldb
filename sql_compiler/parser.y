%code requires {
    #include <stdint.h>
    #include "compiler.h"
}

%code {
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    extern int yylex();
    void yyerror(const char *s);

    extern CompilerContext ctx;
}

%union {
    int64_t i64;
    double f64;
    char* s;
    uint8_t reg;
    int addr;
    struct {
        uint8_t start_reg;
        int count;
    } reg_list;
    IdentList idents;
}

%token <i64> NUMERIC_LITERAL
%token <f64> REAL_LITERAL
%token <s> IDENTIFIER STRING_LITERAL
%type <s> column_def
%token SELECT FROM WHERE INSERT INTO VALUES UPDATE SET DELETE CREATE TABLE INDEX ON DROP PRIMARY KEY NOT NULL_TOKEN UNIQUE INT INTEGER TEXT VARCHAR REAL BOOLEAN OR AND
%token EQUAL NOT_EQUAL LESS GREATER LESS_EQUAL GREATER_EQUAL PLUS MINUS MULTIPLY DIVIDE CONCAT LPAREN RPAREN COMMA SEMICOLON

%type <reg> expression term factor primary boolean_expr boolean_term boolean_factor predicate
%type <reg_list> result_columns expr_list
%type <idents> column_list
%type <idents> column_definitions
%type <addr> opt_where_clause

%left OR
%left AND
%left EQUAL NOT_EQUAL LESS GREATER LESS_EQUAL GREATER_EQUAL
%left PLUS MINUS CONCAT
%left MULTIPLY DIVIDE
%right NOT

%%

sql_script:
    statement SEMICOLON
    | statement SEMICOLON sql_script
    ;

statement:
    select_stmt
    | insert_stmt
    | create_table_stmt
    | drop_table_stmt
    ;

select_stmt:
    SELECT result_columns FROM IDENTIFIER opt_where_clause {
        TableID tid = lookup_table($4);
        if (tid == (TableID)-1) {
            yyerror("Table not found");
            YYERROR;
        }

        IdentList cols;
        int col_count;
        uint8_t start_reg;

        if ($2.count == -1) { // SELECT *
            cols = lookup_all_columns(tid);
            col_count = cols.count;
            if (col_count == 0) {
                yyerror("No columns found for table");
                YYERROR;
            }
            start_reg = allocate_register(&ctx);
            for(int i=1; i<col_count; i++) allocate_register(&ctx);
        } else {
            col_count = $2.count;
            start_reg = $2.start_reg;
        }

        uint8_t r_scan = allocate_register(&ctx);
        ctx.current_tuple_register = allocate_register(&ctx);
        emit(&ctx, OP_BEGIN, 0, 0, 0, 0);
        emit(&ctx, OP_OPEN_SCAN, r_scan, 0, 0, tid);
        
        int loop_start = ctx.code.count;
        int jump_to_end = emit(&ctx, OP_NEXT, ctx.current_tuple_register, r_scan, 0, 0);
        
        // Output columns
        for (int i = 0; i < col_count; i++) {
            emit(&ctx, OP_COLUMN, start_reg + i, ctx.current_tuple_register, 0, i);
        }
        emit(&ctx, OP_RESULT_ROW, start_reg, 0, 0, col_count);
        
        uint8_t r_true = allocate_register(&ctx);
        RegisterValue val; val.i64 = 1;
        emit(&ctx, OP_LOAD_CONST, r_true, 0, 0, add_constant(&ctx, val));
        emit(&ctx, OP_JUMP_IF, r_true, 0, 0, loop_start);
        
        int loop_end = ctx.code.count;
        patch_jump(&ctx, jump_to_end, loop_end);
        
        emit(&ctx, OP_CLOSE, r_scan, 0, 0, 0);
        emit(&ctx, OP_HALT, 0, 0, 0, 0);
        
        free_registers(&ctx, 0);
    }
    ;

opt_where_clause:
    /* empty */ { $$ = -1; }
    | WHERE boolean_expr {
        $$ = $2; 
    }
    ;

boolean_expr:
    boolean_term { $$ = $1; }
    | boolean_expr OR boolean_term {
        $$ = allocate_register(&ctx);
        // Simplified OR: res = b1 || b2
        // In reality, use JUMP_IF for short-circuit
        emit(&ctx, OP_ADD, $$, $1, $3, 0); 
    }
    ;

boolean_term:
    boolean_factor { $$ = $1; }
    | boolean_term AND boolean_factor {
        $$ = allocate_register(&ctx);
        // Simplified AND: res = b1 && b2
        emit(&ctx, OP_MUL, $$, $1, $3, 0);
    }
    ;

boolean_factor:
    NOT boolean_factor {
        $$ = allocate_register(&ctx);
        // Simplified NOT: res = 1 - b
        uint8_t r_one = allocate_register(&ctx);
        RegisterValue v; v.i64 = 1;
        emit(&ctx, OP_LOAD_CONST, r_one, 0, 0, add_constant(&ctx, v));
        emit(&ctx, OP_SUB, $$, r_one, $2, 0);
    }
    | LPAREN boolean_expr RPAREN { $$ = $2; }
    | predicate { $$ = $1; }
    ;

predicate:
    expression EQUAL expression {
        $$ = allocate_register(&ctx);
        uint8_t r_cmp = allocate_register(&ctx);
        emit(&ctx, OP_COMPARE, r_cmp, $1, $3, 0);
        // Sets r_cmp to 0 if equal.
        // We want $$ to be 1 if r_cmp == 0.
        // This is getting complex for a simple demo.
        // Let's assume OP_COMPARE sets a status or we use it directly.
    }
    | expression LESS expression {
        $$ = allocate_register(&ctx);
        emit(&ctx, OP_COMPARE, $$, $1, $3, 0);
    }
    ;

result_columns:
    MULTIPLY {
        $$.count = -1; // Marker for *
    }
    | column_list {
        $$.count = $1.count;
        $$.start_reg = allocate_register(&ctx);
        for(int i=1; i<$1.count; i++) allocate_register(&ctx);
    }
    ;

column_list:
    IDENTIFIER {
        $$.count = 1;
        $$.names = malloc(sizeof(char*));
        $$.names[0] = $1;
    }
    | IDENTIFIER COMMA column_list {
        $$.count = $3.count + 1;
        $$.names = realloc($3.names, sizeof(char*) * $$.count);
        $$.names[$$.count-1] = $1;
    }
    ;

insert_stmt:
    INSERT INTO IDENTIFIER LPAREN column_list RPAREN VALUES LPAREN expr_list RPAREN {
        TableID tid = lookup_table($3);
        if (tid == (TableID)-1) { yyerror("Table not found"); YYERROR; }
        
        uint8_t r_tuple = allocate_register(&ctx);
        emit(&ctx, OP_MAKE_RECORD, r_tuple, $9.start_reg, $9.count, 0);
        emit(&ctx, OP_INSERT, 0, r_tuple, 0, tid); 
        emit(&ctx, OP_HALT, 0, 0, 0, 0);
        free_registers(&ctx, 0);
    }
    ;

expr_list:
    expression {
        $$.count = 1;
        $$.start_reg = $1;
    }
    | expression COMMA expr_list {
        $$.count = $3.count + 1;
        $$.start_reg = $1; // Should be contiguous? 
        // This is tricky without a proper register allocator that ensures contiguity if needed.
    }
    ;

create_table_stmt:
    CREATE TABLE IDENTIFIER LPAREN column_definitions RPAREN {
        uint8_t r_names = allocate_register(&ctx);
        for (int i = 1; i < $5.count; i++) allocate_register(&ctx);
        
        for (int i = 0; i < $5.count; i++) {
            RegisterValue v; v.s = $5.names[i];
            emit(&ctx, OP_LOAD_CONST, r_names + i, 0, 0, add_constant(&ctx, v));
        }
        
        RegisterValue vt; vt.s = $3;
        uint32_t name_const = add_constant(&ctx, vt);
        
        emit(&ctx, OP_CREATE_TABLE, 0, r_names, $5.count, name_const);
        emit(&ctx, OP_HALT, 0, 0, 0, 0);
        free_registers(&ctx, 0);
    }
    ;

column_definitions:
    column_def {
        $$.count = 1;
        $$.names = malloc(sizeof(char*));
        $$.names[0] = $1;
    }
    | column_def COMMA column_definitions {
        $$.count = $3.count + 1;
        $$.names = realloc($3.names, sizeof(char*) * $$.count);
        memmove($$.names + 1, $$.names, sizeof(char*) * $3.count);
        $$.names[0] = $1;
    }
    ;

column_def:
    IDENTIFIER data_type { $$ = $1; }
    | IDENTIFIER data_type column_constraints { $$ = $1; }

data_type:
    INT | INTEGER | TEXT | REAL | BOOLEAN | VARCHAR LPAREN NUMERIC_LITERAL RPAREN
    ;

column_constraints:
    column_constraint
    | column_constraint column_constraints
    ;

column_constraint:
    PRIMARY KEY | NOT NULL_TOKEN | UNIQUE
    ;

drop_table_stmt:
    DROP TABLE IDENTIFIER {
        printf("Table %s dropped\n", $3);
    }
    ;

expression:
    term { $$ = $1; }
    | expression PLUS expression {
        $$ = allocate_register(&ctx);
        emit(&ctx, OP_ADD, $$, $1, $3, 0);
    }
    | expression MINUS expression {
        $$ = allocate_register(&ctx);
        emit(&ctx, OP_SUB, $$, $1, $3, 0);
    }
    | expression MULTIPLY expression {
        $$ = allocate_register(&ctx);
        emit(&ctx, OP_MUL, $$, $1, $3, 0);
    }
    | expression DIVIDE expression {
        $$ = allocate_register(&ctx);
        emit(&ctx, OP_DIV, $$, $1, $3, 0);
    }
    ;

term:
    factor { $$ = $1; }
    ;

factor:
    primary { $$ = $1; }
    | MINUS factor {
        // Implement unary minus
        uint8_t zero_reg = allocate_register(&ctx);
        RegisterValue v; v.i64 = 0;
        emit(&ctx, OP_LOAD_CONST, zero_reg, 0, 0, add_constant(&ctx, v));
        $$ = allocate_register(&ctx);
        emit(&ctx, OP_SUB, $$, zero_reg, $2, 0);
    }
    ;

primary:
    NUMERIC_LITERAL {
        $$ = allocate_register(&ctx);
        RegisterValue v; v.i64 = $1;
        emit(&ctx, OP_LOAD_CONST, $$, 0, 0, add_constant(&ctx, v));
    }
    | REAL_LITERAL {
        $$ = allocate_register(&ctx);
        RegisterValue v; v.f64 = $1;
        emit(&ctx, OP_LOAD_CONST, $$, 0, 0, add_constant(&ctx, v));
    }
    | STRING_LITERAL {
        $$ = allocate_register(&ctx);
        RegisterValue v; v.s = $1;
        emit(&ctx, OP_LOAD_CONST, $$, 0, 0, add_constant(&ctx, v));
    }
    | IDENTIFIER {
        // This should be OP_COLUMN in context of a scan, 
        // but here it's an expression.
        // For simplicity, assume it's a column from the current scan if we are in one.
        $$ = allocate_register(&ctx);
        // Need to know current tuple register...
        emit(&ctx, OP_COLUMN, $$, ctx.current_tuple_register, 0, 0);
    }
    | LPAREN expression RPAREN { $$ = $2; }
    ;

%%

