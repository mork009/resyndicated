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
DOSBox Environment Controller for ReSyndicated Comparison Test Harness.
Manages deterministic execution, input posting, and window lifecycle.
"""

import os
import sys
import time
import subprocess
import ctypes
import ctypes.wintypes

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32
import shutil

# Windows Virtual Keys
VK_RETURN = 0x0D
VK_SPACE = 0x20
VK_ESCAPE = 0x1B
VK_F1 = 0x70
VK_F2 = 0x71
VK_F3 = 0x72
VK_F4 = 0x73
VK_F5 = 0x74
VK_CONTROL = 0x11

# Windows Messages
WM_ACTIVATE = 0x0006
WA_ACTIVE = 1
WM_SETFOCUS = 0x0007
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_MOUSEMOVE = 0x0200
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_RBUTTONDOWN = 0x0204
WM_RBUTTONUP = 0x0205

# Common Scancodes
SCANCODES = {
    VK_SPACE: 0x39,
    VK_RETURN: 0x1C,
    VK_ESCAPE: 0x01,
    VK_F1: 0x3B,
    VK_F2: 0x3C,
    VK_F3: 0x3D,
    VK_F4: 0x3E,
    VK_F5: 0x3F,
    ord('T'): 0x14,
    ord('E'): 0x12,
    ord('S'): 0x1F,
}


def find_dosbox_executable():
    """Finds the DOSBox executable via environment variables, PATH, or standard install dirs."""
    # 1. Direct environment variable override
    for env_var in ("DOSBOX_EXE", "DOSBOX_PATH"):
        custom = os.environ.get(env_var)
        if custom and os.path.isfile(custom):
            return custom

    # 2. Command PATH resolution (e.g. winget, scoop, choco, or user PATH)
    which_path = shutil.which("dosbox") or shutil.which("dosbox.exe")
    if which_path:
        return which_path

    # 3. Dynamic search across system Program Files and AppData directories
    program_roots = []
    for var in ("ProgramFiles(x86)", "ProgramFiles", "ProgramW6432", "LOCALAPPDATA"):
        val = os.environ.get(var)
        if val and os.path.isdir(val):
            program_roots.append(val)

    candidate_subdirs = [
        "DOSBox-0.74-3",
        "DOSBox-0.74",
        "DOSBox-0.74-2",
        "DOSBox",
        "DOSBox-X",
        os.path.join("Programs", "DOSBox"),
    ]

    for root in program_roots:
        for sub in candidate_subdirs:
            candidate = os.path.join(root, sub, "DOSBox.exe")
            if os.path.isfile(candidate):
                return candidate

    return None


def get_configured_dosbox_cycles(workspace_dir):
    """Reads the cycle setting from synd/dosbox.conf, defaulting to 8000."""
    conf_path = os.path.join(workspace_dir, "synd", "dosbox.conf")
    if os.path.exists(conf_path):
        try:
            with open(conf_path, "r", encoding="utf-8", errors="ignore") as f:
                for line in f:
                    line = line.strip()
                    if line.startswith("cycles="):
                        val = line.split("=", 1)[1].strip()
                        if val.isdigit():
                            return int(val)
        except Exception:
            pass
    return 8000


class DOSBoxSession:
    def __init__(self, workspace_dir=None, cycles=None, silent=True):
        if workspace_dir is None:
            self.workspace_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
        else:
            self.workspace_dir = os.path.abspath(workspace_dir)

        self.synd_dir = os.path.join(self.workspace_dir, "synd")
        self.dosbox_path = find_dosbox_executable()
        if not self.dosbox_path:
            raise FileNotFoundError("DOSBox executable not found in default paths.")

        self.cycles = cycles if cycles is not None else get_configured_dosbox_cycles(self.workspace_dir)
        self.silent = silent
        self.proc = None
        self.hwnd = None
        self.conf_path = os.path.join(self.workspace_dir, "scratch", "harness_dosbox.conf")

    def ensure_intro_bypass(self):
        """Ensures a dummy intro.com exists in synd/ so main.exe skips intro cinematic instantly."""
        intro_com = os.path.join(self.synd_dir, "intro.com")
        if not os.path.exists(intro_com):
            with open(intro_com, "wb") as f:
                f.write(b"\xb8\x00\x4c\xcd\x21")  # MOV AX, 4C00h; INT 21h

    def ensure_save_directory(self):
        """Ensures synd/save exists and has default savegame files."""
        save_dir = os.path.join(self.synd_dir, "save")
        os.makedirs(save_dir, exist_ok=True)
        src_00 = os.path.join(self.synd_dir, "data", "00.gam")
        dst_00 = os.path.join(save_dir, "00.gam")
        if os.path.exists(src_00) and not os.path.exists(dst_00):
            import shutil
            shutil.copyfile(src_00, dst_00)

    def write_conf(self):
        os.makedirs(os.path.dirname(self.conf_path), exist_ok=True)
        silent_flag = "/s" if self.silent else ""
        content = f"""[sdl]
