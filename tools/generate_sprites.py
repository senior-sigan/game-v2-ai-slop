#!/usr/bin/env python3
"""Generate pixel-art sprite sheets for Dark Camp (Diablo 2-like prototype).

Character sprites are exported as horizontal sprite sheets:
  - idle:   4 frames  → width = 64,  height = 16
  - run:    6 frames  → width = 96,  height = 16
  - attack: 4 frames  → width = 64,  height = 16

Effects:
  - campfire:   8 frames → width = 128, height = 16
  - projectile: 4 frames → width = 32,  height = 8

Tiles (32x16) and decor (16x16, 8x8) remain single images.
"""

import math
import os
import random

from PIL import Image

ASSETS_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "assets")


# ===========================================================================
# Utility
# ===========================================================================

def put(img, x, y, color):
    """Put a pixel if within bounds."""
    if 0 <= x < img.width and 0 <= y < img.height:
        img.putpixel((x, y), color)


def fill_rect(img, x0, y0, w, h, color):
    for dy in range(h):
        for dx in range(w):
            put(img, x0 + dx, y0 + dy, color)


def make_sheet(frame_w, frame_h, num_frames, draw_fn):
    """Create a horizontal sprite sheet by calling draw_fn(img, ox, frame_idx)."""
    img = Image.new("RGBA", (frame_w * num_frames, frame_h), (0, 0, 0, 0))
    for i in range(num_frames):
        draw_fn(img, i * frame_w, i)
    return img


def save(img, name):
    img.save(os.path.join(ASSETS_DIR, name))
    print(f"  {name}  ({img.width}x{img.height})")


# ===========================================================================
# PLAYER sprite sheets
# ===========================================================================

# Colors
P_HEAD = (220, 190, 150, 255)
P_HAIR = (80, 50, 20, 255)
P_EYE = (40, 30, 20, 255)
P_BODY = (0, 180, 0, 255)
P_BODY_DK = (0, 140, 0, 255)
P_BELT = (80, 60, 20, 255)
P_LEG = (100, 70, 40, 255)
P_SWORD = (200, 200, 200, 255)
P_SWORD_HI = (240, 240, 240, 255)
P_HILT = (160, 130, 50, 255)


def _draw_player_base(img, ox, body_dy=0, head_dy=0):
    """Draw the player torso + head at offset ox. Returns nothing."""
    # Head
    fill_rect(img, ox + 6, 2 + head_dy, 4, 4, P_HEAD)
    put(img, ox + 7, 3 + head_dy, P_EYE)
    put(img, ox + 9, 3 + head_dy, P_EYE)
    fill_rect(img, ox + 6, 1 + head_dy, 4, 1, P_HAIR)
    # Body
    fill_rect(img, ox + 4, 6 + body_dy, 8, 6, P_BODY)
    put(img, ox + 4, 6 + body_dy, P_BODY_DK)
    put(img, ox + 11, 6 + body_dy, P_BODY_DK)
    fill_rect(img, ox + 4, 11 + body_dy, 8, 1, P_BELT)


def _draw_player_legs_stand(img, ox, spread=0):
    fill_rect(img, ox + 5 - spread, 12, 2, 4, P_LEG)
    fill_rect(img, ox + 9 + spread, 12, 2, 4, P_LEG)


def _draw_player_sword(img, ox, tip_y=2, length=8):
    for sy in range(tip_y, tip_y + length):
        put(img, ox + 13, sy, P_SWORD)
    put(img, ox + 13, tip_y, P_SWORD_HI)
    put(img, ox + 13, tip_y + 1, P_SWORD_HI)
    hilt_y = tip_y + 5
    put(img, ox + 12, hilt_y, P_HILT)
    put(img, ox + 13, hilt_y, P_HILT)
    put(img, ox + 14, hilt_y, P_HILT)


def generate_player_idle():
    # 4 frames: subtle breathing (body shifts ±1 px)
    offsets = [0, 0, -1, 0]

    def draw(img, ox, f):
        dy = offsets[f]
        _draw_player_base(img, ox, body_dy=dy, head_dy=dy)
        _draw_player_legs_stand(img, ox)
        _draw_player_sword(img, ox, tip_y=2 + dy)

    save(make_sheet(16, 16, 4, draw), "player_idle.png")


