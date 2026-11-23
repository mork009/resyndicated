# Mission 16: Greenland

**Country**: Greenland (`#CNTRY_16`)

**Map ID**: Map 19 (`map19.dat`)

**Map Tile Bounding Box**: Tile X [15 – 83], Tile Y [10 – 64]

**Total Entities**: 66 Active Peds (4 Player Agents, 62 Key/Scripted NPCs, 0 Generic Civilians), 3 Vehicles, 68 Weapons.

---

## 1. Briefing Summary

SUPPLY LINE.

PERSONNEL ACQUISITION.

A SMALL WEAPONS SUPPLIER OF OURS IS BEING LEANED ON BY THE MILITARY AND WE ARE NOW EXPERIENCING DIFFICULTY WITH OUR HARDWARE SUPPLIES.

WE BELIEVE THE REASON BEHIND THIS IS A MR CARSON WHO HAS BEEN DIVULGING CLASSIFIED INFORMATION TO THE MILITARY. HE HAS BEEN UNDER SURVEILLANCE FOR SOME TIME NOW AND HE HAS JUST CONFIRMED OUR SUSPICIONS BY VISITING A MILITARY TRAINING CAMP.

ALTHOUGH IT WOULD BE FAR SIMPLER JUST TO KILL HIM HE MAY HAVE INFORMATION THAT WE COULD USE ABOUT THE MILITARY AND SO YOUR SQUAD IS TO ASSAULT THE CAMP AND <PERSUADE> MR CARSON.

ONCE HE HAS BEEN PERSUADED PROCEED TO THE LANDING STRIP IN THE CAMP FOR DUST OFF.

TRANSPORT UPDATE:

THE CAMP IS ONLY ACCESSIBLE THROUGH THE MAIN GATE AND THE TRANSPORT DIVISION HAS PROVIDED A GROUND CAR FOR THIS MISSION.

**DEFENCE UPDATE:**

OBVIOUSLY THERE IS A VERY STRONG MILITARY PRESENCE IN THE AREA ALTHOUGH WE BELIEVE THEM TO BE ARMED MAINLY WITH LIGHT AUTOMATICS. THERE ARE ALSO A SERIES OF GUARD TOWERS THAT LINE THE PERIMETER FENCE. IT IS UNKNOWN WHAT SORT OF FIREPOWER THESE HAVE BUT THEY SHOULD BE TAKEN OUT AT ANY OPPORTUNITY.

**TACTICS:**

LONG RANGE SNIPING SHOULD TAKE CARE OF THE TOWER GUARDS BUT BE SURE TO DO IT AT MAXIMUM RANGE OTHERWISE YOU COULD BE IN FOR SOME SERIOUS RETURN FIRE. ONE AGENT SHOULD DRAG BEHIND THE OTHERS TO PROVIDE COVER FIRE WITH LONG RANGE WEAPONRY. HEAVY WEAPONS OR AREA OF EFFECT WEAPONS SHOULD NOT BE USED DUE TO THE PROXIMITY OF THE ENEMY AND POSSIBLE DAMAGE TO THE TARGET. AS SOON AS YOU ENTER THE CAMP EXPECT THE SOLDIERS TO COME AT YOU THICK AND FAST.

---

## 2. Mission Objectives

### Objective #1: Persuade Target

