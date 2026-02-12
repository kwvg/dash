#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Generate transaction confirmation progress icons (transaction0..5).

Produces 6 PNG icons (64x64, grayscale+alpha, black on transparent)
representing incremental confirmation progress as a segmented ring:

  transaction0 = 0 segments visible  (empty ring, all hidden)
  transaction1 = 1 segment  visible  (top arc)
  transaction2 = 2 segments visible  (top + upper-right)
  transaction3 = 3 segments visible  (top + upper-right + lower-right)
  transaction4 = 4 segments visible  (+ bottom)
  transaction5 = 5 segments visible  (+ lower-left; upper-left gap remains)

The ring is divided into 6 arcs separated by small angular gaps.
Hidden segments use gray=255 (white, invisible on transparent bg),
visible segments use gray=0 (black, recolored by Qt theme engine).

Usage:
  python3 contrib/devtools/gen-transaction-icons.py
"""

import math
import os
import struct
import zlib

SIZE = 64
CENTER = SIZE / 2 - 0.5  # 31.5 — center between pixels 31 and 32
OUTER_R = 30.0
INNER_R = 23.6    # inner edge fully opaque at ~r=23.6
INNER_AA = 1.75   # inner anti-aliasing zone width (~1.75px fade)

# Gap definitions: (center_angle, half_width) in degrees
# Angles use 0°=top, clockwise convention
# Measured from existing icons: 4 wider gaps (8°) and 2 narrower (6°)
GAPS = [
    (35.0, 4.0),   # between top and upper-right
    (90.0, 3.0),   # between upper-right and lower-right
    (145.0, 4.0),  # between lower-right and bottom
    (215.0, 4.0),  # between bottom and lower-left
    (270.0, 3.0),  # between lower-left and upper-left
    (325.0, 4.0),  # between upper-left and top
]

# Segment fill order: which segments become visible at each level
# Segments are numbered 0-5 clockwise from top
# 0=top, 1=upper-right, 2=lower-right, 3=bottom, 4=lower-left, 5=upper-left
FILL_ORDER = [0, 1, 2, 3, 4, 5]


def angle_in_gap(angle_deg):
    """Return gap coverage (0.0=not in gap, 1.0=fully in gap) with anti-aliasing."""
    for center, half_width in GAPS:
        # Angular distance, accounting for wrap-around
        diff = (angle_deg - center + 180) % 360 - 180
        abs_diff = abs(diff)
        if abs_diff < half_width + 1.0:
            if abs_diff < half_width:
                return 1.0
            else:
                # Anti-alias the gap edge over 1 degree
                return half_width + 1.0 - abs_diff
    return 0.0


def get_segment_index(angle_deg):
    """Determine which segment (0-5) an angle belongs to."""
    # Segment boundaries (midpoints between gap centers)
    # Gap centers: 35, 90, 145, 215, 270, 325
    # Segment centers: 0 (top), 62.5, 117.5, 180, 242.5, 297.5
    # We define segments by their angular range between gaps
    boundaries = [35.0, 90.0, 145.0, 215.0, 270.0, 325.0]
    # Segment 0 (top): 325° to 35°
    # Segment 1 (upper-right): 35° to 90°
    # etc.
    a = angle_deg % 360
    if a >= boundaries[5] or a < boundaries[0]:
        return 0  # top
    elif a < boundaries[1]:
        return 1  # upper-right
    elif a < boundaries[2]:
        return 2  # lower-right
    elif a < boundaries[3]:
        return 3  # bottom
    elif a < boundaries[4]:
        return 4  # lower-left
    else:
        return 5  # upper-left


def make_png_ga(pixels):
    """Encode grayscale+alpha pixel data as a PNG file (color type 4)."""
    raw = bytearray()
    for y in range(SIZE):
        raw.append(0)  # filter byte: None
        for x in range(SIZE):
            g, a = pixels[y][x]
            raw.append(g)
            raw.append(a)

    def chunk(chunk_type, data):
        c = chunk_type + data
        return (struct.pack('>I', len(data)) + c +
                struct.pack('>I', zlib.crc32(c) & 0xFFFFFFFF))

    # IHDR: width, height, bit_depth=8, color_type=4 (gray+alpha)
    ihdr = struct.pack('>IIBBBBB', SIZE, SIZE, 8, 4, 0, 0, 0)
    idat = zlib.compress(bytes(raw), 9)

    out = b'\x89PNG\r\n\x1a\n'
    out += chunk(b'IHDR', ihdr)
    out += chunk(b'IDAT', idat)
    out += chunk(b'IEND', b'')
    return out


def generate_transaction_icon(num_visible):
    """Generate a transaction icon with num_visible segments (0-5)."""
    visible_segments = set(FILL_ORDER[:num_visible])

    pixels = [[(0, 0)] * SIZE for _ in range(SIZE)]

    for y in range(SIZE):
        for x in range(SIZE):
            dx = x - CENTER
            dy = y - CENTER
            dist = math.sqrt(dx * dx + dy * dy)

            # Outside the ring entirely
            if dist > OUTER_R + 1 or dist < INNER_R - INNER_AA:
                continue

            # Radial anti-aliasing: 1px fade at outer edge, wider at inner
            if dist > OUTER_R:
                radial_alpha = OUTER_R + 1.0 - dist
            elif dist < INNER_R:
                radial_alpha = (dist - (INNER_R - INNER_AA)) / INNER_AA
            else:
                radial_alpha = 1.0

            if radial_alpha <= 0:
                continue

            # Compute angle (0°=top, clockwise)
            angle = math.degrees(math.atan2(dx, -dy)) % 360

            # Check if in a gap
            gap_coverage = angle_in_gap(angle)
            if gap_coverage >= 1.0:
                continue  # fully in gap, no pixel

            ring_alpha = radial_alpha * (1.0 - gap_coverage)
            alpha = int(max(0, min(255, ring_alpha * 255 + 0.5)))

            if alpha <= 0:
                continue

            # Determine gray value: 0=black (visible), 255=white (hidden)
            seg = get_segment_index(angle)
            gray = 0 if seg in visible_segments else 255

            pixels[y][x] = (gray, alpha)

    return pixels


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.join(script_dir, '..', '..')
    icon_dir = os.path.join(repo_root, 'src', 'qt', 'res', 'icons')

    for i in range(6):
        pixels = generate_transaction_icon(i)
        png_data = make_png_ga(pixels)
        filepath = os.path.join(icon_dir, f'transaction{i}.png')
        with open(filepath, 'wb') as f:
            f.write(png_data)
        print(f'  transaction{i}.png ({len(png_data)} bytes)')

    print('Done.')


if __name__ == '__main__':
    main()