def generate_player_run():
    # 6 frames: legs alternate, body bobs
    # leg_left_dy, leg_right_dy, body_dy
    frames = [
        (0, 0, 0),     # neutral
        (-2, 1, -1),   # left forward, right back
        (-1, 2, 0),    # left up, right down
        (0, 0, 0),     # neutral
        (1, -2, -1),   # right forward, left back
        (2, -1, 0),    # right up, left down
    ]

    def draw(img, ox, f):
        l_dy, r_dy, b_dy = frames[f]
        _draw_player_base(img, ox, body_dy=b_dy, head_dy=b_dy)
        # Left leg
        fill_rect(img, ox + 5, 12 + l_dy, 2, max(1, 4 - abs(l_dy)), P_LEG)
        # Right leg
        fill_rect(img, ox + 9, 12 + r_dy, 2, max(1, 4 - abs(r_dy)), P_LEG)
        _draw_player_sword(img, ox, tip_y=2 + b_dy)

    save(make_sheet(16, 16, 6, draw), "player_run.png")


def generate_player_attack():
    # 4 frames: sword swing arc
    # sword_positions: list of (sx, sy_start, sy_end) for sword blade pixels
    def draw(img, ox, f):
        _draw_player_base(img, ox)
        _draw_player_legs_stand(img, ox)
        if f == 0:
            # Windup — sword raised high
            for sy in range(0, 7):
                put(img, ox + 13, sy, P_SWORD)
            put(img, ox + 13, 0, P_SWORD_HI)
            put(img, ox + 12, 6, P_HILT)
            put(img, ox + 13, 6, P_HILT)
            put(img, ox + 14, 6, P_HILT)
        elif f == 1:
            # Swing mid — sword horizontal right
            for sx in range(10, 16):
                put(img, ox + sx, 5, P_SWORD)
            put(img, ox + 15, 5, P_SWORD_HI)
            put(img, ox + 10, 5, P_HILT)
            put(img, ox + 10, 4, P_HILT)
            put(img, ox + 10, 6, P_HILT)
        elif f == 2:
            # Swing low — sword diagonal down-right
            for i in range(7):
                put(img, ox + 10 + i, 6 + i, P_SWORD)
            put(img, ox + 15, 11, P_SWORD_HI)
            put(img, ox + 10, 5, P_HILT)
            put(img, ox + 10, 6, P_HILT)
            put(img, ox + 11, 5, P_HILT)
        else:
            # Recovery — sword returning to idle
            _draw_player_sword(img, ox, tip_y=3, length=7)

    save(make_sheet(16, 16, 4, draw), "player_attack.png")


# ===========================================================================
# SKELETON sprite sheets
# ===========================================================================

S_BONE = (220, 220, 200, 255)
S_BONE_DK = (180, 180, 160, 255)
S_BLACK = (0, 0, 0, 255)


def _draw_skeleton_upper(img, ox, body_dy=0):
    """Skull + spine + ribs."""
    # Skull
    fill_rect(img, ox + 6, 1 + body_dy, 4, 4, S_BONE)
    put(img, ox + 7, 2 + body_dy, S_BLACK)
    put(img, ox + 9, 2 + body_dy, S_BLACK)
    put(img, ox + 7, 4 + body_dy, S_BONE_DK)
    put(img, ox + 8, 4 + body_dy, S_BONE_DK)
    put(img, ox + 9, 4 + body_dy, S_BONE_DK)
    # Spine
    for sy in range(5, 11):
        put(img, ox + 8, sy + body_dy, S_BONE)
    # Ribs
    fill_rect(img, ox + 5, 6 + body_dy, 6, 1, S_BONE)
    fill_rect(img, ox + 6, 7 + body_dy, 4, 1, S_BONE_DK)
    fill_rect(img, ox + 5, 8 + body_dy, 6, 1, S_BONE)
    fill_rect(img, ox + 6, 9 + body_dy, 4, 1, S_BONE_DK)


