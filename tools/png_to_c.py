"""
Helper for the Kernel Image Processing project.

Install once:   pip install pillow

Usage:
  python png_to_c.py make                              -> creates shapes.png and noisy.png
  python png_to_c.py convert photo.png image_photo.h image_photo
                                                       -> creates image_photo.h with a C array
                                                
"""
import sys        # command-line arguments (sys.argv)
import random      # random numbers, used to create the noise
from PIL import Image, ImageOps, ImageDraw   # Pillow: image loading / drawing

SIZE = 64   # must match IMG_WIDTH / IMG_HEIGHT in the C code # side length of the square images in pixels


def make_test_images():  # creates the two synthetic test images
    """Create two synthetic 64x64 test images: simple shapes and a noisy version."""
    # "L" = 8-bit grayscale, 255 = white background
    img = Image.new("L", (SIZE, SIZE), 255)  # new 64x64 grayscale image, filled with white
    d = ImageDraw.Draw(img)        # drawing object: lets us draw shapes on img
    d.rectangle([8, 8, 30, 30], fill=0)      # black square  # corners (x0,y0) = (8,8) and (x1,y1) = (30,30); 0 = black
    d.ellipse([36, 30, 58, 56], fill=100)    # dark-gray circle   # ellipse inside the box (36,30)-(58,56); 100 = dark gray
    img.save("shapes.png") # write the shapes image to disk

    # noisy version: add random noise to every pixel, clamp to 0..255
    noisy = img.copy()  # independent copy, so the original shapes image is not modified
    px = noisy.load()    # pixel access object: px[x, y] reads/writes a pixel value
    for y in range(SIZE):  # every row (0..63)
        for x in range(SIZE):  # every column (0..63)
            v = px[x, y] + random.randint(-40, 40)   # original value plus a random integer between -40 and +40
            px[x, y] = max(0, min(255, v))  # clamp to the valid range: min limits to 255, max limits to 0
    noisy.save("noisy.png")    # write the noisy image to disk
    print("Created shapes.png and noisy.png")     # tell the user the files were created


def to_c_array(in_path, out_path, name):      # converts any image file into a C header with an array
    """Load any image, make it 64x64 grayscale, write it as a C array in a .h file."""
    img = Image.open(in_path).convert("L")   # convert to grayscale # open the file; "L" drops the colour
    img = ImageOps.fit(img, (SIZE, SIZE))    # crop to square, then resize to 64x64 # keeps proportions, cuts the excess
    pixels = list(img.getdata())             # flat list of 4096 values (row by row) # one number (0..255) per pixel

    with open(out_path, "w") as f:  # open the .h file for writing; closed automatically after the block
        f.write("#include <stdint.h>\n")  # first line of the header: needed for the type uint8_t
        f.write(f"const uint8_t {name}[{SIZE * SIZE}] = {{\n")  # start of the array: "const uint8_t name[4096] = {" ({{ prints a single {)
        for row in range(SIZE):        # one iteration per image row (64 rows)
            # one image row per line of text, easier to read
            line = ",".join(str(p) for p in pixels[row * SIZE:(row + 1) * SIZE])  # take this row's 64 pixels, convert to text, join with commas
            f.write("    " + line + ",\n")   # write the row indented; the trailing comma separates it from the next row
        f.write("};\n")        # close the array and the C statement
    print(f"Wrote {out_path} (array name: {name})")      # tell the user which file was written


if __name__ == "__main__": # true only when the file is run directly (not when imported)
    cmd = sys.argv[1] if len(sys.argv) > 1 else ""      # first argument after the script name; empty string if none
    if cmd == "make":    # "python png_to_c.py make"    
        make_test_images()      # create shapes.png and noisy.png
    elif cmd == "convert" and len(sys.argv) == 5:    # "convert" plus exactly 3 more arguments (script name is argv[0])
        to_c_array(sys.argv[2], sys.argv[3], sys.argv[4]) # input image, output .h file, C array name
    else:    # missing or wrong arguments
        print(__doc__)  # show the usage text at the top of the file
