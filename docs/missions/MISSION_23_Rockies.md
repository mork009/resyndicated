# Mission 23: Rockies

**Country**: Rockies (`#CNTRY_23`)

**Map ID**: Map 90 (`map90.dat`)

**Map Tile Bounding Box**: Tile X [0 – 99], Tile Y [8 – 81]

**Total Entities**: 57 Active Peds (4 Player Agents, 53 Key/Scripted NPCs, 0 Generic Civilians), 5 Vehicles, 60 Weapons.

---

## 1. Briefing Summary

OLD MACDONALD.

RAID AND RESCUE.

ONE OF OUR CORPORATE FRIENDS HAS JUST HAD HIS FARM TAKEN OVER BY THE MILITARY. THEY SEEM TO BE USING IT AS A FORWARD OBSERVATION POST AS PART OF THEIR OPERATIONS IN THIS AREA.

WE BELIEVE A MILITARY STRIKE IS PLANNED SOON AND IT WILL BE COORDINATED FROM THE FARMHOUSE.

OUR FRIEND AND HIS WIFE ARE BEING HELD AGAINST THEIR WILL IN ONE OF THE OUT HOUSES. YOUR TEAM IS TO DISRUPT THE ENEMY ACTIVITY WHILST AT THE SAME TIME RESCUING OUR FRIEND AND HIS WIFE. YOU WILL NEED TO <PERSUADE> BOTH OF THEM.

**DEFENCE UPDATE:**

AS IT IS A MILITARY OPERATION YOU CAN EXPECT A WELL PLANNED DEFENCE WITH PLENTY OF BACKUP. THEY WILL PROBABLY BE EQUIPPED WITH LIGHT AUTOMATICS BUT WILL NO DOUBT HAVE THE SUPPORT OF HEAVIER GUNS.

**TARGET UPDATE:**

THEIR MAIN COMMUNICATIONS BUNDLE IS HOUSED IN THE REAR OF AN A.P.C. WHICH IS PARKED IN THE COURTYARD. YOU MUST DESTROY THE A.P.C. TO SUCCESSFULLY DISRUPT THEIR COMMS NETWORK.

**TARGET UPDATE:**

THE PRISONERS ARE KEPT IN ONE OF THE OUT HOUSES. AS FAR AS WE CAN TELL WE BELIEVE THAT IT IS LOCATED TO THE SOUTH OF THE MAIN COMPLEX. THEY ARE BELIEVED TO BE UNDER GUARD AND VERY FRIGHTENED. THEY MAY NEED <PERSUADING> TO LEAVE THE AREA.

**TACTICS:**

THE SQUAD SHOULD BE SPLIT IN TWO FOR THIS MISSION. ONE SECTION SHOULD PROCEED TO THE COURTYARD. ONCE THERE THE APC SHOULD BE DESTROYED WITH ANTI TANK WEAPONRY AND ANY GUARDS SHOULD BE TAKEN CARE OF WITH AUTOMATICS OR ASSAULT WEAPONS. BY THIS TIME THE SECOND SECTION SHOULD HAVE REACHED THE OUT HOUSE AND AS SOON AS THE A.P.C. IS DESTROYED THE OUT HOUSE GUARDS SHOULD BE KILLED AND THE PRISONERS RESCUED.

WE BELIEVE THAT THERE IS ALSO A GROUND CAR ON THE PREMISES. THIS WOULD BE A VALUABLE AID TO THE ESCAPE.

---

## 2. Mission Objectives

### Objective #1: Destroy Target Vehicle

* **Type**: Destroy Target Vehicle
* **Target**: Vehicle #3 (Large Armored Car) at Tile (11, 59, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Persuade Target

* **Type**: Persuade Target
* **Target**: Ped #26 (Civilian) at Tile (10, 73, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #3: Persuade Target

