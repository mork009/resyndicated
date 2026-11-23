# Mission 17: Alaska

**Country**: Alaska (`#CNTRY_17`)

**Map ID**: Map 35 (`map35.dat`)

**Map Tile Bounding Box**: Tile X [40 – 87], Tile Y [26 – 51]

**Total Entities**: 22 Active Peds (4 Player Agents, 18 Key/Scripted NPCs, 0 Generic Civilians), 5 Vehicles, 17 Weapons.

---

## 1. Briefing Summary

ASSAULT THE FORTRESS.

ASSAULT.

THE MILITARY HAS CONSTRUCTED A SMALL FORTRESS CLOSE TO ONE OF OUR PRODUCTION PLANTS. REPORTS INDICATE THAT AN ENEMY SYNDICATE IS ALSO USING IT AS A STOP OFF POINT FOR RAIDS INTO THE PLANT.

AN ASSAULT FORCE IS BEING DISPATCHED TO DESTROY THE FORTRESS.

YOUR ASSAULT SQUAD IS TO LAUNCH A PRELIMINARY ATTACK AND TAKE OUT MUCH OF THE DEFENCES AS POSSIBLE BEFORE WITHDRAWING TO ALLOW THE MAIN STRIKE FORCE TO ATTACK.

THE PRIMARY TARGET IS THE BASE COMMANDER WHO MUST NOT BE ALLOWED TO LEAVE THE BASE. IT IS BELIEVED THAT THERE ARE TWO ARMOURED PERSONNEL CARRIERS IN THE COMPLEX AND THESE MUST ALSO BE DESTROYED. FINALLY THERE IS A MEDICAL RESCUE VEHICLE WITH SOME VERY INTERESTING EQUIPMENT ON BOARD. YOU MUST STEAL THIS AND RETURN TO THE DROP ZONE FOR RETRIEVAL.

TRANSPORT HAS BEEN PROVIDED FOR THIS MISSION.

**CITY UPDATE:**

THERE ARE TWO ENTRANCES TO THE FORTRESS. THE MAIN GATE IS QUITE WELL DEFENDED AND IS ONLY ACCESSIBLE BY CAR. THERE IS ALSO BELIEVED TO BE A SIDE ENTRANCE LEADING TO THE ROOF AREA. THIS IS VERY HEAVILY GUARDED AS MOST OF THE GUARDS ARE ON THE ROOF.

**TACTICS:**

A FRONTAL ATTACK USING THE VEHICLE PROVIDED TO GET THROUGH THE BLAST DOORS WOULD GET YOU INTO THE BASE WITH THE MINIMUM OF FUSS AND EXPOSURE. ALL AGENTS SHOULD BE DEPLOYED AND EQUIPPED WITH CLOSE AND AREA OF EFFECT WEAPONS. AT LEAST ONE SHOULD CARRY SOME SUPPORT FIREPOWER AND EXPLOSIVES FOR DEALING WITH THE A.P.CS. THE AMBULANCE IS YOUR BEST BET FOR ESCAPING SO DO NOT DESTROY IT WITH THE A.P.CS.

---

## 2. Mission Objectives

### Objective #1: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #13 (Enemy Agent (Faction 1: Blue)) at Tile (57, 36, 1)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Destroy Target Vehicle

* **Type**: Destroy Target Vehicle
* **Target**: Vehicle #2 (Large Armored Car) at Tile (59, 48, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #3: Destroy Target Vehicle

* **Type**: Destroy Target Vehicle
* **Target**: Vehicle #3 (Large Armored Car) at Tile (60, 48, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #4: Use Target Vehicle

