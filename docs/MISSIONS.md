# ReSyndicated Mission Architecture & Data Specification

This document provides a comprehensive overview of the mission architecture in **ReSyndicated**. It details how mission levels are organized, how actors, vehicles, and scripted paths function, how enemy factions are configured with specific or wildcard AI tactics, how objectives are structured, and how scripted non-agent actors compare to generic wandering NPCs.

---

## 1. Overview & Data Architecture

### Data-Driven Mission Design
The vast majority of mission data in ReSyndicated is completely data-driven and loaded directly from binary level files (`game01.dat` to `game50.dat`). These files define:
* **The City Map**: Links to the 3D isometric tilemap geometry, environmental palettes, and boundary limits.
* **Entities**: Initial placements, facing directions, and health for player agents, rival agents, police, guards, and civilians.
* **Vehicles**: Spawn locations, speeds, and driver assignments for civilian cars, armored sedans, and trains.
* **Interactive Objects**: Placement of doors, street furniture, lampposts, traffic lights, and barriers.
* **Weapons & Equipment**: Pre-placed ground pickups and initial actor inventories.
* **Scripted Scenarios**: Linked waypoint paths, proximity ambushes, and vehicle boarding routines for non-agent NPCs and vehicles.
* **AI Tactics FSM**: Faction-specific and wildcard finite state machine chains controlling rival syndicate squad behavior.
* **Mission Objectives**: Specific goals required to achieve victory (Assassinate, Persuade, Protect, Evacuate, etc.).

### Engine Enhancements & Handling
To provide a smooth gameplay experience across all missions, the engine incorporates several dedicated systems:
1. **Level Waypoint Adjustments**: A small number of campaign missions feature tailored waypoint adjustments to address historical geometry quirks (such as non-walkable destination coordinates for cars in Mission 10, or completing target patrol paths in Mission 40).
2. **Train & Transit System**: Trains dynamically link locomotives and passenger carriages, enabling smooth platform arrivals, boarding checks, and safe passenger alighting onto platform surfaces.
3. **Objective Robustness**: Built-in validation ensures missions with unusual or legacy objective data are parsed safely without errors.

---

## 2. Mission Level Data Structure

Each mission's level data is organized into structured tables containing all active game elements:

| Data Section | Capacity | Description |
| :--- | :---: | :--- |
| **Header & Info** | 1 record | Map identification, geographic boundaries, and default mission tech level. |
| **Tile Grid References** | 16,384 cells | $128 \times 128$ grid mapping coordinates to active entities on the map. |
| **People (Characters)** | 256 actors | All human entities: player agents, rival syndicate agents, police, guards, and civilians. |
| **Vehicles** | 64 vehicles | Cars, armored vehicles, train locomotives, and carriages. |
| **Static Objects** | 400 objects | Interactive and decorative fixtures (doors, trees, lampposts, traffic lights, barriers). |
| **Weapons & Pickups** | 512 items | Starting inventories and weapons lying on the ground. |
| **Visual Effects** | 256 markers | Transient visual indicators and map effect points. |
| **Scripted Scenarios** | 2,048 nodes | Waypoint paths, delays, ambush triggers, and vehicle interaction sequences (non-agent NPCs & vehicles). |
| **AI Tactics Block** | 1 header + 128 nodes | 1,952-byte tactical FSM block (offset 114,058) governing enemy syndicate factions. |
| **Mission Objectives** | Up to 6 goals | Conditions required to complete or fail the mission. |

---

## 3. Mission Actors & Entities

### 3.1 Characters (Pedestrians)
Every character on the map is defined with:
* **Coordinates**: Precise tile position ($X, Y, Z$) and sub-tile alignment.
* **Location Status**: Active on the map, seated inside a vehicle (as driver or passenger), or positioned on an elevated walkway.
* **Character Category**:
  - **Player Agent**: Squad members controlled by the player.
  - **Enemy Agent**: Rival syndicate operatives armed to eliminate the player's squad.
  - **Police Officer**: City law enforcement upholding public order.
  - **Guard**: Facility sentries defending corporate compounds or VIPs.
  - **Civilian / Criminal**: Urban population and local bystanders.
* **Script Assignment**: Optional link to a waypoint sequence in the scenario table.
* **Vehicle Assignment**: Link to an assigned car or train carriage.
* **Attributes & Cybernetics**: Baseline health, stimulation levels, and cybernetic modifications.

### 3.2 Vehicles
* **Vehicle Types**: Civilian sedans, armored transports, train locomotives, and passenger carriages.
* **Driver & Passenger State**: Tracks assigned driver and passenger capacity.
* **Movement Rules**: Vehicles follow road tiles, accelerate, steer around corners, and stop for pedestrians crossing the street.

