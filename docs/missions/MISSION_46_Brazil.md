# Mission 46: Brazil

**Country**: Brazil (`#CNTRY_46`)

**Map ID**: Map 82 (`map82.dat`)

**Map Tile Bounding Box**: Tile X [0 – 99], Tile Y [8 – 43]

**Total Entities**: 95 Active Peds (4 Player Agents, 54 Key/Scripted NPCs, 37 Generic Civilians), 3 Vehicles, 50 Weapons.

---

## 1. Briefing Summary

DEEP STRIKE.

SABOTAGE.

AN ENEMY SYNDICATE HAS ESTABLISHED A SUPPLY BASE IN A NEARBY CITY. THEY ARE USING IT TO PROVIDE A STOPPING OFF POINT FOR THEIR FORCES.

AS PREPARATION FOR AN ASSAULT STRIKE YOUR TEAM IS TO ENTER THE BASE AND DESTROY ALL VEHICLES PRESENT THEREIN. NO TRACE MUST BE LEFT OF THE ATTACK.

**CITY UPDATE:**

THE BASE IS LOCATED OVER TO THE WEST OF THE CITY AND WE BELIEVE THAT IT IS ONLY ACCESSIBLE BY ROAD.

**DEFENCE UPDATE:**

THE BASE IS CURRENTLY UNDERSTAFFED AND SO NOW IS THE IDEAL TIME TO STRIKE. WE BELIEVE THAT ONLY ONE ENEMY SQUAD IS IN THE AREA TO PROVIDE DEFENCE AND OVERSEE THE FINAL STAGES OF THE PROJECT.

**TACTICS:**

OBVIOUSLY YOU WILL HAVE TO LOCATE A MEANS OF TRANSPORT TO GAIN ACCESS TO THE BASE. HEAVY OR CLOSE ASSAULT WEAPONS WILL BE REQUIRED TO DESTROY THE VEHICLES WHILST AGENTS WITH SUPPORT OR LONG RANGE WEAPONS CAN PICK OFF GUARDS AND KEEP AGENTS AT BAY.

THE VEHICLE THAT YOU USE TO INFILTRATE THE BASE ALSO HAS TO BE DESTROYED.

---

## 2. Mission Objectives

### Objective #1: Destroy Target Vehicle

* **Type**: Destroy Target Vehicle
* **Target**: Vehicle #1 (Fire Engine) at Tile (11, 24, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Destroy Target Vehicle

* **Type**: Destroy Target Vehicle
* **Target**: Vehicle #0 (Civilian Sedan) at Tile (11, 43, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #3: Destroy Target Vehicle

