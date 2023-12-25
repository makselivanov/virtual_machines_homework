#include "iterative_interpreter.h"

int main(int argc, char* argv[]) {
    bytefile *f = read_file (argv[1]);
    auto interpreter = iterative_interpreter(f);
    interpreter.eval();
    return 0;
}