# Mission 01: Western Europe

**Country**: Western Europe (`#CNTRY_01`)

**Map ID**: Map 01 (`map01.dat`)

**Map Tile Bounding Box**: Tile X [38 – 74], Tile Y [20 – 48]

**Total Entities**: 10 Active Peds (4 Player Agents, 6 Key/Scripted NPCs, 0 Generic Civilians), 1 Vehicles, 6 Weapons.

---

## 1. Briefing Summary

MERCENARY CAMP.

ASSASSINATION.

INTELLIGENCE REPORTS HAVE INDICATED THAT AN ARMY COLONEL IS STEALING RESOURCES FROM YOUR WEAPONS DIVISION AND USING THEM TO EQUIP HIS MERCENARIES.

HIS BASE HAS BEEN SET UP JUST OUTSIDE ONE OF OUR CITIES AND THE POPULACE HAVE REPORTED DISTURBANCES CAUSED BY HIS TROOPS.

YOUR SQUAD WILL BE DEPLOYED NEAR THE ENTRANCE TO THE CAMP. THEIR OBJECTIVE IS TO ELIMINATE THE COLONEL.

**DEFENCE UPDATE:**

SURVEILLANCE REPORTS SHOW THAT THE CAMP IS GUARDED BY ABOUT FIVE MILITIA ARMED WITH BOTH SHOTGUNS AND PISTOLS.

**TARGET UPDATE:**

THE COLONEL IS BELIEVED TO STAY IN THE NORTHERN MOST BUILDING OF THE CAMP. IF HE BECOMES AWARE OF YOUR PRESENCE HE MAY DECIDE TO WITHDRAW. HE MUST NOT BE ALLOWED TO ESCAPE.

**TACTICS:**

A SINGLE AGENT COULD INFILTRATE THE CAMP AND ASSASSINATE THE TARGET QUITE EASILY. IT IS IMPORTANT TO NOTE THAT WHILST A SHOTGUN IS DEVASTATING AS A CLOSE IN WEAPON IT IS SERIOUSLY OUTRANGED BY THE PISTOL AND SO PROVISION SHOULD BE MADE FOR BOTH OF THESE WEAPONS TO BE USED TO THEIR BEST POTENTIAL. IT IS ALSO ADVISED THAT A SIMILARLY EQUIPPED AGENT BE KEPT OUTSIDE THE CAMP FOR BACKUP IF NEEDED.

---

## 2. Mission Objectives

### Objective #1: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #8 (Civilian) at Tile (63, 29, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (90, 40, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (90, 40, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (90, 39, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (90, 39, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Civilian | 2 HP (Default) | (63, 29, 0) | None | Standing (Unarmed) | Walk to position (63, 29, 6); Proximity Ambush Trigger at (62, 32, 2) [Triggers when player agent enters 6-tile radius]; Enter vehicle #0 (+2 more) |
| Ped #9 | Guard | 8 HP (Default) | (71, 51, 0) | None | Standing (Equipped: Shotgun (#2)) | Walk to position (71, 51, 6); Proximity Ambush Trigger at (75, 37, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (75, 47, 2) (+1 more) |
| Ped #10 | Guard | 8 HP (Default) | (73, 31, 0) | None | Standing (Equipped: Shotgun (#11)) | Walk to position (73, 31, 5); Proximity Ambush Trigger at (69, 33, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (72, 33, 2) (+1 more) |
| Ped #11 | Guard | 8 HP (Default) | (70, 39, 0) | None | Standing (Equipped: Pistol (#4), Pistol (#6)) | Walk to position (70, 39, 2); Proximity Ambush Trigger at (79, 37, 2) [Triggers when player agent enters 6-tile radius]; Reset / Loop Scripted Action Queue |
| Ped #12 | Guard | 8 HP (Default) | (49, 53, 0) | None | Standing (Equipped: Uzi (#0)) | None (Stationary) |
| Ped #13 | Civilian | 2 HP (Default) | (59, 45, 0) | None | Standing (Equipped: Shotgun (#12)) | Walk to position (59, 45, 9); Proximity Ambush Trigger at (57, 49, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (59, 48, 2) (+1 more) |

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 1`, `peds_per_squad = 8`, `max_rush_factions = 0`

No hostile syndicate factions are deployed in this mission.

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |

* **Fallback Resolution Note**: Any active hostile faction without a matching faction-specific root (`1..7`) in slots `1..127` falls back to `Tactic #0` (when enabled).

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #8 (Civilian)

* **Script Entry**: Scenario Script #55

**Scenario Chain**:
1. Walk to position (63, 29, 6)
1. Proximity Ambush Trigger at (62, 32, 2) [Triggers when player agent enters 6-tile radius]
1. Enter vehicle #0
1. Drive vehicle #0 to (50, 75, 2)
1. Target Escaped (Mission Failure Condition)

### Ped #9 (Guard)

* **Script Entry**: Scenario Script #33

**Scenario Chain**:
1. Walk to position (71, 51, 6)
1. Proximity Ambush Trigger at (75, 37, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (75, 47, 2)
1. Reset / Loop Scripted Action Queue

### Ped #10 (Guard)

* **Script Entry**: Scenario Script #37

**Scenario Chain**:
1. Walk to position (73, 31, 5)
1. Proximity Ambush Trigger at (69, 33, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (72, 33, 2)
1. Reset / Loop Scripted Action Queue

### Ped #11 (Guard)

* **Script Entry**: Scenario Script #25

**Scenario Chain**:
1. Walk to position (70, 39, 2)
1. Proximity Ambush Trigger at (79, 37, 2) [Triggers when player agent enters 6-tile radius]
1. Reset / Loop Scripted Action Queue

### Ped #13 (Civilian)

* **Script Entry**: Scenario Script #46

**Scenario Chain**:
1. Walk to position (59, 45, 9)
1. Proximity Ambush Trigger at (57, 49, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (59, 48, 2)
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (59, 34, 0) | 1024 | None |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 6


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

* **Level Data Adjustment**: Elevation for Ped #8's scenario step is corrected to Z = 2 during level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (90, 40, 0) and establish perimeter control.
* **Enemy Composition**: 0 enemy syndicate agents across 0 active faction(s) plus 6 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
