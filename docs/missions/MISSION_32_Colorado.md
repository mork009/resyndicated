# Mission 32: Colorado

**Country**: Colorado (`#CNTRY_32`)

**Map ID**: Map 60 (`map60.dat`)

**Map Tile Bounding Box**: Tile X [10 – 100], Tile Y [8 – 77]

**Total Entities**: 209 Active Peds (4 Player Agents, 41 Key/Scripted NPCs, 164 Generic Civilians), 7 Vehicles, 44 Weapons.

---

## 1. Briefing Summary

CONVOY.

ASSASSINATION.

A DISTINGUISHED GOVERNMENT MINISTER IS PREPARING TO LEAVE THE COUNTRY. WE BELIEVE THAT HE HAS SOME CONFIDENTIAL INFORMATION ABOUT US THAT COULD PROVE DANGEROUS AS WELL AS EMBARRASSING.

HE IS BEING ESCORTED FROM THE STATION TO THE HELIPORT IN AN ARMED CONVOY.

YOUR AGENTS ARE TO INTERCEPT THIS CONVOY AND KILL HIM BEFORE HE CAN REACH THE HELIPORT.

SECURITY UPDATE:

THE WHOLE ROUTE IS LINED WITH POLICE OFFICERS AS WELL AS CIVVIES. THEY ARE ALL ARMED AND READY FOR TROUBLE.

THE MAIN PART OF THE CONVOY CONSISTS OF TWO APCS AND THE POLICE CAR IN WHICH THE TARGET IS RIDING.

**TACTICS:**

A SINGLE AGENT WOULD STAND JUST AS MUCH OF A CHANCE AS SEVERAL. THE HARD HITTING APPROACH INVOLVES HOT FOOTING IT OVER TO THE CONVOY ROUTE AND DESTROYING THE POLICE CAR WITH HEAVY WEAPONS. A MORE SUBTLE BUT RISKY MANOEUVRE WOULD BE TO STEAL A CAR AND FOLLOW THE CONVOY. WHEN THE TARGET LEAVES HIS CAR AND STARTS TOWARDS THE HELIPORT THEN OPEN UP WITH A BURST OF AUTOMATIC FIRE. TIMING IS EVERYTHING ON THIS ONE.

---

## 2. Mission Objectives