def _draw_skeleton_arms_neutral(img, ox, body_dy=0):
    put(img, ox + 4, 6 + body_dy, S_BONE)
    put(img, ox + 3, 7 + body_dy, S_BONE)
    put(img, ox + 12, 6 + body_dy, S_BONE)
    put(img, ox + 13, 7 + body_dy, S_BONE)


def _draw_skeleton_legs(img, ox, l_dy=0, r_dy=0):
    for sy in range(11, 16):
        if 0 <= sy + l_dy < 16:
            put(img, ox + 6, sy + l_dy, S_BONE)
        if 0 <= sy + r_dy < 16:
            put(img, ox + 10, sy + r_dy, S_BONE)
    put(img, ox + 5, min(15, 15 + l_dy), S_BONE_DK)
    put(img, ox + 11, min(15, 15 + r_dy), S_BONE_DK)


def generate_skeleton_idle():
    sway = [0, 0, -1, 0]

    def draw(img, ox, f):
        dy = sway[f]
        _draw_skeleton_upper(img, ox, dy)
        _draw_skeleton_arms_neutral(img, ox, dy)
        _draw_skeleton_legs(img, ox)

    save(make_sheet(16, 16, 4, draw), "skeleton_idle.png")


def generate_skeleton_run():
    frames = [(0, 0, 0), (-1, 1, -1), (-2, 2, 0), (0, 0, 0), (1, -1, -1), (2, -2, 0)]

    def draw(img, ox, f):
        l_dy, r_dy, b_dy = frames[f]
        _draw_skeleton_upper(img, ox, b_dy)
        _draw_skeleton_arms_neutral(img, ox, b_dy)
        for sy in range(11, 16):
            if 0 <= sy + l_dy < 16:
                put(img, ox + 6, sy + l_dy, S_BONE)
            if 0 <= sy + r_dy < 16:
                put(img, ox + 10, sy + r_dy, S_BONE)

    save(make_sheet(16, 16, 6, draw), "skeleton_run.png")


def generate_skeleton_attack():
    def draw(img, ox, f):
        _draw_skeleton_upper(img, ox)
        _draw_skeleton_legs(img, ox)
        if f == 0:
            # Arm raised
            put(img, ox + 4, 6, S_BONE)
            put(img, ox + 3, 5, S_BONE)
            put(img, ox + 3, 4, S_BONE)
            put(img, ox + 12, 6, S_BONE)
            put(img, ox + 13, 7, S_BONE)
        elif f == 1:
            # Arm swinging
            put(img, ox + 3, 6, S_BONE)
            put(img, ox + 2, 7, S_BONE)
            put(img, ox + 1, 8, S_BONE)
            put(img, ox + 12, 6, S_BONE)
            put(img, ox + 13, 7, S_BONE)
        elif f == 2:
            # Arm down (impact)
            put(img, ox + 3, 8, S_BONE)
            put(img, ox + 2, 9, S_BONE)
            put(img, ox + 1, 10, S_BONE)
            put(img, ox + 12, 6, S_BONE)
            put(img, ox + 13, 7, S_BONE)
        else:
            _draw_skeleton_arms_neutral(img, ox)

    save(make_sheet(16, 16, 4, draw), "skeleton_attack.png")


# ===========================================================================
# ZOMBIE sprite sheets
# ===========================================================================

Z_BODY = (30, 100, 30, 255)
Z_BODY_DK = (20, 70, 20, 255)
Z_HEAD = (60, 80, 50, 255)
Z_CLOTH = (80, 60, 30, 255)
Z_EYE = (120, 40, 30, 255)
Z_MOUTH = (40, 30, 20, 255)
Z_LEG = (25, 80, 25, 255)


def _draw_zombie_head(img, ox, dy=0):
    fill_rect(img, ox + 5, 1 + dy, 5, 4, Z_HEAD)
    put(img, ox + 6, 2 + dy, Z_EYE)
    put(img, ox + 9, 2 + dy, Z_EYE)
    put(img, ox + 7, 4 + dy, Z_MOUTH)
    put(img, ox + 8, 4 + dy, Z_MOUTH)


