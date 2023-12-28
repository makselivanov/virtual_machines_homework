//
// Created by makar on 28.12.23.
//

#ifndef ITERATIVE_INTERPRETER_BOX_H
#define ITERATIVE_INTERPRETER_BOX_H

namespace boxing {

    static inline int32_t box(int32_t value) {
        return (value << 1) | 1;
    }

    static inline int32_t unbox(int32_t value) {
        return value >> 1;
    }

    static inline bool is_boxed(int32_t value) {
        return value & 1;
    }
}

#endif //ITERATIVE_INTERPRETER_BOX_H
