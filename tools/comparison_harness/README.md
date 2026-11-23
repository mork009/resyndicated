# ReSyndicated vs Original Syndicate Automated Comparison Test Harness

This package provides an automated side-by-side test harness for executing scenarios on both **Original DOS Syndicate** (via DOSBox) and **ReSyndicated**, capturing time-synchronized frames, animated comparison GIFs, memory state telemetries, and markdown reports.

---

## 1. Architecture Overview

* **`dosbox_env.py` (`DOSBoxSession`)**:
  - Automatically manages DOSBox execution using fixed CPU cycles (`cycles=fixed 20000`) for 100% deterministic, reproducible timing.
  - Automatically installs a zero-overhead `synd/intro.com` stub to skip the 2-minute intro cinematic instantly on startup.
  - Dispatches virtual key events and mouse events into the SDL message queue via Windows `PostMessageW`.
  - Configures `priority=higher,higher` so emulation does not pause or drop cycles when the window loses focus.

* **`capture.py` (`capture_window_hwnd`, `create_side_by_side`, `create_side_by_side_gif`)**:
  - Background window capture using Windows GDI (`PrintWindow` / DIBits) without requiring desktop focus or foreground elevation.
  - Generates timestamped frame captures (`dos_<ms>ms.png`, `fs_<ms>ms.png`).
  - Stitches side-by-side composite comparison PNGs (`side_by_side_<idx>.png`).
  - Assembles animated side-by-side comparison GIFs (`<scenario>_comparison.gif`).

* **`memory_oracle.py` (`MemoryOracle`)**:
  - Attaches to `DOSBox.exe` using Windows `ReadProcessMemory` / `VirtualQueryEx`.
  - Locates the 16MB emulated DOS RAM block directly in process memory.
  - Enables reading ground-truth simulation variables directly from emulated memory (game ticks, tree burn counter, time bomb fuse, ped coordinates).

* **`resynd_runner.py` (`ReSyndSession`)**:
  - Automatically boots ReSyndicated into matching mission configurations (`resynd_game.exe -w 2 -m <id>`).
  - Finds and attaches to the ReSyndicated window handle for synchronized capture.

* **`compare_runner.py`**:
  - High-level test runner orchestrating scenario execution, timeline recording, deliverable generation, and Markdown reports.

---

## 2. Usage Instructions

### Running a Scenario via CLI
From the repository root, run:
```bash
# Run title boot comparison
python -m tools.comparison_harness.compare_runner --scenario title_boot

# Run tree burn duration comparison
python -m tools.comparison_harness.compare_runner --scenario tree_burn

# Run Time Bomb detonation delay comparison
python -m tools.comparison_harness.compare_runner --scenario timebomb

# Run Gauss Gun impact visual comparison
python -m tools.comparison_harness.compare_runner --scenario gauss_impact
```

### Output Artifacts
Test runs are saved to:
`scratch/comparison_output/<scenario_name>_<timestamp>/`
- `REPORT.md`: Comprehensive breakdown of duration, sample count, and frame timeline.
- `side_by_side_<xx>.png`: Visual side-by-side comparison images with labeled timestamps.
- `<scenario>_comparison.gif`: Animated side-by-side playback comparing animations and timings.
- Raw frames (`dos_<ms>ms.png` and `fs_<ms>ms.png`).
