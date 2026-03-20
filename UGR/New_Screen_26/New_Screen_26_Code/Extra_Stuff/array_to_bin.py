import re
import struct

INPUT_FILE = "../SSD1963_LCD/Core/Src/logos/big_ugr_logo.txt"
OUTPUT_FILE = "big_ugr_logo.bin"

with open(INPUT_FILE, "r", encoding="utf-8") as f:
    text = f.read()

# Find all hex values like 0x1234
values = re.findall(r'0x([0-9A-Fa-f]{1,4})', text)

if not values:
    raise RuntimeError("No hex values found in input file.")

with open(OUTPUT_FILE, "wb") as f:
    for v in values:
        value = int(v, 16)
        f.write(struct.pack("<H", value))   # little-endian uint16

print(f"Wrote {len(values)} pixels to {OUTPUT_FILE}")
print(f"Output size: {len(values) * 2} bytes")