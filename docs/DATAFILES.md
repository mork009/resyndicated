# ReSyndicated Data Files Specification (`data/`)

This document provides a comprehensive reference of the game data files utilized by **ReSyndicated** located in the `data/` directory.

Most binary data files are compressed using **RNC (Rob Northen Computing)** format and are unpacked at runtime by the engine.

---

## 1. File Format Conventions

| Extension / Suffix | Format | Description |
| :--- | :--- | :--- |
| **`gameXX.dat`** | RNC-compressed binary | Mission entity definitions, actor placement, scripted scenarios, and objectives. |
| **`mapXX.dat`** | RNC-compressed binary | 3D isometric height-layer tilemap geometry ($128 \times 128$ grid). |
| **`missXX.dat`** | Plain binary text strings | Mission briefings, target dossiers, and background intelligence. |
| **`*.dat` + `*.tab`** | Binary payload + index table | Asset pairs. The `.tab` file contains 6-byte entries (`offset`, `width`, `height`); the `.dat` file contains raw pixel or PCM audio payloads. |
| **`m*.dat`** | Autodesk Animator FLI / FLC | 256-color animated video sequences used as menu backgrounds and screen transitions. |
| **`hpal*.dat`** | Binary palette table | 16-color in-game VGA palette definitions. |
| **`*.ani`** | Frame-based binary animation | In-game animated map objects (elevators, billboards, antennas). |

---

## 2. Categorized File Reference

### 2.1 Mission Entity & Level Definitions (`game01.dat` – `game99.dat`)

Each `gameXX.dat` file defines the entities, starting positions, patrol scripts, and objectives for a specific mission. When decompressed, it contains structured entity and scenario tables (~114 KB):

| Data Section | Size (bytes) | Count | Description |
| :--- | :---: | :---: | :--- |
| **Header** | 6 | 1 | File identification and format flags |
| **Tile References** | 32,768 | 16,384 | $128 \times 128$ map grid entity reference index |
| **People** | 23,552 | 256 | Characters (agents, police, guards, civilians) |
| **Cars (Vehicles)** | 2,688 | 64 | Cars, armored vehicles, and trains |
| **Static Objects** | 12,000 | 400 | Interactive and decorative map fixtures |
| **Weapons & Pickups** | 18,432 | 512 | Weapon items and map pickups |
| **Visual Effects** | 7,680 | 256 | Visual effect points and indicators |
| **Scenarios** | 16,384 | 2,048 | Scripted waypoint nodes and actions |
| **Map Information** | 14 | 1 | Map ID, boundary limits, and tech level |
| **Objectives** | 84 | Up to 6 | Mission objectives and win/loss conditions |

* **`game01.dat` – `game50.dat`**: The 50 main campaign missions (Western Europe through China).
* **`game90.dat` – `game98.dat`**: Multiplayer, demo, and bonus mission files.
* **`game99.dat`**: ReSyndicated scratch / mission editor playtest template.

---

### 2.2 3D Isometric Map Geometry (`map01.dat` – `map99.dat`)

Defines the physical layout, building structures, roads, and elevations of each city:

* When decompressed, each map contains a $128 \times 128$ tile matrix across vertical elevation layers ($Z = 0$ through $5+$).
* Each grid cell contains an 8-bit index (`0`–`255`) referencing an isometric building block in `hblk01.dat`.
* **Map Sharing**: Multiple missions reuse the same map geometry. For example:
  * **Map 01**: Western Europe (Mission 01)
  * **Map 02**: Far East (Mission 02)
  * **Map 06**: Siberia (Mission 22), Ural (Mission 18), Kamchatka (Mission 12)
* **`map99.dat`**: Custom map binary exported by the ReSyndicated Editor.

---

### 2.3 Mission Briefings & Intelligence (`miss01.dat` – `miss350.dat`)

Contains country background briefings, tactical targets, and primary mission goals displayed on the pre-mission briefing screen:

| Numbering Scheme | Language | Range | Notes |
| :--- | :--- | :--- | :--- |
| **`miss01.dat` – `miss50.dat`** | **English** | Missions 1–50 | Base English language files |
| **`miss101.dat` – `miss150.dat`** | **French** | Missions 1–50 | French translations |
| **`miss201.dat` – `miss250.dat`** | **German** | Missions 1–50 | German translations |
| **`miss301.dat` – `miss350.dat`** | **Italian** | Missions 1–50 | Italian translations |

---

### 2.4 Map Graphics & Collision Data

* **`hblk01.dat`** (High-Resolution Tile Blocks):
  * Master graphical payload containing all **256 isometric building blocks** (64×48 pixels each).
  * Each isometric block is constructed from 6 individual rectangular subtiles of 32×16 pixels.
  * The first 6,144 bytes are file pointers ($256 \text{ tiles} \times 6 \text{ subtiles} = 1,536 \text{ pointers}$).
  * Subtiles are encoded in bottom-to-top order using 4-bit color bitplanes (16 colors) plus 1 transparency bit per pixel (5 bits/pixel total).
* **`col01.dat`** (Tile Collision & Surface Types):
  * 128-byte lookup table defining physical navigation attributes for each tile ID:
    * Flat walkable ground
    * Impassable walls / solid building interiors
    * Slopes (North, South, East, West)
    * Road surfaces / train tracks
    * Void / water boundaries

---

### 2.5 In-Game Color Palettes

Color grading applied to isometric tile graphics according to the mission's geography and lighting:

| Palette File | Environment / Setting | Characteristics |
| :--- | :--- | :--- |
| **`hpal01.dat`** | Temperate Urban | Standard city center (concrete grey, red brick, warm lighting). |
| **`hpal02.dat`** | Industrial Sector | Dirty concrete, dark tarmac, rusty metals. |
| **`hpal03.dat`** | Snow / Arctic / Siberia | White snowbanks, frozen ground, ice-blue highlights. |
| **`hpal04.dat`** | Night / Dusk | Deep navy ambient shadow, glowing amber streetlights and illuminated signage. |
| **`hpal05.dat`** | Desert / Mediterranean | Sandstone, arid yellow dirt, terracotta roofs. |
| **`hpalette.dat`** | Default Gameplay Base | Fallback gameplay palette table. |
| **`mselect.pal`** | Menu & Globe Palette | Full 256-color palette for UI menus and the rotating 3D world globe. |

---

### 2.6 Gameplay Sprites, Cursors & Fonts (`*.dat` / `*.tab`)

Dynamic sprite animations are packaged using paired `.dat` (raw pixel payload) and `.tab` (table of contents) files:

* **`hspr-0.dat` / `hspr-0.tab`**:
  * The main in-game sprite sheet containing thousands of frames:
    * Agents, police, platform guards, and civilians in 8 rotational headings.
    * Movement animations: walking, running, drawing weapon, firing, taking hit, falling dead.
    * Death animations: burning in flames, electric vaporizing, bullet impact collapse.
    * Civilian vehicles, armored sedans, and trains in 8 directions.
    * Ballistic visual effects: bullets, shotgun pellets, laser beams, minigun muzzle flashes, rocket trails, flame jets, explosions, and blood pools.
* **`hpointer.dat` / `hpointer.tab`**:
  * Mouse pointers and targeting reticles: crosshairs, target acquisition frames, directional arrows, and green squad selection markers.
* **`hfnt01.dat`**:
  * Monospaced pixel font used in the in-game HUD (ammo counts, IPA drug meters, agent health numbers).
* **`mspr-0.dat` / `mspr-0.tab`**:
  * Menu interface icons: inventory weapon icons (Pistol, Gauss Gun, Minigun, Energy Shield, etc.), cybernetic body part icons (Brain, Eyes, Chest, Heart, Arms, Legs), and UI buttons.
* **`mfnt-0.dat` / `mfnt-0.tab`**:
  * Proportional UI font used across all menu screens.

---