def _draw_zombie_body(img, ox, dy=0):
    fill_rect(img, ox + 3, 5 + dy, 10, 7, Z_BODY)
    put(img, ox + 4, 6 + dy, Z_BODY_DK)
    put(img, ox + 8, 8 + dy, Z_BODY_DK)
    put(img, ox + 11, 7 + dy, Z_BODY_DK)
    # Rags
    put(img, ox + 3, 5 + dy, Z_CLOTH)
    put(img, ox + 4, 5 + dy, Z_CLOTH)
    put(img, ox + 3, 6 + dy, Z_CLOTH)
    put(img, ox + 10, 10 + dy, Z_CLOTH)
    put(img, ox + 11, 10 + dy, Z_CLOTH)
    put(img, ox + 5, 11 + dy, Z_CLOTH)


def _draw_zombie_arms_forward(img, ox, dy=0, ext=2):
    """Arms stretched forward. ext controls how far."""
    fill_rect(img, ox + 1 - ext + 2, 6 + dy, ext, 2, Z_BODY)
    fill_rect(img, ox + 13, 6 + dy, ext, 2, Z_BODY)
    put(img, ox + 1 - ext + 2, 6 + dy, Z_HEAD)
    put(img, ox + 1 - ext + 2, 7 + dy, Z_HEAD)
    put(img, ox + 13 + ext - 1, 6 + dy, Z_HEAD)
    put(img, ox + 13 + ext - 1, 7 + dy, Z_HEAD)


def _draw_zombie_legs(img, ox, l_dy=0, r_dy=0):
    fill_rect(img, ox + 4, 12 + l_dy, 3, max(1, 4 - abs(l_dy)), Z_LEG)
    fill_rect(img, ox + 9, 12 + r_dy, 3, max(1, 4 - abs(r_dy)), Z_LEG)
    # rags
    if 12 + l_dy + 1 < 16:
        put(img, ox + 5, 13 + l_dy, Z_CLOTH)
    if 12 + r_dy + 2 < 16:
        put(img, ox + 10, 14 + r_dy, Z_CLOTH)


def generate_zombie_idle():
    sway = [0, 0, 1, 0]

    def draw(img, ox, f):
        dy = sway[f]
        _draw_zombie_head(img, ox, dy)
        _draw_zombie_body(img, ox, dy)
        _draw_zombie_arms_forward(img, ox, dy, ext=2)
        _draw_zombie_legs(img, ox)

    save(make_sheet(16, 16, 4, draw), "zombie_idle.png")


def generate_zombie_run():
    # Slow shuffling gait
    frames = [(0, 0, 0), (-1, 1, 0), (-1, 2, 1), (0, 0, 0), (1, -1, 0), (1, -2, 1)]

    def draw(img, ox, f):
        l_dy, r_dy, b_dy = frames[f]
        _draw_zombie_head(img, ox, b_dy)
        _draw_zombie_body(img, ox, b_dy)
        _draw_zombie_arms_forward(img, ox, b_dy, ext=2)
        _draw_zombie_legs(img, ox, l_dy, r_dy)

    save(make_sheet(16, 16, 6, draw), "zombie_run.png")


def generate_zombie_attack():
    # Arms swing forward/down
    arm_ext = [2, 3, 3, 2]
    arm_dy_extra = [0, -1, 1, 0]

    def draw(img, ox, f):
        _draw_zombie_head(img, ox)
        _draw_zombie_body(img, ox)
        _draw_zombie_legs(img, ox)
        ext = arm_ext[f]
        ady = arm_dy_extra[f]
        # Left arm
        for i in range(ext):
            put(img, ox + 1 - i, 6 + ady, Z_BODY)
            put(img, ox + 1 - i, 7 + ady, Z_BODY)
        put(img, ox + 1 - ext + 1, 6 + ady, Z_HEAD)
        put(img, ox + 1 - ext + 1, 7 + ady, Z_HEAD)
        # Right arm
        for i in range(ext):
            put(img, ox + 13 + i, 6 + ady, Z_BODY)
            put(img, ox + 13 + i, 7 + ady, Z_BODY)
        put(img, ox + 13 + ext - 1, 6 + ady, Z_HEAD)
        put(img, ox + 13 + ext - 1, 7 + ady, Z_HEAD)

    save(make_sheet(16, 16, 4, draw), "zombie_attack.png")


# ===========================================================================
# LICH sprite sheets
# ===========================================================================

