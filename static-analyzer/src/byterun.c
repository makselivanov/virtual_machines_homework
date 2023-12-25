/* Lama SM Bytecode interpreter */

# include <string.h>
# include <stdio.h>
# include <errno.h>
# include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
# include "byterun.h"

static void vfailure(char *s, va_list args) {
    fprintf(stderr, "*** FAILURE: ");
    vfprintf(stderr, s, args); // vprintf (char *, va_list) <-> printf (char *, ...)
    exit(255);
}

void failure(const char *s, ...) {
    va_list args;

    va_start (args, s);
    vfailure((char *) s, args);
}

/* Gets a string from a string table by an index */
char *get_string(bytefile *f, int pos) {
    return &f->string_ptr[pos];
}

/* Reads a binary bytecode bf by name and unpacks it */
bytefile *read_file(char *fname) {
    FILE *f = fopen(fname, "rb");
    long size;
    bytefile *file;

    if (f == 0) {
        failure("%s\n", strerror(errno));
    }

    if (fseek(f, 0, SEEK_END) == -1) {
        failure("%s\n", strerror(errno));
    }

    file = (bytefile *) malloc(sizeof(int) * 4 + (size = ftell(f)));

    if (file == 0) {
        failure("*** FAILURE: unable to allocate memory.\n");
    }

    rewind(f);

    if (size != fread(&file->stringtab_size, 1, size, f)) {
        failure("%s\n", strerror(errno));
    }

    fclose(f);

    file->string_ptr = &file->buffer[file->public_symbols_number * 2 * sizeof(int)];
    file->public_ptr = (int *) file->buffer;
    file->code_ptr = &file->string_ptr[file->stringtab_size];
    file->global_ptr = (int *) malloc(file->global_area_size * sizeof(int));

    return file;
}

void logger(FILE *f, const char *format, ...) {
    if (f == NULL) return;

    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);
}

/* Disassembles the bytecode pool */
char *disassemble_instruction(FILE *f, bytefile *bf, char *ip) {

# define INT    (ip += sizeof (int), *(int*)(ip - sizeof (int)))
# define BYTE   *ip++
# define STRING get_string (bf, INT)
# define FAIL   failure ("ERROR: invalid opcode %d-%d\n", h, l)

    char *ops[] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
    char *pats[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
    char *lds[] = {"LD", "LDA", "ST"};
    char x = BYTE,
            h = (x & 0xF0) >> 4,
            l = x & 0x0F;

    switch (h) {
        case 15:
            logger(f, "<end>");
            ip = NULL;
            break;

            /* BINOP */
        case 0:
            logger(f, "BINOP\t%s", ops[l - 1]);
            break;

        case 1:
            switch (l) {
                case 0:
                    logger(f, "CONST\t%d", INT);
                    break;

                case 1:
                    logger(f, "STRING\t%s", STRING);
                    break;

                case 2:
                    logger(f, "SEXP\t%s ", STRING);
                    logger(f, "%d", INT);
                    break;

                case 3:
                    logger(f, "STI");
                    break;

                case 4:
                    logger(f, "STA");
                    break;

                case 5:
                    logger(f, "JMP\t0x%.8x", INT);
                    break;

                case 6:
                    logger(f, "END");
                    break;

                case 7:
                    logger(f, "RET");
                    break;

                case 8:
                    logger(f, "DROP");
                    break;

                case 9:
                    logger(f, "DUP");
                    break;

                case 10:
                    logger(f, "SWAP");
                    break;

                case 11:
                    logger(f, "ELEM");
                    break;

                default:
                    FAIL;
            }
            break;

        case 2:
        case 3:
        case 4:
            logger(f, "%s\t", lds[h - 2]);
            switch (l) {
                case 0:
                    logger(f, "G(%d)", INT);
                    break;
                case 1:
                    logger(f, "L(%d)", INT);
                    break;
                case 2:
                    logger(f, "A(%d)", INT);
                    break;
                case 3:
                    logger(f, "C(%d)", INT);
                    break;
                default:
                    FAIL;
            }
            break;

        case 5:
            switch (l) {
                case 0:
                    logger(f, "CJMPz\t0x%.8x", INT);
                    break;

                case 1:
                    logger(f, "CJMPnz\t0x%.8x", INT);
                    break;

                case 2:
                    logger(f, "BEGIN\t%d ", INT);
                    logger(f, "%d", INT);
                    break;

                case 3:
                    logger(f, "CBEGIN\t%d ", INT);
                    logger(f, "%d", INT);
                    break;

                case 4:
                    logger(f, "CLOSURE\t0x%.8x", INT);
                    {
                        int n = INT;
                        for (int i = 0; i < n; i++) {
                            switch (BYTE) {
                                case 0:
                                    logger(f, "G(%d)", INT);
                                    break;
                                case 1:
                                    logger(f, "L(%d)", INT);
                                    break;
                                case 2:
                                    logger(f, "A(%d)", INT);
                                    break;
                                case 3:
                                    logger(f, "C(%d)", INT);
                                    break;
                                default:
                                    FAIL;
                            }
                        }
                    };
                    break;

                case 5:
                    logger(f, "CALLC\t%d", INT);
                    break;

                case 6:
                    logger(f, "CALL\t0x%.8x ", INT);
                    logger(f, "%d", INT);
                    break;

                case 7:
                    logger(f, "TAG\t%s ", STRING);
                    logger(f, "%d", INT);
                    break;

                case 8:
                    logger(f, "ARRAY\t%d", INT);
                    break;

                case 9:
                    logger(f, "FAIL\t%d", INT);
                    logger(f, "%d", INT);
                    break;

                case 10:
                    logger(f, "LINE\t%d", INT);
                    break;

                default:
                    FAIL;
            }
            break;

        case 6:
            logger(f, "PATT\t%s", pats[l]);
            break;

        case 7: {
            switch (l) {
                case 0:
                    logger(f, "CALL\tLread");
                    break;

                case 1:
                    logger(f, "CALL\tLwrite");
                    break;

                case 2:
                    logger(f, "CALL\tLlength");
                    break;

                case 3:
                    logger(f, "CALL\tLstring");
                    break;

                case 4:
                    logger(f, "CALL\tBarray\t%d", INT);
                    break;

                default:
                    FAIL;
            }
        }
            break;

        default:
            FAIL;
    }

    logger(f, "\n");
    return ip;
}