* **Type**: Destroy Target Vehicle
* **Target**: Vehicle #2 (Civilian Sedan) at Tile (94, 42, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #4: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (93, 41, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (91, 43, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (91, 42, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (91, 42, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (91, 41, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP | (10, 45, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP | (21, 45, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP | (20, 30, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP | (13, 22, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 16 HP | (11, 11, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 16 HP | (19, 13, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 16 HP | (21, 2, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #15 | Enemy Agent (Faction 1: Blue) | 16 HP | (28, 11, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #16 | Police Officer | 6 HP (Default) | (62, 41, 5) | None | Walking (Equipped: Shotgun (#1)) | None (Wandering) |
| Ped #17 | Police Officer | 6 HP (Default) | (26, 32, 0) | None | Walking (Equipped: Shotgun (#2)) | None (Wandering) |
| Ped #18 | Police Officer | 6 HP (Default) | (67, 14, 0) | None | Walking (Equipped: Shotgun (#3)) | None (Wandering) |
| Ped #19 | Police Officer | 6 HP (Default) | (53, 14, 0) | None | Walking (Equipped: Shotgun (#4)) | None (Wandering) |
| Ped #20 | Police Officer | 6 HP (Default) | (76, 9, 0) | None | Walking (Equipped: Shotgun (#5)) | None (Wandering) |
| Ped #21 | Police Officer | 6 HP (Default) | (67, 85, 0) | None | Walking (Equipped: Shotgun (#6)) | None (Wandering) |
| Ped #22 | Police Officer | 6 HP (Default) | (27, 14, 0) | None | Walking (Equipped: Shotgun (#7)) | None (Wandering) |
| Ped #23 | Police Officer | 6 HP (Default) | (62, 39, 5) | None | Walking (Equipped: Shotgun (#8)) | None (Wandering) |
| Ped #24 | Police Officer | 6 HP (Default) | (90, 15, 0) | None | Walking (Equipped: Shotgun (#9)) | None (Wandering) |
| Ped #26 | Police Officer | 6 HP (Default) | (73, 3, 0) | None | Walking (Equipped: Shotgun (#11)) | None (Wandering) |
| Ped #27 | Police Officer | 6 HP (Default) | (96, 40, 0) | None | Walking (Equipped: Shotgun (#12)) | None (Wandering) |
| Ped #29 | Police Officer | 6 HP (Default) | (52, 47, 0) | None | Walking (Equipped: Shotgun (#14)) | None (Wandering) |
| Ped #31 | Police Officer | 6 HP (Default) | (72, 8, 0) | None | Walking (Equipped: Shotgun (#16)) | None (Wandering) |
| Ped #32 | Police Officer | 6 HP (Default) | (79, 43, 0) | None | Walking (Equipped: Shotgun (#17)) | None (Wandering) |
| Ped #34 | Police Officer | 6 HP (Default) | (53, 14, 0) | None | Walking (Equipped: Shotgun (#19)) | None (Wandering) |
| Ped #35 | Police Officer | 6 HP (Default) | (37, 82, 0) | None | Walking (Equipped: Shotgun (#20)) | None (Wandering) |
| Ped #37 | Police Officer | 6 HP (Default) | (69, 44, 0) | None | Walking (Equipped: Shotgun (#22)) | None (Wandering) |
| Ped #38 | Police Officer | 6 HP (Default) | (117, 25, 1) | None | Walking (Equipped: Shotgun (#23)) | None (Wandering) |
| Ped #39 | Police Officer | 6 HP (Default) | (70, 45, 0) | None | Walking (Equipped: Shotgun (#24)) | None (Wandering) |
| Ped #40 | Police Officer | 6 HP (Default) | (56, 14, 0) | None | Walking (Equipped: Shotgun (#25)) | None (Wandering) |
| Ped #41 | Police Officer | 6 HP (Default) | (117, 31, 0) | None | Walking (Equipped: Shotgun (#26)) | None (Wandering) |
| Ped #42 | Police Officer | 6 HP (Default) | (61, 27, 0) | None | Walking (Equipped: Shotgun (#27)) | None (Wandering) |
| Ped #43 | Police Officer | 6 HP (Default) | (33, 16, 0) | None | Walking (Equipped: Shotgun (#28)) | None (Wandering) |
| Ped #44 | Police Officer | 6 HP (Default) | (72, 35, 0) | None | Walking (Equipped: Shotgun (#29)) | None (Wandering) |
| Ped #45 | Police Officer | 6 HP (Default) | (90, 32, 0) | None | Walking (Equipped: Shotgun (#30)) | None (Wandering) |
| Ped #46 | Police Officer | 6 HP (Default) | (31, 2, 0) | None | Walking (Equipped: Shotgun (#31)) | None (Wandering) |
| Ped #47 | Police Officer | 6 HP (Default) | (28, 27, 0) | None | Walking (Equipped: Shotgun (#32)) | None (Wandering) |
| Ped #48 | Police Officer | 6 HP (Default) | (76, 3, 0) | None | Walking (Equipped: Shotgun (#33)) | None (Wandering) |
| Ped #49 | Police Officer | 6 HP (Default) | (65, 20, 0) | None | Walking (Equipped: Shotgun (#34)) | None (Wandering) |
| Ped #52 | Police Officer | 6 HP (Default) | (72, 16, 0) | None | Walking (Equipped: Shotgun (#37)) | None (Wandering) |
| Ped #53 | Police Officer | 6 HP (Default) | (66, 38, 5) | None | Walking (Equipped: Shotgun (#38)) | None (Wandering) |
| Ped #54 | Police Officer | 6 HP (Default) | (29, 2, 0) | None | Walking (Equipped: Shotgun (#39)) | None (Wandering) |
| Ped #55 | Police Officer | 6 HP (Default) | (95, 19, 0) | None | Walking (Equipped: Shotgun (#40)) | None (Wandering) |
| Ped #56 | Police Officer | 6 HP (Default) | (69, 79, 0) | None | Walking (Equipped: Shotgun (#41)) | None (Wandering) |
| Ped #58 | Police Officer | 6 HP (Default) | (32, 50, 1) | None | Walking (Equipped: Shotgun (#43)) | None (Wandering) |
| Ped #59 | Police Officer | 6 HP (Default) | (32, 73, 0) | None | Walking (Equipped: Shotgun (#44)) | None (Wandering) |
| Ped #60 | Police Officer | 6 HP (Default) | (78, 4, 0) | None | Walking (Equipped: Shotgun (#45)) | None (Wandering) |
| Ped #61 | Police Officer | 6 HP (Default) | (119, 14, 0) | None | Walking (Equipped: Shotgun (#46)) | None (Wandering) |
| Ped #64 | Enemy Agent (Faction 8) | 16 HP (Default) | (54, 85, 0) | None | Wandering Patrol (Default: Pistol) | Faction 8 AI Tactics: Defensive Sentry (No active tactic) |
| Ped #65 | Enemy Agent (Faction 8) | 16 HP (Default) | (20, 9, 0) | None | Wandering Patrol (Default: Pistol) | Faction 8 AI Tactics: Defensive Sentry (No active tactic) |
| Ped #66 | Enemy Agent (Faction 8) | 16 HP (Default) | (72, 13, 0) | None | Wandering Patrol (Default: Pistol) | Faction 8 AI Tactics: Defensive Sentry (No active tactic) |
| Ped #67 | Enemy Agent (Faction 8) | 16 HP (Default) | (120, 38, 0) | None | Wandering Patrol (Default: Pistol) | Faction 8 AI Tactics: Defensive Sentry (No active tactic) |
| Ped #69 | Enemy Agent (Faction 8) | 16 HP (Default) | (27, 65, 0) | None | Wandering Patrol (Default: Pistol) | Faction 8 AI Tactics: Defensive Sentry (No active tactic) |
| Ped #75 | Enemy Agent (Faction 9) | 16 HP (Default) | (55, 13, 0) | None | Wandering Patrol (Default: Pistol) | Faction 9 AI Tactics: Defensive Sentry (No active tactic) |
| Ped #148 | Guard | 8 HP (Default) | (26, 26, 4) | None | Standing (Equipped: Long Range Rifle (#49)) | None (Stationary) |
| Ped #149 | Guard | 8 HP (Default) | (26, 20, 4) | None | Standing (Equipped: Long Range Rifle (#48)) | None (Stationary) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 8`, `max_rush_factions = 0`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (8 agents: Peds #8–#15)

