#include <stdint.h>   // fixed-size integer types: uint8_t, int8_t
#include <stdlib.h>   // abs()
#include <stdio.h>    // printf(), fopen(), fprintf(), snprintf()
#include <string.h>   // memcpy()

// ---- all three images are included once (created with the Python script) ----
#include "image_mug.h"      // defines: const uint8_t image_mug[4096]
#include "image_shapes.h"   // defines: const uint8_t image_shapes[4096]
#include "image_noisy.h"    // defines: const uint8_t image_noisy[4096]

// constants for 64 x 64 images
#define IMG_WIDTH  64
#define IMG_HEIGHT 64

// a pixel counts as an "edge pixel" if its value is above this threshold
#define EDGE_THRESHOLD 50

// an image = its name (used in the output file names) + a pointer to its pixels
typedef struct {
    const char    *name;   // prefix used in the output file names
    const uint8_t *data;   // the 4096 pixels
} Image;

// table of images: the program loops over all of them, no manual renaming needed
const Image images[] = {
    {"mug",    image_mug},      // index 0
    {"shapes", image_shapes},   // index 1
    {"noisy",  image_noisy},    // index 2
};
#define NUM_IMAGES (int)(sizeof(images) / sizeof(images[0]))   // number of images = 3

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
                                 0,-1,-2,-1, 0,
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

// Convolution of in_buf with kernel K, result written to out_buf
void apply_kernel(const uint8_t *in_buf, uint8_t *out_buf, const Kernel *K) {
    int r = K->size / 2;   // radius: 3x3 -> 1, 5x5 -> 2

    // set the whole output to 0; this also leaves the borders at 0
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
                    sum += in_buf[(y + ky) * IMG_WIDTH + (x + kx)]
                         * K->k[(ky + r) * K->size + (kx + r)];
                }
            }

            sum /= K->div;              // normalise (blur: average of 9 pixels)
            sum = abs(sum);             // edges can be negative: take magnitude
            if (sum > 255) sum = 255;   // clamp to the 8-bit limit
            out_buf[y * IMG_WIDTH + x] = (uint8_t)sum;   // store final pixel
        }
    }
}

// count pixels above the threshold, ignoring a 3-pixel margin around the image
// (blur leaves a 1px zero ring, and the 5x5 LoG reads 2px around each pixel,
//  so pixels near the border are contaminated by the artificial black frame)
#define MARGIN 3
int count_edges(const uint8_t *buf, int threshold) {
    int n = 0;                                          // counter starts at 0
    for (int y = MARGIN; y < IMG_HEIGHT - MARGIN; y++)  // skip top/bottom margin
        for (int x = MARGIN; x < IMG_WIDTH - MARGIN; x++)   // skip left/right margin
            if (buf[y * IMG_WIDTH + x] > threshold) n++;    // above threshold -> edge
    return n;
}

// write a 64x64 buffer as a text PGM file (P2 = plain-text grayscale)
void write_pgm(const char *filename, const uint8_t *buf) {
    FILE *f = fopen(filename, "w");             // open file for writing
    if (f == NULL) {                            // could not create the file
        printf("Cannot open %s\n", filename);
        return;
    }
    fprintf(f, "P2\n%d %d\n255\n", IMG_WIDTH, IMG_HEIGHT);   // header: format, size, max value
    for (int i = 0; i < IMG_WIDTH * IMG_HEIGHT; i++)
        fprintf(f, "%d\n", buf[i]);             // one pixel value per line
    fclose(f);                                  // close the file
}

static uint8_t bufA[IMG_WIDTH * IMG_HEIGHT];     // working buffer
static uint8_t bufB[IMG_WIDTH * IMG_HEIGHT];     // second buffer (ping-pong)
static uint8_t original[IMG_WIDTH * IMG_HEIGHT]; // untouched copy of the input

int main(void) {
    char filename[96];   // holds the output file name

    // outer loop: every image in the table (mug, shapes, noisy)
    for (int im = 0; im < NUM_IMAGES; im++) {
        const char *tag = images[im].name;   // prefix for this image's output files

        // load the current image into 'original'
        memcpy(original, images[im].data, sizeof(original));

        // write the input too, so it can be viewed next to the results
        snprintf(filename, sizeof(filename), "%s_input.pgm", tag);
        write_pgm(filename, original);

        printf("\nImage: %s, edge threshold: %d\n", tag, EDGE_THRESHOLD);
        printf("%-10s %-8s %s\n", "Kernel", "Blur", "Edge pixels");

        // loop over the four edge kernels (indices 1..4 in the table)
        for (int i = 1; i <= 4; i++) {
            const uint8_t *result;   // will point to the buffer holding the result

            // ---- without blur: one step ----
            memcpy(bufA, original, sizeof(bufA));        // start from the original
            apply_kernel(bufA, bufB, &kernels[i]);       // edge kernel: A -> B
            result = bufB;                               // result is in B
            printf("%-10s %-8s %d\n", kernels[i].name, "no",
                   count_edges(result, EDGE_THRESHOLD));
            snprintf(filename, sizeof(filename), "%s_%s_noblur.pgm", tag, kernels[i].name);
            write_pgm(filename, result);

            // ---- with blur: two steps, blur then edge kernel ----
            memcpy(bufA, original, sizeof(bufA));        // start from the original again
            apply_kernel(bufA, bufB, &kernels[0]);       // blur: A -> B
            apply_kernel(bufB, bufA, &kernels[i]);       // edge kernel: B -> A
            result = bufA;                               // result is in A
            printf("%-10s %-8s %d\n", kernels[i].name, "yes",
                   count_edges(result, EDGE_THRESHOLD));
            snprintf(filename, sizeof(filename), "%s_%s_blur.pgm", tag, kernels[i].name);
            write_pgm(filename, result);
        }
    }

    return 0;
}