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
ReSyndicated vs Original Syndicate Automated Comparison Test Harness.
"""

from .dosbox_env import DOSBoxSession
from .capture import capture_window_hwnd, create_side_by_side, create_side_by_side_gif
from .memory_oracle import MemoryOracle
from .resynd_runner import ReSyndSession

__all__ = [
    "DOSBoxSession",
    "capture_window_hwnd",
    "create_side_by_side",
    "create_side_by_side_gif",
    "MemoryOracle",
    "ReSyndSession",
]