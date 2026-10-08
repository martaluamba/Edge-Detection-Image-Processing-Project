EDGE DETECTION KERNELS IMAGE PROCESSING

Applies image kernels (Sobel X, Sobel Y, Laplacian, Laplacian of Gaussian) to 64x64 grayscale images, with or without a blur pass first on the DTEK-V (RISC-V). The board version is controlled with the switches and the push button, uses polling, and prints the result to the terminal as a text PGM.

FEATURES: 

- Three built-in test images (mug, shapes, noisy).
- Kernels: Sobel X, Sobel Y, Laplacian, Laplacian of Gaussian (LoG).
- Optional 3x3 blur before the kernel (back-to-back).
- Edge counting with a threshold.
- Controlled with switches and a push button, and matching LEDs.
- PC version for testing the algorithms before running on the board.
- Python tools to convert images to C arrays and PGM output to PNG.
- kenerls_board_performance_analysis.c is for the performance analysis of our project.

FOLDER STRUCTURE:

src/board_version/               kernels_board.c, image headers, Makefile, boot code,
                                 linker script and lab libraries
tools/                           img_to_c.py (PNG -> C array), pgm_to_png.py (PGM -> PNG and grid)
images/                          grid of PNG images

REQUIREMENTS:

RISC-V toolchain and the DTEK-V lab tools
Python 3 with Pillow: pip install pillow
Step 1 (optional): generate the image headers
image_mug.h, image_shapes.h and image_noisy.h are already included. 

To regenerate them:

cd tools
python3 img_to_c.py   

To run the main (kernels_board.c): 

cd src/board_version
killall jtagd (optional)
jtagconfig
make 
dtekv-run main.bin 

BOARD CONTROLS:
SW1-SW0	Image: 01 mug, 10 shapes, 11 noisy, 00 none
SW2: Sobel_X
SW3: Sobel_Y
SW4: Laplacian
SW5: LoG
SW6: Blur first (up = blur)
SW8-SW9: Exit condition
Button:	Second push button, one press = one run

RULES:

- Raise exactly one kernel switch (SW2, SW3, SW4, SW5), then press the button.
- Image selected and no kernel switch up: the original image is printed.
- More than one kernel switch up: an error message is printed.
- The LEDs mirror SW0-SW6. LED9 is on while the board is processing.
- After the exit condition, reset and reload to run again.
- Eventually, convert the board output to PNG
- Copy the text printed by the board (from P2 to the last pixel row) into a file, for example mug_LoG_blur.pgm.      Lines before P2 are ignored.

In the folder with the .pgm files, run:

python tools/pgm_to_png_board.py                  # all .pgm files
python tools/pgm_to_png_board.py [pgm_file_name].pgm  # only the listed files
This creates one enlarged .png per .pgm and a grid.png with all the converted images. Each run overwrites grid.png. 

NOTES:

- Edge counting uses a threshold of 50 and ignores a 3-pixel border.
- The board program uses only print() and print_dec() from the lab library.
- handle_interrupt() is an empty stub required by the boot code.

CONTRIBUTIONS:

The project was developed together: kernel selection and design, testing and debugging on the board, the Overleaf document, and GitHub management.

Nzumba Marta Luamba: project design, implementation and testing of the kernels (Sobel Y, LoG), kernels verification code, exit condition, I/O board menu interface (switches, button, LEDs,), testing on the PC and the DTEK-V board.

Marcus Engberg: design of the objectives and requirements, kernels (Sobel X, Laplacian), kernel convolution function and polling (using switches, buttons, and LEDs) based on lab 3.

Parts of the code were written with AI assistance (such as Python tools for download and creating input-images), then reviewed, compiled and tested by us on the PC and the DTEK-V board.