* **Idle Movement State**: 8 Stationary Sentry
* **Assigned Root Tactic**: Tactic #1 *(Wildcard (Generic Pool - Sequential Claim))*
* **Execution Chain**:
  1. **Tactic #1** [Wildcard / Objective Sweep]: Target: Player Squad

#### Faction 8 (5 agents: Peds #64–#69)

* **Idle Movement State**: 5 Wandering Patrol
* **Assigned Root Tactic**: None (Remains local defensive sentry unless triggered in line of sight)

#### Faction 9 (1 agents: Ped #75)

* **Idle Movement State**: 1 Wandering Patrol
* **Assigned Root Tactic**: None (Remains local defensive sentry unless triggered in line of sight)

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |
| #1 | Root (Active at Start) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

No scripted waypoint paths defined for non-agent NPCs or vehicles in this mission.

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (11, 43, 0) | 1024 | None |
| Car #1 | Fire Engine | (11, 24, 0) | 1024 | None |
| Car #2 | Civilian Sedan | (94, 42, 0) | 1024 | None |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 1 | Total Inventory Items: 49

#### Ground Pickups:

| Weapon Index | Weapon Subtype | Ground Tile (X, Y, Z) |
| :--- | :--- | :--- |
| Weapon #0 | Minigun | (9, 42, 0) |

---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (91, 43, 0) and establish perimeter control.
* **Enemy Composition**: 14 enemy syndicate agents across 3 active faction(s) plus 40 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
