"""
Converts the .pgm output of the C program into PNG files.

Install once:   pip install pillow
Usage:          python3 pgm_to_png.py

Creates:
  - one enlarged .png for every .pgm file (same name)
  - one comparison grid per image: mug_grid.png, shapes_grid.png, noisy_grid.png
"""
import glob
import os
from PIL import Image, ImageDraw       # Pillow: image creation / drawing

KERNELS = ["Sobel_X", "Sobel_Y", "Laplacian", "LoG"]   # rows of the grid
IMAGES = ["mug", "shapes", "noisy"]                    # one grid per image
SCALE = 8      # enlargement for the single PNGs (64x64 is tiny)
TILE = 64 * 4  # size of each tile in the grid
LABEL = 18     # space above each tile for the text label


def read_pgm(path):
    """Read a plain-text PGM (P2) file and return a Pillow grayscale image."""
    with open(path) as f:
        tok = f.read().split()                       # ["P2", width, height, 255, pixel, ...]
    w, h = int(tok[1]), int(tok[2])                  # image size from the header
    pixels = bytes(int(v) for v in tok[4:4 + w * h]) # pixel values -> bytes
    return Image.frombytes("L", (w, h), pixels)      # "L" = 8-bit grayscale


# ---- part 1: one enlarged PNG per .pgm file ----
for name in glob.glob("*.pgm"):
    img = read_pgm(name)
    img = img.resize((img.width * SCALE, img.height * SCALE), Image.NEAREST)  # keep sharp pixels
    out = name[:-4] + ".png"                         # same name, .pgm -> .png
    img.save(out)
    print("saved", out)

# ---- part 2: one comparison grid per image ----
# columns: input | without blur | with blur ; rows: the four kernels
COLUMNS = ["input", "noblur", "blur"]
for im in IMAGES:
    sheet = Image.new("L", (TILE * len(COLUMNS), (TILE + LABEL) * len(KERNELS)), 255)
    d = ImageDraw.Draw(sheet)
    for r, k in enumerate(KERNELS):
        for c, col in enumerate(COLUMNS):
            fname = f"{im}_input.pgm" if col == "input" else f"{im}_{k}_{col}.pgm"
            if not os.path.exists(fname):            # skip if the file is missing
                print("missing", fname)
                continue
            tile = read_pgm(fname).resize((TILE, TILE), Image.NEAREST)
            y = r * (TILE + LABEL)                   # top of this row
            sheet.paste(tile, (c * TILE, y + LABEL))
            label = "input" if col == "input" else f"{k} ({col})"
            d.text((c * TILE + 4, y + 3), label, fill=0)
    sheet.save(f"{im}_grid.png")
    print("saved", f"{im}_grid.png")
