"""
Helper for the Kernel Image Processing project.

Install once:   pip install pillow

Usage:
  python png_to_c.py make                              -> creates shapes.png and noisy.png
  python png_to_c.py convert photo.png image_photo.h image_photo
                                                       -> creates image_photo.h with a C array
                                                
"""
import sys
import random
from PIL import Image, ImageOps, ImageDraw   # Pillow: image loading / drawing

SIZE = 64   # must match IMG_WIDTH / IMG_HEIGHT in the C code


def make_test_images():
    """Create two synthetic 64x64 test images: simple shapes and a noisy version."""
    # "L" = 8-bit grayscale, 255 = white background
    img = Image.new("L", (SIZE, SIZE), 255)
    d = ImageDraw.Draw(img)
    d.rectangle([8, 8, 30, 30], fill=0)      # black square
    d.ellipse([36, 30, 58, 56], fill=100)    # dark-gray circle
    img.save("shapes.png") 

    # noisy version: add random noise to every pixel, clamp to 0..255
    noisy = img.copy()
    px = noisy.load()
    for y in range(SIZE):
        for x in range(SIZE):
            v = px[x, y] + random.randint(-40, 40)
            px[x, y] = max(0, min(255, v))
    noisy.save("noisy.png")
    print("Created shapes.png and noisy.png")


def to_c_array(in_path, out_path, name):
    """Load any image, make it 64x64 grayscale, write it as a C array in a .h file."""
    img = Image.open(in_path).convert("L")   # convert to grayscale
    img = ImageOps.fit(img, (SIZE, SIZE))    # crop to square, then resize to 64x64
    pixels = list(img.getdata())             # flat list of 4096 values (row by row)

    with open(out_path, "w") as f:
        f.write("#include <stdint.h>\n")
        f.write(f"const uint8_t {name}[{SIZE * SIZE}] = {{\n")
        for row in range(SIZE):
            # one image row per line of text, easier to read
            line = ",".join(str(p) for p in pixels[row * SIZE:(row + 1) * SIZE])
            f.write("    " + line + ",\n")
        f.write("};\n")
    print(f"Wrote {out_path} (array name: {name})")


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else ""
    if cmd == "make":
        make_test_images()
    elif cmd == "convert" and len(sys.argv) == 5:
        to_c_array(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        print(__doc__)