#!/usr/bin/env python3
#
#  ReSyndicated - a modernized game engine and full map/mission editor for the
#  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
#  project, FreeSynd.
#
#   Copyright (C) 2026  mork009 <mork009resynd@gmail.com>
#
#   This program is free software: you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#  See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#  along with this program. If not, see <https://www.gnu.org/licenses/>.
#

"""
generate_editor_icons.py
------------------------
Production utility for ReSyndicated icon assets (Mission Editor and Application).

1. Extracts, validates, resamples, and packages 24 master editor icons into:
  - Binary master resource: editor/resources/icons_master_48.dat
     (24 icons x 48x48 pixels x 4 bytes BGRA Little-Endian = 221,184 bytes)
  - Companion BMP icon strips in editor/resources/:
     - toolbar_icons_24.bmp (408x24, 17 icons @ 24x24)
     - toolbar_icons_48.bmp (816x48, 17 icons @ 48x48)
     - tab_icons_16.bmp     (64x16,  4 icons @ 16x16)
     - tab_icons_32.bmp     (128x32, 4 icons @ 32x32)
  - Visual preview grid contact sheet (scratch/icons_preview_grid.png)
  - Optional dump of individual 48x48 PNG files.

2. Generates multi-resolution Windows .ico application icons from source logo.png:
  - game/logo.ico (embedded Win32 PE resource for resynd_game.exe)
  - editor/resources/logo.ico (embedded Win32 PE resource for resynd_editor.exe)
  - data/resources/logo.ico (bundled runtime data fallback)
  - Updates timestamps on resynd.rc and resource.rc to trigger MSVC resource re-compilation.

Input formats supported:
  - Grid Image (PNG, JPG, BMP, WEBP, etc.):
      Sheet containing a 6-column x 4-row grid of icons.
      Supports automatic groove detection for beveled/embossed panels
      as well as uniform mathematical grid slicing.
  - Directory Mode:
      Folder containing 24 individual image files (e.g. 00_New.png .. 23_Computer.png).

Usage:
  python editor/scripts/generate_editor_icons.py
  python editor/scripts/generate_editor_icons.py --input data/resources/editor_icons.png --logo data/resources/logo.png
  python editor/scripts/generate_editor_icons.py --no-editor  # Only regenerate .ico files from logo
  python editor/scripts/generate_editor_icons.py --no-logo    # Only regenerate editor icons
"""

import sys
import os
import re
import math
import struct
import argparse
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Error: The 'Pillow' image library is required to run this script.", file=sys.stderr)
    print("Install it with: pip install Pillow", file=sys.stderr)
    sys.exit(1)


# Canonical 24-icon sequence matching IconId enum in editor/graphics/icon_manager.h
CANONICAL_ICONS = [
    (0,  "00_New",           "New Mission (Ctrl+N)"),
    (1,  "01_Open",          "Open Mission (Ctrl+O)"),
    (2,  "02_Save",          "Save Mission (Ctrl+S)"),
    (3,  "03_Undo",          "Undo (Ctrl+Z)"),
    (4,  "04_Redo",          "Redo (Ctrl+Y)"),
    (5,  "05_Pencil",        "Pencil Tool (P)"),
    (6,  "06_BoxFill",       "Box Fill Tool (B)"),
    (7,  "07_Picker",        "[Unused]"),
    (8,  "08_Select",        "Select / Move Entity Tool (V)"),
    (9,  "09_Slice",         "Z-Slice Isolation View (Z)"),
    (10, "10_LayerUp",       "Layer Up (PgUp)"),
    (11, "11_LayerDown",     "Layer Down (PgDn)"),
    (12, "12_Spawns",        "Center on Player's Agents (Home)"),
    (13, "13_Playtest",      "Playtest Mission (F5)"),
    (14, "14_ZoomIn",        "Zoom In (+)"),
    (15, "15_ZoomOut",       "Zoom Out (-)"),
    (16, "16_Zoom100",       "Zoom 100% (1)"),
    (17, "17_TabTiles",      "Tiles Palette Tab"),
    (18, "18_TabEntities",   "Entities List Tab"),
    (19, "19_TabObjectives", "Objectives Tab"),
    (20, "20_Settings",      "Mission Properties / Settings Tab"),
    (21, "21_Search",        "Search Palette / Entities"),
    (22, "22_Trash",         "[Unused]"),
    (23, "23_Computer",      "[Unused]")
]

