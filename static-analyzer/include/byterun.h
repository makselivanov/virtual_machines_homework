/* Lama SM Bytecode interpreter */
#ifndef ITERATIVE_INTERPRETER_BYTEFILE_H
#define ITERATIVE_INTERPRETER_BYTEFILE_H

# include <string.h>
# include <stdio.h>
# include <errno.h>
# include <malloc.h>

/* The unpacked representation of bytecode bf */
typedef struct {
    char *string_ptr;              /* A pointer to the beginning of the string table */
    int *public_ptr;              /* A pointer to the beginning of publics table    */
    char *code_ptr;                /* A pointer to the bytecode itself               */
    int *global_ptr;              /* A pointer to the global area                   */
    int stringtab_size;          /* The size (in bytes) of the string table        */
    int global_area_size;        /* The size (in words) of global area             */
    int bytecode_size;           /* The size (in bytes) of bytecode                */
    int public_symbols_number;   /* The number of public symbols                   */
    char buffer[0];
} bytefile;

/* Gets a string from a string table by an index */
char *get_string(bytefile *f, int pos);

/* Reads a binary bytecode bf by name and unpacks it */
bytefile *read_file(char *fname);

/* Disassembles the bytecode pool */
char *disassemble_instruction(FILE *f, bytefile *bf, char *ip);

#endif //ITERATIVE_INTERPRETER_BYTEFILE_H