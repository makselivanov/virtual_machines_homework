#include <chrono>
#include <iostream>
#include <vector>
#include <map>
#include <random>

/**
 * | Измерение                          | lscpu --cache | измеренные |
 * | ---------------------------------- | ------------- | ---------- |
 * | Размер кеша одного ядра (ONE-SIZE) | 48 KiB        |            |
 * | Размер линии (COHERENCY-SIZE)      | 64 B          |            |
 * | Ассоциативность (WAYS)             | 12            |            |
 */
struct info_L1 {
    size_t cache_size;
    size_t cache_line;
    size_t associativity;
};

std::chrono::duration<double, std::milli>
measure(std::vector<size_t> &array, size_t shift = 0, size_t loop_size = (1 << 20)) {
    size_t index = array[shift];
    auto start = std::chrono::high_resolution_clock::now();
    //pointer chaser for array
    for (size_t loop = 0; loop < loop_size; loop++) {
        index = array[index];
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = finish - start;
    return duration;
}

info_L1 get_cache_size_and_associativity(size_t max_size, size_t max_way_size) {
    fprintf(stderr, "Calculating cache size and associativity\n");
    fprintf(stderr, "way_size, associativity, measurement\n");
    std::random_device dev;
    std::mt19937 rng(dev());
    info_L1 result = {
            .cache_size = 0,
            .cache_line = 0,
            .associativity = 0,
    };
    size_t way_size = max_way_size;
    size_t assoc_prev = 0;
    std::map<size_t, std::chrono::duration<double, std::milli>> measurements;
    while (true) {
        for (size_t assoc = 1;; assoc += 1) {
            //generate
            std::vector<size_t> array(way_size * assoc);
            for (size_t i = 0; i < assoc; ++i) {
                size_t j = std::uniform_int_distribution<>(0, int(i))(rng);
                array[i * way_size] = array[j * way_size];
                array[j * way_size] = i * way_size;
            }
            //measure
            measurements[assoc] = measure(array);
            fprintf(stderr, "%lu, %lu, %f\n", way_size, assoc, measurements[assoc].count());
            //TODO Save measurements to log?
            if (assoc != 1 && measurements[assoc] > 1.01 * measurements[assoc - 1]) {
                if (assoc - 1 == 2 * assoc_prev) {
                    result = {
                            .cache_size = way_size * 2 * assoc_prev,
                            .associativity = assoc_prev
                    };
                    return result;
                } else {
                    assoc_prev = assoc - 1;
                    break;
                }
            }
        }
        way_size = way_size / 2;
    }
}

info_L1 get_info_about_L1_memory(size_t max_size = 5000000, size_t max_way_size = 1u << 20) {
    info_L1 result = get_cache_size_and_associativity(max_size, max_way_size);
    return result;
}

int main() {
    info_L1 info = get_info_about_L1_memory();
    printf("Cache size: %lu bytes\nCache line: %lu bytes\nAssociativity: %lu\n",
           info.cache_size, info.cache_line, info.associativity);
    return 0;
}
