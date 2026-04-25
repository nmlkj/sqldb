/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_SQL_COMPILER_PARSER_TAB_H_INCLUDED
# define YY_YY_SQL_COMPILER_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "../sql_compiler/parser.y"

    #include <stdint.h>
    #include "compiler.h"

#line 54 "../sql_compiler/parser.tab.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    NUMERIC_LITERAL = 258,         /* NUMERIC_LITERAL  */
    REAL_LITERAL = 259,            /* REAL_LITERAL  */
    IDENTIFIER = 260,              /* IDENTIFIER  */
    STRING_LITERAL = 261,          /* STRING_LITERAL  */
    SELECT = 262,                  /* SELECT  */
    FROM = 263,                    /* FROM  */
    WHERE = 264,                   /* WHERE  */
    INSERT = 265,                  /* INSERT  */
    INTO = 266,                    /* INTO  */
    VALUES = 267,                  /* VALUES  */
    UPDATE = 268,                  /* UPDATE  */
    SET = 269,                     /* SET  */
    DELETE = 270,                  /* DELETE  */
    CREATE = 271,                  /* CREATE  */
    TABLE = 272,                   /* TABLE  */
    INDEX = 273,                   /* INDEX  */
    ON = 274,                      /* ON  */
    DROP = 275,                    /* DROP  */
    PRIMARY = 276,                 /* PRIMARY  */
    KEY = 277,                     /* KEY  */
    NOT = 278,                     /* NOT  */
    NULL_TOKEN = 279,              /* NULL_TOKEN  */
    UNIQUE = 280,                  /* UNIQUE  */
    INT = 281,                     /* INT  */
    INTEGER = 282,                 /* INTEGER  */
    TEXT = 283,                    /* TEXT  */
    VARCHAR = 284,                 /* VARCHAR  */
    REAL = 285,                    /* REAL  */
    BOOLEAN = 286,                 /* BOOLEAN  */
    OR = 287,                      /* OR  */
    AND = 288,                     /* AND  */
    EQUAL = 289,                   /* EQUAL  */
    NOT_EQUAL = 290,               /* NOT_EQUAL  */
    LESS = 291,                    /* LESS  */
    GREATER = 292,                 /* GREATER  */
    LESS_EQUAL = 293,              /* LESS_EQUAL  */
    GREATER_EQUAL = 294,           /* GREATER_EQUAL  */
    PLUS = 295,                    /* PLUS  */
    MINUS = 296,                   /* MINUS  */
    MULTIPLY = 297,                /* MULTIPLY  */
    DIVIDE = 298,                  /* DIVIDE  */
    CONCAT = 299,                  /* CONCAT  */
    LPAREN = 300,                  /* LPAREN  */
    RPAREN = 301,                  /* RPAREN  */
    COMMA = 302,                   /* COMMA  */
    SEMICOLON = 303                /* SEMICOLON  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 17 "../sql_compiler/parser.y"

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

#line 132 "../sql_compiler/parser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_SQL_COMPILER_PARSER_TAB_H_INCLUDED  */