* **Type**: Persuade Target
* **Target**: Ped #35 (Civilian) at Tile (10, 76, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #4: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (109, 12, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (68, 49, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (69, 49, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (68, 48, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (69, 48, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Guard | 8 HP | (15, 81, 0) | None | Standing (Equipped: Uzi (#0)) | None (Stationary) |
| Ped #9 | Guard | 8 HP | (15, 83, 0) | None | Standing (Equipped: Uzi (#1)) | None (Stationary) |
| Ped #10 | Guard | 7 HP | (23, 83, 0) | None | Standing (Equipped: Uzi (#2)) | None (Stationary) |
| Ped #11 | Guard | 7 HP | (20, 83, 0) | None | Standing (Equipped: Uzi (#3)) | None (Stationary) |
| Ped #12 | Guard | 8 HP | (29, 82, 0) | None | Standing (Equipped: Uzi (#4)) | None (Stationary) |
| Ped #13 | Guard | 7 HP | (29, 79, 0) | None | Standing (Equipped: Uzi (#5)) | None (Stationary) |
| Ped #14 | Guard | 8 HP | (29, 74, 0) | None | Standing (Equipped: Uzi (#6)) | None (Stationary) |
| Ped #15 | Guard | 7 HP | (29, 72, 0) | None | Standing (Equipped: Uzi (#7)) | None (Stationary) |
| Ped #16 | Guard | 8 HP | (30, 67, 0) | None | Standing (Equipped: Uzi (#8)) | None (Stationary) |
| Ped #17 | Guard | 7 HP | (30, 64, 0) | None | Standing (Equipped: Uzi (#9)) | None (Stationary) |
| Ped #18 | Guard | 8 HP | (8, 65, 1) | None | Standing (Equipped: Uzi (#10)) | None (Stationary) |
| Ped #19 | Guard | 8 HP | (8, 63, 1) | None | Standing (Equipped: Uzi (#11)) | None (Stationary) |
| Ped #20 | Guard | 8 HP | (8, 56, 1) | None | Standing (Equipped: Uzi (#12)) | None (Stationary) |
| Ped #21 | Guard | 8 HP | (8, 58, 1) | None | Standing (Equipped: Uzi (#25)) | None (Stationary) |
| Ped #22 | Guard | 8 HP | (10, 56, 0) | None | Standing (Equipped: Uzi (#14)) | None (Stationary) |
| Ped #23 | Guard | 4 HP | (13, 56, 0) | None | Standing (Equipped: Uzi (#15)) | None (Stationary) |
| Ped #24 | Guard | 8 HP | (11, 54, 0) | None | Standing (Equipped: Uzi (#16)) | Walk to position (6, 57, 11); Proximity Ambush Trigger at (18, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 57, 3) (+1 more) |
| Ped #25 | Guard | 8 HP | (6, 58, 1) | None | Standing (Equipped: Uzi (#13)) | Walk to position (6, 64, 11); Proximity Ambush Trigger at (15, 68, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 64, 3) (+1 more) |
| Ped #26 | Civilian | 8 HP | (10, 73, 0) | None | Standing (Unarmed) | Walk to position (13, 83, 9) |
| Ped #27 | Guard | 8 HP | (6, 57, 1) | None | Standing (Equipped: Uzi (#19)) | Walk to position (6, 57, 11); Proximity Ambush Trigger at (17, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 57, 3) (+1 more) |
| Ped #28 | Guard | 8 HP | (6, 58, 1) | None | Standing (Equipped: Uzi (#18)) | Walk to position (6, 64, 11); Proximity Ambush Trigger at (15, 68, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 64, 3) (+1 more) |
| Ped #29 | Guard | 8 HP | (11, 54, 0) | None | Standing (Equipped: Uzi (#21)) | Walk to position (6, 57, 11); Proximity Ambush Trigger at (17, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 57, 3) (+1 more) |
| Ped #30 | Guard | 8 HP | (8, 69, 1) | None | Standing (Equipped: Uzi (#22)) | Walk to position (6, 57, 11); Proximity Ambush Trigger at (18, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 57, 3) (+1 more) |
| Ped #31 | Guard | 8 HP | (6, 57, 1) | None | Standing (Equipped: Uzi (#23)) | Walk to position (6, 57, 11); Proximity Ambush Trigger at (17, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (7, 57, 3) (+1 more) |
| Ped #32 | Guard | 8 HP | (6, 64, 1) | None | Standing (Equipped: Uzi (#24)) | Walk to position (6, 64, 11); Proximity Ambush Trigger at (15, 68, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 64, 3) (+1 more) |
| Ped #33 | Guard | 8 HP | (6, 64, 1) | None | Standing (Equipped: Uzi (#20)) | Walk to position (6, 64, 11); Proximity Ambush Trigger at (15, 69, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 64, 3) (+1 more) |
| Ped #34 | Guard | 8 HP | (6, 64, 1) | None | Standing (Equipped: Uzi (#26)) | Walk to position (6, 64, 11); Proximity Ambush Trigger at (15, 69, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 64, 3) (+1 more) |
| Ped #35 | Civilian | 8 HP | (10, 76, 0) | None | Standing (Unarmed) | Walk to position (12, 83, 8) |
| Ped #36 | Guard | 8 HP | (6, 64, 1) | None | Standing (Equipped: Uzi (#28)) | Walk to position (6, 64, 12); Proximity Ambush Trigger at (15, 67, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (8, 64, 3) (+1 more) |
| Ped #37 | Guard | 8 HP | (15, 66, 0) | None | Standing (Equipped: Minigun (#17)) | Walk to position (15, 81, 2); Walk to position (15, 56, 2) |
| Ped #38 | Guard | 8 HP | (30, 78, 0) | None | Standing (Equipped: Minigun (#27)) | Walk to position (30, 59, 2); Walk to position (30, 84, 2) |
| Ped #39 | Guard | 8 HP | (20, 62, 6) | None | Standing (Equipped: Minigun (#29)) | None (Stationary) |
| Ped #40 | Guard | 8 HP | (68, 72, 0) | None | Standing (Equipped: Minigun (#32)) | None (Stationary) |
| Ped #41 | Guard | 8 HP | (64, 75, 0) | None | Standing (Equipped: Uzi (#31)) | None (Stationary) |
| Ped #42 | Guard | 8 HP (Default) | (90, 33, 0) | None | Standing (Equipped: Uzi (#33)) | None (Stationary) |
| Ped #43 | Guard | 8 HP (Default) | (71, 31, 0) | None | Standing (Equipped: Uzi (#34)) | Walk to position (88, 32, 8); Proximity Ambush Trigger at (90, 34, 2) [Triggers when player agent enters 6-tile radius] |
| Ped #44 | Guard | 8 HP (Default) | (87, 31, 0) | None | Standing (Equipped: Uzi (#35)) | None (Stationary) |
| Ped #45 | Guard | 8 HP (Default) | (90, 31, 0) | None | Standing (Equipped: Long Range Rifle (#36)) | None (Stationary) |
| Ped #46 | Guard | 8 HP (Default) | (87, 34, 0) | None | Standing (Equipped: Long Range Rifle (#37)) | None (Stationary) |
| Ped #47 | Guard | 8 HP (Default) | (46, 76, 2) | None | Standing (Equipped: Long Range Rifle (#38), Long Range Rifle (#41)) | Walk to position (46, 76, 9); Proximity Ambush Trigger at (44, 62, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (50, 75, 7) (+1 more) |
| Ped #48 | Guard | 8 HP (Default) | (88, 32, 0) | None | Standing (Equipped: Flamer (#39)) | None (Stationary) |
| Ped #49 | Guard | 8 HP (Default) | (88, 31, 0) | None | Standing (Equipped: Flamer (#40)) | None (Stationary) |
| Ped #50 | Guard | 8 HP (Default) | (38, 66, 0) | None | Standing (Equipped: Minigun (#43)) | Walk to position (38, 66, 8); Proximity Ambush Trigger at (44, 62, 2) [Triggers when player agent enters 6-tile radius]; Reset / Loop Scripted Action Queue |
| Ped #51 | Guard | 8 HP (Default) | (39, 65, 0) | None | Standing (Equipped: Uzi (#44)) | Walk to position (39, 65, 8); Proximity Ambush Trigger at (43, 63, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (41, 64, 2) (+1 more) |
| Ped #52 | Guard | 8 HP (Default) | (39, 67, 0) | None | Standing (Equipped: Flamer (#42)) | Walk to position (39, 67, 8); Proximity Ambush Trigger at (41, 63, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (40, 64, 2) (+1 more) |
| Ped #53 | Guard | 8 HP (Default) | (38, 66, 0) | None | Standing (Equipped: Minigun (#45)) | Walk to position (38, 66, 8); Proximity Ambush Trigger at (37, 69, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (38, 68, 2) (+1 more) |
| Ped #54 | Guard | 8 HP (Default) | (49, 53, 2) | None | Standing (Equipped: Long Range Rifle (#46), Long Range Rifle (#47)) | Walk to position (49, 53, 9); Proximity Ambush Trigger at (45, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 54, 7) (+1 more) |
| Ped #55 | Guard | 8 HP (Default) | (10, 74, 0) | None | Standing (Equipped: Flamer (#48)) | None (Stationary) |
| Ped #56 | Guard | 8 HP (Default) | (12, 80, 0) | None | Standing (Equipped: Flamer (#49)) | None (Stationary) |
| Ped #57 | Guard | 8 HP (Default) | (11, 76, 0) | None | Standing (Equipped: Uzi (#50)) | None (Stationary) |
| Ped #58 | Guard | 8 HP (Default) | (11, 81, 0) | None | Standing (Equipped: Uzi (#51)) | None (Stationary) |
| Ped #59 | Guard | 8 HP (Default) | (12, 79, 0) | None | Standing (Equipped: Minigun (#52)) | None (Stationary) |
| Ped #65 | Guard | 8 HP (Default) | (36, 21, 0) | Car #4 (Large Armored Car) | In Vehicle (Car #4 (Large Armored Car)) (Equipped: Minigun (#58)) | Enter vehicle #4; Proximity Ambush Trigger at (20, 55, 2) [Triggers when player agent enters 6-tile radius]; Drive vehicle #4 to (31, 79, 2) (+2 more) |

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