### 3.3 Static Map Objects
* Interactive features such as sliding doors, gates, trees, traffic signals, and street lamps.
* Objects include visual animation frames, orientation headings, and collision boundaries.

### 3.4 Dynamic Enemy Agent Armament
In campaign missions, rival syndicate agents are equipped dynamically at mission start based on the player syndicate's technological progress:
1. The engine checks the player syndicate's unlocked firearm research.
2. Enemy agents are equipped with the two highest-ranked ballistic firearms available in the player's research pool (such as Shotguns early in the campaign, or Miniguns and Long Range rifles later).
3. If Time Bombs are unlocked in research, enemy agents may also carry explosive charges.

### 3.5 Enemy Agent Cyberware & Tactical AI Hierarchy
Rival syndicate agents are augmented with cybernetic modifications (Legs, Arms, Chest, Heart, Eyes, Brain) according to a 3-tier hierarchy, while their combat engagement and multi-stage coordination are driven at the **faction level** by the AI Tactics FSM (see [Section 5](#5-enemy-factions--ai-tactics-system)):

1. **Individual Agent Override**: Specific enemy agents can be configured with customized cyberware bitmasks (`mods_info` bit 15 `0x8000`) in mission data. This allows mission authors to design unique boss encounters, snipers, or heavily armored agents in the editor.
2. **Mission Default Tech Level**: Any agent without an individual override inherits the mission's default tech level (Tech Level 0 through 3), easily configured in the mission editor.
3. **Campaign Territory Progression**: When playing unedited campaign missions, the game automatically applies tech levels matching campaign progression (early territories feature unaugmented enemies, mid-campaign territories feature Tech Levels 1 and 2, and late-game strongholds deploy Tech Level 3 agents).

---

## 4. Scripted Actions & Paths (Scenario System)

Scripted movements and behaviors for **non-agent NPCs** (Guards, Police, Civilians) and **Vehicles** are organized into linked lists of waypoint nodes. **Enemy syndicate agents do not use scenario waypoint routes** (in all 59 original campaign and expansion files, zero enemy agents carry ped scripts, and the Mission Editor restricts route editing to non-agents and vehicles); enemy agent coordination uses the Faction AI Tactics system documented in [Section 5](#5-enemy-factions--ai-tactics-system).

### Scenario Action Types

| Action Code | Action Name | Description |
| :---: | :--- | :--- |
| **1** | **Walk / Drive to Location** | Entity moves directly to target coordinates $(X, Y, Z)$. If inside a car or train, drives the vehicle to the destination. |
| **2** | **Use / Enter Vehicle** | Entity walks to the designated vehicle and boards as driver or passenger. |
| **5** | **Timed Wait / Pause** | Entity pauses at the waypoint for a specified delay (e.g., 5 seconds) before resuming their route. |
| **6** | **Unboard / Leave Vehicle** | Passenger exits the vehicle to an adjacent walkable ground or platform tile and resumes on-foot movement. |
| **7** | **Target Escape** | Triggered when an assassination or capture target reaches their getaway extraction point, resulting in mission failure. |
| **8** | **Proximity Ambush Trigger** | Guard holds position until player agents enter a 6-tile radius, triggering an immediate ambush response. |
| **9** | **Reset / Loop Script** | Loops the waypoint chain back to the beginning for continuous patrol routes. |
| **10** | **Train Station Stop** | Train locomotive halts at a station platform for approximately 7 seconds to allow passengers to board and exit before resuming the line. |

---

## 5. Enemy Factions & AI Tactics System

Unlike civilians, police, and guards (whose scripted movement uses Scenario Waypoints), rival syndicate operatives operate as **corporate squads** controlled by the **AI Tactics Finite State Machine (FSM)**. Tactics can be authored specifically for an individual faction or shared generically across squads via **Wildcard (`255`)** rules.

### 5.1 Corporate Factions & Character Slot Grouping
In level data, the first 64 entries of the 256-slot `People` table (`ped_idx` `0..63`) are reserved for syndicate agents, partitioned into 8 fixed corporate squads of 8 agents each (`factionId = ped_idx / 8`), while slots `64..255` are reserved for non-agents:

| Faction ID | Character Slots | Corporate Syndicate | Visual Recolor | Role |
| :---: | :---: | :--- | :--- | :--- |
| **0** | `0..7` | Player Syndicate | Red | Player-controlled squad (tactics do not apply). |
| **1** | `8..15` | Blue Syndicate | Blue Grey | Rival AI Squad 1. |
| **2** | `16..23` | Orange Syndicate | Light Red / Orange | Rival AI Squad 2. |
| **3** | `24..31` | Yellow Syndicate A | Yellow | Rival AI Squad 3. |
| **4** | `32..39` | Yellow Syndicate B | Yellow | Rival AI Squad 4 (independent tactics, shares Yellow colors). |
| **5** | `40..47` | Purple Syndicate | Purple | Rival AI Squad 5 (American Revolt / custom maps). |
| **6** | `48..55` | Grey Syndicate | Grey | Rival AI Squad 6 (American Revolt / custom maps). |
| **7** | `56..63` | Cyan Syndicate | Cyan / Teal | Rival AI Squad 7 (American Revolt / custom maps). |
| **255** | — | **Wildcard (Generic)** | — | Generic directive eligible for any hostile faction (`1..7`). |

When an enemy agent is idle (not currently sweeping or pursuing a target), its ambient movement is governed by its `People.state` flag: **Patrol (`0x10`)** wanders streets randomly, whereas **Stationary (`0x00`)** holds post as a sentry.

### 5.2 AI Tactics Block Structure (`AITacticsBlock`)
Mission binary data stores a 1,952-byte `AITacticsBlock` at offset `114,058`:
* **Header Padding & `AIMissionHeader` (32 bytes total)**: 24 bytes padding followed by `num_squads`, `peds_per_squad`, and `max_rush_factions`.
* **128 `AITactic` Nodes (`0..127`, 15 bytes each)**:
  - `prev_tactic` / `next_tactic` (`uint16`): Doubly linked chain indices (`0` indicates a root node on `prev_tactic`, or end/fallback on `next_tactic`).
  - `state_counter` (`uint8`): Legacy state counter field.
  - `faction_id` (`uint8`): Owning faction (`1..7` for a specific squad, or `255` for Wildcard).
  - `sub_filter` (`uint8`): Behavior flags — `0x08` (**Target Armed Required**), `0x10` (**Completed**), `0x40` (**Disabled**, bypassed in chain resolution without unlinking).
  - `mode` (`uint8`): Tactical AI mode (`1..10`; `0` = unused slot).
  - `sub_mode` (`uint8`): Secondary target/sub-mode field.
  - `param1`, `param2`, `param3` (`uint16` each): Mode-dependent parameters (target faction, timers, alert radius, vehicle index, or map coordinates).

### 5.3 Faction-Specific vs. Generic (Wildcard) Assignment
At mission start (`Mission::getRootTacticIndex`), hostile factions `1..N` are assigned their starting root tactic (`prev_tactic == 0`) using a sequential two-pass resolution that mirrors original DOS `MAIN.EXE` (`0x14588`):

```
                      Squad s (checked sequentially for s = 1..7)
                                           |
            +------------------------------+------------------------------+
            | Pass 1: Scan root nodes (prev_tactic == 0) in slots 1..127  |
            |   1. First UNCLAIMED enabled root with faction_id == s      |
            |   2. Else first UNCLAIMED enabled root with faction_id == 255
            +------------------------------+------------------------------+
                                           | (none unclaimed)
                                           v
            +-------------------------------------------------------------+
            | Pass 2: Reuse already-claimed roots in slots 1..127         |
            |   1. First enabled root with faction_id == s                |
            |   2. Else first enabled root with faction_id == 255         |
            +------------------------------+------------------------------+
                                           | (no match in 1..127)
                                           v
            +-------------------------------------------------------------+
            | Fallback: Slot 0 (Tactic 0)                                 |
            |   Used ONLY if no tactic in 1..127 matched, provided        |
            |   Tactic 0 is enabled and faction_id == s or 255            |
            +-------------------------------------------------------------+
```

Key design implications for **Faction-Specific** vs **Wildcard (`255`)** setups:
* **Faction-Specific (`faction_id = 1..7`)**:
  - Dedicated strictly to that corporate squad.
  - Always takes precedence over Wildcard (`255`) entries for that faction.
  - Ideal for asymmetrical mission setups — e.g., setting Faction 1 (Blue) to hold a 12-tile **Proximity Tripwire** guarding a compound while Faction 2 (Orange) starts in **Seek & Destroy**, or having Faction 4 (Yellow B) execute a **Rival Sweep** against Faction 3 (Yellow A).
* **Generic / Wildcard (`faction_id = 255`)**:
  - Shared pool directives available to any hostile squad (`1..7`) that does not have an unclaimed faction-specific root.
  - **Sequential Claiming in Pass 1**: Because Pass 1 marks each claimed root slot (`1..127`) as taken as it walks `s = 1 -> 2 -> ... -> 7`, placing two Wildcard roots in slots `1` and `2` assigns slot `1` to the first squad without its own faction tactic, slot `2` to the next, and wraps via Pass 2 for any remaining squads.
  - **Reserved `Tactic 0` Fallback**: Slot `0` (`Tactic 0`) is excluded from slots `1..127` scanning so it acts strictly as a global fallback (in original campaign data, `Tactic 0` is typically pre-populated as `faction_id = 255`, Mode `3` Objective Sweep targeting Player `0`), and also serves as the activation target when a terminal **Proximity Tripwire** (`next_tactic == 0`) triggers.
* **No Matching Tactic (`rootTactic == -1`)**: If all matching tactics are absent or disabled (`0x40`), enemy agents act as local defensive sentries (except in *American Revolt* Combat Sweep missions where unassigned hostile squads default to hunter sweep when custom tactics exist).

### 5.4 Tactical AI Modes & Chained Transitions
When a squad advances (`advanceTactic` -> `Mission::updateSquadTactic`), every living agent in that faction switches together to the next enabled node along `next_tactic` (skipping disabled `0x40` nodes). Entering a hunter mode also boosts squad Adrenaline (`100%`) and clears previous idle actions.

| Mode | Name | Target / Parameters | Runtime Behavior |
| :---: | :--- | :--- | :--- |
| **1** | **Seek & Destroy** | `param1` / `sub_mode`: Target Faction (`0` = Player, `1..7` = Rival, `255` = Other Rival Factions) | Global hunter mode; pursues targets across the map without range cutoffs. |
| **2** | **Rival Sweep** | `param1` / `sub_mode`: Target Faction (`0` = Player, `1..7` = Rival) | Hunts and eliminates the specified faction across the map. Advances to `next_tactic` when all members of that rival faction are eliminated. |
| **3** | **Objective Sweep** | `param1` / `sub_mode`: Target Faction (`0` = Player, `1..7` = Rival) | Sweeps the target faction unconditionally across the map. **Self-Sweep Inhibit**: if `param1 == own faction_id`, hunter sweep is suppressed so the squad stays as stationary sentries. |
| **4** | **Stationary Guard** | `param1`: Guard radius in tiles (`0` = default 12); `0x08` armed flag supported | Non-hunter defensive post; holds position and engages visible hostiles within sight/LOS (respecting `0x08` if checked). |
| **5** | **Weapon Ambush** | `param1`: Hold range in tiles (`0` = equipped weapon range); `0x08` armed flag supported | Holds ground until a target enters lethal weapon range and direct line of sight, then fires and advances along `next_tactic`. |
| **6** | **Patrol / Search** | `param1`, `param2`, `param3`: Target `(X, Y, Z)` tile coordinates | Stored directive targeting specific map patrol coordinates. |
| **7** | **Proximity Tripwire** | `param1`: Stage timer (`param1 * 1350 ms`, `0` = default 10 stages) or radius (`<= 30` if `param2 == 0`); `param2`: Perimeter radius in tiles (`0` = default 12); `0x08` armed flag supported | Waits until the downstream tactic's target faction enters the alert perimeter (or, for **chained** tripwires with `next_tactic > 0`, until the countdown timer expires). Terminal tripwires (`next_tactic == 0`) wait indefinitely for proximity breach and advance to `Tactic 0`. |
| **8** | **Board Vehicle** | `param1`: Target Vehicle index | Stored directive instructing the squad to enter the specified vehicle. |
| **9** | **Evacuate** | `param1`, `param2`, `param3`: Evacuation `(X, Y, Z)` tile coordinates | Stored directive specifying retreat/extraction coordinates. |
| **10** | **Delay Timer** | `param1`: Duration in `0.1s` ticks (`param1 * 100 ms`) | Halts squad movement immediately (bypasses the standard 3s start delay) and advances to `next_tactic` when the countdown reaches zero. |

#### Runtime Combat & Sensing Rules
* **Initial Mission-Start Latency**: At mission start, tactics other than **Delay Timer (`10`)** apply a 3.0-second startup delay before initial tactical movement begins.
* **Proximity Tripwire Anchors (`Mode 7`)**: Against the **Player Squad (`0`)**, *every* living sentry in the faction acts as an independent alert post checking the perimeter radius. Against **Rival Factions (`1..7`)**, proximity is anchored to the squad leader slot (`factionId * 8`) so opposing squads pace cross-city marches cohesively without tripping early on isolated sentries.
* **Rival March Cohesion & Weapon Holstering**: When sweeping another rival faction (`1..7`), hunters march toward the target squad leader slot (`targetFactionId * 8`) and keep weapons holstered whenever distance exceeds `weapon_range + 256` world units (~1 tile outside range), drawing firearms only when closing into engagement distance.
* **Line-of-Sight & Weapon Fallback**: Non-hunter sentries enforce eye-level (`Z + 64`) line-of-sight checks against targets beyond `512` units, whereas active hunter modes bypass sight distance cutoffs (`1e9`). Enemy agents without a static weapon entry in mission data automatically receive a loaded Pistol so tactical engagement always functions.
* **Mission Editor Integration**: Enemy factions and tactics are configured in the **AI Tactics Editor** (accessible from the Enemy Agent inspector's **Edit Faction AI Tactics...** button or `Mission -> AI Tactics...`), where both faction-specific (`1..7`) and Wildcard (`255`) chains can be created, chained, disabled (`0x40`), or undone/redone.

---

## 6. Mission Objectives

Missions support up to 6 simultaneous objectives:

| Objective Code | Objective Name | Primary Target | Win/Loss Condition |
| :---: | :--- | :--- | :--- |
| **1** | **Persuade** | Target Person | Brainwash the target using the Persuadotron. The target must survive and reach the extraction zone. |
| **2** | **Assassinate** | Target Person | Eliminate the target. Mission succeeds upon the target's death. |
| **3** | **Protect** | Target Person | Escort the target safely to the extraction zone. Mission fails if the target is killed. |
| **5** | **Acquire Equipment** | Weapon / Item | Retrieve a specific weapon, access card, or equipment pickup from the map. |
| **10** | **Police Sweep** | All Police | Neutralize all police forces present in the city. |
| **11** | **Combat Sweep (Agent Sweep)** | Rival Agents | Eliminate all rival syndicate agents operating in the sector. |
| **14** | **Destroy Vehicle** | Target Vehicle | Destroy the specified car or transport. |
| **15** | **Use Vehicle** | Target Vehicle | Board and take control of the specified vehicle. |
| **16** | **Evacuate** | Extraction Zone | Assemble all surviving squad agents (and required escort targets) at the extraction point to complete the mission. |

---

## 7. Scripted Actors vs. Generic NPCs

Non-agent characters (Civilians, Police, and Guards) in ReSyndicated are divided into **Scripted Actors** and **Generic NPCs**. *(Rival Syndicate agents are governed instead by the Faction AI Tactics FSM in [Section 5](#5-enemy-factions--ai-tactics-system).)* Their behaviors differ significantly across pathfinding, vehicle interactions, and combat responses:

```
+-------------------------------------------------------------------------------+
|                                MISSION START                                  |
+-------------------------------------------------------------------------------+
                                       |
                           Has Assigned Waypoints?
                                       |
                  +--------------------+--------------------+
                  |                                         |
               [Yes]                                       [No]
                  |                                         |
                  v                                         v
       +-----------------------+                 +-----------------------+
       |    SCRIPTED ACTOR     |                 |      GENERIC NPC      |
       +-----------------------+                 +-----------------------+
       | * Sequential Waypoints|                 | * Generalized Wander  |
       | * Vehicle Operations  |                 | * Random Turns at     |
       | * Proximity Ambushes  |                 |   Intersections       |
       | * Mission-Focused     |                 | * Flee / Panic from   |
       | * Escapes & Getaways  |                 |   Gunfire & Threats   |
       +-----------------------+                 +-----------------------+
```

### Behavior Comparison

| Feature | Scripted Actors | Generic NPCs |
| :--- | :--- | :--- |
| **Route Definition** | Follows a defined list of waypoints, delays, and triggers. | No predefined route; wanders streets and turns randomly at intersections. |
| **Vehicle Interaction** | Board specific cars, drive along planned getaway routes, or alight at specific destinations. | Civilians and platform guards only board trains if standing near platform stops. |
| **Reaction to Gunfire** | Focuses on executing their script (e.g. running to a getaway vehicle or escort route) and does not panic from nearby gunfire. | Screams and panics when hearing gunfire or seeing drawn weapons, sprinting away from danger. |
| **Vehicle Attack Bailout** | If an occupied getaway car is attacked and damaged, or carjacked, passengers bail out, abort their script, and flee in panic on foot. | Civilians run from damaged or exploding vehicles. |
| **Persuasion Effect** | When persuaded, waypoint scripts are cleared and the character joins the squad escort cluster. | When persuaded, wandering stops and the character joins the squad escort cluster. |
| **Tactical Role** | VIPs, assassination targets, getaway drivers, patrol leaders, and ambushes. | City population, background bystanders, and ambient police patrols. |
