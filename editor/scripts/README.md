# ReSyndicated Asset Pipeline - Icon Generator & Compiler

The script `editor/scripts/generate_editor_icons.py` extracts, resamples, validates, and packages button icons for the ReSyndicated Mission Editor from the source grid bitmap and generates multi-resolution application `.ico` icons from the master logo.

---

## Output Targets

When run, the script updates:
1. **`editor/resources/icons_master_48.dat`**:
   The primary binary asset packaged into `resynd_editor.exe`. It contains 24 sequential 48×48 icons in 32-bit BGRA Little-Endian format, totaling exactly **221,184 bytes**.
2. **Companion BMP Strips**:
   - `editor/resources/toolbar_icons_24.bmp` (408×24 px, 17 toolbar icons)
   - `editor/resources/toolbar_icons_48.bmp` (816×48 px, 17 toolbar icons)
   - `editor/resources/tab_icons_16.bmp` (64×16 px, 4 tab icons)
   - `editor/resources/tab_icons_32.bmp` (128×32 px, 4 tab icons)
3. **Application Executable & Data `.ico` Icons** (from `data/resources/logo.png`):
   - `game/logo.ico` (Win32 embedded PE resource for `resynd_game.exe`)
   - `editor/resources/logo.ico` (Win32 embedded PE resource for `resynd_editor.exe`)
   - `data/resources/logo.ico` (ReSyndicated runtime data icon & bundled data file)
   - Contains 7 standard resolutions: 256×256, 128×128, 64×64, 48×48, 32×32, 24×24, 16×16.
   - Automatically touches `game/resynd.rc` and `editor/resource.rc` so MSVC `rc.exe` and CMake/Ninja recompile resources on subsequent builds.
4. **Visual Verification Contact Sheet**:
   - `scratch/icons_preview_grid.png` (2× scaled contact sheet showing all 24 icons with grid frames).

---

## Supported Input Formats

### 1. Grid Image Sheet Mode (Default)
Provide a single raster image file (`.png`, `.jpg`, `.jpeg`, `.bmp`, `.webp`) containing 24 icons arranged in **6 columns × 4 rows** (default: `data/resources/editor_icons.png`).

#### Requirements:
- **Minimum Dimensions**: Width $\ge 288$ px ($6 \times 48$), Height $\ge 192$ px ($4 \times 48$). Higher resolution images (e.g. $2752 \times 1536$ or $1024 \times 687$) are downscaled with Lanczos filtering.
- **Icon Ordering**:
  - **Row 0**: `00_New`, `01_Open`, `02_Save`, `03_Undo`, `04_Redo`, `05_Pencil`
  - **Row 1**: `06_BoxFill`, `07_Picker`, `08_Select`, `09_Slice`, `10_LayerUp`, `11_LayerDown`
  - **Row 2**: `12_Spawns`, `13_Playtest`, `14_ZoomIn`, `15_ZoomOut`, `16_Zoom100`, `17_TabTiles`
  - **Row 3**: `18_TabEntities`, `19_TabObjectives`, `20_Settings`, `21_Search`, `22_Trash`, `23_Computer`
- **Slicing Modes (`--mode`)**:
  - `auto` (default): Checks for known presets, auto-detects dark separator grooves if present, or falls back to uniform slicing.
  - `grooves`: Specifically locates dark groove valleys between beveled/embossed icon panels.
  - `uniform`: Divides the image evenly into $6 \times 4$ cells (supports `--margin` to trim outer borders).

### 2. Directory Mode
Optionally provide a directory containing 24 individual icon images.

#### Requirements:
- Must contain 24 image files numbered `00` through `23` (e.g. `00_New.png`, `01_Open.png`, ... `23_Computer.png`).
- Minimum size: $16 \times 16$ px per icon.
- Aspect ratio: Square ($1:1$) within 15% tolerance.

---

## Usage

### Via VS Code Task
1. Press `Ctrl+Shift+P` (or go to **Terminal** -> **Run Task...**).
2. Select **Generate All Icons**.
3. Accept or customize the prompts:
   - Editor icon source (default: `data/resources/editor_icons.png`)
   - Logo source image (default: `data/resources/logo.png`)

### Via Command Line
```bash
# Default execution (processes data/resources/editor_icons.png and data/resources/logo.png)
python editor/scripts/generate_editor_icons.py

# Process custom grid sheet and custom logo
python editor/scripts/generate_editor_icons.py --input path/to/my_icons.png --logo path/to/logo.png

# Only regenerate .ico icons from logo.png (skip editor icons)
python editor/scripts/generate_editor_icons.py --no-editor

# Only regenerate editor icons (skip .ico icons)
python editor/scripts/generate_editor_icons.py --no-logo

# Slice the grid sheet and dump extracted 48x48 PNGs for inspection
python editor/scripts/generate_editor_icons.py --dump-dir scratch/extracted_icons
```
