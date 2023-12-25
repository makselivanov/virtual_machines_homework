#include "iterative_interpreter.h"
#include <exception>
#include <stdexcept>

extern "C" {
extern void __init(void);
extern void *Bstring(void *);
extern void *Bsexp_arr(int bn, int tag, int *values);
extern int LtagHash(char *s);
extern void *Bsta(void *v, int i, void *x);
extern void *Belem(void *p, int i);
extern int Btag(void *d, int t, int n);
extern int Barray_patt(void *d, int n);
extern int Bstring_patt(void *x, void *y);
extern int Bstring_tag_patt(void *x);
extern int Barray_tag_patt(void *x);
extern int Bsexp_tag_patt(void *x);
extern int Bboxed_patt(void *x);
extern int Bunboxed_patt(void *x);
extern int Bclosure_tag_patt(void *x);
extern int Lread();
extern int Lwrite(int);
extern int Llength(void *);
extern void *Lstring(void *p);
extern void *Barray_arr(int bn, int *values);
extern void *Bclosure_arr(int bn, void *entry, int *values);
}

iterative_interpreter::iterative_interpreter(bytefile *file) : bf(file), ip(bf->code_ptr) {
    __init();
    stack::init();

    fp = stack::get_stack_top();
    stack::reserve(2);
    stack::push(reinterpret_cast<int32_t>(nullptr));
    stack::push(2);
}

iterative_interpreter::~iterative_interpreter() {
    free(bf->global_ptr);
    free(bf);
    stack::clear();
}

int32_t *iterative_interpreter::global(int32_t i) {
    return bf->global_ptr + i;
}

int32_t *iterative_interpreter::local(int32_t i) {
    return fp - i - 1;
}

int32_t *iterative_interpreter::args(int32_t i) {
    return fp + i + 3;
}

int32_t *iterative_interpreter::binded(int32_t i) {
    int32_t nargs = *(fp + 1);
    auto id = reinterpret_cast<int32_t *>(*args(nargs - 1));
    auto result = reinterpret_cast<int32_t>(Belem(id, box(i + 1)));

    return reinterpret_cast<int32_t *>(result);
}

int32_t *iterative_interpreter::lookup(char l, int32_t i) {
    switch (l) {
        case 0:
            return global(i);
        case 1:
            return local(i);
        case 2:
            return args(i);
        case 3:
            return binded(i);
        default:
            failure("Unexpected location: %d", l);
    }

    return nullptr;
}

void iterative_interpreter::jmp(int32_t addr) {
    ip = bf->code_ptr + addr;
}

void iterative_interpreter::eval_binop(char op) {
    int32_t y = stack::unbox_pop();
    int32_t x = stack::unbox_pop();
    int32_t res;
    switch (op) {
        case 1:
            res = x + y;
            break;
        case 2:
            res = x - y;
            break;
        case 3:
            res = x * y;
            break;
        case 4:
            res = x / y;
            break;
        case 5:
            res = x % y;
            break;
        case 6:
            res = x < y;
            break;
        case 7:
            res = x <= y;
            break;
        case 8:
            res = x > y;
            break;
        case 9:
            res = x >= y;
            break;
        case 10:
            res = x == y;
            break;
        case 11:
            res = x != y;
            break;
        case 12:
            res = x && y;
            break;
        case 13:
            res = x || y;
            break;
        default:
            failure("ERROR: invalid opcode %d\n", op);
    }
    stack::push_box(res);
}

void iterative_interpreter::eval_const(int value) {
    stack::push_box(value);
}

inline void iterative_interpreter::eval_string(int offset) {
    stack::push_box(reinterpret_cast<int64_t>(Bstring(bf->string_ptr + offset)));
}

inline void iterative_interpreter::eval_sexp(char *name, int n) {
    int32_t tag = LtagHash(name);
    stack::reverse(n);
    auto res = reinterpret_cast<int32_t>(Bsexp_arr(box(n + 1), tag, stack::get_stack_top()));
    stack::drop(n);
    stack::push(res);
}

inline void iterative_interpreter::eval_sta() {
    void *v = reinterpret_cast<void *>(stack::pop());
    int32_t i = stack::pop();

    if (!is_boxed(i)) {
        return stack::push(reinterpret_cast<int32_t>(Bsta(v, i, nullptr)));
    }
    void *x = reinterpret_cast<void *>(stack::pop());
    stack::push(reinterpret_cast<int32_t>(Bsta(v, i, x)));
}

