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
DOSBox Memory Oracle for ReSyndicated Comparison Test Harness.
Attaches to DOSBox.exe and reads original game state directly from emulated RAM.
"""

import struct
import ctypes
import ctypes.wintypes

kernel32 = ctypes.windll.kernel32

PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010
MEM_COMMIT = 0x1000
PAGE_READWRITE = 0x04


class MEMORY_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [
        ("BaseAddress", ctypes.c_void_p),
        ("AllocationBase", ctypes.c_void_p),
        ("AllocationProtect", ctypes.wintypes.DWORD),
        ("RegionSize", ctypes.c_size_t),
        ("State", ctypes.wintypes.DWORD),
        ("Protect", ctypes.wintypes.DWORD),
        ("Type", ctypes.wintypes.DWORD),
    ]


class MemoryOracle:
    def __init__(self, pid):
        self.pid = pid
        self.hProcess = kernel32.OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid
        )
        if not self.hProcess:
            raise PermissionError(f"Failed to open process {pid} with VM_READ rights.")

        self.ram_base = self._find_dos_ram()

    def _find_dos_ram(self):
        """Scans virtual memory to locate the ~16MB DOSBox emulated physical RAM buffer."""
        addr = 0
        target_size = 16 * 1024 * 1024
        while addr < 0x7FFFFFFF:
            mbi = MEMORY_BASIC_INFORMATION()
            res = kernel32.VirtualQueryEx(
                self.hProcess, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(mbi)
            )
            if not res:
                break
            if mbi.State == MEM_COMMIT and mbi.RegionSize >= target_size and mbi.Protect == PAGE_READWRITE:
                return mbi.BaseAddress
            addr += mbi.RegionSize
        return None

    def read_bytes(self, offset, size):
        if not self.ram_base:
            return None
        buf = ctypes.create_string_buffer(size)
        read = ctypes.c_size_t()
        target_addr = ctypes.c_void_p(self.ram_base + offset)
        res = kernel32.ReadProcessMemory(self.hProcess, target_addr, buf, size, ctypes.byref(read))
        if res:
            return bytes(buf[:read.value])
        return None

    def read_u8(self, offset):
        data = self.read_bytes(offset, 1)
        return data[0] if data else None

    def read_u16(self, offset):
        data = self.read_bytes(offset, 2)
        return struct.unpack("<H", data)[0] if data else None

    def read_u32(self, offset):
        data = self.read_bytes(offset, 4)
        return struct.unpack("<I", data)[0] if data else None

    def find_bytes(self, pattern, max_search_mb=16):
        """Searches emulated RAM for a byte pattern, returns offset relative to RAM base."""
        chunk_size = 1024 * 1024
        for mb in range(max_search_mb):
            chunk = self.read_bytes(mb * chunk_size, chunk_size)
            if chunk and pattern in chunk:
                return mb * chunk_size + chunk.find(pattern)
        return None

    def close(self):
        if self.hProcess:
            kernel32.CloseHandle(self.hProcess)
            self.hProcess = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()