* **Type**: Persuade Target
* **Target**: Ped #23 (Civilian) at Tile (43, 59, 1)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (68, 11, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (78, 13, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (79, 13, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (79, 12, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (80, 12, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Guard | 16 HP | (96, 25, 0) | Car #1 (Large Armored Car) | In Vehicle (Car #1 (Large Armored Car)) (Equipped: Minigun (#0)) | Enter vehicle #1; Drive vehicle #1 to (95, 21, 2); Drive vehicle #1 to (96, 68, 2) (+1 more) |
| Ped #9 | Guard | 16 HP | (91, 64, 5) | None | Walking (Equipped: Minigun (#1)) | None (Wandering) |
| Ped #10 | Guard | 16 HP | (38, 65, 5) | None | Walking (Equipped: Minigun (#2)) | None (Wandering) |
| Ped #11 | Guard | 16 HP | (37, 26, 5) | None | Walking (Equipped: Minigun (#3)) | None (Wandering) |
| Ped #12 | Guard | 16 HP | (93, 26, 5) | None | Walking (Equipped: Minigun (#4)) | None (Wandering) |
| Ped #13 | Guard | 16 HP | (70, 17, 6) | None | Walking (Equipped: Minigun (#5)) | None (Wandering) |
| Ped #14 | Guard | 16 HP | (67, 17, 6) | None | Walking (Equipped: Minigun (#6)) | None (Wandering) |
| Ped #15 | Guard | 16 HP | (75, 41, 0) | None | Standing (Equipped: Uzi (#7)) | Walk to position (75, 41, 9); Proximity Ambush Trigger at (68, 44, 2) [Triggers when player agent enters 6-tile radius]; Reset / Loop Scripted Action Queue |
| Ped #16 | Guard | 16 HP | (71, 40, 0) | None | Standing (Equipped: Uzi (#8)) | Walk to position (71, 40, 9); Proximity Ambush Trigger at (68, 43, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (71, 43, 2) (+1 more) |
| Ped #17 | Guard | 16 HP | (72, 40, 0) | None | Standing (Equipped: Uzi (#9)) | Walk to position (72, 40, 9); Proximity Ambush Trigger at (69, 44, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (71, 43, 2) (+1 more) |
| Ped #18 | Guard | 16 HP | (70, 40, 0) | None | Standing (Equipped: Uzi (#10)) | Walk to position (70, 40, 9); Proximity Ambush Trigger at (66, 53, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (67, 55, 2) (+1 more) |
| Ped #19 | Guard | 16 HP | (70, 39, 0) | None | Standing (Equipped: Uzi (#11)) | Walk to position (70, 39, 9); Proximity Ambush Trigger at (67, 54, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (76, 56, 2) (+1 more) |
| Ped #20 | Guard | 16 HP | (70, 41, 0) | None | Standing (Equipped: Uzi (#12)) | Walk to position (70, 41, 9); Proximity Ambush Trigger at (67, 54, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (66, 53, 2) (+1 more) |
| Ped #21 | Guard | 16 HP | (81, 41, 0) | None | Standing (Equipped: Uzi (#14)) | Walk to position (81, 41, 11) |
| Ped #22 | Guard | 16 HP | (81, 42, 0) | None | Standing (Equipped: Uzi (#13)) | Walk to position (81, 42, 11) |
| Ped #23 | Civilian | 2 HP (Default) | (43, 59, 1) | None | Standing (Unarmed) | Walk to position (43, 59, 8) |
| Ped #24 | Guard | 8 HP | (48, 57, 0) | None | Standing (Equipped: Uzi (#16)) | None (Stationary) |
| Ped #25 | Guard | 8 HP | (45, 59, 1) | None | Standing (Equipped: Long Range Rifle (#15)) | None (Stationary) |
| Ped #26 | Guard | 8 HP (Default) | (62, 56, 0) | None | Standing (Equipped: Uzi (#17)) | Walk to position (63, 53, 10); Proximity Ambush Trigger at (60, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (60, 56, 2) (+1 more) |
| Ped #27 | Guard | 8 HP (Default) | (64, 54, 0) | None | Standing (Equipped: Uzi (#18)) | Walk to position (64, 54, 11); Proximity Ambush Trigger at (56, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (62, 56, 2) (+1 more) |
| Ped #28 | Guard | 8 HP (Default) | (63, 51, 0) | None | Standing (Equipped: Uzi (#19)) | Walk to position (63, 51, 10); Proximity Ambush Trigger at (57, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (63, 55, 2) (+1 more) |
| Ped #29 | Guard | 8 HP (Default) | (63, 53, 0) | None | Standing (Equipped: Uzi (#20)) | Walk to position (63, 53, 10); Proximity Ambush Trigger at (56, 51, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (62, 56, 2) (+1 more) |
| Ped #30 | Guard | 8 HP (Default) | (62, 53, 0) | None | Standing (Equipped: Uzi (#21)) | Walk to position (62, 53, 10); Proximity Ambush Trigger at (56, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (63, 56, 2) (+1 more) |
| Ped #31 | Guard | 8 HP (Default) | (62, 51, 0) | None | Standing (Equipped: Uzi (#22)) | Walk to position (62, 51, 10); Proximity Ambush Trigger at (56, 53, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (62, 56, 2) (+1 more) |
| Ped #32 | Guard | 8 HP (Default) | (62, 53, 0) | None | Standing (Equipped: Minigun (#23)) | Walk to position (62, 53, 10); Proximity Ambush Trigger at (51, 57, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (60, 56, 2) (+1 more) |
| Ped #33 | Guard | 8 HP (Default) | (63, 51, 0) | None | Standing (Equipped: Flamer (#24)) | Walk to position (63, 51, 10); Proximity Ambush Trigger at (54, 57, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (60, 56, 2) (+1 more) |
| Ped #34 | Guard | 8 HP | (69, 53, 0) | None | Standing (Equipped: Long Range Rifle (#27)) | Walk to position (69, 53, 8); Proximity Ambush Trigger at (59, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (69, 56, 2) (+1 more) |
| Ped #35 | Guard | 8 HP | (71, 52, 0) | None | Standing (Equipped: Minigun (#25)) | Walk to position (71, 52, 8); Proximity Ambush Trigger at (66, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (69, 55, 2) (+1 more) |
| Ped #36 | Guard | 8 HP | (69, 53, 0) | None | Standing (Equipped: Flamer (#26)) | Walk to position (69, 53, 8); Proximity Ambush Trigger at (67, 56, 2) [Triggers when player agent enters 6-tile radius]; Reset / Loop Scripted Action Queue |
| Ped #37 | Guard | 8 HP | (70, 52, 0) | None | Standing (Equipped: Uzi (#28)) | Walk to position (70, 52, 7); Proximity Ambush Trigger at (66, 53, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (69, 55, 2) (+1 more) |
| Ped #38 | Guard | 8 HP | (69, 52, 0) | None | Standing (Equipped: Uzi (#29)) | Walk to position (69, 52, 8); Proximity Ambush Trigger at (68, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (69, 55, 2) (+1 more) |
| Ped #39 | Guard | 8 HP | (70, 52, 0) | None | Standing (Equipped: Uzi (#30)) | Walk to position (70, 52, 7); Walk to position (70, 52, 7); Walk to position (70, 52, 7) (+3 more) |
| Ped #40 | Guard | 8 HP (Default) | (78, 53, 0) | None | Walking (Equipped: Uzi (#31)) | None (Wandering) |
| Ped #41 | Guard | 8 HP (Default) | (43, 63, 0) | None | Walking (Equipped: Uzi (#32)) | None (Wandering) |
| Ped #42 | Guard | 8 HP (Default) | (56, 27, 0) | None | Walking (Equipped: Uzi (#33)) | None (Wandering) |
| Ped #43 | Guard | 8 HP (Default) | (78, 47, 0) | None | Walking (Equipped: Uzi (#34)) | None (Wandering) |
| Ped #44 | Guard | 8 HP (Default) | (38, 45, 0) | None | Walking (Equipped: Uzi (#35)) | None (Wandering) |
| Ped #45 | Guard | 8 HP (Default) | (56, 52, 0) | None | Walking (Equipped: Uzi (#36)) | None (Wandering) |
| Ped #46 | Guard | 8 HP (Default) | (44, 26, 0) | None | Walking (Equipped: Uzi (#37)) | None (Wandering) |
| Ped #47 | Guard | 8 HP (Default) | (83, 63, 0) | None | Walking (Equipped: Uzi (#38)) | None (Wandering) |
| Ped #48 | Guard | 8 HP (Default) | (86, 38, 0) | None | Walking (Equipped: Uzi (#39)) | None (Wandering) |
| Ped #49 | Guard | 8 HP (Default) | (91, 33, 0) | None | Walking (Equipped: Uzi (#40)) | None (Wandering) |
| Ped #50 | Guard | 8 HP (Default) | (63, 51, 0) | None | Walking (Equipped: Uzi (#41)) | None (Wandering) |
| Ped #51 | Guard | 8 HP (Default) | (44, 26, 0) | None | Walking (Equipped: Uzi (#42)) | None (Wandering) |
| Ped #52 | Guard | 8 HP (Default) | (48, 37, 0) | None | Walking (Equipped: Uzi (#43)) | None (Wandering) |
| Ped #53 | Guard | 8 HP (Default) | (60, 26, 0) | None | Walking (Equipped: Uzi (#44)) | None (Wandering) |
| Ped #54 | Guard | 8 HP (Default) | (87, 64, 0) | None | Walking (Equipped: Uzi (#45)) | None (Wandering) |
| Ped #55 | Guard | 8 HP (Default) | (60, 49, 0) | None | Walking (Equipped: Uzi (#46)) | None (Wandering) |
| Ped #56 | Guard | 8 HP (Default) | (43, 58, 1) | None | Walking (Equipped: Uzi (#47)) | None (Wandering) |
| Ped #57 | Guard | 8 HP (Default) | (66, 30, 0) | None | Walking (Equipped: Uzi (#48)) | None (Wandering) |
| Ped #58 | Guard | 8 HP (Default) | (43, 32, 0) | None | Walking (Equipped: Uzi (#49)) | None (Wandering) |
| Ped #59 | Guard | 8 HP (Default) | (60, 55, 0) | None | Walking (Equipped: Uzi (#50)) | None (Wandering) |
| Ped #60 | Guard | 8 HP (Default) | (64, 56, 0) | None | Walking (Equipped: Uzi (#51)) | None (Wandering) |
| Ped #61 | Guard | 8 HP (Default) | (87, 37, 0) | None | Walking (Equipped: Uzi (#52)) | None (Wandering) |
| Ped #62 | Guard | 8 HP (Default) | (66, 26, 0) | None | Walking (Equipped: Uzi (#53)) | None (Wandering) |
| Ped #63 | Guard | 8 HP (Default) | (71, 61, 0) | None | Walking (Equipped: Uzi (#54)) | None (Wandering) |
| Ped #64 | Guard | 8 HP (Default) | (66, 30, 0) | None | Walking (Equipped: Uzi (#55)) | None (Wandering) |
| Ped #65 | Guard | 8 HP (Default) | (42, 45, 0) | None | Walking (Equipped: Uzi (#56)) | None (Wandering) |
| Ped #66 | Guard | 8 HP (Default) | (52, 26, 0) | None | Walking (Equipped: Uzi (#57)) | None (Wandering) |
| Ped #67 | Guard | 8 HP (Default) | (69, 37, 0) | None | Walking (Equipped: Uzi (#58)) | None (Wandering) |
| Ped #68 | Guard | 8 HP (Default) | (91, 50, 0) | None | Walking (Equipped: Uzi (#59)) | None (Wandering) |
| Ped #69 | Guard | 8 HP | (87, 27, 0) | Car #2 (Large Armored Car) | In Vehicle (Car #2 (Large Armored Car)) (Equipped: Uzi (#67)) | Enter vehicle #2; Proximity Ambush Trigger at (47, 57, 2) [Triggers when player agent enters 6-tile radius]; Drive vehicle #2 to (69, 31, 2) (+2 more) |

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