### Objective #1: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #10 (Civilian) at Tile (40, 12, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (21, 15, 9)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (77, 40, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (77, 40, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (77, 39, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (77, 39, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Police Officer | 6 HP (Default) | (42, 52, 10) | None | Standing (Equipped: Long Range Rifle (#2)) | None (Stationary) |
| Ped #9 | Guard | 8 HP | (39, 12, 0) | None | Standing (Equipped: Minigun (#0)) | Enter vehicle #1; Drive vehicle #1 to (52, 72, 2); Stand Guard / Hold Position (legacy marker) (+3 more) |
| Ped #10 | Civilian | 2 HP (Default) | (40, 12, 0) | None | Standing (Equipped: Time Bomb (#41)) | Enter vehicle #3; Drive vehicle #3 to (52, 79, 2); Stand Guard / Hold Position (legacy marker) (+2 more) |
| Ped #11 | Guard | 16 HP | (40, 12, 0) | None | Standing (Equipped: Minigun (#1)) | Enter vehicle #2; Drive vehicle #2 to (50, 74, 2); Stand Guard / Hold Position (legacy marker) (+3 more) |
| Ped #12 | Police Officer | 6 HP (Default) | (41, 50, 10) | None | Standing (Equipped: Long Range Rifle (#34)) | None (Stationary) |
| Ped #71 | Police Officer | 6 HP (Default) | (50, 47, 0) | None | Standing (Equipped: Minigun (#35)) | Enter vehicle #5 |
| Ped #76 | Police Officer | 6 HP (Default) | (57, 14, 0) | None | Walking (Equipped: Uzi (#3)) | None (Wandering) |
| Ped #77 | Police Officer | 6 HP (Default) | (19, 64, 0) | None | Walking (Equipped: Uzi (#4)) | None (Wandering) |
| Ped #78 | Police Officer | 6 HP (Default) | (22, 17, 0) | None | Walking (Equipped: Uzi (#5)) | None (Wandering) |
| Ped #79 | Police Officer | 6 HP (Default) | (88, 41, 0) | None | Walking (Equipped: Uzi (#6)) | None (Wandering) |
| Ped #80 | Police Officer | 6 HP (Default) | (21, 13, 9) | None | Walking (Equipped: Uzi (#7)) | None (Wandering) |
| Ped #81 | Police Officer | 6 HP (Default) | (53, 46, 0) | None | Walking (Equipped: Uzi (#8)) | None (Wandering) |
| Ped #82 | Police Officer | 6 HP (Default) | (32, 23, 0) | None | Walking (Equipped: Uzi (#9)) | None (Wandering) |
| Ped #83 | Police Officer | 6 HP (Default) | (95, 21, 0) | None | Walking (Equipped: Uzi (#10)) | None (Wandering) |
| Ped #84 | Police Officer | 6 HP (Default) | (59, 42, 0) | None | Walking (Equipped: Uzi (#11)) | None (Wandering) |
| Ped #85 | Police Officer | 6 HP (Default) | (91, 20, 0) | None | Walking (Equipped: Uzi (#12)) | None (Wandering) |
| Ped #86 | Police Officer | 6 HP (Default) | (64, 4, 0) | None | Walking (Equipped: Uzi (#13)) | None (Wandering) |
| Ped #87 | Police Officer | 6 HP (Default) | (57, 21, 0) | None | Walking (Equipped: Uzi (#14)) | None (Wandering) |
| Ped #88 | Police Officer | 6 HP (Default) | (33, 18, 0) | None | Walking (Equipped: Uzi (#15)) | None (Wandering) |
| Ped #89 | Police Officer | 6 HP (Default) | (21, 8, 0) | None | Walking (Equipped: Uzi (#16)) | None (Wandering) |
| Ped #90 | Police Officer | 6 HP (Default) | (32, 20, 0) | None | Standing (Equipped: Uzi (#17)) | None (Stationary) |
| Ped #91 | Police Officer | 6 HP (Default) | (32, 18, 0) | None | Standing (Equipped: Uzi (#18)) | None (Stationary) |
| Ped #92 | Police Officer | 6 HP (Default) | (28, 17, 0) | None | Standing (Equipped: Uzi (#19)) | None (Stationary) |
| Ped #93 | Police Officer | 6 HP (Default) | (28, 20, 0) | None | Standing (Equipped: Uzi (#20)) | None (Stationary) |
| Ped #94 | Police Officer | 6 HP (Default) | (28, 28, 0) | None | Standing (Equipped: Uzi (#21)) | None (Stationary) |
| Ped #95 | Police Officer | 6 HP (Default) | (28, 31, 0) | None | Standing (Equipped: Uzi (#22)) | None (Stationary) |
| Ped #96 | Police Officer | 6 HP (Default) | (28, 36, 0) | None | Standing (Equipped: Uzi (#23)) | None (Stationary) |
| Ped #97 | Police Officer | 6 HP (Default) | (28, 41, 0) | None | Standing (Equipped: Uzi (#24)) | None (Stationary) |
| Ped #98 | Police Officer | 6 HP (Default) | (28, 45, 0) | None | Standing (Equipped: Uzi (#25)) | None (Stationary) |
| Ped #99 | Police Officer | 6 HP (Default) | (33, 44, 0) | None | Standing (Equipped: Uzi (#26)) | None (Stationary) |
| Ped #100 | Police Officer | 6 HP (Default) | (38, 44, 0) | None | Standing (Equipped: Uzi (#27)) | None (Stationary) |
| Ped #101 | Police Officer | 6 HP (Default) | (43, 44, 0) | None | Standing (Equipped: Uzi (#28)) | None (Stationary) |
| Ped #102 | Police Officer | 6 HP (Default) | (47, 44, 0) | None | Standing (Equipped: Uzi (#29)) | None (Stationary) |
| Ped #103 | Police Officer | 6 HP (Default) | (44, 48, 0) | None | Standing (Equipped: Uzi (#30)) | None (Stationary) |
| Ped #104 | Police Officer | 6 HP (Default) | (49, 48, 0) | None | Standing (Equipped: Uzi (#31)) | None (Stationary) |
| Ped #105 | Police Officer | 6 HP (Default) | (53, 48, 0) | None | Standing (Equipped: Uzi (#32)) | None (Stationary) |
| Ped #208 | Guard | 8 HP | (84, 31, 0) | None | Standing (Equipped: Shotgun (#33)) | Walk to position (85, 30, 8); Proximity Ambush Trigger at (80, 34, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (84, 33, 2) (+1 more) |
| Ped #209 | Police Officer | 6 HP (Default) | (51, 48, 0) | None | Standing (Equipped: Minigun (#36)) | Enter vehicle #4 |
| Ped #210 | Police Officer | 6 HP (Default) | (37, 36, 6) | None | Standing (Equipped: Long Range Rifle (#37)) | None (Stationary) |
| Ped #211 | Police Officer | 6 HP (Default) | (41, 36, 6) | None | Standing (Equipped: Laser (#38)) | None (Stationary) |
| Ped #212 | Police Officer | 6 HP (Default) | (56, 45, 5) | None | Standing (Equipped: Long Range Rifle (#39), Minigun (#40)) | None (Stationary) |

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 1`, `peds_per_squad = 4`, `max_rush_factions = 0`

No hostile syndicate factions are deployed in this mission.

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |

* **Fallback Resolution Note**: Any active hostile faction without a matching faction-specific root (`1..7`) in slots `1..127` falls back to `Tactic #0` (when enabled).

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #9 (Guard)

