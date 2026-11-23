# Mission 10: Eastern Europe

**Country**: Eastern Europe (`#CNTRY_10`)

**Map ID**: Map 10 (`map10.dat`)

**Map Tile Bounding Box**: Tile X [0 – 100], Tile Y [19 – 86]

**Total Entities**: 132 Active Peds (4 Player Agents, 23 Key/Scripted NPCs, 105 Generic Civilians), 2 Vehicles, 21 Weapons.

---

## 1. Briefing Summary

RAIDERS.

ASSASSINATION.

AN ENEMY SYNDICATE HAS BRAINWASHED A NOTABLE SCIENTIST WHO NOW WORKS FOR THEM. HE HAS IMPROVED THEIR EQUIPMENT A TREMENDOUS AMOUNT AND IS A SERIOUS ASSET.

UNFORTUNATELY THEY HAVE INSERTED A SMALL CHIP IN HIS BRAIN WHICH DETECTS THE ACTIVITY OF THE PERSUADERTRON AND BLOCKS THE SIGNAL MAKING HIM IMMUNE TO <PERSUASION>.

HE CANNOT BE ALLOWED TO CONTINUE HIS RESEARCH FOR THE ENEMY AND SO HE MUST BE ELIMINATED.

**CITY UPDATE:**

THE CITY IS QUITE SMALL AND RELATIVELY UNDEFENDED. A SMALL SQUAD SHOULD HAVE NO PROBLEMS IN REACHING THE INSTALLATION IN THE SOUTH WEST SECTOR AND ACHIEVING THE MISSION OBJECTIVE.

**TACTICS:**

RESISTANCE IS EXPECTED TO BE QUITE LIGHT SO HEAVY WEAPONS WILL NOT BE NEEDED. INDEED IT WOULD BE RISKY ALLOWING SUCH TECHNOLOGY TO POSSIBLY FALL INTO ENEMY HANDS. PROCEED WITH ALL SPEED TO THE INSTALLATION IGNORING ALL OTHER ASPECTS OF THE CITY. KEEP IT TIGHT AND YOU SHOULD HAVE NO PROBLEMS.

---

## 2. Mission Objectives

### Objective #1: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #8 (Civilian) at Tile (26, 50, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (75, 28, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (75, 28, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (76, 28, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (76, 28, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Civilian | 2 HP (Default) | (26, 50, 0) | None | Standing (Unarmed) | Walk to position (26, 50, 8); Timed Pause / Delay (50,000ms); Walk to position (26, 61, 10) (+1 more) |
| Ped #9 | Police Officer | 6 HP (Default) | (34, 22, 6) | None | Walking (Equipped: Long Range Rifle (#1)) | Reset / Loop Scripted Action Queue |
| Ped #10 | Police Officer | 6 HP (Default) | (82, 50, 6) | None | Walking (Equipped: Long Range Rifle (#2)) | Reset / Loop Scripted Action Queue |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 8 HP | (41, 64, 0) | None | Stationary Sentry (Equipped: Long Range Rifle (#6)) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 8 HP | (57, 75, 0) | None | Stationary Sentry (Equipped: Shotgun (#3)) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 8 HP | (44, 75, 0) | None | Stationary Sentry (Equipped: Uzi (#4)) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 8 HP | (43, 54, 0) | None | Stationary Sentry (Equipped: Uzi (#5)) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #15 | Police Officer | 6 HP (Default) | (73, 31, 3) | None | Walking (Equipped: Shotgun (#7)) | None (Wandering) |
| Ped #16 | Police Officer | 6 HP (Default) | (56, 15, 0) | None | Walking (Equipped: Shotgun (#8)) | None (Wandering) |
| Ped #17 | Guard | 8 HP (Default) | (25, 61, 0) | None | Standing (Equipped: Uzi (#0)) | Walk to position (25, 61, 9); Proximity Ambush Trigger at (31, 51, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (25, 63, 2) (+2 more) |
| Ped #18 | Police Officer | 6 HP (Default) | (32, 42, 0) | None | Walking (Equipped: Shotgun (#10)) | None (Wandering) |
| Ped #19 | Police Officer | 6 HP (Default) | (47, 76, 0) | None | Walking (Equipped: Shotgun (#11)) | None (Wandering) |
| Ped #20 | Police Officer | 6 HP (Default) | (38, 56, 0) | None | Walking (Equipped: Shotgun (#12)) | None (Wandering) |
| Ped #21 | Guard | 8 HP (Default) | (26, 49, 0) | None | Standing (Equipped: Uzi (#9)) | Walk to position (26, 49, 8); Proximity Ambush Trigger at (30, 51, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (27, 50, 2) (+1 more) |
| Ped #22 | Police Officer | 6 HP (Default) | (98, 57, 0) | None | Walking (Equipped: Shotgun (#14)) | None (Wandering) |
| Ped #23 | Police Officer | 6 HP (Default) | (116, 78, 0) | None | Walking (Equipped: Shotgun (#15)) | None (Wandering) |
| Ped #24 | Police Officer | 6 HP (Default) | (59, 24, 0) | None | Walking (Equipped: Shotgun (#16)) | None (Wandering) |
| Ped #25 | Police Officer | 6 HP (Default) | (33, 60, 0) | None | Walking (Equipped: Shotgun (#17)) | None (Wandering) |
| Ped #26 | Police Officer | 6 HP (Default) | (54, 37, 0) | None | Walking (Equipped: Shotgun (#18)) | None (Wandering) |
| Ped #28 | Guard | 8 HP (Default) | (19, 55, 0) | None | Standing (Equipped: Uzi (#13)) | Walk to position (19, 55, 2); Proximity Ambush Trigger at (28, 51, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (30, 56, 2) (+1 more) |
| Ped #29 | Guard | 8 HP (Default) | (26, 50, 0) | None | Standing (Equipped: Uzi (#20)) | Walk to position (26, 50, 8); Proximity Ambush Trigger at (31, 51, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (28, 50, 2) (+1 more) |
| Ped #168 | Civilian | 2 HP (Default) | (46, 58, 0) | Car #0 (Civilian Sedan) | In Vehicle (Car #0 (Civilian Sedan)) (Unarmed) | Enter vehicle #0; Drive vehicle #0 to (44, 42, 6) |
| Ped #169 | Police Officer | 6 HP (Default) | (48, 65, 0) | Car #1 (Police Cruiser) | In Vehicle (Car #1 (Police Cruiser)) (Equipped: Minigun (#19)) | Enter vehicle #1; Drive vehicle #1 to (78, 28, 2); Drive vehicle #1 to (70, 74, 2) (+1 more) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 8`, `max_rush_factions = 5`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (4 agents: Peds #11–#14)

