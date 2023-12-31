#ifndef ITERATIVE_INTERPRETER_STACK_H
#define ITERATIVE_INTERPRETER_STACK_H

extern "C" {
#include "runtime.h"
};

#include "utility"
#include "box.h"
//#define DEBUG_PRINT 1

extern int32_t *__gc_stack_top, *__gc_stack_bottom;

const int STACK_CAPACITY = sizeof(int32_t) * (1 << 23);

namespace stack {

    inline int32_t *get_stack_bottom() {
        return __gc_stack_bottom;
    }

    inline int32_t *get_stack_max_top() {
        return __gc_stack_bottom - STACK_CAPACITY;
    }

    inline int32_t *get_stack_top() {
        return __gc_stack_top;
    }

    inline void set_stack_top(int32_t *value) {
        __gc_stack_top = value;
    }

    inline void init() {
        __gc_stack_bottom = __gc_stack_top = (new int32_t[STACK_CAPACITY]) + STACK_CAPACITY;
    }

    inline void clear() {
        delete[] get_stack_max_top();
    }

    inline size_t empty_size() {
        return __gc_stack_top - get_stack_max_top();
    }

    inline int32_t size() {
        return __gc_stack_bottom - __gc_stack_top;
    }

    inline void reverse(int32_t n) {
        if (size() < n) {
            failure("STACK: reverse: stack is too small, size=%d, n=%d\n", size(), n);
        }
        int32_t *top = __gc_stack_top;
        int32_t *bot = top + n - 1;
        while (top < bot) {
            std::swap(*(top++), *(bot--));
        }
    }

    inline int32_t pop() {
        if (size() < 1) {
            failure("STACK: pop - stack is empty\n");
        }
#ifdef DEBUG_PRINT
        printf("stack: pop - %d\n", *__gc_stack_top);
#endif
        return *(__gc_stack_top++);
    }

    inline int32_t peek(int32_t index = 0) {
        if (size() < index) {
            failure("STACK: peek - index is too large\n");
        }

        return __gc_stack_top[index];
    }

    inline void push(int32_t value) {
        if (empty_size() < 1) {
            failure("STACK: push - not enough empty space\n");
        }
#ifdef DEBUG_PRINT
        printf("stack: push - %d\n", value);
#endif

        *(--__gc_stack_top) = value;
    }

    inline int32_t unbox_pop() {
        return boxing::unbox(pop());
    }

    inline void push_box(int32_t value) {
        push(boxing::box(value));
    }

    inline void drop(int32_t n) {
        if (size() < n) {
            failure("STACK: drop: stack is too small\n");
        }

        __gc_stack_top += n;
    }

    inline void reserve(int32_t n) {
        if (empty_size() < n) {
            failure("STACK: reserve - not enough empty space\n");
        }
        __gc_stack_top -= n;
    }
}

#endif //ITERATIVE_INTERPRETER_STACK_H
