#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <time.h>

#define ARRAY_SIZE (64 << 20)            
#define MIN_ATTACKS 9000                 
#define MAX_ATTACKS 20000                 
#define TARGET_ADDR_MIN 10000            
#define PAGE_STRIDE 64                

typedef struct {
    size_t offset;
    size_t count;
} AccessRecord;

int main() {
    size_t size = ARRAY_SIZE;
    uint8_t *buf;

    if (posix_memalign((void **)&buf, 4096, size)) {
        perror("posix_memalign");
        return 1;
    }

    // 初始化为全1， 1 → 0 
    memset(buf, 0xFF, size);
    srand(time(NULL));

   
    size_t max_targets = size / PAGE_STRIDE;
    size_t num_targets = TARGET_ADDR_MIN + rand() % (max_targets - TARGET_ADDR_MIN);

    
    volatile uint8_t **addrs = malloc(num_targets * sizeof(uint8_t *));
    AccessRecord *records = malloc(num_targets * sizeof(AccessRecord));

    if (!addrs || !records) {
        perror("malloc failed");
        return 1;
    }

    
    for (size_t i = 0; i < num_targets; i++) {
        size_t offset = i * PAGE_STRIDE;
        addrs[i] = buf + offset;
        records[i].offset = offset;
        records[i].count = 0;
    }

   
    for (size_t i = 0; i < num_targets; i++) {
        int hammer_count = MIN_ATTACKS + rand() % (MAX_ATTACKS - MIN_ATTACKS + 1);

        for (int j = 0; j < hammer_count; j++) {
            volatile uint8_t tmp = *addrs[i];
            _mm_clflush((void *)addrs[i]);
        }
        records[i].count = hammer_count;
    }

    // 检查 bit flips
    long flips = 0;
    printf("\n=== Bit Flip Results ===\n");
    for (size_t i = 0; i < size; i++) {
        if (buf[i] != 0xFF) {
            int bitflips = __builtin_popcount(buf[i] ^ 0xFF);
            printf("Bit flips at offset 0x%zx: %d bits flipped\n", i, bitflips);
            flips += bitflips;
        }
    }

    printf("\nTotal bit flips: %ld\n", flips);

    free(addrs);
    free(records);
    return 0;
}

