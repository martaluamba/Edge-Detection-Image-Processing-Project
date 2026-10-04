# Kernels on DTEK-V: Edge Detection with Convolution Kernels

Applies image kernels (Sobel X, Sobel Y, Laplacian, Laplacian of Gaussian) to 64x64 grayscale images, with or without a blur pass first. There is a PC version for testing and a board version for the DTEK-V (RISC-V). The board version is controlled with the switches and the push button, uses polling, and prints the result to the terminal as a text PGM.

## Folder structure

```
src/pc_version/                  kernels.c, kernels_test.c, the three image headers (PC test)
src/board_version/               kernels_board.c, image headers, Makefile, boot code,
                                 linker script and lab libraries
tools/                           img_to_c.py (PNG -> C array), pgm_to_png.py (PGM -> PNG and grid)
images/                          original PNG images
visuals/pgm/                     PGM output (board and PC)
visuals/[name_input]_kernels/    converted PNG images and grids
```

## Requirements

- PC version: `gcc`
- Board version: RISC-V toolchain and the DTEK-V lab tools
- Python 3 with Pillow: `pip install pillow`

## Step 1 (optional): generate the image headers

`image_mug.h`, `image_shapes.h` and `image_noisy.h` are already included. To regenerate them:

```bash
python3 tools/img_to_c.py   
```
## Step 2a: run the PC version

```bash
cd src/pc_version
gcc kernels_test.c -o kernels_test && ./kernels_test
```
This prints a table per image-input and writes 27 `.pgm` files in the current folder.

## Step 2b: compile and run on the board

```bash
cd src/board_version
killall jtagd (optional)
jtagconfig
make 
dtekv-run main.bin  
```
## Board controls

| Control | Function |
|---|---|
| SW1-SW0 | Image: `01` mug, `10` shapes, `11` noisy, `00` none |
| SW2 | Sobel_X |
| SW3 | Sobel_Y |
| SW4 | Laplacian |
| SW5 | LoG |
| SW6 | Blur first (up = blur) |
| SW8 + SW9 | Both up: stop the program |
| Button | Second push button, one press = one run |

Rules:

- Raise exactly one kernel switch (SW2, SW3, SW4, SW5), then press the button.
- Image selected and no kernel switch up: the original image is printed.
- More than one kernel switch up: an error message is printed.
- The LEDs mirror SW0-SW6. LED9 is on while the board is processing.
- After stopping, reset and reload to run again.

## Step 3: convert the board output to PNG

1. Copy the text printed by the board (from `P2` to the last pixel row) into a file, for example `mug_LoG_blur.pgm`. Lines before `P2` are ignored.
2. Use one file per result.
3. In the folder with the `.pgm` files, run:

```bash
python tools/pgm_to_png_board.py            # all .pgm files
python tools/pgm_to_png_board.py mug_*.pgm  # only the listed files
```

This creates one enlarged `.png` per `.pgm` and a `grid.png` with all the converted images. Each run overwrites `grid.png`.
In the PC version, pgm_to_png_PC.py, the code shows fixed names for all the 27 outputs and print three grid image per each input-image.

## Notes

- Edge counting uses a threshold of 50 and ignores a 3-pixel border.
- The board program uses no printf, memcpy or abs: it uses only `print()` and `print_dec()` from the lab library.
- `handle_interrupt()` is an empty stub required by the boot code.

## Contributions
Nzumba Marta Luamba: project design, kernels (Sobel Y, LoG), verification code,
I/O board interface (switches, button, LEDs, exit condition), Python tools, testing
on the PC and the DTEK-V board. Parts of the code were written with AI assistance (Claude); 
I reviewed, compiled and tested them.

Marcus Engberg: design of the objectives and requirements, kernels (Sobel X, Laplacian), kernel convolution function and pooling (using switches, buttons, and LEDs).
