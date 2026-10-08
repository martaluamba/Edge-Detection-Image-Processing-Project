// Board version (DTEK-V): switches choose image / kernel / blur, a button runs it.
// Uses only the lab-library functions print() and print_dec(); no printf, memcpy or abs.
// commands: jtagconfig / make / dtekv-run main.bin / (optional killall jtagd)

#include <stdint.h>

#include "image_mug.h"      // const uint8_t image_mug[4096]
#include "image_shapes.h"   // const uint8_t image_shapes[4096]
#include "image_noisy.h"    // const uint8_t image_noisy[4096]

// lab-library output functions (same as in lab 3)
extern void print(const char *);
extern void print_dec(unsigned int);

#define IMG_WIDTH  64
#define IMG_HEIGHT 64
#define IMG_SIZE   (IMG_WIDTH * IMG_HEIGHT)
#define EDGE_THRESHOLD 50
#define MARGIN 3

void handle_interrupt(unsigned cause) {

}
// ---------------- board I/O (addresses from lab 3) ----------------
void set_leds(int led_mask) {
    volatile unsigned int * const led_ptr = (unsigned int *) 0x04000000;
    *led_ptr = led_mask;
}
int get_sw(void) {
    volatile unsigned int * const sw_ptr = (unsigned int *) 0x04000010;
    return (*sw_ptr) & 0x3FF;      // 10 switches
}
int get_btn(void) {
    volatile unsigned int * const btn_ptr = (unsigned int *) 0x040000d0;
    return (*btn_ptr) & 0x1;       // second push button
}

// ---------------- images ----------------
typedef struct {
    const char    *name;
    const uint8_t *data;
} Image;

const Image images[] = {
    {"mug",    image_mug},      // SW1..SW0 = 0
    {"shapes", image_shapes},   // SW1..SW0 = 1
    {"noisy",  image_noisy},    // SW1..SW0 = 2
};

// ---------------- kernels ----------------
typedef struct {
    const char   *name;
    const int8_t *k;
    int size;   // 3 or 5
    int div;
} Kernel;

const int8_t k_sobel_x[9]   = { -1,0,1,  -2,0,2,  -1,0,1 };
const int8_t k_sobel_y[9]   = { -1,-2,-1,  0,0,0,  1,2,1 };
const int8_t k_laplacian[9] = { 0,1,0,  1,-4,1,  0,1,0 };
const int8_t k_blur[9]      = { 1,1,1,  1,1,1,  1,1,1 };
const int8_t k_log[25]      = {  0, 0,-1, 0, 0,
                                 0,-1,-2,-1, 0,
                                -1,-2,16,-2,-1,
                                 0,-1,-2,-1, 0,
                                 0, 0,-1, 0, 0 };

const Kernel kernels[] = {
    {"Blur",      k_blur,      3, 9},   // 0
    {"Sobel_X",   k_sobel_x,   3, 1},   // 1
    {"Sobel_Y",   k_sobel_y,   3, 1},   // 2
    {"Laplacian", k_laplacian, 3, 1},   // 3
    {"LoG",       k_log,       5, 1},   // 4
};

void apply_kernel(const uint8_t *in_buf, uint8_t *out_buf, const Kernel *K) {
    int r = K->size / 2;
    for (int i = 0; i < IMG_SIZE; i++) out_buf[i] = 0;

    for (int y = r; y < IMG_HEIGHT - r; y++) {
        for (int x = r; x < IMG_WIDTH - r; x++) {
            int sum = 0;
            for (int ky = -r; ky <= r; ky++)
                for (int kx = -r; kx <= r; kx++)
                    sum += in_buf[(y + ky) * IMG_WIDTH + (x + kx)]
                         * K->k[(ky + r) * K->size + (kx + r)];
            sum /= K->div;
            if (sum < 0) sum = -sum;        // own abs(): no libc needed
            if (sum > 255) sum = 255;
            out_buf[y * IMG_WIDTH + x] = (uint8_t)sum;
        }
    }
}

int count_edges(const uint8_t *buf, int threshold) {
    int n = 0;
    for (int y = MARGIN; y < IMG_HEIGHT - MARGIN; y++)
        for (int x = MARGIN; x < IMG_WIDTH - MARGIN; x++)
            if (buf[y * IMG_WIDTH + x] > threshold) n++;
    return n;
}

static uint8_t bufA[IMG_SIZE];
static uint8_t bufB[IMG_SIZE];

// returns the buffer that holds the result
const uint8_t *process(const Image *img, int kernel_id, int use_blur) {
    for (int i = 0; i < IMG_SIZE; i++) bufA[i] = img->data[i];   // own memcpy
    if (use_blur) {
        apply_kernel(bufA, bufB, &kernels[0]);           // blur: A -> B
        apply_kernel(bufB, bufA, &kernels[kernel_id]);   // edge: B -> A
        return bufA;
    }
    apply_kernel(bufA, bufB, &kernels[kernel_id]);       // edge: A -> B
    return bufB;
}

// "download": print the result as a text PGM, one image row per line
void print_pgm(const uint8_t *buf) {
    print("P2\n64 64\n255\n");
    for (int y = 0; y < IMG_HEIGHT; y++) {
        for (int x = 0; x < IMG_WIDTH; x++) {
            print_dec(buf[y * IMG_WIDTH + x]);
            print(" ");
        }
        print("\n");
    }
}

