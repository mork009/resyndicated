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
GDI Screen Capture and Visual Diff Generator for Comparison Harness.
Captures windows in background without desktop focus requirements,
and generates side-by-side visual comparisons and animated GIFs.
"""

import os
import ctypes
import ctypes.wintypes
from PIL import Image, ImageDraw, ImageFont, ImageChops

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", ctypes.wintypes.DWORD),
        ("biWidth", ctypes.wintypes.LONG),
        ("biHeight", ctypes.wintypes.LONG),
        ("biPlanes", ctypes.wintypes.WORD),
        ("biBitCount", ctypes.wintypes.WORD),
        ("biCompression", ctypes.wintypes.DWORD),
        ("biSizeImage", ctypes.wintypes.DWORD),
        ("biXPelsPerMeter", ctypes.wintypes.LONG),
        ("biYPelsPerMeter", ctypes.wintypes.LONG),
        ("biClrUsed", ctypes.wintypes.DWORD),
        ("biClrImportant", ctypes.wintypes.DWORD),
    ]


def capture_window_hwnd(hwnd):
    """Captures client area of a window handle using Windows GDI."""
    rect = ctypes.wintypes.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rect))
    w = rect.right - rect.left
    h = rect.bottom - rect.top
    if w <= 0 or h <= 0:
        return None

    hwndDC = user32.GetDC(hwnd)
    memDC = gdi32.CreateCompatibleDC(hwndDC)
    hBitmap = gdi32.CreateCompatibleBitmap(hwndDC, w, h)
    oldBitmap = gdi32.SelectObject(memDC, hBitmap)

    # Try PW_RENDERFULLCONTENT (2), fallback to 0
    res = user32.PrintWindow(hwnd, memDC, 2)
    if not res:
        res = user32.PrintWindow(hwnd, memDC, 0)

    bmi = BITMAPINFOHEADER()
    bmi.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bmi.biWidth = w
    bmi.biHeight = -h  # top-down DIB
    bmi.biPlanes = 1
    bmi.biBitCount = 32
    bmi.biCompression = 0

    buf = ctypes.create_string_buffer(w * h * 4)
    gdi32.GetDIBits(memDC, hBitmap, 0, h, buf, ctypes.byref(bmi), 0)

    gdi32.SelectObject(memDC, oldBitmap)
    gdi32.DeleteObject(hBitmap)
    gdi32.DeleteDC(memDC)
    user32.ReleaseDC(hwnd, hwndDC)

    return Image.frombuffer("RGBA", (w, h), buf, "raw", "BGRA", 0, 1).convert("RGB")


def create_side_by_side(img_dos, img_rs, label_dos="Original DOS", label_rs="ReSyndicated", timestamp_str=""):
    """
    Creates a side-by-side comparison image of equal height,
    with title bars and optional timestamp.
    """
    target_h = max(img_dos.height, img_rs.height)
    scale_dos = target_h / img_dos.height
    scale_rs = target_h / img_rs.height

    w_dos = int(img_dos.width * scale_dos)
    w_rs = int(img_rs.width * scale_rs)

    r_dos = img_dos.resize((w_dos, target_h), Image.Resampling.NEAREST)
    r_rs = img_rs.resize((w_rs, target_h), Image.Resampling.NEAREST)

    header_h = 32
    total_w = w_dos + w_rs + 10
    total_h = target_h + header_h

    canvas = Image.new("RGB", (total_w, total_h), (20, 24, 28))
    draw = ImageDraw.Draw(canvas)

    # Paste images
    canvas.paste(r_dos, (0, header_h))
    canvas.paste(r_rs, (w_dos + 10, header_h))

    # Divider bar
    draw.rectangle([w_dos, 0, w_dos + 9, total_h], fill=(45, 52, 60))

    # Draw header text
    text_dos = f"{label_dos} {timestamp_str}".strip()
    text_rs = f"{label_rs} {timestamp_str}".strip()

    draw.text((12, 8), text_dos, fill=(240, 200, 80))
    draw.text((w_dos + 22, 8), text_rs, fill=(80, 200, 240))

    return canvas


def create_side_by_side_gif(frames_dos, frames_rs, output_path, fps=5):
    """Generates an animated comparison GIF from sequences of (time_ms, image) tuples."""
    combined_frames = []
    min_len = min(len(frames_dos), len(frames_rs))

    for i in range(min_len):
        t_dos, img_dos = frames_dos[i]
        t_rs, img_rs = frames_rs[i]
        t_label = f"[{t_dos/1000.0:.1f}s]"
        c = create_side_by_side(img_dos, img_rs, timestamp_str=t_label)
        combined_frames.append(c)

    if combined_frames:
        os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
        duration_ms = int(1000 / fps)
        combined_frames[0].save(
            output_path,
            save_all=True,
            append_images=combined_frames[1:],
            duration=duration_ms,
            loop=0
        )