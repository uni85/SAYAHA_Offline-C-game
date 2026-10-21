import math
import random
from PIL import Image, ImageDraw

# PICO-8 Palette mapping from main.c
PICO8_PALETTE = [
    (0, 0, 0),        # 0: Black
    (29, 43, 83),     # 1: Dark Blue
    (126, 37, 83),    # 2: Dark Purple
    (0, 135, 81),     # 3: Dark Green
    (171, 82, 54),    # 4: Brown
    (95, 87, 79),     # 5: Dark Gray
    (194, 195, 199),  # 6: Light Gray
    (255, 241, 232),  # 7: White
    (255, 0, 77),     # 8: Red
    (255, 163, 0),    # 9: Orange
    (255, 236, 39),   # 10: Yellow
    (0, 228, 54),     # 11: Green
    (41, 173, 255),   # 12: Blue
    (131, 118, 156),  # 13: Lavender
    (255, 119, 168),  # 14: Pink
    (255, 204, 170)   # 15: Peach
]

def create_pixel_canvas(width, height):
    return Image.new("RGB", (width, height), PICO8_PALETTE[0])

def draw_cityscape(draw, width, height, num_buildings=18):
    random.seed(42)  # Deterministic skyline layout
    
    # Stars in the night sky
    for _ in range(60):
        sx = random.randint(0, width - 1)
        sy = random.randint(0, int(height * 0.5))
        color = PICO8_PALETTE[6] if random.random() > 0.3 else PICO8_PALETTE[7]
        draw.point((sx, sy), fill=color)

    # Distant skyline silhouette
    for i in range(num_buildings):
        bw = random.randint(15, 35)
        bh = random.randint(30, 90)
        bx = int(i * (width / num_buildings))
        by = height - bh
        
        # Draw distant building (Dark Blue)
        draw.rectangle([bx, by, bx + bw, height], fill=PICO8_PALETTE[1])
        
        # Lit windows
        for wx in range(bx + 3, bx + bw - 3, 5):
            for wy in range(by + 5, height - 10, 8):
                if random.random() > 0.5:
                    win_color = PICO8_PALETTE[10] if random.random() > 0.3 else PICO8_PALETTE[12]
                    draw.rectangle([wx, wy, wx + 2, wy + 3], fill=win_color)

    # Ground base line
    draw.rectangle([0, height - 8, width, height], fill=PICO8_PALETTE[5])

def render_banner():
    # Native low-res pixel scale (320x160 scaled up x4)
    lw, lh = 320, 160
    img = create_pixel_canvas(lw, lh)
    draw = ImageDraw.Draw(img)

    # 1. Background Cityscape
    draw_cityscape(draw, lw, lh)

    # 2. Shadow Reflection Split
    mid_y = lh - 30
    draw.line([(0, mid_y), (lw, mid_y)], fill=PICO8_PALETTE[14], width=1)
    
    # 3. Silhouette Player Character
    px, py = 50, mid_y - 16
    draw.rectangle([px, py, px + 8, py + 16], fill=PICO8_PALETTE[0])  # Body
    draw.rectangle([px + 2, py - 4, px + 6, py], fill=PICO8_PALETTE[0])  # Head
    
    # Glowing Shadow Reflection
    draw.rectangle([px, mid_y + 2, px + 8, mid_y + 18], fill=PICO8_PALETTE[2])
    draw.rectangle([px + 2, mid_y + 18, px + 6, mid_y + 22], fill=PICO8_PALETTE[14])

    # Scale up using Nearest Neighbor to maintain crisp pixels
    banner = img.resize((1280, 640), Image.NEAREST)
    
    # Overlay Title Box on scaled canvas
    b_draw = ImageDraw.Draw(banner)
    b_draw.rectangle([340, 180, 940, 300], fill=PICO8_PALETTE[0], outline=PICO8_PALETTE[14], width=4)
    b_draw.text((380, 220), "SHADOW TAKEOVER", fill=PICO8_PALETTE[10], font_size=48)
    
    banner.save("shadow_takeover_banner.png")
    print("Generated shadow_takeover_banner.png (1280x640)")

def render_background():
    # Native low-res pixel scale (480x270 scaled up x4)
    lw, lh = 480, 270
    img = create_pixel_canvas(lw, lh)
    draw = ImageDraw.Draw(img)

    # Full Parallax Cityscape
    draw_cityscape(draw, lw, lh, num_buildings=28)

    # Scale up to 1080p using Nearest Neighbor
    background = img.resize((1920, 1080), Image.NEAREST)
    background.save("shadow_takeover_background.png")
    print("Generated shadow_takeover_background.png (1920x1080)")

if __name__ == "__main__":
    render_banner()
    render_background()