void iterative_interpreter::eval_jmp(int32_t offset) {
    ip = bf->code_ptr + offset;
}

void iterative_interpreter::eval_end() {
    int32_t result = stack::pop();
    stack::set_stack_top(fp);
    fp = reinterpret_cast<int32_t *>(stack::pop());
    int32_t argc = stack::pop();
    ip = reinterpret_cast<char *>(stack::pop());
    stack::drop(argc);
    stack::push(result);
}

void iterative_interpreter::eval_drop() {
    stack::pop();
}

void iterative_interpreter::eval_dup() {
    stack::push(stack::peek());
}

void iterative_interpreter::eval_swap() {
    int32_t v1 = stack::pop();
    int32_t v2 = stack::pop();

    stack::push(v1);
    stack::push(v2);
}

void iterative_interpreter::eval_elem() {
    int32_t i = stack::pop();
    void *p = reinterpret_cast<void *>(stack::pop());
    stack::push(reinterpret_cast<int32_t>(Belem(p, i)));
}

void iterative_interpreter::eval_ld(int32_t l, int32_t i) {
    int32_t value = *lookup(l, i);
    stack::push(value);
}

void iterative_interpreter::eval_lda(int32_t l, int32_t i) {
    int32_t *ptr = lookup(l, i);
    stack::push(reinterpret_cast<int32_t>(ptr));
}

void iterative_interpreter::eval_st(int32_t l, int32_t i) {
    int32_t *ptr = lookup(l, i);
    int32_t value = stack::peek();
    *ptr = value;
}

void iterative_interpreter::eval_cjmpz(int32_t addr) {
    if (stack::unbox_pop() == 0) {
        jmp(addr);
    }
}

void iterative_interpreter::eval_cjmpnz(int32_t addr) {
    if (stack::unbox_pop() != 0) {
        jmp(addr);
    }
}

void iterative_interpreter::eval_begin(int32_t argc, int32_t nlocals) {
    stack::push(reinterpret_cast<int32_t>(fp));
    fp = stack::get_stack_top();
    stack::reserve(nlocals);
}

void iterative_interpreter::eval_closure(int32_t addr, int32_t argc) {
    int32_t args[argc];
# define INT    (ip += sizeof (int), *(int*)(ip - sizeof (int)))
# define BYTE   *ip++
# define STRING get_string (this->bf, INT)

    for (int i = 0; i < argc; i++) {
        char l = BYTE;
        int value = INT;
        args[i] = *lookup(l, value);
    }

    void *result = Bclosure_arr(box(argc), bf->code_ptr + addr, args);

    stack::push(reinterpret_cast<int32_t>(result));
}

void iterative_interpreter::eval_callc(int32_t argc) {
    void *label = Belem(reinterpret_cast<int32_t *>(stack::peek(argc)), box(0));
    stack::reverse(argc);
    stack::push(reinterpret_cast<int32_t>(ip));
    stack::push(argc + 1);
    ip = reinterpret_cast<char *>(label);
}

void iterative_interpreter::eval_call(int32_t addr, int32_t argc) {
    //fprintf(stderr, "eval_call addr=%d, argc=%d\n", addr, argc);
    stack::reverse(argc);
    stack::push(reinterpret_cast<int32_t>(ip));
    stack::push(argc);
    jmp(addr);
}

void iterative_interpreter::eval_tag(char *name, int32_t n) {
    void *d = reinterpret_cast<void *>(stack::pop());
    int32_t t = LtagHash(name);
    stack::push(Btag(d, t, box(n)));
}

void iterative_interpreter::eval_array(int32_t n) {
    void *d = reinterpret_cast<void *>(stack::pop());
    int32_t res = Barray_patt(d, box(n));
    stack::push(res);
}

void iterative_interpreter::eval_fail(int32_t h, int32_t l) {
    failure("FAIL %d %d", h, l);
}

void iterative_interpreter::eval_line(int32_t _) {
    //nothing
}

