#include <stdint.h>   // fixed-size integer types: uint8_t, int8_t
#include <stdlib.h>   // abs()
#include <stdio.h>    // printf()
#include <string.h>   // memcpy()

// constants for 64 x 64 images
#define IMG_WIDTH  64
#define IMG_HEIGHT 64

// description of a kernel (filter)
typedef struct {
    const char   *name; // text label, e.g. "Blur"
    const int8_t *k;    // pointer to the weights (signed: -128..127)
    int size;           // side length: 5 for LoG, 3 for the rest
    int div;            // divisor: 9 for blur (average), 1 for the rest
} Kernel;

// weights stored row by row: 9 = 3x3, 25 = 5x5
const int8_t k_sobel_x[9]   = { -1,0,1,  -2,0,2,  -1,0,1 };   // vertical edges
const int8_t k_sobel_y[9]   = { -1,-2,-1,  0,0,0,  1,2,1 };   // horizontal edges
const int8_t k_laplacian[9] = { 0,1,0,  1,-4,1,  0,1,0 };     // second derivative
const int8_t k_blur[9]      = { 1,1,1,  1,1,1,  1,1,1 };      // sum of 9 pixels, /9 = mean
const int8_t k_log[25]      = {  0, 0,-1, 0, 0,               // Laplacian of Gaussian
                                 0,-1,-2,-1, 0,               // (weights add up to 0)
                                -1,-2,16,-2,-1,
                                 0,-1,-2,-1, 0,
                                 0, 0,-1, 0, 0 };

// table of kernels: name, weights, size, divisor
const Kernel kernels[] = {
    {"Blur",      k_blur,      3, 9},   // index 0
    {"Sobel_X",   k_sobel_x,   3, 1},   // index 1
    {"Sobel_Y",   k_sobel_y,   3, 1},   // index 2
    {"Laplacian", k_laplacian, 3, 1},   // index 3
    {"LoG",       k_log,       5, 1},   // index 4
};

// Convolution: uint8_t = unsigned 0 (black)..255 (white), 1 byte per pixel;
// weights are int8_t (signed) because they can be negative.
void apply_kernel(const uint8_t *in_buf, uint8_t *out_buf, const Kernel *K) {
    int r = K->size / 2;   // radius: 3x3 -> 1, 5x5 -> 2

    // set the whole output to 0; this also leaves the border pixels (r wide) at 0
    for (int i = 0; i < IMG_WIDTH * IMG_HEIGHT; i++) {
        out_buf[i] = 0;
    }

    // visit only pixels whose whole neighbourhood is inside the image
    for (int y = r; y < IMG_HEIGHT - r; y++) {
        for (int x = r; x < IMG_WIDTH - r; x++) {

            int sum = 0;   // int: sums can exceed 255 or go negative

            // slide over the neighbourhood, offsets -r..+r
            for (int ky = -r; ky <= r; ky++) {
                for (int kx = -r; kx <= r; kx++) {
                    // pixel (row y+ky, col x+kx) in the flat 1D array
                    // times its weight (row ky+r, col kx+r) in the kernel
                    sum += in_buf[(y + ky) * IMG_WIDTH + (x + kx)]
                         * K->k[(ky + r) * K->size + (kx + r)];
                }
            }

            // still inside the x loop, so `sum` exists here
            sum /= K->div;        // normalise (blur: average of 9 pixels)
            sum = abs(sum);       // edges can be negative: take magnitude
            if (sum > 255) sum = 255;   // clamp to the 8-bit limit

            out_buf[y * IMG_WIDTH + x] = (uint8_t)sum;   // store final pixel
        }
    }
}

static uint8_t bufA[IMG_WIDTH * IMG_HEIGHT];   // working buffer / uploaded image
static uint8_t bufB[IMG_WIDTH * IMG_HEIGHT];   // second buffer (ping-pong)
static uint8_t original[IMG_WIDTH * IMG_HEIGHT]; // untouched copy of the input

int main(void) {
    // test image: left half black, right half white (one vertical edge)
    for (int y = 0; y < IMG_HEIGHT; y++)
        for (int x = 0; x < IMG_WIDTH; x++)
            original[y * IMG_WIDTH + x] = (x < IMG_WIDTH / 2) ? 0 : 255;
/* PRINT TEST THE RESULTS (with and without the blur)*/
    // Pipeline 1: blur then Sobel X
    memcpy(bufA, original, sizeof(bufA));        // load image into A
    apply_kernel(bufA, bufB, &kernels[0]);       // blur:    A -> B 
    apply_kernel(bufB, bufA, &kernels[1]);       // Sobel_X: B -> A
    printf("%s result, pixel(32,32) = %d\n", kernels[1].name,
           bufA[32 * IMG_WIDTH + 32]);
    
     // Pipeline 2: blur then Sobel Y
    memcpy(bufA, original, sizeof(bufA));        // load image into A
    apply_kernel(bufA, bufB, &kernels[0]);       // blur:    A -> B
    apply_kernel(bufB, bufA, &kernels[2]);       // Sobel_Y: B -> A
    printf("%s result, pixel(32,32) = %d\n", kernels[2].name,
           bufA[32 * IMG_WIDTH + 32]);

    // Pipeline 3: blur then Sobel Y
    memcpy(bufA, original, sizeof(bufA));        // load image into A
    apply_kernel(bufA, bufB, &kernels[0]);       // blur:    A -> B
    apply_kernel(bufB, bufA, &kernels[3]);       // Laplacian B -> A
    printf("%s result, pixel(32,32) = %d\n", kernels[3].name,
           bufA[32 * IMG_WIDTH + 32]);

    // Pipeline 4: blur then LoG (reload the original first!)
    memcpy(bufA, original, sizeof(bufA));
    apply_kernel(bufA, bufB, &kernels[0]);       // blur: A -> B
    apply_kernel(bufB, bufA, &kernels[4]);       // LoG:  B -> A
    printf("%s result, pixel(32,32) = %d\n", kernels[4].name,
           bufA[32 * IMG_WIDTH + 32]);

//Pipelines without blur: Sobel X
    memcpy(bufA, original, sizeof(bufA));        // load image into A
    apply_kernel(bufA, bufB, &kernels[1]);       // Sobel X:    A -> B 
    printf("%s result, pixel(32,32) = %d\n", kernels[1].name,
           bufB[32 * IMG_WIDTH + 32]);
    
     // Pipeline 2: Sobel Y
    memcpy(bufA, original, sizeof(bufA));        // load image into A
    apply_kernel(bufA, bufB, &kernels[2]);       // Sobel Y:    A -> B
    printf("%s result, pixel(32,32) = %d\n", kernels[2].name,
           bufB[32 * IMG_WIDTH + 32]);

    // Pipeline 3: Laplacian
    memcpy(bufA, original, sizeof(bufA));        // load image into A
    apply_kernel(bufA, bufB, &kernels[3]);       // Laplacian:    A -> B
    printf("%s result, pixel(32,32) = %d\n", kernels[3].name,
           bufB[32 * IMG_WIDTH + 32]);

    // Pipeline 4: LoG 
    memcpy(bufA, original, sizeof(bufA));
    apply_kernel(bufA, bufB, &kernels[4]);       // LoG: A -> B
    printf("%s result, pixel(32,32) = %d\n", kernels[4].name,
           bufB[32 * IMG_WIDTH + 32]);

    return 0;
}