### 2.7 Menu Screens & Animated Transitions (`m*.dat`)

Menu screens are Autodesk Animator **FLI / FLC video animations**. They provide full-screen animated backdrops when entering and exiting menus:

| Show Animation | Leave Animation | Screen / Menu Function |
| :--- | :--- | :--- |
| **`mtitle.dat`** | — | Main title screen and logo sequence. |
| **`moption.dat`** | **`moptout.dat`** | Game Options menu (sound volume, controls, quit). |
| **`mbrief.dat`** | **`mbrieout.dat`** | Mission Briefing screen with mission dossier. |
| **`mconfup.dat`** | **`mconfout.dat`** | Agent Equip & Cybernetics modification chamber (`mconscr.dat` static backdrop). |
| **`mdebrief.dat`** | **`mdeout.dat`** | Mission Debriefing and combat statistics screen. |
| **`mmap.dat`** | **`mmapout.dat`** | World Syndicate Conquest map (`mmapblk.dat` territory overlays). |
| **`mselect.dat`** | **`mselout.dat`** | Country selection and mission deployment menu. |
| **`mresrch.dat`** | **`mresout.dat`** | Syndicate R&D Laboratory (weapons and cybernetics research). |
| **`mlosa.dat`** | **`mlosaout.dat`** | Save & Load Game menu. |
| **`mmulti.dat`** | **`mmultout.dat`** | Multiplayer connection screen. |
| **`mendwin.dat`** | **`mendlose.dat`** | Campaign victory and game-over cinematic ending sequences. |
| **`mgamewin.dat`** | **`mlosegam.dat`** | Full-screen victory and defeat splash art. |
| **`mlogos.dat`** | **`mminlogo.dat`** | Corporate developer and publisher intro logos. |
| **`mscrenup.dat`** | — | Top menu status bar / frame animation. |

---

### 2.8 Audio & Sound Effect Banks

Raw 8-bit mono PCM wave samples paired with `.tab` index tables:

* **`sound-0.dat` / `sound-0.tab`**:
  * In-game sound effects: Pistol shots, Uzi bursts, Minigun whirr, Gauss Gun explosions, Flamethrower roar, car engines, sirens, ricochets, door opens, and civilian/guard death screams.
* **`sound-1.dat` / `sound-1.tab`**:
  * Tactical voice lines (*"Selected"*, *"Target Eliminated"*, *"Mission Objective Complete"*).
* **`isnds-0.dat` / `isnds-0.tab` & `isnds-1.dat` / `isnds-1.tab`**:
  * Sound effects synchronized specifically to the cinematic intro animation sequence.

---

### 2.9 Cinematic Intro & Auxiliary Assets

* **`intro.dat`**: 3.6 MB full-motion video animation containing the cyberpunk intro cutscene (flying car arrival, alleyway execution, and civilian conversion into a cybernetic agent).
* **`hreq.dat`**: Disk prompt screen graphic.
* **`00.gam`**: Syndicate campaign saved game file (Slot 0).
* **`hele-0.ani`**: Elevator car movement animation.
* **`hfra-0.ani`**: Animated neon billboards, holographic advertising, and signs.
* **`hsta-0.ani`**: Animated static map fixtures (rotating radar dishes, flashing beacons, rooftop vent fans).
* **Audio Drivers & Music**:
  * `gamedg.dll`: Digital audio driver binary.
  * `gamefm.dll`: FM synthesizer driver binary.
  * `sample.ad` / `sample.opl`: Instrument timbres for FM synthesizers.
  * `intro.xmi` / `syngame.xmi`: XMIDI music tracks for intro and gameplay.

---

## 3. Alphabetical Quick-Lookup Table