typedef struct {
    uint32_t cycles, instrs, mem_instr, icache_miss, dcache_miss,
             icache_stall, dcache_stall, hazard_stall, alu_stall;
    } 
Counters;

static Counters cnt;   // filled by read_counters()

static inline void clear_counters(void) {
    __asm__ volatile ("csrw mcycle, zero");
    __asm__ volatile ("csrw minstret, zero");
    __asm__ volatile ("csrw mhpmcounter3, zero");
    __asm__ volatile ("csrw mhpmcounter4, zero");
    __asm__ volatile ("csrw mhpmcounter5, zero");
    __asm__ volatile ("csrw mhpmcounter6, zero");
    __asm__ volatile ("csrw mhpmcounter7, zero");
    __asm__ volatile ("csrw mhpmcounter8, zero");
    __asm__ volatile ("csrw mhpmcounter9, zero");
    }

static inline void read_counters(void) {
    __asm__ volatile ("csrr %0, mcycle"        : "=r"(cnt.cycles));
    __asm__ volatile ("csrr %0, minstret"      : "=r"(cnt.instrs));
    __asm__ volatile ("csrr %0, mhpmcounter3"  : "=r"(cnt.mem_instr));
    __asm__ volatile ("csrr %0, mhpmcounter4"  : "=r"(cnt.icache_miss));
    __asm__ volatile ("csrr %0, mhpmcounter5"  : "=r"(cnt.dcache_miss));
    __asm__ volatile ("csrr %0, mhpmcounter6"  : "=r"(cnt.icache_stall));
    __asm__ volatile ("csrr %0, mhpmcounter7"  : "=r"(cnt.dcache_stall));
    __asm__ volatile ("csrr %0, mhpmcounter8"  : "=r"(cnt.hazard_stall));
    __asm__ volatile ("csrr %0, mhpmcounter9"  : "=r"(cnt.alu_stall));
    }

int main(void) {
    int last_btn = 0;   // previous button state: one press = one run

    print("Ready. SW1-0: image (01 mug, 10 shapes, 11 noisy).\n");
    print("SW2: Sobel_X, SW3: Sobel_Y, SW4: Laplacian, SW5: LoG. SW6:blur first.\n");
    print("Raise ONE kernel switch, SW6 blur, then press button to run.\n");

    while (1) {
        int sw = get_sw();
        int img_sel = sw & 0x3; //lsb: SW1-SW0: 0=nothing, mug=01, shapes=10, noisy=11
        int k_sel = (sw >> 2) & 0xF;           // 4 kernel switches SW2-SW5
        int use_blur  = (sw >> 6) & 0x1;   // SW6: blur first (back-to-back)

        int img_id = img_sel - 1; // 01->0 mug, 10->1 shapes, 11->noisy, 00->-1 none

        int kernel_id = 0; // 0= none/invalid
        if      (k_sel == 1) kernel_id = 1; //SW2 only: Sobel_X
        else if (k_sel == 2) kernel_id = 2; //SW3 only: Sobel_Y
        else if (k_sel == 4) kernel_id = 3;//SW4 only: Laplacian
        else if (k_sel == 8) kernel_id = 4;//SW5 only: LoG

        set_leds(sw & 0x7F);   // show the current selection on the LEDs
        int measure = (sw >> 7) & 0x1;   // SW7: measure mode
        
        int btn = get_btn();
        if (btn && !last_btn) { //rising edge button just pressed
            if(img_id < 0){
                print("No image selected (SW1-0 = 00)\n");
                }
            else if(k_sel == 0){ //no switch up: print the original output
                print("# ");
                print(images[img_id].name);
                print(", input\n");
                print_pgm (images[img_id].data);
                }
            else if(kernel_id == 0){
                print("Select only one kernel among SW2, SW3, SW4, SW5\n");
                }
            else{
                set_leds((sw & 0xFF) | 0x200);   // LED9 on = busy
                process(&images[img_id], kernel_id, use_blur);
                clear_counters();
                const uint8_t *res = process(&images[img_id], kernel_id, use_blur);
                read_counters();                 // read BEFORE any printing

                if (measure) {
                    // one CSV line: easy to paste into Excel
                    print("# ");
                    print(images[img_id].name);   print(",");
                    print(kernels[kernel_id].name); print(",");
                    print(use_blur ? "blur," : "noblur,");
                    print_dec(cnt.cycles);        print(",");
                    print_dec(cnt.instrs);        print(",");
                    print_dec(cnt.mem_instr);     print(",");
                    print_dec(cnt.icache_miss);   print(",");
                    print_dec(cnt.dcache_miss);   print(",");
                    print_dec(cnt.icache_stall);  print(",");
                    print_dec(cnt.dcache_stall);  print(",");
                    print_dec(cnt.hazard_stall);  print(",");
                    print_dec(cnt.alu_stall);     print("\n");
                    } 
                else {
                        print("# ");
                        print(images[img_id].name);
                        print(", ");
                        print(kernels[kernel_id].name);
                        print(use_blur ? ", blur, edges=" : ", no blur, edges=");
                        print_dec((unsigned int)count_edges(res, EDGE_THRESHOLD));
                        print("\n");
                        print_pgm(res);
                    }
                    set_leds(sw & 0xFF);
                }
       
            }
            last_btn = btn;
    }
    return 0;
}