fullscreen=false
windowresolution=original
output=surface
autolock=false
priority=higher,higher

[cpu]
core=auto
cycles=fixed {self.cycles}

[dosbox]
memsize=16

[autoexec]
mount c "{self.workspace_dir}"
c:
cd synd
main.exe {silent_flag} /c0
"""
        with open(self.conf_path, "w", encoding="utf-8") as f:
            f.write(content)

    def start(self, wait_sec=3.0):
        self.ensure_intro_bypass()
        self.ensure_save_directory()
        self.write_conf()

        cmd = [self.dosbox_path, "-noconsole", "-conf", self.conf_path]
        self.proc = subprocess.Popen(cmd)
        time.sleep(wait_sec)

        self.hwnd = self._find_window()
        if not self.hwnd:
            self.stop()
            raise RuntimeError("Failed to locate DOSBox window handle.")

        # Activate window in SDL message queue
        user32.PostMessageW(self.hwnd, WM_ACTIVATE, WA_ACTIVE, 0)
        user32.PostMessageW(self.hwnd, WM_SETFOCUS, 0, 0)
        return self.hwnd

    def _find_window(self):
        found = []
        def enum_cb(hwnd, lparam):
            length = user32.GetWindowTextLengthW(hwnd)
            if length > 0:
                buff = ctypes.create_unicode_buffer(length + 1)
                user32.GetWindowTextW(hwnd, buff, length + 1)
                if "DOSBox" in buff.value:
                    found.append(hwnd)
            return True

        cb = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_int, ctypes.c_int)(enum_cb)
        user32.EnumWindows(cb, 0)
        return found[0] if found else None

    def post_key(self, vk, scan=None, hold_sec=0.05):
        if scan is None:
            scan = SCANCODES.get(vk, 0)
        user32.PostMessageW(self.hwnd, WM_KEYDOWN, vk, (scan << 16) | 1)
        time.sleep(hold_sec)
        user32.PostMessageW(self.hwnd, WM_KEYUP, vk, (scan << 16) | 0xC0000001)
        time.sleep(0.05)

    def post_mouse_move(self, x, y):
        lParam = (y << 16) | (x & 0xFFFF)
        user32.PostMessageW(self.hwnd, WM_MOUSEMOVE, 0, lParam)

    def post_mouse_click(self, x, y, button="left", hold_sec=0.15):
        lParam = (y << 16) | (x & 0xFFFF)
        self.post_mouse_move(x, y)
        time.sleep(0.05)

        msg_down = WM_LBUTTONDOWN if button == "left" else WM_RBUTTONDOWN
        msg_up = WM_LBUTTONUP if button == "left" else WM_RBUTTONUP
        wparam = 1 if button == "left" else 2

        user32.PostMessageW(self.hwnd, msg_down, wparam, lParam)
        time.sleep(hold_sec)
        user32.PostMessageW(self.hwnd, msg_up, 0, lParam)
        time.sleep(0.05)

    def skip_title(self, delay_sec=1.5):
        self.post_key(VK_SPACE, hold_sec=0.05)
        time.sleep(delay_sec)

    def stop(self):
        if self.proc:
            try:
                self.proc.terminate()
                self.proc.wait(timeout=2)
            except Exception:
                try:
                    self.proc.kill()
                except Exception:
                    pass
            self.proc = None
            self.hwnd = None

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()