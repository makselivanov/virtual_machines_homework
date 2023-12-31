/* Lama SM Bytecode interpreter */
#ifndef ITERATIVE_INTERPRETER_BYTEFILE_H
#define ITERATIVE_INTERPRETER_BYTEFILE_H

# include <string.h>
# include <stdio.h>
# include <errno.h>
# include <malloc.h>
# include "runtime.h"

extern void *__start_custom_data;
extern void *__stop_custom_data;

/* The unpacked representation of bytecode bf */
typedef struct {
    char *string_ptr;              /* A pointer to the beginning of the string table */
    int *public_ptr;              /* A pointer to the beginning of publics table    */
    char *code_ptr;                /* A pointer to the bytecode itself               */
    int *global_ptr;              /* A pointer to the global area                   */
    int stringtab_size;          /* The size (in bytes) of the string table        */
    int global_area_size;        /* The size (in words) of global area             */
    int public_symbols_number;   /* The number of public symbols                   */
    char buffer[0];
} bytefile;

/* Gets a string from a string table by an index */
char *get_string(bytefile *f, int pos);

/* Gets a name for a public symbol */
char *get_public_name(bytefile *f, int i);

/* Gets an offset for a publie symbol */
int get_public_offset(bytefile *f, int i);

/* Reads a binary bytecode bf by name and unpacks it */
bytefile *read_file(char *fname);

/* Disassembles the bytecode pool */
void disassemble(FILE *f, bytefile *bf);

/* Dumps the contents of the bf */
void dump_file(FILE *f, bytefile *bf);

#endif //ITERATIVE_INTERPRETER_BYTEFILE_H