import cv2
import sys
import numpy as np

if len(sys.argv) < 3:
    print("Usage: python script.py <source_image> <binary_file>")
    sys.exit(1)

SOURCE_FILE = sys.argv[1]
BINARY_FILE = sys.argv[2]

# Read image in grayscale
img = cv2.imread(SOURCE_FILE, cv2.IMREAD_GRAYSCALE)
if img is None:
    print(f"Error: Could not read the image file {SOURCE_FILE}")
    sys.exit(1)

print("Original Image shape:", img.shape)

# Resize image
img_re = cv2.resize(img, (96, 96), interpolation=cv2.INTER_LINEAR)
print("Resized Image shape:", img_re.shape)

# Write to binary file
with open(BINARY_FILE, "wb") as f:
    f.write(img_re.tobytes())  # Convert NumPy array to bytes before writing

print(f"Binary file saved: {BINARY_FILE}")

# Optional: Save intermediate images for debugging
cv2.imwrite("resized.png", img_re)
