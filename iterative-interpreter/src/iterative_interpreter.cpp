//#define DEBUG_PRINT 1

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
extern void* Belem_closure (void *p, int i);
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

# define INT    (ip += sizeof (int), *(int*)(ip - sizeof (int)))
# define BYTE   *ip++
# define STRING get_string (this->bf, INT)
# define FAIL   failure ("ERROR: invalid opcode %d-%d\n", h, l)

#define STOP           0xF
#define BINOP          0x0
#define BINOP_ADD      0x01
#define BINOP_SUB      0x02
#define BINOP_PROD     0x03
#define BINOP_DIV      0x04
#define BINOP_MOD      0x05
#define BINOP_LESS     0x06
#define BINOP_ELESS    0x07
#define BINOP_GREATER  0x08
#define BINOP_EGREATER 0x09
#define BINOP_EQUAL    0x0A
#define BINOP_NEQUAL   0x0B
#define BINOP_AND      0x0C
#define BINOP_OR       0x0D
#define BLOCK_DATE     0x1
#define BLOCK_CONST    0x10
#define BLOCK_STRING   0x11
#define BLOCK_SEXP     0x12
#define BLOCK_STI      0x13
#define BLOCK_STA      0x14
#define BLOCK_JMP      0x15
#define BLOCK_END      0x16
#define BLOCK_RET      0x17
#define BLOCK_DROP     0x18
#define BLOCK_DUP      0x19
#define BLOCK_SWAP     0x1A
#define BLOCK_ELEM     0x1B
#define LD             0x2
#define LDA            0x3
#define ST             0x4
#define BLOCK_MOVE     0x5
#define CJMPZ          0x50
#define CJMPNZ         0x51
#define BEGIN          0x52
#define CBEGIN         0x53
#define CLOSUSRE       0x54
#define CALLC          0x55
#define CALL           0x56
#define PLACE_TAG      0x57
#define ARRAY          0x58
#define CALL_FAIL      0x59
#define LINE           0x5A
#define PATT           0x6
#define PATT_BSTRING   0x60
#define PATT_BSTRING_T 0x61
#define PATT_BARRAY_T  0x62
#define PATT_BSEXP_T   0x63
#define PATT_BBOXED    0x64
#define PATT_BUNBOXED  0x65
#define PATT_BCLOSURE_T 0x66
#define BLOCK_CALL     0x7
#define CALL_LREAD     0x70
#define CALL_LWRITE    0x71
#define CALL_LLENGTH   0x72
#define CALL_LSRTING   0x73
#define CALL_BARRAY    0x74

using namespace boxing;

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
    auto result = reinterpret_cast<int32_t>(Belem_closure(id, box(i + 1)));

    return reinterpret_cast<int32_t *>(result);
}

int32_t *iterative_interpreter::lookup(char l, int32_t i) {
#define GLOBAL 0
#define LOCAL 1
#define ARGS 2
#define BINDED 3
    switch (l) {
        case GLOBAL:
            return global(i);
        case LOCAL:
            return local(i);
        case ARGS:
            return args(i);
        case BINDED:
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
        case BINOP_ADD:
            res = x + y;
            break;
        case BINOP_SUB:
            res = x - y;
            break;
        case BINOP_PROD:
            res = x * y;
            break;
        case BINOP_DIV:
            res = x / y;
            break;
        case BINOP_MOD:
            res = x % y;
            break;
        case BINOP_LESS:
            res = x < y;
            break;
        case BINOP_ELESS:
            res = x <= y;
            break;
        case BINOP_GREATER:
            res = x > y;
            break;
        case BINOP_EGREATER:
            res = x >= y;
            break;
        case BINOP_EQUAL:
            res = x == y;
            break;
        case BINOP_NEQUAL:
            res = x != y;
            break;
        case BINOP_AND:
            res = x && y;
            break;
        case BINOP_OR:
            res = x || y;
            break;
        default:
            failure("ERROR: invalid opcode %d\n", op);
    }
    stack::push_box(res);
}

inline void iterative_interpreter::eval_const(int value) {
    stack::push_box(value);
}