* **Type**: Use Target Vehicle
* **Target**: Vehicle #4 (Ambulance) at Tile (62, 48, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #5: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (99, 38, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (99, 36, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (100, 36, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (100, 40, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (100, 40, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Guard | 8 HP | (73, 31, 5) | None | Standing (Equipped: Long Range Rifle (#0)) | Walk to position (52, 51, 7); Walk to position (52, 29, 7); Walk to position (73, 30, 7) (+1 more) |
| Ped #9 | Guard | 8 HP | (74, 34, 5) | None | Standing (Equipped: Minigun (#1)) | None (Stationary) |
| Ped #10 | Guard | 8 HP | (73, 51, 5) | None | Standing (Equipped: Gauss Gun (#2)) | None (Stationary) |
| Ped #11 | Guard | 8 HP | (49, 50, 5) | None | Standing (Equipped: Uzi (#3)) | None (Stationary) |
| Ped #12 | Guard | 8 HP | (49, 27, 5) | None | Standing (Equipped: Minigun (#4)) | None (Stationary) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (57, 36, 1) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #14 | Guard | 8 HP | (76, 27, 5) | None | Standing (Equipped: Minigun (#5)) | None (Stationary) |
| Ped #15 | Guard | 8 HP | (69, 29, 5) | None | Standing (Equipped: Minigun (#6)) | None (Stationary) |
| Ped #16 | Guard | 8 HP | (52, 27, 5) | None | Standing (Equipped: Minigun (#7)) | None (Stationary) |
| Ped #17 | Guard | 8 HP | (51, 34, 5) | None | Standing (Equipped: Minigun (#8)) | None (Stationary) |
| Ped #18 | Guard | 8 HP | (57, 51, 5) | None | Standing (Equipped: Minigun (#9)) | None (Stationary) |
| Ped #19 | Guard | 8 HP (Default) | (61, 31, 1) | None | Standing (Equipped: Long Range Rifle (#10)) | None (Stationary) |
| Ped #20 | Guard | 8 HP (Default) | (55, 35, 1) | None | Standing (Equipped: Long Range Rifle (#15), Long Range Rifle (#16)) | Walk to position (55, 35, 12); Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (58, 40, 2) (+1 more) |
| Ped #21 | Guard | 8 HP (Default) | (56, 33, 1) | None | Standing (Unarmed) | Walk to position (56, 33, 12); Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (61, 35, 3) (+1 more) |
| Ped #22 | Guard | 8 HP (Default) | (57, 35, 1) | None | Standing (Equipped: Uzi (#14)) | Walk to position (57, 35, 12); Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (61, 34, 3) (+1 more) |
| Ped #23 | Guard | 8 HP (Default) | (57, 34, 1) | None | Standing (Equipped: Uzi (#13)) | Walk to position (57, 34, 12); Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (61, 35, 3) (+1 more) |
| Ped #24 | Guard | 8 HP (Default) | (55, 35, 1) | None | Standing (Equipped: Flamer (#12)) | Walk to position (55, 35, 12); Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (61, 35, 3) (+1 more) |
| Ped #25 | Guard | 8 HP (Default) | (58, 35, 1) | None | Standing (Equipped: Flamer (#11)) | Walk to position (58, 35, 12); Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (61, 35, 3) (+1 more) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 1`, `peds_per_squad = 4`, `max_rush_factions = 0`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (1 agents: Ped #13)

* **Idle Movement State**: 1 Stationary Sentry
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

### Ped #8 (Guard)

* **Script Entry**: Scenario Script #1

**Scenario Chain**:
1. Walk to position (52, 51, 7)
1. Walk to position (52, 29, 7)
1. Walk to position (73, 30, 7)
1. Walk to position (73, 51, 7)

### Ped #20 (Guard)

* **Script Entry**: Scenario Script #30

**Scenario Chain**:
1. Walk to position (55, 35, 12)
1. Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (58, 40, 2)
1. Reset / Loop Scripted Action Queue

### Ped #21 (Guard)

* **Script Entry**: Scenario Script #26

**Scenario Chain**:
1. Walk to position (56, 33, 12)
1. Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (61, 35, 3)
1. Reset / Loop Scripted Action Queue

### Ped #22 (Guard)

* **Script Entry**: Scenario Script #22

**Scenario Chain**:
1. Walk to position (57, 35, 12)
1. Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (61, 34, 3)
1. Reset / Loop Scripted Action Queue

### Ped #23 (Guard)

* **Script Entry**: Scenario Script #18

**Scenario Chain**:
1. Walk to position (57, 34, 12)
1. Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (61, 35, 3)
1. Reset / Loop Scripted Action Queue

### Ped #24 (Guard)

* **Script Entry**: Scenario Script #14

**Scenario Chain**:
1. Walk to position (55, 35, 12)
1. Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (61, 35, 3)
1. Reset / Loop Scripted Action Queue

### Ped #25 (Guard)

* **Script Entry**: Scenario Script #10

**Scenario Chain**:
1. Walk to position (58, 35, 12)
1. Proximity Ambush Trigger at (67, 36, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (61, 35, 3)
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (56, 43, 0) | 1024 | None |
| Car #1 | Small Armored Transport | (105, 37, 0) | 768 | None |
| Car #2 | Large Armored Car | (59, 48, 0) | 640 | None |
| Car #3 | Large Armored Car | (60, 48, 0) | 640 | None |
| Car #4 | Ambulance | (62, 48, 0) | 1150 | None |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 17


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (99, 36, 0) and establish perimeter control.
* **Enemy Composition**: 1 enemy syndicate agents across 1 active faction(s) plus 17 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