* **Script Entry**: Scenario Script #1

**Scenario Chain**:
1. Enter vehicle #1
1. Drive vehicle #1 to (95, 21, 2)
1. Drive vehicle #1 to (96, 68, 2)
1. Drive vehicle #1 to (34, 69, 2)

### Ped #15 (Guard)

* **Script Entry**: Scenario Script #25

**Scenario Chain**:
1. Walk to position (75, 41, 9)
1. Proximity Ambush Trigger at (68, 44, 2) [Triggers when player agent enters 6-tile radius]
1. Reset / Loop Scripted Action Queue

### Ped #16 (Guard)

* **Script Entry**: Scenario Script #21

**Scenario Chain**:
1. Walk to position (71, 40, 9)
1. Proximity Ambush Trigger at (68, 43, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (71, 43, 2)
1. Reset / Loop Scripted Action Queue

### Ped #17 (Guard)

* **Script Entry**: Scenario Script #17

**Scenario Chain**:
1. Walk to position (72, 40, 9)
1. Proximity Ambush Trigger at (69, 44, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (71, 43, 2)
1. Reset / Loop Scripted Action Queue

### Ped #18 (Guard)

* **Script Entry**: Scenario Script #13

**Scenario Chain**:
1. Walk to position (70, 40, 9)
1. Proximity Ambush Trigger at (66, 53, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (67, 55, 2)
1. Reset / Loop Scripted Action Queue

### Ped #19 (Guard)

* **Script Entry**: Scenario Script #8

**Scenario Chain**:
1. Walk to position (70, 39, 9)
1. Proximity Ambush Trigger at (67, 54, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (76, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #20 (Guard)

* **Script Entry**: Scenario Script #5

**Scenario Chain**:
1. Walk to position (70, 41, 9)
1. Proximity Ambush Trigger at (67, 54, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (66, 53, 2)
1. Reset / Loop Scripted Action Queue

### Ped #21 (Guard)

* **Script Entry**: Scenario Script #28

**Scenario Chain**:
1. Walk to position (81, 41, 11)

### Ped #22 (Guard)

* **Script Entry**: Scenario Script #29

**Scenario Chain**:
1. Walk to position (81, 42, 11)

### Ped #23 (Civilian)

* **Script Entry**: Scenario Script #30

**Scenario Chain**:
1. Walk to position (43, 59, 8)

### Ped #26 (Guard)

* **Script Entry**: Scenario Script #59

**Scenario Chain**:
1. Walk to position (63, 53, 10)
1. Proximity Ambush Trigger at (60, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (60, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #27 (Guard)

* **Script Entry**: Scenario Script #55

**Scenario Chain**:
1. Walk to position (64, 54, 11)
1. Proximity Ambush Trigger at (56, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (62, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #28 (Guard)

* **Script Entry**: Scenario Script #51

**Scenario Chain**:
1. Walk to position (63, 51, 10)
1. Proximity Ambush Trigger at (57, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (63, 55, 2)
1. Reset / Loop Scripted Action Queue

### Ped #29 (Guard)

* **Script Entry**: Scenario Script #47

**Scenario Chain**:
1. Walk to position (63, 53, 10)
1. Proximity Ambush Trigger at (56, 51, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (62, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #30 (Guard)

* **Script Entry**: Scenario Script #43

**Scenario Chain**:
1. Walk to position (62, 53, 10)
1. Proximity Ambush Trigger at (56, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (63, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #31 (Guard)

* **Script Entry**: Scenario Script #39

**Scenario Chain**:
1. Walk to position (62, 51, 10)
1. Proximity Ambush Trigger at (56, 53, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (62, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #32 (Guard)

* **Script Entry**: Scenario Script #35

**Scenario Chain**:
1. Walk to position (62, 53, 10)
1. Proximity Ambush Trigger at (51, 57, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (60, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #33 (Guard)

* **Script Entry**: Scenario Script #31

**Scenario Chain**:
1. Walk to position (63, 51, 10)
1. Proximity Ambush Trigger at (54, 57, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (60, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #34 (Guard)

* **Script Entry**: Scenario Script #63

**Scenario Chain**:
1. Walk to position (69, 53, 8)
1. Proximity Ambush Trigger at (59, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (69, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #35 (Guard)

* **Script Entry**: Scenario Script #67

**Scenario Chain**:
1. Walk to position (71, 52, 8)
1. Proximity Ambush Trigger at (66, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (69, 55, 2)
1. Reset / Loop Scripted Action Queue

### Ped #36 (Guard)

* **Script Entry**: Scenario Script #71

**Scenario Chain**:
1. Walk to position (69, 53, 8)
1. Proximity Ambush Trigger at (67, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Reset / Loop Scripted Action Queue

### Ped #37 (Guard)

* **Script Entry**: Scenario Script #74

**Scenario Chain**:
1. Walk to position (70, 52, 7)
1. Proximity Ambush Trigger at (66, 53, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (69, 55, 2)
1. Reset / Loop Scripted Action Queue

### Ped #38 (Guard)

* **Script Entry**: Scenario Script #78

**Scenario Chain**:
1. Walk to position (69, 52, 8)
1. Proximity Ambush Trigger at (68, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (69, 55, 2)
1. Reset / Loop Scripted Action Queue

### Ped #39 (Guard)

* **Script Entry**: Scenario Script #82

**Scenario Chain**:
1. Walk to position (70, 52, 7)
1. Walk to position (70, 52, 7)
1. Walk to position (70, 52, 7)
1. Proximity Ambush Trigger at (66, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (69, 55, 2)
1. Reset / Loop Scripted Action Queue

### Ped #69 (Guard)

* **Script Entry**: Scenario Script #88

**Scenario Chain**:
1. Enter vehicle #2
1. Proximity Ambush Trigger at (47, 57, 2) [Triggers when player agent enters 6-tile radius]
1. Drive vehicle #2 to (69, 31, 2)
1. Stand Guard / Hold Position (legacy marker)
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (67, 10, 0) | 1024 | None |
| Car #1 | Large Armored Car | (96, 25, 0) | 640 | Ped #8 |
| Car #2 | Large Armored Car | (87, 27, 0) | 640 | Ped #69 |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 68


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (78, 13, 0) and establish perimeter control.
* **Enemy Composition**: 0 enemy syndicate agents across 0 active faction(s) plus 62 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
