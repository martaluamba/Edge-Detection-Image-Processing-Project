"""
Converts the .pgm files printed by the board into PNG files.

Install once:   pip install pillow

Usage:
  python3 pgm_to_png_board.py                   # every .pgm in the current folder
  python3 pgm_to_png_board.py a.pgm b.pgm ...   # only the files you list

Creates:
  - one enlarged .png for every .pgm file (same name)
  - one grid.png with all the converted images together (if there are 2 or more)
"""
import glob
import math
import sys
from PIL import Image, ImageDraw       # Pillow: image creation / drawing

SCALE = 8      # enlargement for the single PNGs (64x64 is tiny)
TILE = 64 * 4  # size of each tile in the grid
LABEL = 18     # space above each tile for the text label
MAX_COLS = 3   # maximum number of columns in the grid


def read_pgm(path):
    """Read a plain-text PGM (P2) file; ignore any text before 'P2'."""
    with open(path) as f:
        tok = f.read().split()
    start = tok.index("P2")                          # skips '# ...' lines and other text
    w, h = int(tok[start + 1]), int(tok[start + 2])  # image size from the header
    pixels = bytes(int(v) for v in tok[start + 4 : start + 4 + w * h])
    return Image.frombytes("L", (w, h), pixels)      # "L" = 8-bit grayscale


files = sorted(sys.argv[1:]) if len(sys.argv) > 1 else sorted(glob.glob("*.pgm"))
if not files:
    sys.exit("No .pgm files found.")

# ---- part 1: one enlarged PNG per .pgm file ----
images = []   # (label, image) pairs, reused for the grid
for name in files:
    img = read_pgm(name)
    big = img.resize((img.width * SCALE, img.height * SCALE), Image.NEAREST)  # keep sharp pixels
    out = name[:-4] + ".png"                         # same name, .pgm -> .png
    big.save(out)
    print("saved", out)
    images.append((name[:-4], img))

# ---- part 2: one grid with all the images ----
if len(images) > 1:
    cols = min(MAX_COLS, len(images))
    rows = math.ceil(len(images) / cols)
    sheet = Image.new("L", (TILE * cols, (TILE + LABEL) * rows), 255)
    d = ImageDraw.Draw(sheet)
    for i, (label, img) in enumerate(images):
        r, c = divmod(i, cols)
        y = r * (TILE + LABEL)
        sheet.paste(img.resize((TILE, TILE), Image.NEAREST), (c * TILE, y + LABEL))
        d.text((c * TILE + 4, y + 3), label, fill=0)  # label = file name
    sheet.save("grid.png")
    print("saved grid.png")