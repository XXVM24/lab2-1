#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <x86intrin.h>

#define ARRAY_SIZE (8 << 20)           
#define NUM_TARGETS 400                
#define MIN_ATTACKS 10000             
#define MAX_ATTACKS 150000            
#define ROW_STRIDE 4096                
#define FLIP_THRESHOLD 100000         
#define FLIP_PROBABILITY 0.1          

typedef struct {
    size_t victim_offset;
    size_t agg1_offset;
    size_t agg2_offset;
    size_t count;
    int flipped;
} HammerRecord;

int main() {
    size_t size = ARRAY_SIZE;
    uint8_t *buf;

    if (posix_memalign((void **)&buf, 4096, size)) {
        perror("posix_memalign");
        return 1;
    }

    memset(buf, 0, size);

    srand(time(NULL));

    HammerRecord *records = malloc(NUM_TARGETS * sizeof(HammerRecord));
    if (!records) {
        perror("malloc failed");
        return 1;
    }

    // 初始化地址组
    for (int i = 0; i < NUM_TARGETS; i++) {
        size_t victim_offset;
        int duplicate;
        do {
            victim_offset = (rand() % ((size - 2 * ROW_STRIDE) / ROW_STRIDE)) * ROW_STRIDE + ROW_STRIDE;
            duplicate = 0;
            for (int j = 0; j < i; j++) {
                if (records[j].victim_offset == victim_offset) {
                    duplicate = 1;
                    break;
                }
            }
        } while (duplicate);

        records[i].victim_offset = victim_offset;
        records[i].agg1_offset = victim_offset - ROW_STRIDE;
        records[i].agg2_offset = victim_offset + ROW_STRIDE;
        records[i].count = 0;
        records[i].flipped = 0;
    }

  
    for (int i = 0; i < NUM_TARGETS; i++) {
        volatile uint8_t *addr1 = buf + records[i].agg1_offset;
        volatile uint8_t *addr2 = buf + records[i].agg2_offset;
        int attack_times = MIN_ATTACKS + rand() % (MAX_ATTACKS - MIN_ATTACKS + 1);

        for (int j = 0; j < attack_times; j++) {
            *addr1;
            *addr2;
            __asm__ __volatile__("clflush (%0)" :: "r"(addr1));
            __asm__ __volatile__("clflush (%0)" :: "r"(addr2));
            records[i].count++;
        }

        if (records[i].count > FLIP_THRESHOLD) {
            double r = rand() / (double)RAND_MAX;
            if (r < FLIP_PROBABILITY) {
                int bit_to_flip = rand() % 8;
                buf[records[i].victim_offset] ^= (1 << bit_to_flip);
                records[i].flipped = 1;
            }
        }
    }

 
    printf("=== Hammered Victim Rows ===\n");
    for (int i = 0; i < NUM_TARGETS; i++) {
        printf("Victim Offset: 0x%zx (Agg1: 0x%zx, Agg2: 0x%zx), Accesses: %zu",
               records[i].victim_offset, records[i].agg1_offset, records[i].agg2_offset, records[i].count);
        if (records[i].flipped) {
            printf("  <== BIT FLIPPED!");
        }
        printf("\n");
    }

    // 检查 bit flip
    long flips = 0;
    printf("\n=== Bit Flip Results ===\n");
    for (int i = 0; i < NUM_TARGETS; i++) {
        size_t offset = records[i].victim_offset;
        if (buf[offset] != 0) {
            int bitflips = __builtin_popcount(buf[offset]);
            printf("Bit flips at victim 0x%zx: %d bits flipped\n", offset, bitflips);
            flips += bitflips;
        }
    }

    printf("\nTotal bit flips: %ld\n", flips);

    free(records);
    return 0;
}

