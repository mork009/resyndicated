# Mission 19: Northern Territories

**Country**: Northern Territories (`#CNTRY_19`)

**Map ID**: Map 34 (`map34.dat`)

**Map Tile Bounding Box**: Tile X [6 – 91], Tile Y [22 – 56]

**Total Entities**: 133 Active Peds (4 Player Agents, 41 Key/Scripted NPCs, 88 Generic Civilians), 2 Vehicles, 41 Weapons.

---

## 1. Briefing Summary

TWIN CITIES.

ASSASSINATION.

A COUPLE OF REBEL ACTIVISTS HAVE BEEN SPOTTED STIRRING UP TROUBLE IN THE NEARBY DOUBLE CITY. THE LOCAL POLICE FORCE SEEM TO BE RELUCTANT TO DEAL WITH THEM AND SO YOUR SQUAD IS TO BE DEPLOYED TO TAKE CARE OF THEM BOTH.

**CITY UPDATE:**

THE CITY IS SPLIT BY A LARGE EXPANSE OF WATER WHICH IS SPANNED BY TWO ROAD BRIDGES. ACCESS TO THESE BRIDGES IS RESTRICTED SO YOU WILL HAVE TO FIND TRANSPORT TO CROSS.

**TACTICS:**

A LOW KEY ASSASSIN SQUAD OF ONE MAYBE TWO AGENTS WOULD STAND THE BEST CHANCE OF ELIMINATING THE TARGETS WITHOUT ANNOYING THE POLICE. LONG RANGE RIFLES ARE THE PRIME WEAPONS FOR PINPOINT ASSASSINATIONS AND SHOULD BE USED TO THEIR BEST ADVANTAGE. ONE AGENT COULD ENTER THE HOUSE TO FLUSH OUT THE REBELS WHILE THE OTHER LIES IN WAIT WITH A SNIPER RIFLE.

KEEP AN EYE ON YOUR TRANSPORT AS IT MAY BE REQUIRED TO REACH YOUR PICKUP POINT.

---

## 2. Mission Objectives