L_ROBE = (140, 40, 180, 255)
L_ROBE_DK = (100, 20, 140, 255)
L_HOOD = (80, 20, 100, 255)
L_EYES = (200, 255, 0, 255)
L_EYE_GLOW = (100, 140, 0, 180)
L_ORB = (180, 0, 255, 200)


def _draw_lich_hood(img, ox, dy=0):
    fill_rect(img, ox + 5, 1 + dy, 6, 4, L_HOOD)
    for x in range(6, 10):
        put(img, ox + x, 0 + dy, L_HOOD)
    put(img, ox + 7, 3 + dy, L_EYES)
    put(img, ox + 9, 3 + dy, L_EYES)
    put(img, ox + 6, 3 + dy, L_EYE_GLOW)
    put(img, ox + 10, 3 + dy, L_EYE_GLOW)


def _draw_lich_robe(img, ox, dy=0, wave=0):
    """Triangular robe. wave offsets bottom edge for animation."""
    for row in range(5, 14):
        half_w = 2 + (row - 5)
        cx = 8
        for dx in range(-half_w, half_w + 1):
            x = cx + dx
            if 0 <= x < 16:
                c = L_ROBE_DK if abs(dx) == half_w else L_ROBE
                put(img, ox + x, row + dy, c)
    # Bottom fringe with wave
    for x in range(2, 14):
        y = 14 + dy
        if wave:
            y += (1 if (x + wave) % 3 == 0 else 0)
        if 0 <= y < 16:
            put(img, ox + x, y, L_ROBE_DK if x % 2 == 0 else L_ROBE)


def _draw_lich_arms(img, ox, dy=0, ext=0):
    put(img, ox + 3 - ext, 7 + dy, L_ROBE)
    put(img, ox + 2 - ext, 8 + dy, L_ROBE_DK)
    put(img, ox + 13 + ext, 7 + dy, L_ROBE)
    put(img, ox + 14 + ext, 8 + dy, L_ROBE_DK)
    put(img, ox + 2 - ext, 9 + dy, L_ORB)
    put(img, ox + 14 + ext, 9 + dy, L_ORB)


def generate_lich_idle():
    # 4 frames: hovering up/down, robe sways
    hover = [0, -1, 0, 1]
    waves = [0, 1, 2, 3]

    def draw(img, ox, f):
        dy = hover[f]
        _draw_lich_hood(img, ox, dy)
        _draw_lich_robe(img, ox, dy, wave=waves[f])
        _draw_lich_arms(img, ox, dy)

    save(make_sheet(16, 16, 4, draw), "lich_idle.png")


def generate_lich_run():
    hover = [0, -1, -1, 0, 1, 1]
    waves = [0, 1, 2, 0, 1, 2]

    def draw(img, ox, f):
        dy = hover[f]
        _draw_lich_hood(img, ox, dy)
        _draw_lich_robe(img, ox, dy, wave=waves[f])
        _draw_lich_arms(img, ox, dy)

    save(make_sheet(16, 16, 6, draw), "lich_run.png")


def generate_lich_attack():
    # 4 frames: arms extend, orb grows bright, flash
    arm_ext = [0, 1, 2, 1]
    eye_bright = [False, False, True, True]

    def draw(img, ox, f):
        _draw_lich_hood(img, ox)
        _draw_lich_robe(img, ox, wave=f)
        # Override eyes for cast flash
        if eye_bright[f]:
            put(img, ox + 7, 3, (255, 255, 100, 255))
            put(img, ox + 9, 3, (255, 255, 100, 255))
            put(img, ox + 6, 3, (200, 255, 0, 220))
            put(img, ox + 10, 3, (200, 255, 0, 220))
        ext = arm_ext[f]
        _draw_lich_arms(img, ox, ext=ext)
        # Casting glow at max extension
        if f == 2:
            put(img, ox + 0, 9, (180, 0, 255, 100))
            put(img, ox + 15, 9, (180, 0, 255, 100))
            put(img, ox + 1, 8, (180, 0, 255, 80))
            put(img, ox + 14, 8, (180, 0, 255, 80))

    save(make_sheet(16, 16, 4, draw), "lich_attack.png")


