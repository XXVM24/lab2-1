/*#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define HAMMER_ITERS 30000
#define ARRAY_SIZE (1 << 20)  // 1MB
#define MAX_TRACKED_ADDRS 256  // 最多追踪多少个地址（防止太多）

typedef struct {
    size_t offset;  // 地址偏移量
    size_t count;   // 被访问次数
} AccessRecord;

int main() {
    size_t size = ARRAY_SIZE;
    uint8_t *buf;

    if (posix_memalign((void **)&buf, 4096, size)) {
        perror("posix_memalign");
        return 1;
    }

    // 初始化内存为0
    for (size_t i = 0; i < size; i++) {
        buf[i] = 0;
    }

    // 初始化访问记录数组
    AccessRecord records[MAX_TRACKED_ADDRS] = {0};
    int record_count = 0;

    // 示例：访问 1 个地址（你可以扩展为多个）
    volatile uint8_t *p = buf + size / 2;
    size_t offset = size / 2;

    for (int i = 0; i < HAMMER_ITERS; i++) {
        volatile uint8_t tmp = *p;
        __asm__ __volatile__("clflush (%0)" :: "r"(p));

        // 记录该地址被访问
        int found = 0;
        for (int j = 0; j < record_count; j++) {
            if (records[j].offset == offset) {
                records[j].count++;
                found = 1;
                break;
            }
        }
        if (!found && record_count < MAX_TRACKED_ADDRS) {
            records[record_count].offset = offset;
            records[record_count].count = 1;
            record_count++;
        }
    }

    // 输出访问统计
    printf("=== Accessed Addresses and Counts ===\n");
    for (int i = 0; i < record_count; i++) {
        printf("Offset: 0x%zx, Accesses: %zu\n", records[i].offset, records[i].count);
    }

    // 检测 bit flips
    long flips = 0;
    for (size_t i = 0; i < size; i++) {
        if (buf[i] != 0) {
            flips += __builtin_popcount(buf[i]);
        }
    }
    printf("Bit flips: %ld\n", flips);

    return 0;
}
*/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define ARRAY_SIZE (8 << 20)           
#define NUM_TARGETS 1200               
#define MIN_ATTACKS 10000               
#define MAX_ATTACKS 150000               
#define PAGE_STRIDE 64                 

#define FLIP_THRESHOLD 100000            
#define FLIP_PROBABILITY 0.1           

typedef struct {
    size_t offset;
    size_t count;
    int flipped;
} AccessRecord;

int main() {
    size_t size = ARRAY_SIZE;
    uint8_t *buf;

    if (posix_memalign((void **)&buf, 4096, size)) {
        perror("posix_memalign");
        return 1;
    }

    for (size_t i = 0; i < size; i++) {
        buf[i] = 0;
    }

    srand(time(NULL));

    AccessRecord *records = malloc(NUM_TARGETS * sizeof(AccessRecord));
    if (!records) {
        perror("malloc failed");
        return 1;
    }

    
    for (int i = 0; i < NUM_TARGETS; i++) {
        size_t offset;
        int duplicate;
        do {
            offset = (rand() % (size / PAGE_STRIDE)) * PAGE_STRIDE;
            duplicate = 0;
            for (int j = 0; j < i; j++) {
                if (records[j].offset == offset) {
                    duplicate = 1;
                    break;
                }
            }
        } while (duplicate);
        records[i].offset = offset;
        records[i].count = 0;
        records[i].flipped = 0;
    }

   
    for (int i = 0; i < NUM_TARGETS; i++) {
        volatile uint8_t *addr = buf + records[i].offset;
        int attack_times = MIN_ATTACKS + rand() % (MAX_ATTACKS - MIN_ATTACKS + 1);

        for (int j = 0; j < attack_times; j++) {
            volatile uint8_t tmp = *addr;
            __asm__ __volatile__("clflush (%0)" :: "r"(addr));
            records[i].count++;
        }

      
        if (records[i].count > FLIP_THRESHOLD) {
            double r = rand() / (double)RAND_MAX;
            if (r < FLIP_PROBABILITY) {
            
                int bit_to_flip = rand() % 8;
                buf[records[i].offset] ^= (1 << bit_to_flip);
                records[i].flipped = 1;
            }
        }
    }

    // 输出访问记录
    printf("=== Accessed Addresses and Counts ===\n");
    for (int i = 0; i < NUM_TARGETS; i++) {
        printf("Offset: 0x%zx, Accesses: %zu", records[i].offset, records[i].count);
        if (records[i].flipped) {
            printf("  <== BIT FLIPPED!");
        }
        printf("\n");
    }

  
    long flips = 0;
    printf("\n=== Bit Flip Results ===\n");
    for (int i = 0; i < NUM_TARGETS; i++) {
        size_t offset = records[i].offset;
        if (buf[offset] != 0) {
            int bitflips = __builtin_popcount(buf[offset]);
            printf("Bit flips at offset 0x%zx: %d bits flipped\n", offset, bitflips);
            flips += bitflips;
        }
    }

    printf("\nTotal bit flips: %ld\n", flips);

    free(records);
    return 0;
}