TOTAL_ICON_COUNT = 24
ICON_WIDTH = 48
ICON_HEIGHT = 48
BYTES_PER_PIXEL = 4
EXPECTED_DAT_SIZE = TOTAL_ICON_COUNT * ICON_WIDTH * ICON_HEIGHT * BYTES_PER_PIXEL # 221,184 bytes

# Verified groove coordinates for 2752 x 1536 grid sheets (e.g. icons_new_4.jpg)
PRESET_GROOVES_2752 = {
    "x": [170, 568, 976, 1385, 1793, 2202, 2610],
    "y": [43, 401, 761, 1116, 1483]
}

# Multi-resolution icon specifications for Windows PE executables (.ico)
ICO_SIZES = [(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (24, 24), (16, 16)]
DEFAULT_LOGO_PATH = "data/resources/logo.png"
DEFAULT_ICO_TARGETS = [
    "game/logo.ico",
    "editor/resources/logo.ico",
    "data/resources/logo.ico"
]


class ValidationError(Exception):
    """Raised when an input image or directory fails specification validation."""
    pass


def find_groove_dividers(im_gray, num_cells, axis="x", search_radius=40):
    """
    Detects groove divider positions along an axis using projection intensity valleys.
    axis='x': detects vertical dividers along horizontal axis.
    axis='y': detects horizontal dividers along vertical axis.
    """
    w, h = im_gray.size
    step = 4

    if axis == "x":
        # Average brightness across middle 60% of height
        y_range = range(int(h * 0.2), int(h * 0.8), step)
        prof = [sum(im_gray.getpixel((x, y)) for y in y_range) / len(y_range) for x in range(w)]
        length = w
    else:
        # Average brightness across middle 60% of width
        x_range = range(int(w * 0.2), int(w * 0.8), step)
        prof = [sum(im_gray.getpixel((x, y)) for x in x_range) / len(x_range) for y in range(h)]
        length = h

    # Detect active content boundaries (where intensity exceeds 15% threshold)
    threshold = 25.0
    start = 0
    for i in range(0, length // 4):
        if prof[i] > threshold:
            start = i
            break

    end = length - 1
    for i in range(length - 1, length - length // 4, -1):
        if prof[i] > threshold:
            end = i
            break

    span = end - start
    cell_nominal = span / float(num_cells)

    divs = [start]
    for c in range(1, num_cells):
        expected = int(start + c * cell_nominal)
        r_start = max(0, expected - search_radius)
        r_end = min(length, expected + search_radius)
        min_pos = min(range(r_start, r_end), key=lambda idx: prof[idx])
        divs.append(min_pos)
    divs.append(end)

    return divs


def slice_grid_image(image_path, mode="auto", margin=(0, 0, 0, 0), grooves_x=None, grooves_y=None):
    """
    Validates and slices a grid image into 24 icon images.
    Returns a list of 24 PIL Image objects (RGB/RGBA).
    """
    if not os.path.isfile(image_path):
        raise ValidationError(f"Source image file does not exist: {image_path}")

    try:
        im = Image.open(image_path)
        im.load()
    except Exception as e:
        raise ValidationError(f"Failed to open image file '{image_path}': {e}")

    w, h = im.size

    # Validate minimum dimensions (must be at least 6x4 of 48x48 = 288x192)
    min_w = 6 * ICON_WIDTH
    min_h = 4 * ICON_HEIGHT
    if w < min_w or h < min_h:
        raise ValidationError(
            f"Image dimensions ({w}x{h}) are too small. "
            f"Grid image must be at least {min_w}x{min_h} pixels."
        )

    # Determine slicing coordinates
    x_divs = None
    y_divs = None

    if grooves_x and grooves_y:
        # User-specified explicit groove coordinates
        if len(grooves_x) != 7 or len(grooves_y) != 5:
            raise ValidationError(
                f"Explicit groove coordinates must provide 7 X values and 5 Y values. "
                f"Got {len(grooves_x)} X and {len(grooves_y)} Y."
            )
        x_divs = sorted(grooves_x)
        y_divs = sorted(grooves_y)
    elif mode in ("auto", "grooves"):
        # Check if matching known 2752x1536 preset
        if w == 2752 and h == 1536:
            x_divs = PRESET_GROOVES_2752["x"]
            y_divs = PRESET_GROOVES_2752["y"]
        else:
            # Auto-detect grooves from intensity valleys
            try:
                gray = im.convert("L")
                cand_x = find_groove_dividers(gray, 6, axis="x")
                cand_y = find_groove_dividers(gray, 4, axis="y")

                # Validate auto-detected dividers
                x_spacings = [cand_x[i+1] - cand_x[i] for i in range(6)]
                y_spacings = [cand_y[i+1] - cand_y[i] for i in range(4)]

                avg_x = sum(x_spacings) / 6.0
                avg_y = sum(y_spacings) / 4.0

                # If spacings are within 25% of average, groove detection is valid
                if all(abs(s - avg_x) / avg_x < 0.25 for s in x_spacings) and \
                   all(abs(s - avg_y) / avg_y < 0.25 for s in y_spacings):
                    x_divs = cand_x
                    y_divs = cand_y
                elif mode == "grooves":
                    raise ValidationError(
                        f"Groove detection failed: cell sizes are irregular "
                        f"(X widths: {x_spacings}, Y heights: {y_spacings})."
                    )
            except Exception as e:
                if mode == "grooves":
                    raise ValidationError(f"Groove detection error: {e}")

    # Fallback to uniform grid slicing if auto did not resolve grooves or uniform mode was chosen
    if x_divs is None or y_divs is None:
        m_left, m_top, m_right, m_bottom = margin
        usable_w = w - m_left - m_right
        usable_h = h - m_top - m_bottom
        if usable_w < min_w or usable_h < min_h:
            raise ValidationError(
                f"Image margins {margin} leave insufficient usable area ({usable_w}x{usable_h})."
            )

        step_x = usable_w / 6.0
        step_y = usable_h / 4.0

        x_divs = [int(round(m_left + i * step_x)) for i in range(7)]
        y_divs = [int(round(m_top + j * step_y)) for j in range(5)]

    # Validate dividers
    for i in range(6):
        if x_divs[i+1] <= x_divs[i]:
            raise ValidationError(f"Invalid X divider sequence at col {i}: {x_divs}")
    for j in range(4):
        if y_divs[j+1] <= y_divs[j]:
            raise ValidationError(f"Invalid Y divider sequence at row {j}: {y_divs}")

    # Extract 24 cells
    icons = []
    for row in range(4):
        for col in range(6):
            x0 = x_divs[col]
            x1 = x_divs[col + 1]
            y0 = y_divs[row]
            y1 = y_divs[row + 1]

            cell = im.crop((x0, y0, x1, y1))
            cw, ch = cell.size
            if cw < 8 or ch < 8:
                raise ValidationError(f"Cell ({col}, {row}) has invalid dimensions: {cw}x{ch}")

            # Check aspect ratio distortion
            aspect = cw / float(ch)
            if aspect < 0.65 or aspect > 1.50:
                print(f"Warning: Cell ({col}, {row}) has distorted aspect ratio ({aspect:.2f}).", file=sys.stderr)

            # Resample to 48x48
            res = cell.resize((ICON_WIDTH, ICON_HEIGHT), Image.Resampling.LANCZOS)
            icons.append(res)

    return icons


def load_icon_directory(dir_path):
    """
    Loads 24 individual icon images from a directory.
    Files are matched by numeric index 00..23 (e.g. 00_New.png, 01_Open.png, 00.png, etc.).
    Returns a list of 24 PIL Image objects (48x48).
    """
    if not os.path.isdir(dir_path):
        raise ValidationError(f"Icon directory does not exist: {dir_path}")

    # Scan for image files
    valid_exts = {".png", ".jpg", ".jpeg", ".bmp", ".webp", ".tif", ".tiff"}
    files = [f for f in os.listdir(dir_path) if os.path.splitext(f.lower())[1] in valid_exts]

    if not files:
        raise ValidationError(f"No valid image files found in directory: {dir_path}")

    indexed_images = {}
    for fname in files:
        # Match leading integer (e.g., '00_New.png', '0_Open.png', 'icon_05.png')
        m = re.match(r"^(?:icon[_-]?)?(\d+)", fname, re.IGNORECASE)
        if m:
            idx = int(m.group(1))
            if 0 <= idx < TOTAL_ICON_COUNT:
                if idx in indexed_images:
                    print(f"Warning: Duplicate icon index {idx} ('{fname}' and '{indexed_images[idx]}'). Using '{fname}'.", file=sys.stderr)
                indexed_images[idx] = fname

    missing = [i for i in range(TOTAL_ICON_COUNT) if i not in indexed_images]
    if missing:
        missing_names = [CANONICAL_ICONS[i][1] for i in missing]
        raise ValidationError(
            f"Directory is missing {len(missing)} required icons (expected 24 icons 00..23):\n"
            f"Missing indices: {missing_names}"
        )

    # Load and resample each to 48x48
    icons = []
    for i in range(TOTAL_ICON_COUNT):
        fpath = os.path.join(dir_path, indexed_images[i])
        try:
            im = Image.open(fpath)
            im.load()
        except Exception as e:
            raise ValidationError(f"Failed to read icon file '{fpath}': {e}")

        w, h = im.size
        if w < 16 or h < 16:
            raise ValidationError(f"Icon file '{fpath}' is too small ({w}x{h}). Minimum size is 16x16.")

        aspect = w / float(h)
        if aspect < 0.75 or aspect > 1.33:
            print(f"Warning: Icon '{indexed_images[i]}' is not square ({w}x{h}, aspect {aspect:.2f}).", file=sys.stderr)

        if (w, h) != (ICON_WIDTH, ICON_HEIGHT):
            im = im.resize((ICON_WIDTH, ICON_HEIGHT), Image.Resampling.LANCZOS)
        icons.append(im)

    return icons


def pack_icons_to_dat(icons, output_dat_path):
    """
    Packs 24 48x48 icons into 32-bit BGRA Little-Endian DWORD binary format.
    Validates output size is exactly 221,184 bytes.
    """
    if len(icons) != TOTAL_ICON_COUNT:
        raise ValidationError(f"Cannot pack: expected {TOTAL_ICON_COUNT} icons, got {len(icons)}")

    raw_bytes = bytearray()
    for idx, im in enumerate(icons):
        if im.size != (ICON_WIDTH, ICON_HEIGHT):
            im = im.resize((ICON_WIDTH, ICON_HEIGHT), Image.Resampling.LANCZOS)

        # Convert to RGBA for clean channel extraction
        rgba = im.convert("RGBA")
        for y in range(ICON_HEIGHT):
            for x in range(ICON_WIDTH):
                r, g, b, a = rgba.getpixel((x, y))
                # BGRA Little-Endian: [B, G, R, A]
                raw_bytes.extend(struct.pack("<BBBB", b, g, r, a))

    if len(raw_bytes) != EXPECTED_DAT_SIZE:
        raise ValidationError(
            f"Generated binary size mismatch: {len(raw_bytes)} bytes "
            f"(expected exactly {EXPECTED_DAT_SIZE} bytes)."
        )

    os.makedirs(os.path.dirname(os.path.abspath(output_dat_path)), exist_ok=True)
    with open(output_dat_path, "wb") as f:
        f.write(raw_bytes)

    return len(raw_bytes)


def generate_companion_bmps(icons, output_dir):
    """
    Generates the companion BMP icon strips in output_dir:
      - toolbar_icons_24.bmp: 17 icons @ 24x24 (408x24)
      - toolbar_icons_48.bmp: 17 icons @ 48x48 (816x48)
      - tab_icons_16.bmp:     4 tab icons (17..20) @ 16x16 (64x16)
      - tab_icons_32.bmp:     4 tab icons (17..20) @ 32x32 (128x32)
    """
    os.makedirs(output_dir, exist_ok=True)
    results = {}

    # 1. toolbar_icons_48.bmp (icons 0..16 @ 48x48)
    tb48 = Image.new("RGB", (17 * 48, 48))
    for i in range(17):
        im_rgb = icons[i].convert("RGB")
        tb48.paste(im_rgb, (i * 48, 0))
    p_tb48 = os.path.join(output_dir, "toolbar_icons_48.bmp")
    tb48.save(p_tb48, "BMP")
    results["toolbar_icons_48"] = (p_tb48, tb48.size)

    # 2. toolbar_icons_24.bmp (icons 0..16 @ 24x24)
    tb24 = Image.new("RGB", (17 * 24, 24))
    for i in range(17):
        im24 = icons[i].resize((24, 24), Image.Resampling.LANCZOS).convert("RGB")
        tb24.paste(im24, (i * 24, 0))
    p_tb24 = os.path.join(output_dir, "toolbar_icons_24.bmp")
    tb24.save(p_tb24, "BMP")
    results["toolbar_icons_24"] = (p_tb24, tb24.size)

    # 3. tab_icons_32.bmp (tab icons 17..20 @ 32x32)
    tab32 = Image.new("RGB", (4 * 32, 32))
    for i in range(4):
        im32 = icons[17 + i].resize((32, 32), Image.Resampling.LANCZOS).convert("RGB")
        tab32.paste(im32, (i * 32, 0))
    p_tab32 = os.path.join(output_dir, "tab_icons_32.bmp")
    tab32.save(p_tab32, "BMP")
    results["tab_icons_32"] = (p_tab32, tab32.size)

    # 4. tab_icons_16.bmp (tab icons 17..20 @ 16x16)
    tab16 = Image.new("RGB", (4 * 16, 16))
    for i in range(4):
        im16 = icons[17 + i].resize((16, 16), Image.Resampling.LANCZOS).convert("RGB")
        tab16.paste(im16, (i * 16, 0))
    p_tab16 = os.path.join(output_dir, "tab_icons_16.bmp")
    tab16.save(p_tab16, "BMP")
    results["tab_icons_16"] = (p_tab16, tab16.size)

    return results


def save_preview_grid(icons, preview_path, scale=2):
    """
    Saves a contact sheet grid showing all 24 icons with boundaries for visual verification.
    """
    os.makedirs(os.path.dirname(os.path.abspath(preview_path)), exist_ok=True)
    cell_size = ICON_WIDTH * scale
    pad = 4 * scale
    cols = 6
    rows = 4

    grid_w = cols * cell_size + (cols + 1) * pad
    grid_h = rows * cell_size + (rows + 1) * pad

    sheet = Image.new("RGB", (grid_w, grid_h), (24, 28, 34))
    draw = ImageDraw.Draw(sheet)

    for row in range(rows):
        for col in range(cols):
            idx = row * cols + col
            x = pad + col * (cell_size + pad)
            y = pad + row * (cell_size + pad)

            # Draw cell frame
            draw.rectangle([x - 1, y - 1, x + cell_size, y + cell_size], outline=(60, 70, 85))

            # Resize icon
            im = icons[idx]
            if scale != 1:
                im_scaled = im.resize((cell_size, cell_size), Image.Resampling.NEAREST)
            else:
                im_scaled = im
            sheet.paste(im_scaled.convert("RGB"), (x, y))

    sheet.save(preview_path)
    return preview_path


def dump_icons(icons, dump_dir):
    """
    Dumps individual 48x48 PNG icons into a directory.
    """
    os.makedirs(dump_dir, exist_ok=True)
    for idx, (num, name, desc) in enumerate(CANONICAL_ICONS):
        out_path = os.path.join(dump_dir, f"{name}.png")
        icons[idx].save(out_path, "PNG")


def parse_margin(margin_str):
    """Parses margin string '10' or '10,20,10,20' into tuple (left, top, right, bottom)."""
    if not margin_str:
        return (0, 0, 0, 0)
    parts = [int(p.strip()) for p in margin_str.split(",")]
    if len(parts) == 1:
        return (parts[0], parts[0], parts[0], parts[0])
    elif len(parts) == 4:
        return tuple(parts)
    else:
        raise argparse.ArgumentTypeError("Margin must be a single integer or 4 comma-separated integers (left,top,right,bottom).")


def parse_int_list(coords_str):
    """Parses comma-separated list of integers."""
    if not coords_str:
        return None
    return [int(p.strip()) for p in coords_str.split(",")]


def generate_app_icons(logo_path, ico_targets=None, sizes=None):
    """
    Generates multi-resolution .ico files from a source logo image (e.g. data/resources/logo.png).
    Targets:
      - game/logo.ico (embedded Win32 PE resource for resynd_game.exe)
      - editor/resources/logo.ico (embedded Win32 PE resource for resynd_editor.exe)
      - data/resources/logo.ico (bundled runtime data fallback)
    """
    if ico_targets is None:
        ico_targets = DEFAULT_ICO_TARGETS
    if sizes is None:
        sizes = ICO_SIZES

    if not os.path.isfile(logo_path):
        raise ValidationError(f"Logo image file does not exist: {logo_path}")

    try:
        im = Image.open(logo_path)
        im.load()
    except Exception as e:
        raise ValidationError(f"Failed to open logo image '{logo_path}': {e}")

    im_rgba = im.convert("RGBA")
    w, h = im_rgba.size
    if w < 16 or h < 16:
        raise ValidationError(f"Logo image '{logo_path}' is too small ({w}x{h}). Minimum size is 16x16.")

    results = []
    for target in ico_targets:
        target_path = Path(target)
        target_path.parent.mkdir(parents=True, exist_ok=True)
        im_rgba.save(target_path, format="ICO", sizes=sizes)
        file_size = target_path.stat().st_size
        results.append((str(target_path), file_size))

    return results


def main():
    parser = argparse.ArgumentParser(
        description="Extract, validate, and compile ReSyndicated editor button icons and application .ico icons."
    )
    parser.add_argument(
        "-i", "--input",
        default="data/resources/editor_icons.png",
        help="Input 6x4 icon grid image or directory of 24 icon images (default: data/resources/editor_icons.png)."
    )
    parser.add_argument(
        "-l", "--logo",
        default=DEFAULT_LOGO_PATH,
        help=f"Path to source logo image (.png) for generating application .ico files (default: {DEFAULT_LOGO_PATH})."
    )
    parser.add_argument(
        "--no-editor",
        action="store_true",
        help="Skip generating editor toolbar and tab icons."
    )
    parser.add_argument(
        "--no-logo",
        action="store_true",
        help="Skip generating application .ico files."
    )
    parser.add_argument(
        "--ico-targets",
        type=lambda s: [p.strip() for p in s.split(",") if p.strip()],
        help="Custom comma-separated list of output .ico paths (default: game/logo.ico, editor/resources/logo.ico, data/resources/logo.ico)."
    )
    parser.add_argument(
        "-o", "--output-dat",
        default="editor/resources/icons_master_48.dat",
        help="Path for generated master binary icons file (default: editor/resources/icons_master_48.dat)."
    )
    parser.add_argument(
        "--bmps-dir",
        default="editor/resources",
        help="Directory to write companion BMP strips (default: editor/resources)."
    )
    parser.add_argument(
        "--no-bmps",
        action="store_true",
        help="Skip updating companion BMP strips (toolbar_icons_*.bmp, tab_icons_*.bmp)."
    )
    parser.add_argument(
        "--mode",
        choices=["auto", "grooves", "uniform"],
        default="auto",
        help="Grid slicing mode: 'auto' (detect grooves or fallback), 'grooves' (beveled panel valleys), 'uniform' (even division)."
    )
    parser.add_argument(
        "--margin",
        type=parse_margin,
        default=(0, 0, 0, 0),
        help="Margins to trim before uniform grid slicing: 'M' or 'left,top,right,bottom'."
    )
    parser.add_argument(
        "--grooves-x",
        type=parse_int_list,
        help="Explicit X groove coordinates as 7 comma-separated integers (e.g. 170,568,976,1385,1793,2202,2610)."
    )
    parser.add_argument(
        "--grooves-y",
        type=parse_int_list,
        help="Explicit Y groove coordinates as 5 comma-separated integers (e.g. 43,401,761,1116,1483)."
    )
    parser.add_argument(
        "--preview",
        default="scratch/icons_preview_grid.png",
        help="Path to save preview contact sheet (default: scratch/icons_preview_grid.png)."
    )
    parser.add_argument(
        "--dump-dir",
        help="Optional directory to save individual 48x48 PNG icon files (e.g. scratch/extracted_icons)."
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable verbose progress output."
    )

    args = parser.parse_args()

    # Find project root if running from elsewhere
    workspace_root = Path(__file__).resolve().parent.parent.parent
    os.chdir(workspace_root)

    print("=" * 60)
    print("ReSyndicated Asset Pipeline - Icon Generator & Compiler")
    print("=" * 60)

    try:
        # 1. Process Editor Icons (unless --no-editor)
        if not args.no_editor:
            input_path = os.path.normpath(args.input)
            if not os.path.exists(input_path):
                alt_input = os.path.normpath(os.path.join(workspace_root, input_path))
                if os.path.exists(alt_input):
                    input_path = alt_input
                else:
                    # Check fallback directory
                    fallback_dir = os.path.normpath(os.path.join(workspace_root, "data", "resources", "editor_icons.png"))
                    if os.path.exists(fallback_dir):
                        print(f"Note: Input '{input_path}' not found. Falling back to '{os.path.relpath(fallback_dir, workspace_root)}'")
                        input_path = fallback_dir

            print(f"Editor icon source: {input_path}")
            print(f"Output dat:         {args.output_dat}")
            if not args.no_bmps:
                print(f"Output BMPs dir:    {args.bmps_dir}")
            print("-" * 60)

            if os.path.isdir(input_path):
                print(f"[1/4] Processing directory mode: scanning 24 icon images...")
                icons = load_icon_directory(input_path)
            elif os.path.isfile(input_path):
                print(f"[1/4] Processing grid sheet mode (mode={args.mode})...")
                icons = slice_grid_image(
                    input_path,
                    mode=args.mode,
                    margin=args.margin,
                    grooves_x=args.grooves_x,
                    grooves_y=args.grooves_y
                )
            else:
                raise ValidationError(f"Input path does not exist: {input_path}")

            print(f"       Successfully extracted and resampled {len(icons)} icons to {ICON_WIDTH}x{ICON_HEIGHT}px.")

            # Compile icons_master_48.dat
            print(f"[2/4] Packaging binary master resource...")
            bytes_written = pack_icons_to_dat(icons, args.output_dat)
            print(f"       Wrote '{args.output_dat}' ({bytes_written:,} bytes, 24 icons @ 32-bit BGRA).")

            # Generate companion BMP strips
            if not args.no_bmps:
                print(f"[3/4] Updating companion BMP icon strips...")
                bmps = generate_companion_bmps(icons, args.bmps_dir)
                for key, (path, dims) in bmps.items():
                    print(f"       Generated {os.path.basename(path)} ({dims[0]}x{dims[1]}px)")
            else:
                print(f"[3/4] Skipping companion BMP strips (--no-bmps).")

            # Visual preview contact sheet & dump
            if args.preview:
                prev = save_preview_grid(icons, args.preview, scale=2)
                print(f"       Preview contact sheet saved: '{prev}'")

            if args.dump_dir:
                dump_icons(icons, args.dump_dir)
                print(f"       Extracted individual PNG icons to: '{args.dump_dir}'")
        else:
            print("[Editor Icons] Skipped (--no-editor).")

        # 2. Process Application .ico Icons (unless --no-logo)
        if not args.no_logo:
            logo_path = os.path.normpath(args.logo)
            if not os.path.exists(logo_path):
                alt_logo = os.path.normpath(os.path.join(workspace_root, logo_path))
                if os.path.exists(alt_logo):
                    logo_path = alt_logo
                else:
                    raise ValidationError(f"Logo image file does not exist: {logo_path}")

            print("-" * 60)
            print(f"[4/4] Generating multi-resolution Windows .ico icons from '{os.path.relpath(logo_path, workspace_root)}'...")
            targets = args.ico_targets if args.ico_targets else DEFAULT_ICO_TARGETS
            ico_results = generate_app_icons(logo_path, ico_targets=targets)
            sizes_str = ", ".join(f"{w}x{h}" for w, h in ICO_SIZES)
            print(f"       Resolutions included: {sizes_str}")
            for path, fsize in ico_results:
                rel = os.path.relpath(path, workspace_root) if os.path.isabs(path) else path
                print(f"       Wrote '{rel}' ({fsize:,} bytes)")
        else:
            print("[App Icons] Skipped (--no-logo).")

        # 3. Touch resource files for build tracking
        rc_files = [
            os.path.join(workspace_root, "editor", "resource.rc"),
            os.path.join(workspace_root, "game", "resynd.rc")
        ]
        touched = []
        for rc_path in rc_files:
            if os.path.isfile(rc_path):
                os.utime(rc_path, None)
                touched.append(os.path.relpath(rc_path, workspace_root))
        if touched:
            print(f"       Updated timestamps on {', '.join(touched)} for build re-linking.")

        print("=" * 60)
        print("Success: All icons generated and validated successfully!")
        print("=" * 60)
        return 0

    except ValidationError as err:
        print(f"\nVALIDATION ERROR:\n  {err}\n", file=sys.stderr)
        return 1
    except Exception as err:
        print(f"\nUNEXPECTED ERROR:\n  {err}\n", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())