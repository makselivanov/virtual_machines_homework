#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <algorithm>

/**
 * | Измерение                          | lscpu --cache | измеренные |
 * | ---------------------------------- | ------------- | ---------- |
 * | Размер кеша одного ядра (ONE-SIZE) | 48 KiB        | 49152      |
 * | Размер линии (COHERENCY-SIZE)      | 64 B          | 64         |
 * | Ассоциативность (WAYS)             | 12            | 12         |
 */
struct info_L1 {
    size_t cache_size;
    size_t cache_line;
    size_t associativity;

    bool operator<(const info_L1 &other) const {
        return cache_size < other.cache_size || (cache_size == other.cache_size &&
                                                 (associativity < other.associativity ||
                                                  (associativity == other.associativity &&
                                                   cache_line < other.cache_line)));
    }
};

struct measured_time {
    std::chrono::duration<double, std::nano> total;
    double nano_per_loop;
};

const size_t DEFAULT_LOOP_SIZE = 1 << 20;
const size_t MAX_SIZE = 1 << 23;
const double TIME_THRESHOLD = 1.2;
const size_t MAX_ASSOC = 32;
const size_t MIN_ASSOC = 4;
const size_t MIN_WAY_SIZE = 1 << 10;
const size_t MAX_WAY_SIZE = 1 << 16;
const size_t EPOCHS = 20;
const size_t MIN_CACHE_LINE = 8;
std::random_device dev;
std::mt19937 rng(dev());

alignas(256) uint32_t array[MAX_SIZE];
long long trash = 0;

measured_time
measure(size_t assoc_size, size_t loop_size = DEFAULT_LOOP_SIZE) {
    //cache
    size_t index = 0;
    for (size_t loop = 0; loop < assoc_size; loop++) {
        index = array[index];
    }
    trash ^= index;
    index = 0;
    //measure
    auto start = std::chrono::high_resolution_clock::now();
    //pointer chaser for array
    for (size_t loop = 0; loop < loop_size; loop++) {
        index = array[index];
    }
    auto finish = std::chrono::high_resolution_clock::now();
    trash ^= index;
    std::chrono::duration<double, std::nano> duration = finish - start;
    double duration_per_loop = double(duration.count()) / loop_size;
    return {duration, duration_per_loop};
}

void generate_test_array(size_t size, size_t step) {
    step /= sizeof(uint32_t);
    std::vector<int> buffer(size);
    std::iota(buffer.begin(), buffer.end(), 0);
    std::shuffle(buffer.begin() + 1, buffer.end(), rng);
    for (size_t i = 0; i < size; ++i) {
        array[buffer[i] * step] = buffer[(i + 1) % size] * step;
    }
}

info_L1 get_cache_size_and_associativity() {
//    fprintf(stderr, "Calculating cache size and associativity\n");
//    fprintf(stderr, "way_size, associativity, total nano sec, nano sec per loop\n");
    std::map<size_t, size_t> counter_for_cache, min_assoc_for_cache;
    std::map<size_t, measured_time> measurements;
    for (size_t way_size = MIN_WAY_SIZE; way_size < MAX_WAY_SIZE; way_size *= 2) {
        size_t prev_assoc = MIN_ASSOC;
        for (size_t assoc = MIN_ASSOC; assoc < MAX_ASSOC; assoc += 2) {
            if (assoc * way_size < sizeof(uint32_t) * MAX_SIZE) {
                //generate
                generate_test_array(assoc, way_size);
                //measure
                measurements[assoc] = measure(assoc);
//                fprintf(stderr, "%lu, %lu, %f, %f\n", way_size, assoc, measurements[assoc].total.count(),
//                        measurements[assoc].nano_per_loop);
                if (assoc != MIN_ASSOC &&
                    measurements[assoc].nano_per_loop > TIME_THRESHOLD * measurements[prev_assoc].nano_per_loop) {
                    size_t cache_size = prev_assoc * way_size;
                    min_assoc_for_cache[cache_size] = prev_assoc;
                    counter_for_cache[cache_size]++;
                }
                prev_assoc = assoc;
            } else {
                break;
            }
        }
    }
    size_t max_count = 0;
    size_t cache_size = 0, assoc = 0;
    for (auto [curr_cache_size, count]: counter_for_cache) {
        if (count > max_count || (count == max_count && curr_cache_size < cache_size)) {
            cache_size = curr_cache_size;
            max_count = count;
            assoc = min_assoc_for_cache[cache_size];
        }
    }
    return {.cache_size = cache_size, .associativity = assoc};
}