### Ped #24 (Guard)

* **Script Entry**: Scenario Script #21

**Scenario Chain**:
1. Walk to position (6, 57, 11)
1. Proximity Ambush Trigger at (18, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 57, 3)
1. Reset / Loop Scripted Action Queue

### Ped #25 (Guard)

* **Script Entry**: Scenario Script #43

**Scenario Chain**:
1. Walk to position (6, 64, 11)
1. Proximity Ambush Trigger at (15, 68, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 64, 3)
1. Reset / Loop Scripted Action Queue

### Ped #26 (Civilian)

* **Script Entry**: Scenario Script #64

**Scenario Chain**:
1. Walk to position (13, 83, 9)

### Ped #27 (Guard)

* **Script Entry**: Scenario Script #5

**Scenario Chain**:
1. Walk to position (6, 57, 11)
1. Proximity Ambush Trigger at (17, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 57, 3)
1. Reset / Loop Scripted Action Queue

### Ped #28 (Guard)

* **Script Entry**: Scenario Script #37

**Scenario Chain**:
1. Walk to position (6, 64, 11)
1. Proximity Ambush Trigger at (15, 68, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 64, 3)
1. Reset / Loop Scripted Action Queue

### Ped #29 (Guard)

* **Script Entry**: Scenario Script #1

