# Mission 13: Mozambique

**Country**: Mozambique (`#CNTRY_13`)

**Map ID**: Map 13 (`map13.dat`)

**Map Tile Bounding Box**: Tile X [36 – 53], Tile Y [13 – 75]

**Total Entities**: 48 Active Peds (4 Player Agents, 44 Key/Scripted NPCs, 0 Generic Civilians), 0 Vehicles, 53 Weapons.

---

## 1. Briefing Summary

THE ALLEYWAY.

EQUIPMENT AQUISTION.

ONE OF OUR ARMY CONTACTS HAS JUST INFORMED US OF A MAJOR WEAPON PROJECT THAT THE ARMY HAS BEEN WORKING ON.

THE RLX 240 IS AN EXPERIMENTAL PORTABLE ROCKET LAUNCHER AND IT IS BEING DEVELOPED IN A SCIENCE BASE IN THE DESERT. REPORTS SHOW THAT IT HAS JUST REACHED THE PROTOTYPE STAGE.

NATURALLY OUR TECH BOYS WANT THIS WEAPON AND ITS DOWN TO YOU TO GET IT.

**DEFENCE UPDATE:**

THE BASE ITSELF CONSISTS OF A LONG ALLEY FROM THE ENTRANCE DOWN TO THE MAIN BUILDING AT THE FAR END. IT IS GUARDED BY SEVERAL SECURITY UNITS WITH SOME QUITE HEAVY WEAPONRY. WE ALSO BELIEVE THAT THERE ARE BUNKERS WITH HEAVY WEAPON EMPLACEMENTS IN THE MAIN BUILDING ITSELF. IF THIS IS THE CASE THEN THEY WOULD HAVE AN INCREDIBLE FIELD OF FIRE AND BE ABLE TO TAKE OUT ANYTHING THAT MOVED DOWN THE MAIN ALLEY.

**TARGET UPDATE:**

THE WEAPON ITSELF IS BELIEVED TO BE HOUSED IN A SECURE ROOM ON THE ROOF OF THE FAR BUILDING.

**TACTICS:**

AS RESISTANCE IS GOING TO BE TOUGH YOU SHOULD TAKE IN THE MOST POWERFUL SQUAD YOU CAN MUSTER. YOU WILL BE ATTACKED FROM BOTH SIDES AS YOU WALK ALONG THE ROAD SO TRY TO STICK TO ONE SIDE. IT MAY BE WORTH WHILE SENDING A LONE AGENT AROUND THE SIDE AND TRY TO REACH THE BUILDING THAT WAY. AS NOTHING IS KNOWN ABOUT THE INTERNAL STRUCTURE OF THE MAIN BUILDING YOU ARE VERY MUCH ON YOUR OWN WHEN YOU ENTER IT.

---

## 2. Mission Objectives

### Objective #1: Acquire Equipment / Take Weapon

