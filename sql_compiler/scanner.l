%{
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "parser.tab.h"

void yyerror(const char *s);
%}

%option noyywrap
%option case-insensitive

DIGIT           [0-9]
LETTER          [a-zA-Z]
IDENTIFIER      {LETTER}[a-zA-Z0-9_]*
WHITESPACE      [ \t\r\n]+
STRING          '([^']|'')*'

%%

"SELECT"            { return SELECT; }
"FROM"              { return FROM; }
"WHERE"             { return WHERE; }
"INSERT"            { return INSERT; }
"INTO"              { return INTO; }
"VALUES"            { return VALUES; }
"UPDATE"            { return UPDATE; }
"SET"               { return SET; }
"DELETE"            { return DELETE; }
"CREATE"            { return CREATE; }
"TABLE"             { return TABLE; }
"INDEX"             { return INDEX; }
"ON"                { return ON; }
"DROP"              { return DROP; }
"PRIMARY"           { return PRIMARY; }
"KEY"               { return KEY; }
"NOT"               { return NOT; }
"NULL"              { return NULL_TOKEN; }
"UNIQUE"            { return UNIQUE; }
"INT"               { return INT; }
"INTEGER"           { return INTEGER; }
"TEXT"              { return TEXT; }
"VARCHAR"           { return VARCHAR; }
"REAL"              { return REAL; }
"BOOLEAN"           { return BOOLEAN; }
"OR"                { return OR; }
"AND"               { return AND; }

"="                 { return EQUAL; }
"<>"                { return NOT_EQUAL; }
"<"                 { return LESS; }
">"                 { return GREATER; }
"<="                { return LESS_EQUAL; }
">="                { return GREATER_EQUAL; }
"+"                 { return PLUS; }
"-"                 { return MINUS; }
"*"                 { return MULTIPLY; }
"/"                 { return DIVIDE; }
"||"                { return CONCAT; }

"("                 { return LPAREN; }
")"                 { return RPAREN; }
","                 { return COMMA; }
";"                 { return SEMICOLON; }

{DIGIT}+            { yylval.i64 = atoll(yytext); return NUMERIC_LITERAL; }
{DIGIT}+"."{DIGIT}* { yylval.f64 = atof(yytext); return REAL_LITERAL; }
{STRING}            { 
                      yylval.s = strndup(yytext + 1, strlen(yytext) - 2); 
                      // Handle escaped single quotes if necessary, but grammar simple
                      return STRING_LITERAL; 
                    }
{IDENTIFIER}        { yylval.s = strdup(yytext); return IDENTIFIER; }

{WHITESPACE}        { /* ignore */ }

.                   { printf("Unknown character: %s\n", yytext); }

%%