**Scenario Chain**:
1. Walk to position (6, 57, 11)
1. Proximity Ambush Trigger at (17, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 57, 3)
1. Reset / Loop Scripted Action Queue

### Ped #30 (Guard)

* **Script Entry**: Scenario Script #29

**Scenario Chain**:
1. Walk to position (6, 57, 11)
1. Proximity Ambush Trigger at (18, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 57, 3)
1. Reset / Loop Scripted Action Queue

### Ped #31 (Guard)

* **Script Entry**: Scenario Script #25

**Scenario Chain**:
1. Walk to position (6, 57, 11)
1. Proximity Ambush Trigger at (17, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (7, 57, 3)
1. Reset / Loop Scripted Action Queue

### Ped #32 (Guard)

* **Script Entry**: Scenario Script #47

**Scenario Chain**:
1. Walk to position (6, 64, 11)
1. Proximity Ambush Trigger at (15, 68, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 64, 3)
1. Reset / Loop Scripted Action Queue

### Ped #33 (Guard)

* **Script Entry**: Scenario Script #33

**Scenario Chain**:
1. Walk to position (6, 64, 11)
1. Proximity Ambush Trigger at (15, 69, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 64, 3)
1. Reset / Loop Scripted Action Queue

### Ped #34 (Guard)

* **Script Entry**: Scenario Script #51

**Scenario Chain**:
1. Walk to position (6, 64, 11)
1. Proximity Ambush Trigger at (15, 69, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 64, 3)
1. Reset / Loop Scripted Action Queue

### Ped #35 (Civilian)

* **Script Entry**: Scenario Script #63

**Scenario Chain**:
1. Walk to position (12, 83, 8)

### Ped #36 (Guard)

* **Script Entry**: Scenario Script #59

**Scenario Chain**:
1. Walk to position (6, 64, 12)
1. Proximity Ambush Trigger at (15, 67, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (8, 64, 3)
1. Reset / Loop Scripted Action Queue

### Ped #37 (Guard)

* **Script Entry**: Scenario Script #65

**Scenario Chain**:
1. Walk to position (15, 81, 2)
1. Walk to position (15, 56, 2)

### Ped #38 (Guard)

* **Script Entry**: Scenario Script #67

**Scenario Chain**:
1. Walk to position (30, 59, 2)
1. Walk to position (30, 84, 2)

### Ped #43 (Guard)

* **Script Entry**: Scenario Script #69

**Scenario Chain**:
1. Walk to position (88, 32, 8)
1. Proximity Ambush Trigger at (90, 34, 2) [Triggers when player agent enters 6-tile radius]

### Ped #47 (Guard)

* **Script Entry**: Scenario Script #71

**Scenario Chain**:
1. Walk to position (46, 76, 9)
1. Proximity Ambush Trigger at (44, 62, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (50, 75, 7)
1. Reset / Loop Scripted Action Queue

### Ped #50 (Guard)

* **Script Entry**: Scenario Script #83

**Scenario Chain**:
1. Walk to position (38, 66, 8)
1. Proximity Ambush Trigger at (44, 62, 2) [Triggers when player agent enters 6-tile radius]
1. Reset / Loop Scripted Action Queue

### Ped #51 (Guard)

* **Script Entry**: Scenario Script #79

**Scenario Chain**:
1. Walk to position (39, 65, 8)
1. Proximity Ambush Trigger at (43, 63, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (41, 64, 2)
1. Reset / Loop Scripted Action Queue

### Ped #52 (Guard)

* **Script Entry**: Scenario Script #75

**Scenario Chain**:
1. Walk to position (39, 67, 8)
1. Proximity Ambush Trigger at (41, 63, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (40, 64, 2)
1. Reset / Loop Scripted Action Queue

### Ped #53 (Guard)

* **Script Entry**: Scenario Script #86

**Scenario Chain**:
1. Walk to position (38, 66, 8)
1. Proximity Ambush Trigger at (37, 69, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (38, 68, 2)
1. Reset / Loop Scripted Action Queue

### Ped #54 (Guard)

* **Script Entry**: Scenario Script #90

**Scenario Chain**:
1. Walk to position (49, 53, 9)
1. Proximity Ambush Trigger at (45, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 54, 7)
1. Reset / Loop Scripted Action Queue

### Ped #65 (Guard)

* **Script Entry**: Scenario Script #95

**Scenario Chain**:
1. Enter vehicle #4
1. Proximity Ambush Trigger at (20, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Drive vehicle #4 to (31, 79, 2)
1. Stand Guard / Hold Position (legacy marker)
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (7, 87, 0) | 1024 | None |
| Car #1 | Small Armored Transport | (10, 58, 0) | 768 | None |
| Car #2 | Small Armored Transport | (10, 58, 0) | 768 | None |
| Car #3 | Large Armored Car | (11, 59, 0) | 640 | None |
| Car #4 | Large Armored Car | (36, 21, 0) | 640 | Ped #65 |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 1 | Total Inventory Items: 59

#### Ground Pickups:

| Weapon Index | Weapon Subtype | Ground Tile (X, Y, Z) |
| :--- | :--- | :--- |
| Weapon #30 | Flamer | (64, 75, 0) |

---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

* **Rockies Objective Handling**: Destroy Vehicle and Persuade objectives are validated safely during level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (68, 49, 0) and establish perimeter control.
* **Enemy Composition**: 0 enemy syndicate agents across 0 active faction(s) plus 53 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
