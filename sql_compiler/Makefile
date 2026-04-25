CC = gcc
CFLAGS = -Wall -g -I.
LEX = flex
YACC = bison

all: sqlc

sqlc: lex.yy.o parser.tab.o compiler.o vm_mock.o
	$(CC) $(CFLAGS) -o sqlc lex.yy.o parser.tab.o compiler.o vm_mock.o

lex.yy.c: scanner.l parser.tab.h
	$(LEX) scanner.l

parser.tab.c parser.tab.h: parser.y
	$(YACC) -d parser.y

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c lex.yy.c

parser.tab.o: parser.tab.c
	$(CC) $(CFLAGS) -c parser.tab.c

compiler.o: compiler.c compiler.h
	$(CC) $(CFLAGS) -c compiler.c

vm_mock.o: vm_mock.c vm.h
	$(CC) $(CFLAGS) -c vm_mock.c

clean:
	rm -f sqlc *.o lex.yy.c parser.tab.c parser.tab.h