* **Idle Movement State**: 4 Stationary Sentry
* **Assigned Root Tactic**: Tactic #1 *(Faction-Specific Root)*
* **Execution Chain**:
  1. **Tactic #1** [Faction 1 / Proximity Tripwire]: Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer)
  2. **Tactic #2** [Faction 1 / Objective Sweep]: Target: Player Squad

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |
| #1 | Root (Active at Start) | Faction 1: Blue | Proximity Tripwire | Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer) | Tactic #2 | Active |
| #2 | Child (after Tactic #1) | Faction 1: Blue | Objective Sweep | Target: Player Squad | End | Active |

* **Fallback Resolution Note**: Any active hostile faction without a matching faction-specific root (`1..7`) in slots `1..127` falls back to `Tactic #0` (when enabled).

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #8 (Civilian)

* **Script Entry**: Scenario Script #1

**Scenario Chain**:
1. Walk to position (26, 50, 8)
1. Timed Pause / Delay (50,000ms)
1. Walk to position (26, 61, 10)
1. Timed Pause / Delay (50,000ms)

### Ped #9 (Police Officer)

* **Script Entry**: Scenario Script #5

**Scenario Chain**:
1. Reset / Loop Scripted Action Queue

### Ped #10 (Police Officer)

* **Script Entry**: Scenario Script #6

**Scenario Chain**:
1. Reset / Loop Scripted Action Queue

### Ped #17 (Guard)

* **Script Entry**: Scenario Script #17

**Scenario Chain**:
1. Walk to position (25, 61, 9)
1. Proximity Ambush Trigger at (31, 51, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (25, 63, 2)
1. Walk to position (30, 63, 2)
1. Reset / Loop Scripted Action Queue

### Ped #21 (Guard)

* **Script Entry**: Scenario Script #22

**Scenario Chain**:
1. Walk to position (26, 49, 8)
1. Proximity Ambush Trigger at (30, 51, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (27, 50, 2)
1. Reset / Loop Scripted Action Queue

### Ped #28 (Guard)

* **Script Entry**: Scenario Script #26

**Scenario Chain**:
1. Walk to position (19, 55, 2)
1. Proximity Ambush Trigger at (28, 51, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (30, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #29 (Guard)

* **Script Entry**: Scenario Script #13

**Scenario Chain**:
1. Walk to position (26, 50, 8)
1. Proximity Ambush Trigger at (31, 51, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (28, 50, 2)
1. Reset / Loop Scripted Action Queue

### Ped #168 (Civilian)

* **Script Entry**: Scenario Script #7

**Scenario Chain**:
1. Enter vehicle #0
1. Drive vehicle #0 to (44, 42, 6)

### Ped #169 (Police Officer)

* **Script Entry**: Scenario Script #9

**Scenario Chain**:
1. Enter vehicle #1
1. Drive vehicle #1 to (78, 28, 2)
1. Drive vehicle #1 to (70, 74, 2)
1. Drive vehicle #1 to (43, 25, 2)

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (46, 58, 0) | 1024 | Ped #168 |
| Car #1 | Police Cruiser | (48, 65, 0) | 1280 | Ped #169 |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 21


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

* **Level Data Adjustment**: Car destination coordinates for Ped #168 are adjusted to tile (36, 59, 2) to correct unwalkable tile coordinates in original level data.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (75, 28, 0) and establish perimeter control.
* **Enemy Composition**: 4 enemy syndicate agents across 1 active faction(s) plus 19 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
