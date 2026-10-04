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
import glob    #finds files by pattern (e.g. "*.pgm")
import math    # used for math.ceil (round up)
import sys    # gives access to the command-line arguments (sys.argv) and sys.exit
from PIL import Image, ImageDraw       # Pillow: image creation / drawing

SCALE = 8      # enlargement for the single PNGs, 64 * 8 = 512 pixels per side
TILE = 64 * 4  # size of each tile in the grid,  each image is shown at 256x256 in the grid
LABEL = 18     # space above each tile for the text label
MAX_COLS = 3   # maximum number of columns in the grid


def read_pgm(path): # reads one .pgm file and returns a Pillow image
    """Read a plain-text PGM (P2) file; ignore any text before 'P2'."""
    with open(path) as f: # open the file for reading; it is closed automatically at the end of the block
        tok = f.read().split() # read the whole text and split it into "words" (tokens) at spaces/newlines
    start = tok.index("P2")                          # skips '# ...' lines and other text; position of the "P2" token;
    w, h = int(tok[start + 1]), int(tok[start + 2])  # image size from the header; the two tokens after P2 are width and height
    pixels = bytes(int(v) for v in tok[start + 4 : start + 4 + w * h]) # skip the max value (255), take w*h pixel tokens, convert each to a number, pack them as bytes
    return Image.frombytes("L", (w, h), pixels)      # "L" = 8-bit grayscale; build the image from the raw bytes

# files given on the command line, otherwise every .pgm in the current folder; sorted = alphabetical order
files = sorted(sys.argv[1:]) if len(sys.argv) > 1 else sorted(glob.glob("*.pgm"))
if not files:    # the list is empty: nothing to convert
    sys.exit("No .pgm files found.")    # print the message and stop the script

# ---- part 1: one enlarged PNG per .pgm file ----
images = []   # (label, image) pairs, reused for the grid; collects every converted image for part 2
for name in files:   # repeat for each .pgm file
    img = read_pgm(name)    # load the file as a 64x64 image
    big = img.resize((img.width * SCALE, img.height * SCALE), Image.NEAREST)  # keep sharp pixels # enlarge 8x; NEAREST copies pixels instead of smoothing them
    out = name[:-4] + ".png"       # drop the last 4 characters (".pgm") and add ".png"
    big.save(out)        # write the PNG file to disk
    print("saved", out)  # tell the user which file was created
    images.append((name[:-4], img)) # store the name without extension (used as label) and the original small image

# ---- part 2: one grid with all the images ----
if len(images) > 1:      # a grid only makes sense with 2 or more images
    cols = min(MAX_COLS, len(images))  # number of columns: at most 3, fewer if there are fewer images
    rows = math.ceil(len(images) / cols) # number of rows needed, rounded up (e.g. 7 images / 3 cols = 3 rows)
    sheet = Image.new("L", (TILE * cols, (TILE + LABEL) * rows), 255)  # blank grayscale canvas; 255 = white background
    d = ImageDraw.Draw(sheet)  # drawing object, used to write text on the canvas
    for i, (label, img) in enumerate(images):  # i = position number (0, 1, 2...), label = file name, img = image
        r, c = divmod(i, cols)   # row and column of this tile (i divided by cols: quotient = row, remainder = column)
        y = r * (TILE + LABEL)    # top edge (in pixels) of this tile's row
        sheet.paste(img.resize((TILE, TILE), Image.NEAREST), (c * TILE, y + LABEL))  # enlarge the image to a tile and paste it, leaving the label strip above
        d.text((c * TILE + 4, y + 3), label, fill=0)  # label = file name # write the name in the strip, 4 px from the left; fill=0 = black text
    sheet.save("grid.png")  # write the grid to disk (overwrites the previous grid.png)
    print("saved grid.png")  # tell the user the grid was created
