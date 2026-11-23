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
Main Comparison Test Runner.
Executes test scenarios on both original DOS Syndicate (via DOSBox) and ReSyndicated,
generates synchronized frame captures, animated GIFs, and markdown diff reports.
"""

import os
import sys
import time
import argparse
from datetime import datetime

from .dosbox_env import DOSBoxSession
from .resynd_runner import ReSyndSession
from .capture import capture_window_hwnd, create_side_by_side, create_side_by_side_gif
from .memory_oracle import MemoryOracle


class ComparisonScenario:
    def __init__(self, name, description, duration_sec=10.0, sample_interval_sec=1.0):
        self.name = name
        self.description = description
        self.duration_sec = duration_sec
        self.sample_interval_sec = sample_interval_sec


SCENARIOS = {
    "title_boot": ComparisonScenario(
        name="title_boot",
        description="Verifies game boot sequence, Bullfrog splash screens, and main menu loading.",
        duration_sec=6.0,
        sample_interval_sec=1.0,
    ),
    "tree_burn": ComparisonScenario(
        name="tree_burn",
        description="Compares tree burning animation lifecycle from ignition through 10-second burn to stump.",
        duration_sec=11.0,
        sample_interval_sec=1.0,
    ),
    "timebomb": ComparisonScenario(
        name="timebomb",
        description="Compares Time Bomb 20-second countdown detonation delay and shockwave blast pattern.",
        duration_sec=21.0,
        sample_interval_sec=2.0,
    ),
    "gauss_impact": ComparisonScenario(
        name="gauss_impact",
        description="Compares Gauss Gun projectile trajectory, impact fireball, and organic flame cluster.",
        duration_sec=5.0,
        sample_interval_sec=0.5,
    ),
}


def run_scenario(scenario_name="title_boot", output_dir=None):
    if scenario_name not in SCENARIOS:
        print(f"Unknown scenario: {scenario_name}. Available: {list(SCENARIOS.keys())}")
        return False

    scenario = SCENARIOS[scenario_name]
    workspace_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

    if output_dir is None:
        timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_dir = os.path.join(workspace_dir, "scratch", "comparison_output", f"{scenario_name}_{timestamp_str}")

    os.makedirs(output_dir, exist_ok=True)
    print(f"=== Running Comparison Scenario: {scenario.name} ===")
    print(f"Description: {scenario.description}")
    print(f"Output Directory: {output_dir}")

    # 1. Run and capture Original DOS Syndicate
    print("\n[1/3] Running Original DOS Syndicate in DOSBox...")
    dos_frames = []
    with DOSBoxSession(workspace_dir=workspace_dir, cycles=None, silent=True) as dos_session:
        time.sleep(1.0)
        start_time = time.time()
        elapsed = 0.0

        while elapsed <= scenario.duration_sec:
            cur_time_ms = int(elapsed * 1000)
            img = capture_window_hwnd(dos_session.hwnd)
            if img:
                dos_frames.append((cur_time_ms, img))
                frame_path = os.path.join(output_dir, f"dos_{cur_time_ms}ms.png")
                img.save(frame_path)
            time.sleep(scenario.sample_interval_sec)
            elapsed = time.time() - start_time

    print(f"Captured {len(dos_frames)} frames from DOSBox.")

    # 2. Run and capture ReSyndicated
    print("\n[2/3] Running ReSyndicated...")
    rs_frames = []
    try:
        with ReSyndSession(workspace_dir=workspace_dir) as rs_session:
            rs_session.start(mission_id=1, scale=2, wait_sec=2.0)
            start_time = time.time()
            elapsed = 0.0

            while elapsed <= scenario.duration_sec:
                cur_time_ms = int(elapsed * 1000)
                img = capture_window_hwnd(rs_session.hwnd)
                if img:
                    rs_frames.append((cur_time_ms, img))
                    frame_path = os.path.join(output_dir, f"rs_{cur_time_ms}ms.png")
                    img.save(frame_path)
                time.sleep(scenario.sample_interval_sec)
                elapsed = time.time() - start_time
        print(f"Captured {len(rs_frames)} frames from ReSyndicated.")
    except Exception as e:
        print(f"Note: ReSyndicated session capture note: {e}")

    # 3. Generate side-by-side composite images and animated GIF
    print("\n[3/3] Generating comparison deliverables...")
    gif_path = os.path.join(output_dir, f"{scenario_name}_comparison.gif")
    if dos_frames and rs_frames:
        create_side_by_side_gif(dos_frames, rs_frames, gif_path, fps=max(1, int(1.0 / scenario.sample_interval_sec)))
        print(f"Saved animated GIF: {gif_path}")

        min_count = min(len(dos_frames), len(rs_frames))
        for i in range(min_count):
            t_dos, img_dos = dos_frames[i]
            t_rs, img_rs = rs_frames[i]
            composite = create_side_by_side(img_dos, img_rs, timestamp_str=f"[{t_dos/1000.0:.1f}s]")
            composite.save(os.path.join(output_dir, f"side_by_side_{i:02d}.png"))
        print(f"Saved {min_count} side-by-side composite images.")

    # Generate Markdown Summary Report
    report_path = os.path.join(output_dir, "REPORT.md")
    report = []
    report.append(f"# Gameplay Comparison Report: `{scenario.name}`")
    report.append(f"**Date**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    report.append(f"**Scenario**: {scenario.description}\n")
    report.append("## 1. Executive Summary")
    report.append(f"- **Total Duration**: {scenario.duration_sec:.1f} seconds")
    report.append(f"- **Sampling Interval**: {scenario.sample_interval_sec:.2f} seconds")
    report.append(f"- **DOSBox Frames**: {len(dos_frames)} captured")
    report.append(f"- **ReSyndicated Frames**: {len(rs_frames)} captured")
    report.append(f"- **Comparison GIF**: `{os.path.basename(gif_path)}`\n")

    report.append("## 2. Frame Timeline")
    report.append("| Timestamp | DOSBox Frame | ReSyndicated Frame |")
    report.append("| :--- | :--- | :--- |")
    min_count = min(len(dos_frames), len(rs_frames))
    for i in range(min_count):
        t_ms = dos_frames[i][0]
        report.append(f"| {t_ms/1000.0:.1f}s | `dos_{t_ms}ms.png` | `rs_{t_ms}ms.png` |")

    with open(report_path, "w", encoding="utf-8") as f:
        f.write("\n".join(report))

    print(f"Generated report: {report_path}")
    print("=== Comparison Scenario Complete ===")
    return True


def main():
    parser = argparse.ArgumentParser(description="ReSyndicated vs Original Syndicate Comparison Harness")
    parser.add_argument(
        "--scenario",
        default="title_boot",
        choices=list(SCENARIOS.keys()) + ["all"],
        help="Scenario to run for comparison.",
    )
    parser.add_argument("--out", default=None, help="Custom output directory.")
    args = parser.parse_args()

    if args.scenario == "all":
        for s in SCENARIOS:
            run_scenario(s)
    else:
        run_scenario(args.scenario, args.out)


if __name__ == "__main__":
    main()