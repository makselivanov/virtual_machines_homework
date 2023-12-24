#include "runtime.h"
#include "bytefile.h"

int main(int argc, char* argv[]) {
    bytefile *f = read_file (argv[1]);
    auto interpreter = iterative_interperter(f);
    interpreter.eval();
}