### Objective #1: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #12 (Guard) at Tile (37, 57, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #13 (Guard) at Tile (22, 61, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (78, 58, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (78, 58, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (78, 57, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (78, 57, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP | (104, 21, 0) | None | Stationary Sentry (Equipped: Uzi (#4)) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP | (104, 21, 0) | None | Stationary Sentry (Equipped: Uzi (#5)) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP | (20, 21, 0) | None | Stationary Sentry (Equipped: Minigun (#2)) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP | (21, 21, 0) | None | Stationary Sentry (Equipped: Laser (#0)) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #12 | Guard | 16 HP | (37, 57, 0) | None | Standing (Equipped: Minigun (#1)) | Walk to position (37, 57, 11); Proximity Ambush Trigger at (41, 60, 2) [Triggers when player agent enters 6-tile radius]; Enter vehicle #1 (+7 more) |
| Ped #13 | Guard | 8 HP | (22, 61, 0) | None | Standing (Equipped: Uzi (#3)) | Walk to position (22, 61, 8) |
| Ped #32 | Police Officer | 6 HP (Default) | (79, 21, 0) | None | Walking (Equipped: Shotgun (#6)) | None (Wandering) |
| Ped #33 | Police Officer | 6 HP (Default) | (99, 57, 0) | None | Walking (Equipped: Shotgun (#7)) | None (Wandering) |
| Ped #34 | Police Officer | 6 HP (Default) | (106, 40, 0) | None | Walking (Equipped: Shotgun (#8)) | None (Wandering) |
| Ped #35 | Police Officer | 6 HP (Default) | (96, 21, 0) | None | Walking (Equipped: Shotgun (#9)) | None (Wandering) |
| Ped #36 | Police Officer | 6 HP (Default) | (79, 21, 0) | None | Walking (Equipped: Shotgun (#10)) | None (Wandering) |
| Ped #37 | Police Officer | 6 HP (Default) | (91, 63, 0) | None | Walking (Equipped: Shotgun (#11)) | None (Wandering) |
| Ped #38 | Police Officer | 6 HP (Default) | (83, 27, 0) | None | Walking (Equipped: Shotgun (#12)) | None (Wandering) |
| Ped #39 | Police Officer | 6 HP (Default) | (98, 56, 0) | None | Walking (Equipped: Shotgun (#13)) | None (Wandering) |
| Ped #40 | Police Officer | 6 HP (Default) | (78, 51, 0) | None | Walking (Equipped: Shotgun (#14)) | None (Wandering) |
| Ped #41 | Police Officer | 6 HP (Default) | (78, 39, 0) | None | Walking (Equipped: Shotgun (#15)) | None (Wandering) |
| Ped #42 | Police Officer | 6 HP (Default) | (13, 61, 0) | None | Walking (Equipped: Shotgun (#16)) | None (Wandering) |
| Ped #43 | Police Officer | 6 HP (Default) | (37, 41, 0) | None | Walking (Equipped: Shotgun (#17)) | None (Wandering) |
| Ped #44 | Police Officer | 6 HP (Default) | (21, 50, 0) | None | Walking (Equipped: Shotgun (#18)) | None (Wandering) |
| Ped #45 | Police Officer | 6 HP (Default) | (38, 57, 0) | None | Walking (Equipped: Shotgun (#19)) | None (Wandering) |
| Ped #46 | Police Officer | 6 HP (Default) | (47, 32, 0) | None | Walking (Equipped: Shotgun (#20)) | None (Wandering) |
| Ped #47 | Police Officer | 6 HP (Default) | (15, 48, 0) | None | Walking (Equipped: Shotgun (#21)) | None (Wandering) |
| Ped #48 | Police Officer | 6 HP (Default) | (37, 41, 0) | None | Walking (Equipped: Shotgun (#22)) | None (Wandering) |
| Ped #49 | Police Officer | 6 HP (Default) | (46, 39, 0) | None | Walking (Equipped: Shotgun (#23)) | None (Wandering) |
| Ped #50 | Police Officer | 6 HP (Default) | (42, 35, 0) | None | Walking (Equipped: Shotgun (#24)) | None (Wandering) |
| Ped #51 | Police Officer | 6 HP (Default) | (46, 63, 0) | None | Walking (Equipped: Shotgun (#25)) | None (Wandering) |
| Ped #52 | Police Officer | 6 HP (Default) | (46, 45, 0) | None | Walking (Equipped: Shotgun (#26)) | None (Wandering) |
| Ped #53 | Police Officer | 6 HP (Default) | (35, 32, 0) | None | Walking (Equipped: Shotgun (#27)) | None (Wandering) |
| Ped #54 | Police Officer | 6 HP (Default) | (32, 21, 0) | None | Walking (Equipped: Shotgun (#28)) | None (Wandering) |
| Ped #55 | Police Officer | 6 HP (Default) | (26, 21, 0) | None | Walking (Equipped: Shotgun (#29)) | None (Wandering) |
| Ped #56 | Police Officer | 6 HP (Default) | (38, 58, 0) | None | Walking (Equipped: Shotgun (#30)) | None (Wandering) |
| Ped #57 | Police Officer | 6 HP (Default) | (13, 23, 0) | None | Walking (Equipped: Shotgun (#31)) | None (Wandering) |
| Ped #58 | Police Officer | 6 HP (Default) | (13, 44, 0) | None | Walking (Equipped: Shotgun (#32)) | None (Wandering) |
| Ped #59 | Police Officer | 6 HP (Default) | (34, 41, 0) | None | Walking (Equipped: Shotgun (#33)) | None (Wandering) |
| Ped #60 | Police Officer | 6 HP (Default) | (13, 44, 0) | None | Walking (Equipped: Shotgun (#34)) | None (Wandering) |
| Ped #61 | Police Officer | 6 HP (Default) | (42, 50, 0) | None | Walking (Equipped: Shotgun (#35)) | None (Wandering) |
| Ped #62 | Police Officer | 6 HP (Default) | (46, 35, 0) | None | Walking (Equipped: Shotgun (#36)) | None (Wandering) |
| Ped #63 | Police Officer | 6 HP (Default) | (42, 21, 0) | None | Walking (Equipped: Shotgun (#37)) | None (Wandering) |
| Ped #64 | Police Officer | 6 HP (Default) | (47, 44, 0) | None | Walking (Equipped: Shotgun (#38)) | None (Wandering) |
| Ped #65 | Police Officer | 6 HP (Default) | (13, 22, 0) | None | Walking (Equipped: Shotgun (#39)) | None (Wandering) |
| Ped #66 | Police Officer | 6 HP (Default) | (18, 64, 0) | None | Walking (Equipped: Shotgun (#40)) | None (Wandering) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 4`, `max_rush_factions = 0`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (4 agents: Peds #8–#11)

* **Idle Movement State**: 4 Stationary Sentry
* **Assigned Root Tactic**: Tactic #0 *(Tactic #0 Fallback (Wildcard))*
* **Execution Chain**:
  1. **Tactic #0** [Wildcard / Objective Sweep]: Target: Player Squad

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |

* **Fallback Resolution Note**: Any active hostile faction without a matching faction-specific root (`1..7`) in slots `1..127` falls back to `Tactic #0` (when enabled).

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #12 (Guard)

* **Script Entry**: Scenario Script #1

**Scenario Chain**:
1. Walk to position (37, 57, 11)
1. Proximity Ambush Trigger at (41, 60, 2) [Triggers when player agent enters 6-tile radius]
1. Enter vehicle #1
1. Drive vehicle #1 to (105, 56, 2)
1. Stand Guard / Hold Position (legacy marker)
1. Drive vehicle #1 to (99, 56, 7)
1. Proximity Ambush Trigger at (101, 54, 2) [Triggers when player agent enters 6-tile radius]
1. Enter vehicle #1
1. Drive vehicle #1 to (43, 58, 2)
1. Stand Guard / Hold Position (legacy marker)

### Ped #13 (Guard)

* **Script Entry**: Scenario Script #11

**Scenario Chain**:
1. Walk to position (22, 61, 8)

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (99, 31, 0) | 1024 | None |
| Car #1 | Civilian Sedan | (36, 62, 0) | 1024 | None |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 41


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (78, 58, 0) and establish perimeter control.
* **Enemy Composition**: 4 enemy syndicate agents across 1 active faction(s) plus 37 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
