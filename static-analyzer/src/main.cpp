#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

extern "C" {
#include "byterun.h"
}

struct instruction {
    instruction(char *name, long len) : name(name), len(len) {}

    instruction(const instruction &other) = default;

    char *name;
    long len;

    bool operator<(const instruction &other) const {
        int i = 0;
        for (; i < std::min(this->len, other.len); i++) {
            if (this->name[i] != other.name[i]) return this->name[i] < other.name[i];
        }
        return (i != other.len);
    }
};

struct statistic {
    statistic(instruction instr, size_t count) : instr(instr), count(count) {}

    statistic(const statistic &other) = default;

    instruction instr;
    size_t count;

    bool operator<(const statistic &other) const {
        return this->count > other.count || (this->count == other.count && this->instr < other.instr);
    }
};

int main(int argc, char *argv[]) {
    bytefile *bf = read_file(argv[1]);
    char *ip = bf->code_ptr;
    std::map<instruction, size_t> counter;

    while (ip < bf->code_ptr + bf->bytecode_size) {
        char *next_ip = disassemble_instruction(nullptr, bf, ip);
        if (next_ip == nullptr) break;
        long len = next_ip - ip;
        auto instr = instruction(ip, len);
        counter[instr]++;
        ip = next_ip;
    }

    // <end> instruction
    counter[{ip, 1}]++;

    std::vector<statistic> sorted_instr;
    sorted_instr.reserve(counter.size());
    for (auto [key, item]: counter) {
        sorted_instr.emplace_back(key, item);
    }

    std::sort(sorted_instr.begin(), sorted_instr.end());

    FILE *f = stdout;

    for (auto [instr, count]: sorted_instr) {
        auto [name, len] = instr;
        fprintf(f, "%zu: ", count);
        disassemble_instruction(f, bf, name);
    }

    free(bf->global_ptr);
    free(bf);

    return 0;
}