* **Type**: Acquire Equipment / Take Weapon
* **Target**: Weapon #0 (Gauss Gun) at Tile (55, 12, 6)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (55, 87, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (53, 84, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (57, 85, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (53, 85, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (57, 85, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Guard | 8 HP | (48, 75, 1) | None | Standing (Equipped: Uzi (#35)) | Walk to position (48, 75, 11); Proximity Ambush Trigger at (53, 76, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (50, 75, 3) (+1 more) |
| Ped #9 | Guard | 8 HP | (48, 76, 1) | None | Standing (Equipped: Uzi (#36)) | Walk to position (48, 76, 11); Proximity Ambush Trigger at (53, 76, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (50, 75, 3) (+1 more) |
| Ped #10 | Guard | 8 HP | (47, 67, 1) | None | Standing (Equipped: Minigun (#37)) | Walk to position (47, 67, 12); Proximity Ambush Trigger at (53, 75, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (51, 68, 3) |
| Ped #11 | Guard | 8 HP | (48, 67, 1) | None | Standing (Equipped: Long Range Rifle (#38)) | Walk to position (47, 67, 12); Proximity Ambush Trigger at (53, 78, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (51, 68, 3) |
| Ped #12 | Guard | 8 HP | (47, 67, 1) | None | Standing (Equipped: Uzi (#39)) | Walk to position (47, 67, 12); Proximity Ambush Trigger at (53, 69, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (51, 67, 3) (+1 more) |
| Ped #13 | Guard | 8 HP | (53, 28, 2) | None | Standing (Equipped: Long Range Rifle (#26)) | Walk to position (53, 28, 9); Proximity Ambush Trigger at (53, 58, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 41, 2) |
| Ped #14 | Guard | 8 HP | (57, 30, 2) | None | Standing (Equipped: Long Range Rifle (#25)) | Walk to position (57, 30, 9); Proximity Ambush Trigger at (53, 41, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (57, 35, 3) |
| Ped #15 | Guard | 8 HP | (52, 14, 6) | None | Standing (Equipped: Minigun (#27)) | None (Stationary) |
| Ped #16 | Guard | 8 HP | (58, 14, 6) | None | Standing (Equipped: Minigun (#28)) | None (Stationary) |
| Ped #17 | Guard | 8 HP | (65, 25, 0) | None | Standing (Equipped: Uzi (#29)) | Walk to position (65, 25, 5) |
| Ped #18 | Guard | 8 HP | (45, 25, 0) | None | Standing (Equipped: Uzi (#30)) | Walk to position (45, 25, 5) |
| Ped #19 | Guard | 8 HP | (47, 67, 1) | None | Standing (Equipped: Minigun (#31)) | Walk to position (47, 67, 12); Proximity Ambush Trigger at (53, 74, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (51, 67, 3) (+1 more) |
| Ped #20 | Guard | 8 HP | (47, 67, 1) | None | Standing (Equipped: Uzi (#42)) | Walk to position (47, 67, 12); Proximity Ambush Trigger at (53, 65, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (50, 67, 3) (+1 more) |
| Ped #21 | Guard | 8 HP | (48, 75, 1) | None | Standing (Equipped: Minigun (#43)) | Walk to position (48, 75, 11); Proximity Ambush Trigger at (53, 69, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (51, 74, 3) (+1 more) |
| Ped #22 | Guard | 8 HP | (48, 67, 1) | None | Standing (Equipped: Long Range Rifle (#34)) | Walk to position (47, 67, 12); Proximity Ambush Trigger at (53, 78, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (51, 66, 3) |
| Ped #23 | Guard | 8 HP | (47, 67, 1) | None | Standing (Equipped: Minigun (#44)) | Walk to position (47, 67, 12); Proximity Ambush Trigger at (53, 65, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (51, 67, 3) (+1 more) |
| Ped #24 | Guard | 8 HP | (52, 55, 7) | None | Standing (Equipped: Uzi (#40)) | None (Stationary) |
| Ped #25 | Guard | 8 HP | (57, 55, 7) | None | Standing (Equipped: Long Range Rifle (#41)) | None (Stationary) |
| Ped #26 | Guard | 8 HP | (63, 79, 1) | None | Standing (Equipped: Minigun (#50)) | Proximity Ambush Trigger at (53, 78, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (60, 79, 3); Walk to position (59, 79, 3) (+1 more) |
| Ped #27 | Guard | 8 HP | (52, 38, 1) | None | Standing (Equipped: Uzi (#46), Long Range Rifle (#48)) | None (Stationary) |
| Ped #28 | Guard | 8 HP | (56, 13, 6) | None | Standing (Equipped: Minigun (#47)) | None (Stationary) |
| Ped #29 | Guard | 3 HP | (55, 12, 6) | None | Standing (Equipped: Minigun (#49)) | None (Stationary) |
| Ped #30 | Guard | 8 HP | (68, 26, 0) | None | Standing (Equipped: Uzi (#52), Long Range Rifle (#53)) | None (Stationary) |
| Ped #31 | Guard | 8 HP | (42, 26, 0) | None | Standing (Equipped: Uzi (#54), Long Range Rifle (#55)) | None (Stationary) |
| Ped #32 | Guard | 8 HP | (47, 59, 1) | None | Standing (Equipped: Uzi (#56)) | Walk to position (47, 59, 12); Proximity Ambush Trigger at (53, 54, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (51, 58, 3) (+1 more) |
| Ped #33 | Guard | 8 HP | (47, 59, 1) | None | Standing (Equipped: Uzi (#57)) | Walk to position (47, 59, 12); Proximity Ambush Trigger at (53, 53, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 59, 2) (+1 more) |
| Ped #34 | Guard | 8 HP | (47, 59, 1) | None | Standing (Equipped: Uzi (#58)) | Walk to position (47, 59, 12); Proximity Ambush Trigger at (53, 65, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 59, 2) (+1 more) |
| Ped #35 | Guard | 8 HP | (48, 60, 1) | None | Standing (Equipped: Uzi (#59)) | Walk to position (48, 60, 12); Proximity Ambush Trigger at (53, 74, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 67, 2) (+1 more) |
| Ped #36 | Guard | 8 HP | (51, 57, 1) | None | Standing (Equipped: Uzi (#68)) | Proximity Ambush Trigger at (53, 67, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (51, 59, 3); Reset / Loop Scripted Action Queue |
| Ped #37 | Guard | 8 HP | (51, 37, 1) | None | Standing (Equipped: Uzi (#71), Long Range Rifle (#72)) | None (Stationary) |
| Ped #38 | Guard | 8 HP | (51, 49, 1) | None | Standing (Equipped: Uzi (#66)) | None (Stationary) |
| Ped #39 | Guard | 8 HP | (53, 26, 2) | None | Standing (Equipped: Minigun (#73)) | Walk to position (54, 28, 10); Proximity Ambush Trigger at (53, 46, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 36, 2) (+1 more) |
| Ped #40 | Guard | 8 HP | (66, 48, 1) | None | Standing (Equipped: Uzi (#64)) | Walk to position (66, 48, 10); Proximity Ambush Trigger at (53, 75, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (57, 76, 2) (+1 more) |
| Ped #41 | Guard | 8 HP | (66, 48, 1) | None | Standing (Equipped: Uzi (#63)) | Walk to position (66, 48, 10); Proximity Ambush Trigger at (53, 76, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (57, 76, 2) (+1 more) |
| Ped #42 | Guard | 8 HP | (65, 46, 1) | None | Standing (Equipped: Uzi (#62)) | Walk to position (65, 46, 8); Proximity Ambush Trigger at (53, 76, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (57, 76, 2) (+1 more) |
| Ped #43 | Guard | 8 HP | (48, 60, 1) | None | Standing (Equipped: Uzi (#60)) | Walk to position (48, 60, 12); Proximity Ambush Trigger at (53, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 59, 2) (+1 more) |
| Ped #44 | Guard | 8 HP | (47, 59, 1) | None | Standing (Equipped: Uzi (#61)) | Walk to position (47, 59, 12); Proximity Ambush Trigger at (53, 60, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 59, 2) (+1 more) |
| Ped #45 | Guard | 8 HP | (48, 59, 1) | None | Standing (Equipped: Uzi (#69)) | Walk to position (48, 59, 12); Proximity Ambush Trigger at (53, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 59, 2) (+1 more) |
| Ped #46 | Guard | 8 HP | (47, 59, 1) | None | Standing (Equipped: Uzi (#70)) | Walk to position (47, 59, 12); Proximity Ambush Trigger at (53, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 59, 2) (+1 more) |
| Ped #47 | Guard | 8 HP | (53, 30, 2) | None | Standing (Equipped: Minigun (#74)) | Walk to position (53, 30, 8); Proximity Ambush Trigger at (53, 44, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 37, 2) (+1 more) |
| Ped #48 | Guard | 8 HP | (53, 29, 2) | None | Standing (Equipped: Uzi (#75)) | Walk to position (53, 29, 9); Proximity Ambush Trigger at (53, 49, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 38, 2) (+1 more) |
| Ped #49 | Guard | 8 HP | (53, 28, 2) | None | Standing (Equipped: Uzi (#76)) | Walk to position (53, 28, 9); Proximity Ambush Trigger at (53, 50, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (53, 40, 2) (+1 more) |
| Ped #50 | Guard | 8 HP | (51, 34, 0) | None | Standing (Equipped: Gauss Gun (#80), Gauss Gun (#81), Gauss Gun (#82)) | None (Stationary) |
| Ped #51 | Guard | 8 HP | (59, 34, 0) | None | Standing (Equipped: Gauss Gun (#77), Gauss Gun (#78), Gauss Gun (#79)) | None (Stationary) |

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