size_t generate_test_array_for_line(size_t cache_size, size_t associativity, size_t cache_line) {
    size_t indices = cache_size / associativity / cache_line;
    size_t cache_line_uint32 = cache_line / sizeof(uint32_t);
    size_t size = indices * associativity * cache_line_uint32;
    std::vector<uint32_t> buffer(size);
    for (size_t i = 0; i < indices; ++i) {
        for (size_t j = 0; j < associativity; ++j) {
            for (size_t k = 0; k < cache_line_uint32; ++k) {
                size_t index = k + cache_line_uint32 * (j + i * associativity);
                size_t offset = (j + i * associativity) * cache_size / associativity;
                buffer[index] = k + (i * cache_line + offset) / sizeof(uint32_t);
            }
        }
    }
    std::shuffle(buffer.begin() + 1, buffer.end(), rng);
    for (size_t i = 0; i < size; ++i) {
        array[buffer[i]] = buffer[(i + 1) % size];
    }
    return size;
}

info_L1 get_cache_line(size_t cache_size, size_t associativity) {
    info_L1 result = {.cache_size = cache_size, .cache_line = 0, .associativity = associativity};
    measured_time prev_time{};
    for (int cache_line = MIN_CACHE_LINE; cache_line <= cache_size / associativity; cache_line *= 2) {
        size_t len = generate_test_array_for_line(cache_size, associativity, cache_line);
        measured_time time = measure(len);
        //fprintf(stderr, "%d, %f, %f\n", cache_line, time.total.count(), time.nano_per_loop);
        if (cache_line != MIN_CACHE_LINE && prev_time.nano_per_loop > TIME_THRESHOLD * time.nano_per_loop) {
            result.cache_line = cache_line * sizeof(uint32_t);
            return result;
        }
        prev_time = time;
    }

    return result;
}

info_L1 get_info_about_L1_memory() {
    std::map<info_L1, size_t> counter;
    fprintf(stderr, "Calculating cache size and associativity\n");
    for (int epoch = 0; epoch < EPOCHS; ++epoch) {
        fprintf(stderr, "Current epoch %d of %zu\n", epoch + 1, EPOCHS);
        info_L1 buffer = get_cache_size_and_associativity();
        counter[buffer]++;
    }
    info_L1 result{};
    size_t max_count = 0;
    for (auto [measure, count]: counter) {
        if (count > max_count) {
            max_count = count;
            result = measure;
        }
    }
    counter.clear();
    fprintf(stderr, "Calculating cache line\n");
    for (int epoch = 0; epoch < EPOCHS; ++epoch) {
        fprintf(stderr, "Current epoch %d of %zu\n", epoch + 1, EPOCHS);
        info_L1 buffer = get_cache_line(result.cache_size, result.associativity);
        counter[buffer]++;
    }
    max_count = 0;
    for (auto [measure, count]: counter) {
        if (count > max_count && measure.cache_line != 0) {
            max_count = count;
            result = measure;
        }
    }
    return result;
}

int main() {
    info_L1 info = get_info_about_L1_memory();
    printf("Cache size: %lu bytes\nCache line: %lu bytes\nAssociativity: %lu\n",
           info.cache_size, info.cache_line, info.associativity);
    return 0;
}