inline void iterative_interpreter::eval_string(char *str) {
    stack::push(reinterpret_cast<int32_t>(Bstring(str)));
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

inline void iterative_interpreter::eval_jmp(int32_t offset) {
    ip = bf->code_ptr + offset;
}

inline void iterative_interpreter::eval_end() {
    int32_t result = stack::pop();
    stack::set_stack_top(fp);
    fp = reinterpret_cast<int32_t *>(stack::pop());
    int32_t argc = stack::pop();
    ip = reinterpret_cast<char *>(stack::pop());
    stack::drop(argc);
    stack::push(result);
}

inline void iterative_interpreter::eval_drop() {
    stack::pop();
}

inline void iterative_interpreter::eval_dup() {
    stack::push(stack::peek());
}

inline void iterative_interpreter::eval_swap() {
    int32_t v1 = stack::pop();
    int32_t v2 = stack::pop();

    stack::push(v1);
    stack::push(v2);
}

inline void iterative_interpreter::eval_elem() {
    int32_t i = stack::pop();
    void *p = reinterpret_cast<void *>(stack::pop());
    stack::push(reinterpret_cast<int32_t>(Belem(p, i)));
}

inline void iterative_interpreter::eval_ld(int32_t l, int32_t i) {
    int32_t value = *lookup(l, i);
    stack::push(value);
}

inline void iterative_interpreter::eval_lda(int32_t l, int32_t i) {
    int32_t *ptr = lookup(l, i);
    stack::push(reinterpret_cast<int32_t>(ptr));
}

inline void iterative_interpreter::eval_st(int32_t l, int32_t i) {
    int32_t *ptr = lookup(l, i);
    int32_t value = stack::peek();
    *ptr = value;
}

inline void iterative_interpreter::eval_cjmpz(int32_t addr) {
    if (stack::unbox_pop() == 0) {
        jmp(addr);
    }
}

inline void iterative_interpreter::eval_cjmpnz(int32_t addr) {
    if (stack::unbox_pop() != 0) {
        jmp(addr);
    }
}

inline void iterative_interpreter::eval_begin(int32_t argc, int32_t nlocals) {
    stack::push(reinterpret_cast<int32_t>(fp));
    fp = stack::get_stack_top();
    stack::reserve(nlocals);
}

inline void iterative_interpreter::eval_closure(int32_t addr, int32_t argc) {
    int32_t args[argc];
    for (int i = 0; i < argc; i++) {
        char l = BYTE;
        int value = INT;
        args[i] = *lookup(l, value);
    }

    void *result = Bclosure_arr(box(argc), bf->code_ptr + addr, args);

    stack::push(reinterpret_cast<int32_t>(result));
}

inline void iterative_interpreter::eval_callc(int32_t argc) {
    void *label = Belem(reinterpret_cast<int32_t *>(stack::peek(argc)), box(0));
    stack::reverse(argc);
    stack::push(reinterpret_cast<int32_t>(ip));
    stack::push(argc + 1);
    ip = reinterpret_cast<char *>(label);
}

inline void iterative_interpreter::eval_call(int32_t addr, int32_t argc) {
    //fprintf(stdout, "eval_call addr=%d, argc=%d\n", addr, argc);
    stack::reverse(argc);
    stack::push(reinterpret_cast<int32_t>(ip));
    stack::push(argc);
    jmp(addr);
}

inline void iterative_interpreter::eval_tag(char *name, int32_t n) {
    void *d = reinterpret_cast<void *>(stack::pop());
    int32_t t = LtagHash(name);
    stack::push(Btag(d, t, box(n)));
}

inline void iterative_interpreter::eval_array(int32_t n) {
    void *d = reinterpret_cast<void *>(stack::pop());
    int32_t res = Barray_patt(d, box(n));
    stack::push(res);
}

inline void iterative_interpreter::eval_fail(int32_t h, int32_t l) {
    failure("FAIL %d %d", h, l);
}

inline void iterative_interpreter::eval_line(int32_t _) {
    //nothing
}

void iterative_interpreter::eval_patt(char ch) {
    char h = 0xF0 & ch, l = 0x0F & ch;
    auto value = reinterpret_cast<int32_t *>(stack::pop());
    int32_t result = 0;
    switch (ch) {
        case PATT_BSTRING:
            result = Bstring_patt(value, reinterpret_cast<int32_t *>(stack::pop()));
            break;
        case PATT_BSTRING_T:
            result = Bstring_tag_patt(value);
            break;
        case PATT_BARRAY_T:
            result = Barray_tag_patt(value);
            break;
        case PATT_BBOXED:
            result = Bsexp_tag_patt(value);
            break;
        case PATT_BSEXP_T:
            result = Bboxed_patt(value);
            break;
        case PATT_BUNBOXED:
            result = Bunboxed_patt(value);
            break;
        case PATT_BCLOSURE_T:
            result = Bclosure_tag_patt(value);
            break;
        default:
            FAIL;
    }
    stack::push(result);
}

inline void iterative_interpreter::eval_call_lread() {
    int32_t value = Lread();
    stack::push(value);
}

inline void iterative_interpreter::eval_call_lwrite() {
    int32_t value = stack::pop();
    stack::push(Lwrite(value));
}

inline void iterative_interpreter::eval_call_llength() {
    stack::push(Llength(reinterpret_cast<void *>(stack::pop())));
}

inline void iterative_interpreter::eval_call_lstring() {
    void *str = Lstring(reinterpret_cast<void *>(stack::pop()));
    stack::push(reinterpret_cast<int32_t>(str));
}

inline void iterative_interpreter::eval_call_barray(int32_t n) {
    //fprintf(stdout, "eval_call_barray n=%d\n", n);
    stack::reverse(n);
    auto result = reinterpret_cast<int32_t>(Barray_arr(box(n), stack::get_stack_top()));
    stack::drop(n);
    stack::push(result);
}

void iterative_interpreter::eval() {
    do {
        char x = BYTE,
                h = (x & 0xF0) >> 4,
                l = x & 0x0F;
#ifdef DEBUG_PRINT
        fprintf(stdout, "h = %d | l = %d\n", h, l);
#endif
        int arg1 = 0;
        char *str = nullptr;
        switch (h) {
            case STOP:
                return;

                /* BINOP */
            case BINOP:
                eval_binop(x);
                break;

            case BLOCK_DATE:
                switch (x) {
                    case BLOCK_CONST:
                        eval_const(INT);
                        break;

                    case BLOCK_STRING:
                        eval_string(STRING);
                        break;

                    case BLOCK_SEXP:
                        str = STRING;
                        eval_sexp(str, INT);
                        break;

                    case BLOCK_STA:
                        eval_sta();
                        break;

                    case BLOCK_JMP:
                        eval_jmp(INT);
                        break;

                    case BLOCK_END:
                        eval_end();
                        break;

                    case BLOCK_DROP:
                        eval_drop();
                        break;

                    case BLOCK_DUP:
                        eval_dup();
                        break;

                    case BLOCK_SWAP:
                        eval_swap();
                        break;

                    case BLOCK_ELEM:
                        eval_elem();
                        break;

                    default:
                        FAIL;
                }
                break;

            case LD:
                eval_ld(l, INT);
                break;
            case LDA:
                eval_lda(l, INT);
                break;
            case ST:
                eval_st(l, INT);
                break;

            case BLOCK_MOVE:
                switch (x) {
                    case CJMPZ:
                        eval_cjmpz(INT);
                        break;

                    case CJMPNZ:
                        eval_cjmpnz(INT);
                        break;

                    case BEGIN:
                    case CBEGIN:
                        arg1 = INT;
                        eval_begin(arg1, INT);
                        break;

                    case CLOSUSRE:
                        arg1 = INT;
                        eval_closure(arg1, INT);
                        break;

                    case CALLC:
                        eval_callc(INT);
                        break;

                    case CALL:
                        arg1 = INT;
                        eval_call(arg1, INT);
                        break;

                    case PLACE_TAG:
                        str = STRING;
                        eval_tag(str, INT);
                        break;

                    case ARRAY:
                        eval_array(INT);
                        break;

                    case CALL_FAIL:
                        arg1 = INT;
                        eval_fail(arg1, INT);
                        break;

                    case LINE:
                        eval_line(INT);
                        break;

                    default:
                        FAIL;
                }
                break;

            case PATT:
                eval_patt(x);
                break;

            case BLOCK_CALL: {
                switch (x) {
                    case CALL_LREAD:
                        eval_call_lread();
                        break;

                    case CALL_LWRITE:
                        eval_call_lwrite();
                        break;

                    case CALL_LLENGTH:
                        eval_call_llength();
                        break;

                    case CALL_LSRTING:
                        eval_call_lstring();
                        break;

                    case CALL_BARRAY:
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


