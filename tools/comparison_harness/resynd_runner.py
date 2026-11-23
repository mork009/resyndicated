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
ReSyndicated Runner for Comparison Test Harness.
Launches ReSyndicated into specific missions or test scenarios, locates its window,
and captures visual frames or execution telemetry.
"""

import os
import time
import subprocess
import ctypes
import ctypes.wintypes

user32 = ctypes.windll.user32


class ReSyndSession:
    def __init__(self, workspace_dir=None, build_preset="ninja-msvc-x64-debug"):
        if workspace_dir is None:
            self.workspace_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
        else:
            self.workspace_dir = os.path.abspath(workspace_dir)

        self.exe_path = os.path.join(
            self.workspace_dir, "build", build_preset, "game", "resynd_game.exe"
        )
        if not os.path.exists(self.exe_path):
            # Check release fallback
            rel_path = os.path.join(self.workspace_dir, "build", "ninja-msvc-x64-release", "game", "resynd_game.exe")
            if os.path.exists(rel_path):
                self.exe_path = rel_path
            else:
                raise FileNotFoundError(f"ReSyndicated executable not found at {self.exe_path}")

        self.proc = None
        self.hwnd = None

    def start(self, mission_id=1, scale=2, wait_sec=2.0):
        working_dir = os.path.dirname(self.exe_path)
        cmd = [self.exe_path, "-w", str(scale), "-m", str(mission_id)]
        self.proc = subprocess.Popen(cmd, cwd=working_dir)
        time.sleep(wait_sec)

        self.hwnd = self._find_window()
        return self.hwnd

    def _find_window(self):
        found = []
        def enum_cb(hwnd, lparam):
            length = user32.GetWindowTextLengthW(hwnd)
            if length > 0:
                buff = ctypes.create_unicode_buffer(length + 1)
                user32.GetWindowTextW(hwnd, buff, length + 1)
                # ReSyndicated window title (case-insensitive)
                if "resyndicated" in buff.value.lower():
                    found.append(hwnd)
            return True

        cb = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_int, ctypes.c_int)(enum_cb)
        user32.EnumWindows(cb, 0)
        return found[0] if found else None

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
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()