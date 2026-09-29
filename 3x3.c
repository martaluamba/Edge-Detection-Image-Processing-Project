#include <stdint.h>
#include <stdlib.h> // for abs()

#define IMG_WIDTH 64
#define IMG_HEIGHT 64

// define kernels as 2D arrays
// 3x3 sobel kernel along the x-axis
const int8_t kernel_sobel_x[3][3] = {
    {-1,  0,  1},
    {-2,  0,  2},
    {-1,  0,  1}
};

// 3x3 laplacian kernel
const int8_t kernel_laplacian[3][3] = {
    { 0,  1,  0},
    { 1, -4,  1},
    { 0,  1,  0}
};

// 3x3 convolution function
void apply_3x3_edge_kernel(const uint8_t* in_buf, uint8_t* out_buf, const int8_t kernel[3][3]) {
    // initialize output buffer to 0, automatically handles the 1 pixel borders
    for (int i = 0; i < IMG_WIDTH * IMG_HEIGHT; i++) {
        out_buf[i] = 0;
    }

    // iterate over the image, skipping the outer 1 pixel border to prevent memory faults
    for (int y = 1; y < IMG_HEIGHT - 1; y++) {
        for (int x = 1; x < IMG_WIDTH - 1; x++) {
            int sum = 0;
            
            // apply the 3x3 kernel to the current pixel neighborhood
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    // calculate 1D index from 2D coordinates
                    int pixel_index = (y + ky) * IMG_WIDTH + (x + kx);
                    int pixel_val = in_buf[pixel_index];
                    int weight = kernel[ky + 1][kx + 1];
                    
                    sum += pixel_val * weight;
                }
            }
            
            // absolute value for edges, then clamp to 8-bit limit
            sum = abs(sum);
            if (sum > 255) {
                sum = 255;
            }
            
            // the final pixel
            out_buf[y * IMG_WIDTH + x] = (uint8_t)sum;
        }
    }
}