| File Pattern | Category | Description |
| :--- | :--- | :--- |
| `00.gam` | Save Data | Default campaign save game (slot 0). |
| `col01.dat` | Tile Physics | 128-byte tile collision, walkability, and slope attributes table. |
| `game01.dat`–`game50.dat` | Mission Levels | Campaign mission definitions (entities, spawns, scenarios, objectives). |
| `game90.dat`–`game98.dat` | Mission Levels | Bonus, multiplayer, and demo mission definitions. |
| `game99.dat` | Mission Levels | ReSyndicated editor scratch/playtest template. |
| `hblk01.dat` | Tile Graphics | 256 isometric block tiles (64×48 pixels, 6 subtiles each, 4-bit color). |
| `hfnt01.dat` | HUD Font | In-game tactical HUD font. |
| `hpal01.dat`–`hpal05.dat` | Palettes | 16-color environment palettes (city, industrial, snow, night, desert). |
| `hpalette.dat` | Palettes | Base in-game color palette. |
| `hpointer.dat` / `.tab` | Cursors | Targeting reticles, directional arrows, and squad markers. |
| `hreq.dat` | UI Screen | Disk prompt dialog graphic. |
| `hspr-0.dat` / `.tab` | Gameplay Sprites | Sprites for agents, civilians, cars, weapons, bullet VFX, and explosions. |
| `intro.dat` | Video | 3.6 MB full-motion intro movie. |
| `isnds-0.dat` / `isnds-1.dat` | Audio | Sound effects synchronized to the intro movie. |
| `map01.dat`–`map94.dat` | Map Geometry | 3D isometric tilemap height grids ($128 \times 128$ tiles). |
| `map99.dat` | Map Geometry | ReSyndicated editor custom map export. |
| `mbrief.dat` / `mbrieout.dat` | Menu Video | Mission Briefing screen enter/leave animations. |
| `mconfup.dat` / `mconfout.dat` | Menu Video | Agent Equip & Cybernetics chamber enter/leave animations. |
| `mconscr.dat` | Menu Screen | Static background for the agent configuration screen. |
| `mdebrief.dat` / `mdeout.dat` | Menu Video | Mission Debriefing screen enter/leave animations. |
| `mendwin.dat` / `mendlose.dat` | Menu Video | Campaign victory and defeat cinematic ending sequences. |
| `mfnt-0.dat` / `.tab` | Menu Font | Proportional UI font for menu screens. |
| `mgamewin.dat` / `mlosegam.dat`| Menu Screen | Full-screen game victory and defeat splash art. |
| `miss01.dat`–`miss50.dat` | Briefing Text | English mission briefings and country intelligence dossiers. |
| `miss101.dat`–`miss150.dat`| Briefing Text | French mission briefings. |
| `miss201.dat`–`miss250.dat`| Briefing Text | German mission briefings. |
| `miss301.dat`–`miss350.dat`| Briefing Text | Italian mission briefings. |
| `mlogos.dat` / `mminlogo.dat` | Menu Video | Corporate developer and publisher intro logos. |
| `mlosa.dat` / `mlosaout.dat` | Menu Video | Save & Load menu enter/leave animations. |
| `mmap.dat` / `mmapout.dat` | Menu Video | World Syndicate Conquest map enter/leave animations. |
| `mmapblk.dat` | Menu Overlay | World map territory overlays. |
| `moption.dat` / `moptout.dat` | Menu Video | Game Options menu enter/leave animations. |
| `mresrch.dat` / `mresout.dat` | Menu Video | Syndicate R&D Laboratory enter/leave animations. |
| `mscrenup.dat` | Menu Video | Top menu frame / header animation. |
| `mselect.dat` / `mselout.dat` | Menu Video | Country selection and deployment menu enter/leave animations. |
| `mselect.pal` | Palettes | 256-color palette for UI menus and rotating world globe. |
| `mspr-0.dat` / `.tab` | Menu Sprites | Weapon inventory icons, cybernetic implants, and UI buttons. |
| `mtitle.dat` | Menu Video | Main title screen animation. |
| `sound-0.dat` / `.tab` | Audio | In-game weapon, environmental, and combat sound effects. |
| `sound-1.dat` / `.tab` | Audio | Tactical speech announcements (*"Selected"*, *"Target Eliminated"*). |
