# Mission 20: Scandinavia

**Country**: Scandinavia (`#CNTRY_20`)

**Map ID**: Map 32 (`map32.dat`)

**Map Tile Bounding Box**: Tile X [20 – 69], Tile Y [21 – 64]

**Total Entities**: 38 Active Peds (4 Player Agents, 10 Key/Scripted NPCs, 24 Generic Civilians), 1 Vehicles, 8 Weapons.

---

## 1. Briefing Summary

THE MILK RUN.

PERSONNEL ACQUISITION.

TWO NOTABLE SCIENTISTS ARE BOTH STAYING IN A SMALL HAMLET CLOSE TO ONE OF OUR RESEARCH COMPLEXES. THEY WOULD OBVIOUSLY BE A GREAT HELP TO OUR R AND D BOYS.

YOUR SQUAD HAS INSTRUCTIONS TO <PERSUADE> BOTH THE SCIENTISTS TO JOIN OUR REPUTABLE ORGANISATION.

**CITY UPDATE:**

THERE IS A SEWER SYSTEM THAT RUNS FROM THE RESEARCH COMPLEX TO THE RIVER AND IT HAS INLETS IN THE HAMLET ITSELF. YOUR SQUAD WILL BE TRANSPORTED BY SEWER RAFT TO A DROP OFF POINT IN THE TOWN ITSELF. THE SEWERS ARE VERY DANGEROUS HOWEVER AND WITHOUT THE PROPER EQUIPMENT IT IS INADVISABLE TO ENTER THEM.

**DEFENCE UPDATE:**

THERE IS A VERY SMALL POLICE PRESENCE IN THE TOWN THAT SHOULD PROVIDE NO PROBLEM AT ALL. THE SCIENTISTS HOWEVER HAVE BROUGHT ALONG BODYGUARDS. THESE HAVE TO BE ELIMINATED CAREFULLY SO AS NOT TO DAMAGE THE TARGET.

**TACTICS:**

SMALL ARMS ARE ALL THAT IS REALLY NECESSARY FOR THIS MISSION. DO NOT USE SHOTGUNS FOR DEALING WITH THE BODYGUARDS OR ANYONE WHO HAPPENS TO BE CLOSE TO THE TARGET. ONLY ONE AGENT IS REQUIRED ALTHOUGH YOU MAY WANT TO TAKE IN ANOTHER AS BACKUP.

---

## 2. Mission Objectives

### Objective #1: Persuade Target

* **Type**: Persuade Target
* **Target**: Ped #10 (Civilian) at Tile (35, 57, 3)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Persuade Target

* **Type**: Persuade Target
* **Target**: Ped #9 (Civilian) at Tile (45, 34, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #3: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (81, 41, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (81, 42, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (81, 42, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (81, 41, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (81, 41, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #9 | Civilian | 2 HP (Default) | (45, 34, 0) | None | Standing (Unarmed) | Walk to position (45, 34, 8) |
| Ped #10 | Civilian | 2 HP (Default) | (35, 57, 3) | None | Standing (Unarmed) | Walk to position (35, 57, 11) |
| Ped #11 | Guard | 8 HP (Default) | (38, 52, 3) | None | Standing (Equipped: Pistol (#3)) | None (Stationary) |
| Ped #12 | Guard | 8 HP (Default) | (38, 55, 3) | None | Standing (Equipped: Shotgun (#1)) | None (Stationary) |
| Ped #13 | Guard | 8 HP (Default) | (38, 59, 3) | None | Standing (Equipped: Uzi (#2)) | None (Stationary) |
| Ped #14 | Police Officer | 6 HP (Default) | (66, 24, 0) | None | Walking (Equipped: Pistol (#4)) | None (Wandering) |
| Ped #16 | Police Officer | 6 HP (Default) | (44, 9, 3) | None | Walking (Equipped: Pistol (#6)) | None (Wandering) |
| Ped #17 | Police Officer | 6 HP (Default) | (38, 33, 0) | None | Walking (Equipped: Pistol (#7)) | None (Wandering) |
| Ped #18 | Police Officer | 6 HP (Default) | (51, 55, 0) | None | Walking (Equipped: Pistol (#8)) | None (Wandering) |
| Ped #19 | Police Officer | 6 HP (Default) | (72, 27, 1) | None | Walking (Equipped: Pistol (#9)) | None (Wandering) |

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 1`, `peds_per_squad = 8`, `max_rush_factions = 2`

No hostile syndicate factions are deployed in this mission.

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 10 tiles (terminal: no timer; advances to Tactic #0 on breach) | Terminal | Active |

* **Fallback Resolution Note**: Any active hostile faction without a matching faction-specific root (`1..7`) in slots `1..127` falls back to `Tactic #0` (when enabled).

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #9 (Civilian)

* **Script Entry**: Scenario Script #1

**Scenario Chain**:
1. Walk to position (45, 34, 8)

### Ped #10 (Civilian)

* **Script Entry**: Scenario Script #2

**Scenario Chain**:
1. Walk to position (35, 57, 11)

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (63, 25, 0) | 1024 | None |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 8


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (81, 42, 0) and establish perimeter control.
* **Enemy Composition**: 0 enemy syndicate agents across 0 active faction(s) plus 10 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
