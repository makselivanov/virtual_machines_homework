#ifndef ITERATIVE_INTERPRETER_ITERATIVE_INTERPRETER_H
#define ITERATIVE_INTERPRETER_ITERATIVE_INTERPRETER_H

#include "stack.h"

extern "C" {
#include "bytefile.h"
}

class iterative_interpreter {
public:
    explicit iterative_interpreter(bytefile *file);

    ~iterative_interpreter();

    void eval();

private:
    bytefile *bf;
    char *ip;
    int32_t *fp;

    //util
    void jmp(int32_t addr);

    int32_t *lookup(char l, int32_t i);

    int32_t *binded(int32_t i);

    int32_t *args(int32_t i);

    int32_t *local(int32_t i);

    int32_t *global(int32_t i);

    //eval
    void eval_binop(char op);

    void eval_const(int32_t value);

    void eval_string(int32_t offset);

    void eval_sexp(char *name, int n);

    void eval_sta();

    void eval_jmp(int32_t offset);

    void eval_end();

    void eval_drop();

    void eval_dup();

    void eval_swap();

    void eval_elem();

    void eval_ld(int32_t l, int32_t i);

    void eval_lda(int32_t l, int32_t i);

    void eval_st(int32_t l, int32_t i);

    void eval_cjmpz(int32_t addr);

    void eval_cjmpnz(int32_t addr);

    void eval_begin(int32_t argc, int32_t nlocals);

    void eval_closure(int32_t addr, int32_t argc);

    void eval_callc(int32_t argc);

    void eval_call(int32_t addr, int32_t argc);

    void eval_tag(char *name, int32_t n);

    void eval_array(int32_t n);

    void eval_fail(int32_t h, int32_t l);

    void eval_line(int32_t b);

    void eval_patt(char l);

    void eval_call_lread();

    void eval_call_lwrite();

    void eval_call_llength();

    void eval_call_lstring();

    void eval_call_barray(int32_t n);
};

#endif //ITERATIVE_INTERPRETER_ITERATIVE_INTERPRETER_H