# ===========================================================================
# CAMPFIRE — 8 frames (128x16)
# ===========================================================================

def generate_campfire():
    wood = (120, 80, 30, 255)
    wood_dark = (80, 50, 20, 255)
    orange = (255, 160, 0, 255)
    yellow = (255, 220, 50, 255)
    red = (255, 80, 0, 255)

    rng = random.Random(999)

    def draw(img, ox, f):
        # Logs (same every frame)
        for i in range(8):
            put(img, ox + 3 + i, 13 - i // 2, wood)
            put(img, ox + 3 + i, 14 - i // 2, wood_dark)
        for i in range(8):
            put(img, ox + 12 - i, 13 - i // 2, wood)
            put(img, ox + 12 - i, 14 - i // 2, wood_dark)

        # Fire varies per frame
        jitter_x = rng.randint(-1, 1)
        jitter_y = rng.randint(-1, 0)
        fx, fy = 6 + jitter_x, 6 + jitter_y

        # Main flame
        fill_rect(img, ox + fx, fy, 4, 4, orange)
        fill_rect(img, ox + fx + 1, fy - 2, 2, 2, orange)
        # Hot center
        fill_rect(img, ox + fx + 1, fy + 1, 2, 2, yellow)
        put(img, ox + fx + 1, fy - 1, yellow)
        put(img, ox + fx + 2, fy - 1, yellow)
        # Tip
        tip_h = rng.randint(2, 4)
        put(img, ox + fx + 1, fy - tip_h, yellow)
        if tip_h > 2:
            put(img, ox + fx + 2, fy - tip_h + 1, (255, 200, 50, 200))

        # Side tongues
        if f % 2 == 0:
            put(img, ox + fx - 1, fy + 1, orange)
            put(img, ox + fx + 4, fy + 2, (255, 120, 0, 200))
        else:
            put(img, ox + fx - 1, fy + 2, (255, 120, 0, 200))
            put(img, ox + fx + 4, fy + 1, orange)

        # Sparks — random per frame
        for _ in range(2):
            sx = rng.randint(4, 11)
            sy = rng.randint(0, 3)
            put(img, ox + sx, sy, (255, rng.randint(60, 120), 0, rng.randint(140, 220)))

    save(make_sheet(16, 16, 8, draw), "campfire.png")


# ===========================================================================
# PROJECTILE — 4 frames (32x8)
# ===========================================================================

def generate_projectile():
    core = (180, 0, 255, 255)
    bright = (220, 100, 255, 255)
    glow = (180, 0, 255, 120)
    glow2 = (140, 0, 200, 60)

    pulse_sizes = [3, 4, 4, 3]  # core pixel size
    glow_alphas = [80, 120, 140, 100]

    def draw(img, ox, f):
        sz = pulse_sizes[f]
        ga = glow_alphas[f]
        offset = (8 - sz) // 2
        # Core
        fill_rect(img, ox + offset, offset, sz, sz, core)
        # Bright center
        cx, cy = 3, 3
        put(img, ox + cx, cy, bright)
        put(img, ox + cx + 1, cy, bright)
        put(img, ox + cx, cy + 1, bright)
        put(img, ox + cx + 1, cy + 1, bright)
        # Glow ring
        for x, y in [(1, 2), (1, 3), (1, 4), (1, 5),
                     (6, 2), (6, 3), (6, 4), (6, 5),
                     (2, 1), (3, 1), (4, 1), (5, 1),
                     (2, 6), (3, 6), (4, 6), (5, 6)]:
            put(img, ox + x, y, (180, 0, 255, ga))
        # Outer glow
        outer_a = max(30, ga - 40)
        for x, y in [(0, 3), (0, 4), (7, 3), (7, 4),
                     (3, 0), (4, 0), (3, 7), (4, 7)]:
            put(img, ox + x, y, (140, 0, 200, outer_a))

    save(make_sheet(8, 8, 4, draw), "projectile.png")


# ===========================================================================
# TILES (32x16 isometric diamonds) — static, single images
# ===========================================================================

def draw_iso_diamond(img, base_color, shade_color=None):
    w, h = img.width, img.height
    cx, cy = w // 2, h // 2
    rng = random.Random(hash(base_color))
    for y in range(h):
        for x in range(w):
            dx = abs(x - cx + 0.5)
            dy = abs(y - cy + 0.5)
            if dx / (w / 2) + dy / (h / 2) <= 1.0:
                r, g, b = base_color
                v = rng.randint(-8, 8)
                c = (max(0, min(255, r + v)), max(0, min(255, g + v)), max(0, min(255, b + v)), 255)
                put(img, x, y, c)
    if shade_color:
        sr, sg, sb = shade_color
        for y in range(h):
            for x in range(w):
                dx = abs(x - cx + 0.5)
                dy = abs(y - cy + 0.5)
                ratio = dx / (w / 2) + dy / (h / 2)
                if 0.85 <= ratio <= 1.0:
                    cur = img.getpixel((x, y))
                    if cur[3] > 0:
                        put(img, x, y, (sr, sg, sb, 255))


def generate_tile_grass():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    draw_iso_diamond(img, (34, 60, 34), shade_color=(24, 45, 24))
    rng = random.Random(42)
    grass_colors = [(28, 50, 28, 255), (40, 70, 40, 255), (30, 55, 30, 255)]
    for _ in range(12):
        x, y = rng.randint(4, 27), rng.randint(2, 13)
        if img.getpixel((x, y))[3] > 0:
            put(img, x, y, rng.choice(grass_colors))
    save(img, "tile_grass.png")


def generate_tile_dirt():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    draw_iso_diamond(img, (80, 80, 70), shade_color=(60, 60, 50))
    rng = random.Random(101)
    for _ in range(10):
        x, y = rng.randint(5, 26), rng.randint(2, 13)
        if img.getpixel((x, y))[3] > 0:
            put(img, x, y, rng.choice([(90, 90, 80, 255), (70, 70, 60, 255)]))
    save(img, "tile_dirt.png")


def generate_tile_camp():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    draw_iso_diamond(img, (100, 70, 40), shade_color=(75, 50, 28))
    rng = random.Random(77)
    for _ in range(8):
        x, y = rng.randint(6, 25), rng.randint(3, 12)
        if img.getpixel((x, y))[3] > 0:
            v = rng.randint(-10, 10)
            put(img, x, y, (100 + v, 70 + v, 40 + v, 255))
    save(img, "tile_camp.png")


def generate_tile_grave():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    draw_iso_diamond(img, (80, 80, 70), shade_color=(60, 60, 50))
    stone, stone_dk = (150, 150, 140, 255), (120, 120, 110, 255)
    fill_rect(img, 14, 5, 4, 6, stone)
    put(img, 15, 4, stone)
    put(img, 16, 4, stone)
    put(img, 15, 3, stone_dk)
    put(img, 16, 3, stone_dk)
    put(img, 15, 7, stone_dk)
    put(img, 16, 8, stone_dk)
    put(img, 15, 9, stone_dk)
    save(img, "tile_grave.png")


# ===========================================================================
# DECOR — static, single images
# ===========================================================================

def generate_tree():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    trunk = (80, 50, 20, 255)
    trunk_dk = (60, 35, 15, 255)
    trunk_lt = (95, 60, 25, 255)
    canopy = (20, 60, 20, 255)
    canopy_lt = (30, 75, 30, 255)
    canopy_dk = (15, 45, 15, 255)
    canopy_dk2 = (10, 35, 10, 255)
    # Trunk: 6x14 at bottom center
    fill_rect(img, 13, 18, 6, 14, trunk)
    # Bark texture
    for bx, by in [(14, 20), (16, 23), (15, 26), (17, 29), (13, 22), (18, 25)]:
        put(img, bx, by, trunk_dk)
    for bx, by in [(15, 19), (17, 22), (14, 27)]:
        put(img, bx, by, trunk_lt)
    # Roots at base
    put(img, 12, 30, trunk_dk)
    put(img, 12, 31, trunk_dk)
    put(img, 19, 30, trunk_dk)
    put(img, 19, 31, trunk_dk)
    # Canopy: large circle ~12px radius centered on upper portion
    cx, cy, r2 = 16, 10, 11 * 11
    rng = random.Random(55)
    for y in range(0, 22):
        for x in range(2, 30):
            d = (x - cx + 0.5) ** 2 + (y - cy + 0.5) ** 2
            if d <= r2:
                ratio = d / r2
                if ratio > 0.8:
                    c = canopy_dk2
                elif ratio > 0.6:
                    c = canopy_dk
                elif ratio < 0.2:
                    c = canopy_lt
                else:
                    c = canopy
                # Slight random texture variation
                r, g, b, a = c
                v = rng.randint(-5, 5)
                put(img, x, y, (max(0, min(255, r + v)), max(0, min(255, g + v)), max(0, min(255, b + v)), a))
    # Leaf highlights (scattered lighter spots)
    for _ in range(8):
        lx = rng.randint(6, 26)
        ly = rng.randint(3, 17)
        if img.getpixel((lx, ly))[3] > 0:
            put(img, lx, ly, (35, 85, 35, 255))
    save(img, "tree.png")


def generate_tombstone():
    img = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    stone, stone_dk, crack = (150, 150, 140, 255), (120, 120, 110, 255), (80, 80, 70, 255)
    fill_rect(img, 4, 5, 8, 10, stone)
    fill_rect(img, 5, 3, 6, 2, stone)
    for x in [6, 7, 8, 9]:
        put(img, x, 2, stone)
    put(img, 7, 1, stone_dk)
    put(img, 8, 1, stone_dk)
    for y in range(5, 15):
        put(img, 4, y, stone_dk)
        put(img, 11, y, stone_dk)
    put(img, 5, 3, stone_dk)
    put(img, 10, 3, stone_dk)
    for x, y in [(7, 6), (8, 7), (7, 8), (7, 9), (8, 10)]:
        put(img, x, y, crack)
    fill_rect(img, 3, 14, 10, 2, (90, 80, 60, 255))
    fill_rect(img, 3, 15, 10, 1, (70, 60, 45, 255))
    save(img, "tombstone.png")


def generate_bones():
    img = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
    bone, bone_dk = (220, 220, 200, 255), (190, 190, 170, 255)
    for i in range(8):
        put(img, i, i, bone)
        put(img, 7 - i, i, bone)
    for x, y in [(0, 1), (1, 0), (6, 0), (7, 1), (0, 6), (1, 7), (6, 7), (7, 6)]:
        put(img, x, y, bone_dk)
    save(img, "bones.png")


# ===========================================================================
# MAIN
# ===========================================================================

def main():
    os.makedirs(ASSETS_DIR, exist_ok=True)
    print(f"Generating sprites in {ASSETS_DIR}/\n")

    print("Player sprite sheets:")
    generate_player_idle()
    generate_player_run()
    generate_player_attack()
    print()

    print("Skeleton sprite sheets:")
    generate_skeleton_idle()
    generate_skeleton_run()
    generate_skeleton_attack()
    print()

    print("Zombie sprite sheets:")
    generate_zombie_idle()
    generate_zombie_run()
    generate_zombie_attack()
    print()

    print("Lich sprite sheets:")
    generate_lich_idle()
    generate_lich_run()
    generate_lich_attack()
    print()

    print("Effects (animated):")
    generate_campfire()
    generate_projectile()
    print()

    print("Tiles (32x16 iso):")
    generate_tile_grass()
    generate_tile_dirt()
    generate_tile_camp()
    generate_tile_grave()
    print()

    print("Decor (static):")
    generate_tree()
    generate_tombstone()
    generate_bones()
    print()

    expected = [
        "player_idle.png", "player_run.png", "player_attack.png",
        "skeleton_idle.png", "skeleton_run.png", "skeleton_attack.png",
        "zombie_idle.png", "zombie_run.png", "zombie_attack.png",
        "lich_idle.png", "lich_run.png", "lich_attack.png",
        "campfire.png", "projectile.png",
        "tile_grass.png", "tile_dirt.png", "tile_camp.png", "tile_grave.png",
        "tree.png", "tombstone.png", "bones.png",
    ]
    missing = [f for f in expected if not os.path.exists(os.path.join(ASSETS_DIR, f))]
    if missing:
        print(f"ERROR: Missing files: {missing}")
    else:
        print(f"All {len(expected)} sprites generated successfully!")


if __name__ == "__main__":
    main()