### Ped #8 (Guard)

* **Script Entry**: Scenario Script #27

**Scenario Chain**:
1. Walk to position (48, 75, 11)
1. Proximity Ambush Trigger at (53, 76, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (50, 75, 3)
1. Reset / Loop Scripted Action Queue

### Ped #9 (Guard)

* **Script Entry**: Scenario Script #30

**Scenario Chain**:
1. Walk to position (48, 76, 11)
1. Proximity Ambush Trigger at (53, 76, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (50, 75, 3)
1. Reset / Loop Scripted Action Queue

### Ped #10 (Guard)

* **Script Entry**: Scenario Script #33

**Scenario Chain**:
1. Walk to position (47, 67, 12)
1. Proximity Ambush Trigger at (53, 75, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (51, 68, 3)

### Ped #11 (Guard)

* **Script Entry**: Scenario Script #36

**Scenario Chain**:
1. Walk to position (47, 67, 12)
1. Proximity Ambush Trigger at (53, 78, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (51, 68, 3)

### Ped #12 (Guard)

* **Script Entry**: Scenario Script #42

**Scenario Chain**:
1. Walk to position (47, 67, 12)
1. Proximity Ambush Trigger at (53, 69, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (51, 67, 3)
1. Reset / Loop Scripted Action Queue

### Ped #13 (Guard)

* **Script Entry**: Scenario Script #13

**Scenario Chain**:
1. Walk to position (53, 28, 9)
1. Proximity Ambush Trigger at (53, 58, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 41, 2)

### Ped #14 (Guard)

* **Script Entry**: Scenario Script #16

**Scenario Chain**:
1. Walk to position (57, 30, 9)
1. Proximity Ambush Trigger at (53, 41, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (57, 35, 3)

### Ped #17 (Guard)

* **Script Entry**: Scenario Script #19

**Scenario Chain**:
1. Walk to position (65, 25, 5)

### Ped #18 (Guard)

* **Script Entry**: Scenario Script #20

**Scenario Chain**:
1. Walk to position (45, 25, 5)

### Ped #19 (Guard)

* **Script Entry**: Scenario Script #21

**Scenario Chain**:
1. Walk to position (47, 67, 12)
1. Proximity Ambush Trigger at (53, 74, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (51, 67, 3)
1. Reset / Loop Scripted Action Queue

### Ped #20 (Guard)

* **Script Entry**: Scenario Script #46

**Scenario Chain**:
1. Walk to position (47, 67, 12)
1. Proximity Ambush Trigger at (53, 65, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (50, 67, 3)
1. Reset / Loop Scripted Action Queue

### Ped #21 (Guard)

* **Script Entry**: Scenario Script #50

**Scenario Chain**:
1. Walk to position (48, 75, 11)
1. Proximity Ambush Trigger at (53, 69, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (51, 74, 3)
1. Reset / Loop Scripted Action Queue

### Ped #22 (Guard)

* **Script Entry**: Scenario Script #24

**Scenario Chain**:
1. Walk to position (47, 67, 12)
1. Proximity Ambush Trigger at (53, 78, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (51, 66, 3)

### Ped #23 (Guard)

* **Script Entry**: Scenario Script #54

**Scenario Chain**:
1. Walk to position (47, 67, 12)
1. Proximity Ambush Trigger at (53, 65, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (51, 67, 3)
1. Reset / Loop Scripted Action Queue

### Ped #26 (Guard)

* **Script Entry**: Scenario Script #62

**Scenario Chain**:
1. Proximity Ambush Trigger at (53, 78, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (60, 79, 3)
1. Walk to position (59, 79, 3)
1. Reset / Loop Scripted Action Queue

### Ped #32 (Guard)

* **Script Entry**: Scenario Script #69

**Scenario Chain**:
1. Walk to position (47, 59, 12)
1. Proximity Ambush Trigger at (53, 54, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (51, 58, 3)
1. Reset / Loop Scripted Action Queue

### Ped #33 (Guard)

* **Script Entry**: Scenario Script #73

**Scenario Chain**:
1. Walk to position (47, 59, 12)
1. Proximity Ambush Trigger at (53, 53, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 59, 2)
1. Reset / Loop Scripted Action Queue

### Ped #34 (Guard)

* **Script Entry**: Scenario Script #77

**Scenario Chain**:
1. Walk to position (47, 59, 12)
1. Proximity Ambush Trigger at (53, 65, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 59, 2)
1. Reset / Loop Scripted Action Queue

### Ped #35 (Guard)

* **Script Entry**: Scenario Script #81

**Scenario Chain**:
1. Walk to position (48, 60, 12)
1. Proximity Ambush Trigger at (53, 74, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 67, 2)
1. Reset / Loop Scripted Action Queue

### Ped #36 (Guard)

* **Script Entry**: Scenario Script #101

**Scenario Chain**:
1. Proximity Ambush Trigger at (53, 67, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (51, 59, 3)
1. Reset / Loop Scripted Action Queue

### Ped #39 (Guard)

* **Script Entry**: Scenario Script #129

**Scenario Chain**:
1. Walk to position (54, 28, 10)
1. Proximity Ambush Trigger at (53, 46, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 36, 2)
1. Reset / Loop Scripted Action Queue

### Ped #40 (Guard)

* **Script Entry**: Scenario Script #93

**Scenario Chain**:
1. Walk to position (66, 48, 10)
1. Proximity Ambush Trigger at (53, 75, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (57, 76, 2)
1. Reset / Loop Scripted Action Queue

### Ped #41 (Guard)

* **Script Entry**: Scenario Script #89

**Scenario Chain**:
1. Walk to position (66, 48, 10)
1. Proximity Ambush Trigger at (53, 76, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (57, 76, 2)
1. Reset / Loop Scripted Action Queue

### Ped #42 (Guard)

* **Script Entry**: Scenario Script #85

**Scenario Chain**:
1. Walk to position (65, 46, 8)
1. Proximity Ambush Trigger at (53, 76, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (57, 76, 2)
1. Reset / Loop Scripted Action Queue

### Ped #43 (Guard)

* **Script Entry**: Scenario Script #108

**Scenario Chain**:
1. Walk to position (48, 60, 12)
1. Proximity Ambush Trigger at (53, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 59, 2)
1. Reset / Loop Scripted Action Queue

### Ped #44 (Guard)

* **Script Entry**: Scenario Script #116

**Scenario Chain**:
1. Walk to position (47, 59, 12)
1. Proximity Ambush Trigger at (53, 60, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 59, 2)
1. Reset / Loop Scripted Action Queue

### Ped #45 (Guard)

* **Script Entry**: Scenario Script #112

**Scenario Chain**:
1. Walk to position (48, 59, 12)
1. Proximity Ambush Trigger at (53, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 59, 2)
1. Reset / Loop Scripted Action Queue

### Ped #46 (Guard)

* **Script Entry**: Scenario Script #104

**Scenario Chain**:
1. Walk to position (47, 59, 12)
1. Proximity Ambush Trigger at (53, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 59, 2)
1. Reset / Loop Scripted Action Queue

### Ped #47 (Guard)

* **Script Entry**: Scenario Script #133

**Scenario Chain**:
1. Walk to position (53, 30, 8)
1. Proximity Ambush Trigger at (53, 44, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 37, 2)
1. Reset / Loop Scripted Action Queue

### Ped #48 (Guard)

* **Script Entry**: Scenario Script #125

**Scenario Chain**:
1. Walk to position (53, 29, 9)
1. Proximity Ambush Trigger at (53, 49, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 38, 2)
1. Reset / Loop Scripted Action Queue

### Ped #49 (Guard)

* **Script Entry**: Scenario Script #121

**Scenario Chain**:
1. Walk to position (53, 28, 9)
1. Proximity Ambush Trigger at (53, 50, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (53, 40, 2)
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

No vehicles placed in map level.


### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 1 | Total Inventory Items: 52

#### Ground Pickups:

| Weapon Index | Weapon Subtype | Ground Tile (X, Y, Z) |
| :--- | :--- | :--- |
| Weapon #0 | Gauss Gun | (55, 12, 6) |

---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (53, 84, 0) and establish perimeter control.
* **Enemy Composition**: 0 enemy syndicate agents across 0 active faction(s) plus 44 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
