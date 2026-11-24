# ReSyndicated Editor User Guide

ReSyndicated Editor (`resynd_editor.exe`) is a full-featured isometric map, entity, route, AI tactic, objective, and playtest editor for **ReSyndicated**, compatible with original Bullfrog *Syndicate*, *American Revolt*, and *Syndicate Plus* mission files.

For game controls and installation steps, see [`README.md`](README.md). For weapon and character reference tables, see [`docs/WEAPONS.md`](docs/WEAPONS.md) and [`docs/CHARACTERS.md`](docs/CHARACTERS.md).

---

## Table of Contents

1. [Launching, CLI Options & Settings](#1-launching-cli-options--settings)
2. [Workspace Layout & Navigation](#2-workspace-layout--navigation)
3. [Complete Controls, Menus & Shortcuts Reference](#3-complete-controls-menus--shortcuts-reference)
4. [Creating, Opening, Saving & Configuring Missions](#4-creating-opening-saving--configuring-missions)
5. [Isometric Tilemap Construction & Z-Layer System](#5-isometric-tilemap-construction--z-layer-system)
6. [Entities: Placement, Inspection & Gameplay Mechanics](#6-entities-placement-inspection--gameplay-mechanics)
7. [Character & Vehicle Route Tool (`R`)](#7-character--vehicle-route-tool-r)
8. [Enemy Factions & the AI Tactics System](#8-enemy-factions--the-ai-tactics-system)
9. [Mission Objectives & Completion Rules](#9-mission-objectives--completion-rules)
10. [Undo/Redo, Validation & Sandboxed Playtesting (`F5`)](#10-undoredo-validation--sandboxed-playtesting-f5)

---

## 1. Launching, CLI Options & Settings

ReSyndicated Editor runs natively on Windows (`resynd_editor.exe`) and on macOS via Wine (see [`README.md`](README.md#running-the-editor-on-macos)).

### Command-Line Arguments

| Option | Alternative    | Argument | Description                                                                             |
|:------ |:-------------- |:--------:|:--------------------------------------------------------------------------------------- |
| `-m`   | `--mission`    | `<id>`   | Jump directly into editing mission `id` (for example, `-m 1` or `-m 99`).               |
| —      | `--settings`   | None     | Open the Data Directory / Settings dialog and exit without launching the editor window. |
| `-h`   | `--help`, `/?` | None     | Display command-line options to the terminal and exit.                                  |

### Settings, Asset Verification & Logs

* **Shared Config File**: Settings are shared with the game in `%USERPROFILE%\.resynd\resynd.conf` on Windows (or `~/.resynd/resynd.conf` on macOS/Wine). It remembers your Syndicate `DATA` folder path, editor window size and layout, playtest squad loadout, and playtest display/debug preferences.
* **First-Run Asset Check (`File -> Settings...`, `Ctrl+,`)**: Verifies that your chosen `DATA` folder contains all required original *Syndicate* game files (maps, palettes, sprites, animations, and mission files). If *American Revolt* expansion files are present instead, expansion items (**Air Raid Com** and **Clone Shield**) are automatically unlocked in the editor and Playtest Configuration.
* **Logs**: Diagnostic output is written to `editor.log` and crash backtraces are written to `editor_crash.log` alongside `resynd.conf`.

---

## 2. Workspace Layout & Navigation

The main window consists of a **Top Toolbar**, the **Map Viewport** on the left, a draggable vertical splitter, the **Right Palette & Inspector Panel**, and a 6-part **Status Bar** along the bottom.

### Top Toolbar (Left to Right)

| Button     | Action                                                       | Shortcut                                |
|:---------- |:------------------------------------------------------------ |:--------------------------------------- |
| New        | Create a New Mission                                         | `Ctrl+N`                                |
| Open       | Open an existing Mission                                     | `Ctrl+O`                                |
| Save       | Save Mission and Map files                                   | `Ctrl+S`                                |
| Undo       | Undo last action (up to 50 steps)                            | `Ctrl+Z`                                |
| Redo       | Redo last undone action                                      | `Ctrl+Y`                                |
| Slice      | Toggle Slice View (Z-Isolation)                              | `Z`                                     |
| Layer Up   | Step active layer up (hold `Shift` to raise selected item)   | `PgUp` / `]` (`Shift+PgUp` / `Shift+]`) |
| Layer Down | Step active layer down (hold `Shift` to lower selected item) | `PgDn` / `[` (`Shift+PgDn` / `Shift+[`) |
| Spawns     | Center viewport on Player's Agents                           | `Home`                                  |
| Playtest   | Validate and launch Playtest                                 | `F5`                                    |
| Zoom Out   | Zoom Out (`-25%`)                                            | `-`                                     |
| Zoom 100%  | Reset Zoom to `100%`                                         | `1`                                     |
| Zoom In    | Zoom In (`+25%`)                                             | `+`                                     |

### Map Viewport Overlays & HUD

* **Layer HUD Chip (top-right)**: Displays the current layer, whether layer tracking is `AUTO` or `MANUAL`, and whether `SLICE ACTIVE` or `ALL LAYERS` is shown.
* **3D Hover Wiregrid Column**: Displays stacked wireframe boxes up to Layer 12 over the hovered tile column:
  * **When painting tiles (`Pencil`, `Box Fill`)**: Shows a translucent preview of your active tile framed in neon green on the active layer. If a roof sits above that layer, an amber `(OBSCURED)` badge warns you so you do not blindly paint underneath roofs.
  * **When using Entity or Select tools**: Highlights your active layer in **yellow** (`Layer: Z (Active)`), marks the solid floor or roof surface beneath the cursor with a **neon-green** diamond (`Layer: Z (Surface)`), and fades layers below the surface.
* **X-Ray Obscured Rendering**: Selected or hovered entities hidden behind roofs or tall walls render as translucent X-ray ghosts; selected roof-obscured entities also display an elevation badge, a vertical dashed gold guideline, and a dashed roof marker.
* **Live Stats Overlay (`View -> Show Stats`)**: Displays floating health meters, entity IDs, and equipped weapons over characters and vehicles directly on the map.
* **View Filters (`View -> Filters`)**: Toggle visibility for `Buildings / Structures`, `Vehicles`, `Civilians`, `Police`, `Guards`, `Criminals`, `Agents`, and `Weapon Drops` (plus `Select All` / `Clear All`). Turning off `Buildings / Structures` lets you click straight through walls and roofs to select or inspect interior floors and characters underneath.

### Right Panel Tabs & Status Bar

* **Tiles Tab**: Category dropdown, tile tool buttons (`Pencil`, `Box`, `Select`, `Search`), scrollable tile grid (`#000`–`#255`), draggable horizontal splitter, and **Tile Inspector** (or **Tile Search Results**).
* **Entities Tab**: Category dropdown, entity tool buttons (`Place`, `Select`, `Search`), scrollable sprite grid, draggable horizontal splitter, and **Entity Inspector / Placement Template** (or **Entity Search Results**).
* **Objectives Tab**: Ordered mission objectives list (`1`–`6`) with `Add...`, `Edit...`, `Delete`, and `Clear All Objectives`.
* **Status Bar (left to right)**:
  1. **Coordinates**: Tile coordinates (`TX`, `TY`, `TZ`), sub-tile offsets (`OX`, `OY`), and `[Roof Layer]` when indoors.
  2. **Hovered Tile**: Tile number, category, and whether it is `Walkable` or `Not Walkable`.
  3. **Hint / Action Message**.
  4. **Layer & Tool State**: Active layer, Auto/Manual status, Slice status, and active tool.
  5. **Undo Depth**: Number of steps currently in the undo history.
  6. **Zoom Percentage**.

---

## 3. Complete Controls, Menus & Shortcuts Reference

### Keyboard Shortcuts

*(Single-key shortcuts without `Ctrl` or function keys are automatically disabled while typing in text fields.)*

| Context            | Key                      | Action                                                                                    |
|:------------------ |:------------------------ |:----------------------------------------------------------------------------------------- |
| **File**           | `Ctrl+N`                 | New Mission                                                                               |
|                    | `Ctrl+O`                 | Open Mission                                                                              |
|                    | `Ctrl+W`                 | Close Mission                                                                             |
|                    | `Ctrl+S`                 | Save Mission (opens Save As if the mission has not been saved yet)                        |
|                    | `Ctrl+,`                 | Open Data Directory / Settings dialog                                                     |
| **Edit**           | `Ctrl+Z` / `Ctrl+Y`      | Undo / Redo                                                                               |
|                    | `Del`                    | Delete selected tile (clears to empty `#000`) or delete selected entity                   |
|                    | `Esc`                    | Deselect selection, exit `Route` mode, or cancel `Pick on Map`                            |
|                    | `Tab`                    | Switch selection between an entity and the underlying tile (`Select` mode)                |
| **Tools**          | `P`                      | **Pencil / Place** (paints tiles on `Tiles` tab; spawns entities on `Entities` tab)       |
|                    | `B`                      | **Box Fill** (rectangular tile fill on `Tiles` tab)                                       |
|                    | `V`                      | **Select / Move** (selects/moves tiles on `Tiles` tab, or entities on `Entities` tab)     |
|                    | `S`                      | **Search** (finds all instances of the active tile or entity type across the map)         |
|                    | `R`                      | Toggle **Character / Vehicle Route Tool** (when a routeable character/driver is selected) |
| **Layers**         | `PgUp` / `]`             | Step active layer up (or cycle up through non-empty tiles in the selected column)         |
|                    | `PgDn` / `[`             | Step active layer down (or cycle down through non-empty tiles in the selected column)     |
|                    | `Shift+PgUp` / `Shift+]` | Raise selected tile or entity up 1 layer                                                  |
|                    | `Shift+PgDn` / `Shift+[` | Lower selected tile or entity down 1 layer                                                |
|                    | `Alt+L`                  | Toggle **Auto Layer Switching**                                                           |
|                    | `Z`                      | Toggle **Slice View (Z-Isolation)**                                                       |
| **Viewport**       | `Home`                   | Center viewport on Player's Agents                                                        |
|                    | `+` / `-`                | Zoom In / Zoom Out (`±25%`, range `25%`–`300%`)                                           |
|                    | `1` / `2` / `3`          | Set Zoom to `100%` / `150%` / `200%`                                                      |
|                    | Arrow Keys               | Pan viewport while picking a target/zone on the map (Objectives / AI Tactics)             |
| **Mission & Help** | `F5`                     | Playtest Mission in ReSyndicated                                                          |
|                    | `F1`                     | Open Keyboard & Mouse Shortcuts reference                                                 |

### Mouse Controls

* **Middle-Click Drag** or **Right-Click Drag** (anywhere except on a waypoint in `Route` mode): Pan viewport smoothly.
* **Mouse Wheel** (over viewport): Zoom in or out centered on the cursor (`25%` to `300%`).
* **Left-Click / Drag**:
  * **Pencil / Place (`P`)**: Click or drag to paint tiles continuously, or click to spawn an entity.
  * **Box Fill (`B`)**: Drag an isometric rectangle on the active layer and release to fill.
  * **Select Tile (`V`)**: Click a tile to inspect it and set the active layer and palette brush to match. Drag to move it across the active layer; hold **`Ctrl` while releasing** to **copy** the tile instead of moving it.
  * **Select Entity (`V`)**: Click an entity to select it; click again on the same spot to cycle through overlapping entities (priority: Player Squad > Characters & Weapon Drops > Vehicles > Static Objects). Drag to reposition horizontally while keeping its layer height.
  * **Route Tool (`R`)**: Left-click the map to append a movement waypoint; left-drag any waypoint circle to move it (dragging Waypoint `#1` moves the entity's spawn point); right-click a waypoint to open the waypoint action menu.
* **Search Results List**: Single-click centers the camera on that match and highlights it in amber; double-click exits Search mode, selects that tile or entity, and opens its inspector.

---

## 4. Creating, Opening, Saving & Configuring Missions

### Mission Files & Format Compatibility

Every *Syndicate* mission consists of three compressed binary files linked by **Mission ID (`01`–`99`)** and **Map ID (`01`–`99`)**:

1. `gameXX.dat`: Stores all characters, vehicles, weapons, static props, scripted routes, enemy AI tactics, objectives, and map metadata.
2. `mapYY.dat`: Stores the 3D isometric city tilemap (typically 128×128 tiles across 12 playable height layers). Each mission points to a Map ID, so multiple missions can share a single city layout or each use its own custom map.
3. `missXX.dat` (plus optional `miss1XX.dat` French, `miss2XX.dat` Italian, `miss3XX.dat` German): Stores briefing text, intel costs, and enhancement prices.

Saving uses standard RNC compression, keeping exported missions compatible with both ReSyndicated and original DOS *Syndicate*.

> **Protect Your Original Files**: Always keep a backup of your original *Syndicate* `DATA` directory. Saving directly onto campaign mission numbers (`01`–`50`) inside your main `DATA` folder overwrites original campaign files.

### New Mission (`Ctrl+N`)

* **Mission ID (`1`–`99`)**: Choose the mission slot number (defaults to `99`, a convenient custom/playtest slot).
* **Base Map Template**:
  * **Blank Map**: Creates an empty map using Palette #1 with a `6×6` concrete platform (`#012`) at layer 1 and all 4 player squad agents standing on layer 2.
  * **Copy Map `01`–`50`**: Copies the full 3D city buildings, roads, palette, and starting squad spawn from that campaign map while clearing all original NPCs, vehicles, weapons, objectives, and AI tactics.
* **Briefing Codename**: Pre-fills the opening title line of your English briefing (default `OPERATION NEW DAWN`).
* Newly created missions have no file path until you save (`Ctrl+S` opens **Save As...**).

### Open Mission (`Ctrl+O`) & Save As (`File -> Save As...`)

* **Open Mission**: Browse Campaign Missions `01`–`50` and Custom Slot `99` with live text filtering, or click **Browse File...** to pick any `game*.dat`, `map*.dat`, or `miss*.dat` file on disk (the editor automatically locates and validates the matching trio).
* **Save As**: Choose the target **Mission ID (`1`–`99`)**, destination folder, and **Map Strategy**:
  * **Save custom copy of map**: Saves a dedicated `mapXX.dat` matching your Mission ID so map edits never affect other missions.
  * **Keep linked to current map**: Keeps pointing to an existing `mapYY.dat` without duplicating map geometry.
  * Displays whether each target file is `[NEW FILE]` or `[EXISTS - WILL OVERWRITE]` and asks for confirmation before overwriting.

### Mission Properties (`Mission -> Mission Properties...`)

* **Language & Multi-Language Briefings (`English`, `French`, `Italian`, `German`)**: Edit the **Base Mission Briefing** plus **Level 1–3 Intelligence Intel Texts**. Switching to a language whose briefing file does not exist yet offers to copy your English text as a starting point or use a blank template.
* **Intel & Enhancement Costs**: Set `Info 1..3 ($)` and `Enh 1..3 ($)` (default `100`, `250`, `400`) charged on the in-game briefing screen.
* **Mission Cybernetics Tech Level (`0`–`3`)**: Sets the baseline cyberware version (**Tier 0 Unaugmented**, **Tier 1 V1**, **Tier 2 V2**, or **Tier 3 V3**) inherited by all Enemy Agents that do not have a per-agent override enabled:
  * **Tier 0**: On untouched original campaign missions (`01`–`50`), the game applies world-map progression dynamically; on custom or edited maps, Tier 0 leaves inheriting enemy agents unaugmented.
  * **Tier 1 / 2 / 3**: Equips inheriting enemy agents with full V1 / V2 / V3 Legs, Arms, Chest, Heart, Eyes, and Brain mods.
* **Environment Palette Behavior**: Mission files do not embed palette numbers inside `gameXX.dat`. Campaign Mission IDs `01`–`50` automatically load their authentic territory palette (for example, Industrial or Snow), while custom missions (`51`–`99`) default to Palette 1 (Temperate Urban).

---

## 5. Isometric Tilemap Construction & Z-Layer System

### Tile Palette & Categories

Browse all 256 isometric tiles (`#000`–`#255`) by category:

* `All Tiles`, `Ground (Floors)`, `Roads & Crossings`, `Walls & Barriers`, `Roofs`, `Stairs & Slopes`, and `Specials (Rails/Empty)`.
* Clicking a tile selects it as your active brush and shows its **Type**, **Surface walkability**, **Position** (when inspecting a placed tile), and total **Placements** on the map.

### Working with Layers (`0` to `12`) & Auto Layer Switching

*Syndicate* maps span `128 × 128` tiles across height layers `0` to `12` (layer `12` acts as an upper ceiling bound):

* **Surface vs. Placement Layer**: Floor and road tiles sit on layer `Z` (such as layer `1`), while pedestrians, vehicles, dropped weapons, and walls stand **one layer higher** (`Z + 1`, such as layer `2`).
* **Auto Layer Switching (`Alt+L`, enabled by default)**: When using placement tools (`Pencil`, `Box Fill`, `Place`), hovering over a surface automatically aims one layer above the hovered tile so roads, walls, and actors snap cleanly on top.
* **Manual Layer Control**: Choose `Layer -> Layer 0..12` or press `PgUp`/`PgDn` (`[`/`]`) to lock onto a specific layer (`Manual` mode remains active until you switch tools or press `Alt+L`).
* **Slice View (`Z`)**: Hides all tiles and entities above your active layer and restricts tile picking to that layer, making it easy to inspect and build building interiors or tunnels beneath roofs.
* **Raising / Lowering (`Shift+PgUp` / `Shift+PgDn` or `Shift+[/]`)**: Moves the currently selected tile or entity up or down by 1 layer.
* **Copying Tiles (`Ctrl+Drag`)**: In `Select Tile (V)` mode, dragging moves a tile; holding `Ctrl` when releasing leaves the original in place and stamps a copy.

---

## 6. Entities: Placement, Inspection & Gameplay Mechanics

### Map Capacity Limits

ReSyndicated preserves the original mission format's capacity limits (enforced during placement, validation, and saving):

| Category                 | Hard Limit                   | Notes                                                                                                           |
|:------------------------ |:----------------------------:|:--------------------------------------------------------------------------------------------------------------- |
| **Characters (Peds)**    | `256`                        | Divided across Player Squad (`4`), Enemy Syndicate squads (up to `8` per faction), and Civilians/Police/Guards. |
| **Vehicles & Trains**    | `64`                         | Includes cars, armored vehicles, train engines, and individual train carriages (a 6-car train uses `6` slots).  |
| **Static Map Objects**   | `400`                        | Sliding doors, gates, windows, trees, street lamps, traffic lights, mailboxes, trash bins, ads, and neon signs. |
| **Weapons & Pickups**    | `512`                        | **Combined** cap for all ground pickups plus all non-player NPC inventories (Player Agents do not count here).  |
| **Scripted Route Steps** | `2,048`                      | Shared pool of waypoint/action steps across all scripted NPCs and vehicles.                                     |
| **AI Tactics**           | `127` authored (`128` total) | Up to `127` faction tactic nodes editable in AI Tactics; slot `0` is reserved by the game as a fallback slot.   |
| **Objectives**           | `6`                          | Evaluated strictly in order from top to bottom.                                                                 |

### Inspector Workflow (`Placement Template` vs. `Selected Entity`)

* **Nothing selected**: The Entity panel configures **Placement Template** defaults (orientation, health, movement mode, faction, cyberware, weapons, driver) applied to every new entity placed with `Place (P)`.
* **Entity selected**: Changes preview immediately on the map while **Apply Changes** and **Cancel** become active. Click **Apply Changes** to commit edits to the Undo stack, or **Cancel** to revert. Attempting to switch tools or select another entity before applying prompts you to Apply, Discard, or Stay.
* **Preview Box Controls**: Rotate (`↺` / `↻`) entity facing or toggle live animation playback (`▶`/`||`).

---

### Characters, Factions & Cybernetics

#### Palette Categories (9 Templates)

* **Civilians**: Male, Female, Punk, Suit
* **Criminals**: Punk, Merc
* **Platform Guard**
* **Police Officer**
* **Agent**: Toggle between **Player's Agent** and **Enemy Agent** using the Inspector **Type** dropdown.

#### 1. Player's Agents

* Every playable mission must have **exactly 4** Player's Agents (the validator flags an error otherwise).
* Missions only store Player Agents' **spawn positions and facing directions**—their starting weapons and cybernetic upgrades come from the player's company inventory during a campaign, or from **Mission -> Playtest Configuration...** during `F5` playtesting.
* Selecting any Player's Agent provides a quick **Playtest Configuration...** button in the Inspector.

#### 2. Corporate Factions

Enemy Syndicate agents belong to **7 rival corporate squads of up to 8 agents each**:

| Faction ID | Corporate Faction  | Visual Coat / Head Color                            |
|:----------:|:------------------ |:--------------------------------------------------- |
| **0**      | Player Syndicate   | Player company color                                |
| **1**      | Blue Syndicate     | Blue Grey                                           |
| **2**      | Orange Syndicate   | Light Red / Orange                                  |
| **3**      | Yellow Syndicate A | Yellow                                              |
| **4**      | Yellow Syndicate B | Yellow (independent AI tactics, shares Yellow coat) |
| **5**      | Purple Syndicate   | Purple                                              |
| **6**      | Grey Syndicate     | Grey                                                |
| **7**      | Cyan Syndicate     | Cyan / Teal                                         |

* **Automatic Slot Management**: When you change an Enemy Agent's faction in the Inspector, the editor automatically assigns it to an open slot for that corporate squad. If a civilian, guard, or police officer happens to occupy one of those slots (common in original Bullfrog missions), the editor transparently relocates that non-agent into general civilian slots without breaking objectives or routes.

#### 3. Enemy Cyberware Inheritance vs. Override

In the Enemy Agent Inspector, **Cyberware** can be set to:

* **Inherit Mission Tier**: Uses whatever tech level (`0`–`3`) is set in `Mission -> Mission Properties...`.
* **Override Tier**: Pins that specific agent to Tier 0 (Unaugmented), Tier 1 (V1), Tier 2 (V2), or Tier 3 (V3) regardless of the mission's default tech level (useful for bosses or elite defenders).

#### 4. Enemy Weapons & Fallback Rules

* Weapons selected in an NPC's **Weapons Grid** (up to 8 items) are saved into the mission and equipped verbatim on mission start.
* **Empty Inventory Fallback**: If an Enemy Agent has **no** weapons assigned, the game will not leave them unarmed—they automatically receive a fallback **Pistol** (and in dynamic campaign missions without static loadouts, may equip top unlocked firearms or Time Bombs matching the player's research).
* **Guaranteeing Loadouts**: Explicitly assign weapons in the **Weapons Grid** whenever you want enemies to carry specific gear such as Lasers, Gauss Guns, Miniguns, or Flamers.

#### 5. Movement Mode & Dead / Destroyed State

* **Wandering Patrol**: Unscripted live NPCs wander streets randomly (Police roam intersections; Civilians walk until startled; unengaged Enemy Agents patrol).
* **Stationary Sentry**: Unscripted live NPCs hold their exact position until a hostile enters range/sight or triggers them (ideal for compound guards and ambush agents).
* **Dead / Destroyed Checkbox**: Starts the character already dead on the ground (or vehicle already burnt wreckage).

---

### Vehicles, Trains & Static Objects

#### Road Vehicles (`Regular Car`, `Police Car`, `Ambulance`, `Firetruck`, `Armored Car`, `Large Armored`)

* **Drivers**: Use the **Driver** dropdown to leave a car empty, spawn a new driver automatically, or assign an existing pedestrian.
* **Detaching Drivers**: Dragging a seated pedestrian out of a vehicle asks if you want to detach them onto the street. Dragging the **vehicle** keeps its driver seated and updates Waypoint `#1` (`Spawn`) automatically.
* **Road Pathing & Bailout**: Driven cars follow road tiles, yield to pedestrians, and catch fire / eject passengers if destroyed or attacked.

#### Trains (`Train Engine`, `Train Carriage (Windows / Door / Raised Roof)`, `Train (Full Group)`)

* **`Train (Full Group)`**: Places a full 6-car train in one click (Front `Train Engine` + 4 `Train Carriages` + Rear `Train Engine` facing opposite).
* **Automatic Train Linking**: On mission start, carriages automatically attach to the nearest `Train Engine` within 7 tiles (`1792` units), so multiple independent trains on one map work automatically as long as each train's cars sit closer to its own engine than another train's.
* **Drivers & Damage**: Assign drivers and routes only to the **Train Engine** (`Train Carriages` lock to `Automatic Rail System` and follow the engine). If **any** engine or carriage starts destroyed or is destroyed in combat, the entire train halts permanently.
* **Station Boarding & Alighting Rules**:
  * At a `Train Station Dwell` stop (`10s`) or end of route, carriages drop NPC passengers onto an adjacent walkable platform tile (`track + 1`), give them a `15s` cooldown before re-boarding, and start them walking away.
  * Whenever a train is stopped at a station, each carriage automatically boards **any** unscripted non-player, non-persuaded living pedestrian within ~2 tiles (`550` units)—including both **Civilians** and **unscripted Platform Guards**. *(Tip: Give station guards at least one route waypoint or keep them outside boarding range if you do not want them riding away on trains!)*
* **Track Placement**: Place trains directly on the track layer; the game automatically handles visual layering above track bodies and ground pickups.

#### Static Objects (38 Templates)

* **Sliding Doors**: Automatically open for nearby living pedestrians (`800ms` delay). Rotate between `NW ↔ SE` and `SW ↔ NE` to align with building walls.
* **Large Gate Door**: Blocks road traffic across a 3-tile wide passage when closed, and crushes pedestrians caught underneath when closing.
* **Destructible Props**: **Windows** (`Closed`, `Open`, `Damaged`, `Animated`) shatter on gunfire; **Trees** (`Conifer`, `Deciduous`, `Palm`, `Bush`) catch fire and burn when hit.
* **Decorative & Street Props**: **Phone Booths**, **Traffic Lights**, **Mailboxes**, **Trash Bins**, **Wall Ads**, and **16 Neon Signs**.

---

## 7. Character & Vehicle Route Tool (`R`)

Routes control scripted paths and actions for **Civilians, Police, Guards, Criminals, and driven Vehicles (including Trains)**.

> **Important**: **Enemy Agents do not use Waypoint Routes** (the editor disables the Route Tool for Enemy Agents). Use **AI Tactics** (`Mission -> AI Tactics...`) to control Enemy Syndicate squads instead.

### Creating & Editing a Route

1. Select a non-agent pedestrian—or a car / `Train Engine` with a driver assigned—using `Select (V)`.
2. Click **Edit Route (R)** in the Inspector or press **`R`**. (If no route exists yet, Waypoint `#1 (Spawn)` is created at the entity's current spot and **Loop Route** turns on automatically.)
3. **Left-click** on the map to add `Goto` waypoints:
   * On-foot characters snap to walkable tiles near the active layer.
   * Driven cars snap to valid road tiles and draw the real road-following path in gold.
4. **Left-drag** any waypoint circle to move it (dragging Waypoint `#1` moves the entity's starting spawn point).
5. **Right-click** any waypoint on the map (or select a step in the Inspector list and click **`+ Action`**) to change its **Action Type**, reorder (`Move Up` / `Move Down`), or `Delete Waypoint`.
6. Press **`R`**, **`Esc`**, or click **Done Route (Esc)** when finished. (If you leave a brand-new route with only its single auto-created Spawn point `#1`, it resets cleanly back to an unscripted NPC.)

*Tip: When editing steps from the Inspector list (`▲ Up`, `▼ Down`, `Delete`, `Clear`, `Loop Route`), click **Apply Changes** to commit them to the Undo stack. Direct map clicks and drags apply immediately.*

### Waypoint Color Legend

* **Amber / Gold**: Road vehicle route (renders road-following curves).
* **Cyan**: Train / non-road vehicle route.
* **Emerald Green**: On-foot pedestrian route.
* **Waypoint `#1` Outer Gold Ring**: Starting spawn position.

### Route Action Types

| Action                         | Description & Gameplay Notes                                                                                                                                                               |
|:------------------------------ |:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **`Goto` (`Walk` / `Drive`)**  | Walks or drives to the target tile. If a driven car only has 1 looped `Goto`, it automatically drives back to its starting spot.                                                           |
| **`Board Vehicle`**            | Actor walks to the chosen vehicle and enters as driver or passenger. Subsequent `Goto` steps drive that vehicle until an `Unboard` step.                                                   |
| **`Wait 5s` / `Wait 10s`**     | Pauses at the waypoint for 5 or 10 seconds before moving to the next step.                                                                                                                 |
| **`Train Station Dwell`**      | Halts a `Train Engine` at a station platform for `10s`, letting passengers exit and waiting civilians board.                                                                               |
| **`Unboard / Exit Vehicle`**   | Exits the current vehicle onto an adjacent walkable tile or station platform and resumes walking.                                                                                          |
| **`Proximity Ambush Trigger`** | Actor waits at this step until a live Player Agent enters the chosen trigger radius (`2, 4, 6 [Default], 8, 10, 12, or 16` tiles, or clear engagement range), then opens fire immediately. |
| **`Target Escape / Evacuate`** | Reaching this waypoint marks the character as **escaped**—immediately **failing** any `Assassinate` objective targeting them!                                                              |
| **`Loop Route`**               | Loops the route back to Waypoint `#1` continuously (ideal for patrols, getaway loops, and train lines). *(Any script ending while inside a vehicle automatically loops.)*                  |

### Scripted Actors vs. Generic NPCs

* **Ignore Gunfire Panic**: Generic unscripted civilians scream and flee whenever shots are fired or weapons are drawn. **Scripted actors stay focused on their route** (running to getaway cars or finishing patrols) and ignore ambient gunfire.
* **Vehicle Attack Bailout**: If an occupied getaway vehicle takes heavy damage or is carjacked, scripted passengers bail out, abort their script, and flee on foot.
* **Persuasion Overrides Routes**: Persuading a scripted NPC clears their remaining waypoints so they escort your squad.
* **Safe Vehicle Deletion**: Deleting a vehicle automatically clears its driver's route, strips any `Board -> Exit` vehicle segments from other characters' routes, and updates remaining vehicle numbers cleanly (undoable with `Ctrl+Z`).

---

## 8. Enemy Factions & the AI Tactics System

While non-agent NPCs use Waypoint Routes, **Enemy Syndicate Factions (`1`–`7`)** are controlled by **AI Tactics** (`Mission -> AI Tactics...`, or **Edit Faction AI Tactics...** in an Enemy Agent's Inspector).

### Chains, Root Nodes & Reserved `Tactic 0`

* Each mission supports up to **127 authored tactic nodes** (slots `1`–`127`), plus a special reserved fallback slot (**`Tactic 0`**).
* For any faction (`1`–`7` or **Wildcard**), tactics can be linked into sequential chains:
  * **`🚩` Root Node**: Starting directive evaluated at mission start.
  * **`↳` Child Node**: Linked step triggered when a preceding node advances to its **Next Tactic**.
  * **`[End of Chain / Halt]`**: Setting **Next Tactic** to `[End of Chain / Halt]` has an important nuance:
    * On **modified original campaign missions**, `Tactic 0` is usually active as a global sweep targeting the player—so advancing off the end of a chain transitions the squad into **`Tactic 0`** (global hunt).
    * On **new or blank custom maps**, `Tactic 0` starts disabled, so advancing off the end halts. When authoring custom maps, explicitly link triggers or delay timers to a downstream **Seek & Destroy**, **Rival Sweep**, or **Objective Sweep** child node!

### Faction-Specific (`1`–`7`) vs. Wildcard Assignment

At mission start, the game assigns each hostile faction (`1` through `7`) its starting root tactic in sequential order using a two-pass rule:

1. **Pass 1 (Unclaimed Roots)**:
   * First looks for an **unclaimed** enabled root created specifically for that faction (`1`–`7`).
   * If none exists, claims the first **unclaimed** enabled **Wildcard** root (`Global / All Hostile Factions`).
   * *Design Tip*: Placing two Wildcard roots lets the first hostile faction claim the first Wildcard chain and the second faction claim the second chain—great for giving generic squads staggered timing or varied roles!
2. **Pass 2 (Shared Reuse)**: If every matching root was already claimed by earlier squads, remaining squads reuse the first matching faction or Wildcard root.
3. **Fallback (`Tactic 0`)**: Used only if no matching root exists in slots `1`–`127` and `Tactic 0` is enabled.
4. **No Matching Tactics**: Enemy agents hold their posts as local sentries—*except* in **Combat Sweep** missions (**Combat Sweep: Enemy Agents** or **Police Sweep**), where original *Syndicate* activates **Faction 1 (Blue)** as hunters and *American Revolt* activates **all rival squads**.

### AI Tactic Modes — Settings & Exact In-Game Behavior

| Mode        | Name                                                                   | Editor Settings                                                       | Exact In-Game Behavior & Nuances                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|:-----------:|:---------------------------------------------------------------------- |:--------------------------------------------------------------------- |:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **1**       | **Seek & Destroy**                                                     | Target Faction                                                        | **Global Hunter**: Hunts across the entire map with unlimited sight range and `100%` Adrenaline. **Important:** In-game, `Seek & Destroy` always targets the **Player Squad** regardless of the Target Faction dropdown—use **Rival Sweep** or **Objective Sweep** to fight other AI factions!                                                                                                                                                                                                                                                                                                                                   |
| **2**       | **Rival Sweep**                                                        | Target Faction (`Player` or `Faction 1–7`)                            | **Global Hunter + Auto-Advance**: Hunts the chosen faction (`Player` or a rival syndicate) across the map. If targeting a rival syndicate, once every agent in that rival faction is eliminated, the squad automatically advances to its **Next Tactic**.                                                                                                                                                                                                                                                                                                                                                                        |
| **3**       | **Objective Sweep**                                                    | Target Faction (`Player`, `Faction 1–7`, or `Wildcard`)               | **Global Hunter + Self-Sweep Inhibit**: Sweeps the chosen faction unconditionally across the map (advances once a rival target faction is wiped out). **Self-Sweep Inhibit**: Setting a faction to sweep *itself* disables hunting so the squad behaves as stationary defensive sentries.                                                                                                                                                                                                                                                                                                                                        |
| **4**       | **Stationary Guard**                                                   | Guard Radius, Target Faction, **Target Armed Required**               | **Defensive Sentry**: Holds ground and shoots hostiles within perception-adjusted weapon range and clear line of sight. Respects **Target Armed Required** (ignores unarmed/holstered player agents until they draw a weapon). Does not advance to **Next Tactic**. *(Note: Guard Radius and Target Faction UI fields are stored for compatibility; the engine uses weapon/perception range against the Player.)*                                                                                                                                                                                                                |
| **5**       | **Weapon Ambush**                                                      | Hold Range, Target Faction, **Target Armed Required**                 | **Hold Fire Until Range + Advance**: Waits until a player agent enters lethal weapon range with clear line of sight (respecting **Target Armed Required**), opens fire, and immediately advances the squad to **Next Tactic** (for example, triggering a full counterattack).                                                                                                                                                                                                                                                                                                                                                    |
| **7**       | **Proximity Tripwire**                                                 | Timer (`1.35s` stages), Perimeter (`tiles`, `0 = 12`), Target Faction | **Perimeter Sensor -> Advance**: Watches for the target faction of the downstream sweep node (default: Player). Advances to **Next Tactic** as soon as the perimeter is breached—or, if linked to a **Next Tactic**, also advances when the countdown finishes (`Timer × 1.35s`, defaulting to `13.5s` if `Timer = 0`). Terminal tripwires wait indefinitely until breached. **Anchor Rule**: Against **Player**, *every* living agent in the squad checks the radius; against a **Rival Faction**, distance is measured to that rival's squad leader. *(Note: The Armed checkbox has no runtime effect on Proximity Tripwire.)* |
| **10**      | **Delay Timer**                                                        | Duration (`0.1s` ticks; e.g. `100` = `10.0s`)                         | **Timed Hold -> Advance**: Stops squad movement right at mission start (skipping the normal `3.0s` startup delay) and advances to **Next Tactic** once the timer expires. Ideal for timed reinforcements and staggered waves.                                                                                                                                                                                                                                                                                                                                                                                                    |
| **6, 8, 9** | **Patrol / Search (`6`)**, **Board Vehicle (`8`)**, **Evacuate (`9`)** | Target Coordinates or Target Vehicle                                  | **Stored Data Only (Not Executed in Gameplay)**: Retained for compatibility with Bullfrog's mission format, but enemy squads ignore these modes at runtime and act as passive sentries. Use the **Route Tool (`R`)** on Guards/Civilians for patrols, vehicles, and escapes instead.                                                                                                                                                                                                                                                                                                                                             |

### Critical AI Tactics Design Tips & Pitfalls

1. **Squad-Wide Sync**: Tactics always apply to the **entire faction**. When a single sentry trips a **Proximity Tripwire** or **Weapon Ambush**, *every* surviving agent in that faction switches to the next tactic together, surges to `100%` Adrenaline, and charges.
2. **3-Second Startup Delay**: All modes except **Delay Timer (`10`)** wait `3.0` seconds after mission start before moving.
3. **Rival March & Holstering**: When hunting another AI faction (`1`–`7`), operatives march toward the opposing squad leader with weapons holstered until closing within ~1 tile of weapon range.
4. **Chained Tripwire Timers**: Linking a **Proximity Tripwire (`7`)** to a **Next Tactic** automatically arms a fallback countdown (`13.5` seconds when Timer is `0`). If you want a chained squad to wait much longer before timing out, increase **Timer** (up to `65,535` stages).
5. **Re-Enabling Disabled Campaign Nodes**: Opening and saving any disabled slot in the AI Tactics dialog clears its hidden disabled state so it becomes active.

---

## 9. Mission Objectives & Completion Rules

Add up to **6 objectives** in the **Objectives** tab (`Add...`, `Edit...` / double-click, `Delete`, `Clear All Objectives`). Objectives evaluate **strictly in order from top to bottom**—Objective 2 does not activate until Objective 1 finishes, and if all 4 Player Agents die, the current objective immediately fails.

### Supported Objective Types

| Objective Type                            | Target              | Win / Fail Rules & Nuances                                                                                                                                                                                                                                                                             |
|:----------------------------------------- |:------------------- |:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Persuade Target Ped**                   | Any non-player Ped  | Must brainwash the target with the Persuadertron. Persuaded targets automatically become part of any **Evacuate** objective, meaning they must survive and reach the extraction zone too!                                                                                                              |
| **Assassinate Target Ped**                | Any non-player Ped  | Completes when the target dies (waits for laser vaporization or drowning animations to finish). **Fails** if the target reaches a **Target Escape / Evacuate** route waypoint.                                                                                                                         |
| **Protect Target Ped**                    | Any non-player Ped  | **Fails** if the target is killed. **Completes** when the target reaches the end of their scripted waypoint route—always give a Protect target a non-looping route ending at a safe destination!                                                                                                       |
| **Combat Sweep (Eliminate Enemy Agents)** | None                | Completes when all Enemy Agents are eliminated. **Tip:** Persuading an enemy agent converts them to your side, so **persuaded enemies count as eliminated**!                                                                                                                                           |
| **Destroy Target Vehicle**                | Any Vehicle         | Completes when the target vehicle is destroyed.                                                                                                                                                                                                                                                        |
| **Use / Steal Target Vehicle**            | Any Vehicle         | **Fails** if the vehicle is destroyed. Completes only when a **Player Agent enters the driver's seat** (sitting as a passenger is not enough).                                                                                                                                                         |
| **Evacuate Squad & Persuaded**            | Tile (`TX, TY, TZ`) | All surviving Player Agents **and** all **Persuade** objective targets must gather within **2 tiles** of the evacuation zone simultaneously. **Tip:** Add **Persuade** objectives *before* creating **Evacuate** so targets link immediately; if any persuaded target dies beforehand, Evacuate fails. |

### Map Picking, Ordering & Legacy Objective Types

* Inside the Objective dialog, click **Pick on Map** or **Pick Destination Tile** to pick a character, vehicle, or tile directly on the map (use arrow keys to pan, `Esc` to cancel), or **Find Target** / **Find Zone** to jump the camera there.
* **Order Matters**: Because objectives do not have Move Up / Move Down buttons, add them in the exact order you want them evaluated.
* **Legacy Campaign Objectives**: **Acquire Equipment** and **Police Sweep** from original Bullfrog missions remain intact when untouched, though they cannot be created from the Objective dialog (if you open one in `Edit...`, click `Cancel` rather than saving so it is not overwritten as Persuade).

---

## 10. Undo/Redo, Validation & Sandboxed Playtesting (`F5`)

### Undo/Redo & Unsaved Tracking

* **50-Step Undo History**: Map tile paints/fills/moves, entity spawns/deletes/moves/edits, route edits, objective edits, and AI tactic edits can all be undone (`Ctrl+Z`) and redone (`Ctrl+Y`).
* **Unsaved Indicator (`*`)**: Appears in the title bar whenever changes differ from the last save/load point. *(Note: `Mission Properties...` writes briefing file changes to disk immediately on Save.)*

### Mission Validation (`Mission -> Validate Entities...`)

Validation runs on demand from the menu, warns before saving if issues exist, and runs automatically before `F5` playtesting (Playtest **blocks** on hard errors and asks confirmation on warnings):

* **Hard Errors (must fix to playtest cleanly)**:
  * Player's Agents count is not **4**, or a Player Agent is out of bounds, over empty void, inside a solid wall, or on an unwalkable tile.
  * Exceeding format limits (`> 256` characters, `> 64` vehicles, `> 400` statics, `> 6` objectives, or `> 8` agents in a single rival faction), or duplicate/out-of-range character IDs.
  * Out-of-bounds entities, vehicles floating over empty void, or evacuation zones outside the map.
  * Objectives referencing deleted targets, or `Board Vehicle` route steps pointing to a missing vehicle.
* **Warnings (allowed with confirmation)**:
  * Non-player characters or static objects placed over empty void.
  * Enemy agents temporarily outside their faction's slot block (automatically repaired when saving).
  * Vehicles embedded in walls, or initially destroyed vehicles with a live driver assigned.
  * `0` objectives configured, `0` walkable tiles on the map, or `Persuade` / `Protect` / `Use Vehicle` objectives without an `Evacuate` objective.
  * `Board Vehicle` steps targeting an initially destroyed vehicle.

### Sandboxed Playtesting (`F5` & `Mission -> Playtest Configuration...`)

Pressing **`F5`** exports your active mission into a temporary isolated playtest folder in your system temp directory (`%TEMP%\resynd_playtest`), launches the game straight into that mission, and deletes the temporary folder automatically when you exit—**never modifying your source campaign files**.

In **Mission -> Playtest Configuration...**, you can configure:

* **Squad & Gear**: Active squad size (`1`–`4` agents), per-agent cyberware (`None`, `V1`–`V3`, or quick `All V1`/`V2`/`V3`), up to 8 starting weapons per agent, and presets (`Pistol Only`, `Heavy Assault`, `Stealth & Persuade`, `Copy to All Agents`).
  * *Note*: **Air Raid Com** requires at least `$50,000` in **Starting Funds** to call an airstrike in-game, and expansion items require *American Revolt* data files.
* **Syndicate Identity**: `Company Name` (up to 16 characters—supports built-in debug cheats like `COOPER TEAM` or `DO IT AGAIN`), `Starting Funds`, `Faction Color`, and `Logo` (`1`–`40`).
* **Launch & Debug Options**: `Window Scale` (`1x`–`4x`) or `Fullscreen`, `Map Zoom` (`1.0x`–`3.0x`), `No Sound`, debug log flags (`ALL`, `GAME`, `KERNEL`, or custom), and visual overlays (`Show Coordinates`, `Show Agent Paths`, `Show Stats`).