void iterative_interpreter::eval_patt(char l) {
    auto value = reinterpret_cast<int32_t *>(stack::pop());
    int32_t result;
    switch (l) {
        case 0:
            result = Bstring_patt(value, reinterpret_cast<int32_t *>(stack::pop()));
            break;
        case 1:
            result = Bstring_tag_patt(value);
            break;
        case 2:
            result = Barray_tag_patt(value);
            break;
        case 3:
            result = Bsexp_tag_patt(value);
            break;
        case 4:
            result = Bboxed_patt(value);
            break;
        case 5:
            result = Bunboxed_patt(value);
            break;
        case 6:
            result = Bclosure_tag_patt(value);
            break;
        default:
            failure("Unexpected patt: %d", l);
    }
    stack::push(result);
}

void iterative_interpreter::eval_call_lread() {
    int32_t value = Lread();
    stack::push(value);
}

void iterative_interpreter::eval_call_lwrite() {
    int32_t value = stack::pop();
    stack::push(Lwrite(value));
}

void iterative_interpreter::eval_call_llength() {
    stack::push(Llength(reinterpret_cast<void*>(stack::pop())));
}

void iterative_interpreter::eval_call_lstring() {
    void *str = Lstring(reinterpret_cast<void*>(stack::pop()));
    stack::push(reinterpret_cast<int32_t>(str));
}

void iterative_interpreter::eval_call_barray(int32_t n) {
    stack::reverse(n);
    auto result = reinterpret_cast<int32_t>(Barray_arr(box(n), stack::get_stack_top()));
    stack::drop(n);
    stack::push(result);
}

void iterative_interpreter::eval() {
# define INT    (ip += sizeof (int), *(int*)(ip - sizeof (int)))
# define BYTE   *ip++
# define STRING get_string (this->bf, INT)
# define FAIL   failure ("ERROR: invalid opcode %d-%d\n", h, l)
    do {
        char x = BYTE,
                h = (x & 0xF0) >> 4,
                l = x & 0x0F;
        //fprintf(stderr, "h = %d | l = %d\n", h, l);
        int arg1, arg2;
        char *str;
        switch (h) {
            case 15:
                return;

                /* BINOP */
            case 0:
                eval_binop(l);
                break;

            case 1:
                switch (l) {
                    case 0:
                        eval_const(INT);
                        break;

                    case 1:
                        eval_string(INT);
                        break;

                    case 2:
                        str = STRING;
                        eval_sexp(str, INT);
                        break;

                    case 4:
                        eval_sta();
                        break;

                    case 5:
                        eval_jmp(INT);
                        break;

                    case 6:
                        eval_end();
                        break;

                    case 8:
                        eval_drop();
                        break;

                    case 9:
                        eval_dup();
                        break;

                    case 10:
                        eval_swap();
                        break;

                    case 11:
                        eval_elem();
                        break;

                    default:
                        FAIL;
                }
                break;

            case 2:
                eval_ld(l, INT);
                break;
            case 3:
                eval_lda(l, INT);
                break;
            case 4:
                eval_st(l, INT);
                break;

            case 5:
                switch (l) {
                    case 0:
                        eval_cjmpz(INT);
                        break;

                    case 1:
                        eval_cjmpnz(INT);
                        break;

                    case 2:
                    case 3:
                        arg1 = INT;
                        eval_begin(arg1, INT);
                        break;

                    case 4:
                        arg1 = INT;
                        eval_closure(arg1, INT);
                        break;

                    case 5:
                        eval_callc(INT);
                        break;

                    case 6:
                        arg1 = INT;
                        eval_call(arg1, INT);
                        break;

                    case 7:
                        str = STRING;
                        eval_tag(str, INT);
                        break;

                    case 8:
                        eval_array(INT);
                        break;

                    case 9:
                        arg1 = INT;
                        eval_fail(arg1, INT);
                        break;

                    case 10:
                        eval_line(INT);
                        break;

                    default:
                        FAIL;
                }
                break;

            case 6:
                eval_patt(l);
                break;

            case 7: {
                switch (l) {
                    case 0:
                        eval_call_lread();
                        break;

                    case 1:
                        eval_call_lwrite();
                        break;

                    case 2:
                        eval_call_llength();
                        break;

                    case 3:
                        eval_call_lstring();
                        break;

                    case 4:
                        eval_call_barray(INT);
                        break;

                    default:
                        FAIL;
                }
            }
                break;

            default:
                FAIL;
        }
    } while (ip != nullptr);
}