* **Script Entry**: Scenario Script #44

**Scenario Chain**:
1. Enter vehicle #1
1. Drive vehicle #1 to (52, 72, 2)
1. Stand Guard / Hold Position (legacy marker)
1. Drive vehicle #1 to (53, 71, 2)
1. Proximity Ambush Trigger at (51, 69, 2) [Triggers when player agent enters 6-tile radius]
1. Reset / Loop Scripted Action Queue

### Ped #10 (Civilian)

* **Script Entry**: Scenario Script #50

**Scenario Chain**:
1. Enter vehicle #3
1. Drive vehicle #3 to (52, 79, 2)
1. Stand Guard / Hold Position (legacy marker)
1. Drive vehicle #3 to (55, 79, 2)
1. Target Escaped (Mission Failure Condition)

### Ped #11 (Guard)

* **Script Entry**: Scenario Script #55

**Scenario Chain**:
1. Enter vehicle #2
1. Drive vehicle #2 to (50, 74, 2)
1. Stand Guard / Hold Position (legacy marker)
1. Drive vehicle #2 to (50, 71, 2)
1. Proximity Ambush Trigger at (48, 69, 2) [Triggers when player agent enters 6-tile radius]
1. Reset / Loop Scripted Action Queue

### Ped #71 (Police Officer)

* **Script Entry**: Scenario Script #61

**Scenario Chain**:
1. Enter vehicle #5

### Ped #208 (Guard)

* **Script Entry**: Scenario Script #40

**Scenario Chain**:
1. Walk to position (85, 30, 8)
1. Proximity Ambush Trigger at (80, 34, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (84, 33, 2)
1. Reset / Loop Scripted Action Queue

### Ped #209 (Police Officer)

* **Script Entry**: Scenario Script #62

**Scenario Chain**:
1. Enter vehicle #4

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Small Armored Transport | (28, 21, 0) | 768 | None |
| Car #1 | Large Armored Car | (28, 23, 0) | 640 | None |
| Car #2 | Large Armored Car | (28, 25, 0) | 640 | None |
| Car #3 | Police Cruiser | (27, 24, 0) | 1280 | None |
| Car #4 | Police Cruiser | (52, 46, 0) | 1280 | None |
| Car #5 | Police Cruiser | (50, 45, 0) | 1280 | None |
| Car #11 | Small Armored Transport | (83, 33, 0) | 768 | None |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 44


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (77, 40, 0) and establish perimeter control.
* **Enemy Composition**: 0 enemy syndicate agents across 0 active faction(s) plus 41 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
