#include "iterative_interpreter.h"

extern int32_t* __gc_stack_top;
extern int32_t* __gc_stack_bottom;
extern const int stack_size;

iterative_interpreter::iterative_interpreter(bytefile *file): file(file) {

}

void iterative_interpreter::eval() {
    size_t index = 0;
    do {
        char b = get_byte();
        ++index;
    } while (true);
    return;
}


