#ifndef ITERATIVE_INTERPRETER_ITERATIVE_INTERPRETER_H
#define ITERATIVE_INTERPRETER_ITERATIVE_INTERPRETER_H

#include "bytefile.h"

class iterative_interpreter {
    bytefile *file;

public:
    explicit iterative_interpreter(bytefile *file);
    void eval();
};

#endif //ITERATIVE_INTERPRETER_ITERATIVE_INTERPRETER_H
