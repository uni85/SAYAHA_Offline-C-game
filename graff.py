from PIL import Image, ImageDraw, ImageFont

# Create a small 64x64 transparent image for high clarity at small sizes
img = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)

# Draw a bold, stylized graffiti-like block letter 'S'
# (Using built-in default or drawing polygon coordinates for zero-dependency font issues)
draw.polygon([(16, 12), (48, 12), (48, 24), (28, 24), (28, 32), (48, 32), 
              (48, 52), (16, 52), (16, 40), (36, 40), (36, 32), (16, 32)], 
             fill="#701C1C")

# Save directly as an icon with multi-resolutions to ensure tiny file size (~2-4 KB)
img.save("icon.ico", format="ICO", sizes=[(16, 16), (32, 32), (48, 48)])