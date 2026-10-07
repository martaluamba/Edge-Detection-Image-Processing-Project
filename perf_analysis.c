#include <stdint.h>

// read total clock cycles
static inline uint32_t read_cycles(void) {
    uint32_t cycles;
    asm volatile ("csrr %0, cycle" : "=r" (cycles));
    return cycles;
}

// read total instructions retired
static inline uint32_t read_instret(void) {
    uint32_t insts;
    asm volatile ("csrr %0, instret" : "=r" (insts));
    return insts;
}


static inline uint32_t read_cache_stalls(void) {
    uint32_t stalls;
    asm volatile ("csrr %0, mhpmcounter3" : "=r" (stalls)); // Example CSR
    return stalls;
}
