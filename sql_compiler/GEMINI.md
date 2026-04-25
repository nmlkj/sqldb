Project: Bytecode Compiler Implementation
I need to develop a bytecode compiler that targets a specific virtual machine. Please provide the implementation based on the following specifications:
Interface to VM is via @vm.h
Tools & Frameworks
Parser Generator: Bison

Lexical Analyzer: Flex

Language: C/C++ (standard for these tools)

Reference Documentation
Language Syntax: See grammar.txt for the formal grammar definitions.

Target Architecture: Refer to VM_SPECIFICATION.md and vm.h for details on the virtual machine's instruction set, register layout, and binary format.

Key Deliverables
A Flex scanner (.l) to tokenize the source input.

A Bison parser (.y) to build the AST or generate bytecode directly.

Logic to map high-level constructs to the specific opcodes defined in vm.h.

A Makefile to automate the build process.