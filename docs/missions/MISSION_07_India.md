# Mission 07: India

**Country**: India (`#CNTRY_07`)

**Map ID**: Map 07 (`map07.dat`)

**Map Tile Bounding Box**: Tile X [31 – 75], Tile Y [15 – 67]

**Total Entities**: 147 Active Peds (4 Player Agents, 45 Key/Scripted NPCs, 98 Generic Civilians), 2 Vehicles, 46 Weapons.

---

## 1. Briefing Summary

RANSOM.

ASSAULT.

A TEAM OF BANDITS HAS TAKEN OVER A SMALL RESEARCH COMPLEX IN THE AREA AND THEY ARE HOLDING THE CITY TO RANSOM AND TERRIFYING THE PEOPLE. THEIR LEADER IS AN EX ARMY CORPORAL WHO HAS TRAINED HIS OWN MEN AND USES THEM TO GUARD THE AREA. HE MUST BE TAKEN OUT.

**DEFENCE UPDATE:**

AS FAR AS WE KNOW THEIR WEAPONS CONSIST MAINLY OF LIGHT MACHINE GUNS WITH AN OCCASIONAL MINI GUN. IT IS BELIEVED THAT THERE ARE A LOT OF GUARDS AND REPORTS INDICATE THAT THEY ARE QUITE WELL TRAINED AS A GROUP.

**TACTICS:**

AS THERE IS ONLY ONE ENTRANCE TO THE ACTUAL COMPLEX CAUTION MUST BE USED IN THE ASSAULT. LONG RANGE WEAPONS SHOULD BE DEPLOYED TO TAKE OUT ANY PERIMETER GUARDS WHILST HEAVY WEAPONS ARE USED TO TAKE OUT CONCENTRATED GUARD FORMATIONS. WITH ANY LUCK HE MAY HAVE UNDERESTIMATED OUR WEAPONS TECHNOLOGY AND YOU MAY BE ABLE TO PICK HIM OFF WITHOUT ENTERING THE CAMP. IF YOU ARE FORCED TO ENTER THE CAMP TRY TO TAKE THE GUARDS ON ONE AT A TIME AND NOT LET THEM RUSH YOU AS A GROUP.

---

## 2. Mission Objectives

### Objective #1: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #16 (Guard) at Tile (86, 44, 7)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (59, 14, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (57, 26, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (57, 27, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (57, 27, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (57, 28, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP | (39, 70, 0) | None | Stationary Sentry (Equipped: Minigun (#8)) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP | (62, 75, 0) | None | Stationary Sentry (Equipped: Minigun (#16)) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP | (70, 69, 0) | None | Stationary Sentry (Equipped: Uzi (#18)) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP | (82, 53, 0) | None | Stationary Sentry (Equipped: Uzi (#19)) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 16 HP | (84, 68, 0) | None | Stationary Sentry (Equipped: Flamer (#20)) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 16 HP | (39, 42, 0) | None | Stationary Sentry (Equipped: Flamer (#21)) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 16 HP | (39, 27, 0) | None | Stationary Sentry (Equipped: Long Range Rifle (#22)) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #15 | Enemy Agent (Faction 1: Blue) | 16 HP | (61, 56, 0) | None | Stationary Sentry (Equipped: Long Range Rifle (#23)) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #16 | Guard | 16 HP | (86, 44, 7) | None | Standing (Equipped: Minigun (#40)) | None (Stationary) |
| Ped #17 | Guard | 16 HP | (82, 37, 0) | None | Standing (Equipped: Uzi (#41)) | Walk to position (82, 37, 8); Proximity Ambush Trigger at (79, 38, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (81, 39, 2) |
| Ped #18 | Guard | 16 HP | (84, 37, 0) | None | Standing (Equipped: Uzi (#42)) | Walk to position (84, 37, 8); Proximity Ambush Trigger at (81, 39, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (84, 39, 2) |
| Ped #19 | Guard | 16 HP | (84, 36, 0) | None | Standing (Equipped: Uzi (#43)) | Walk to position (84, 36, 8); Proximity Ambush Trigger at (81, 40, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (81, 41, 2) |
| Ped #20 | Guard | 16 HP | (82, 36, 0) | None | Standing (Equipped: Uzi (#44)) | Walk to position (82, 36, 8); Proximity Ambush Trigger at (81, 39, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (81, 41, 2) |
| Ped #21 | Guard | 16 HP | (88, 36, 0) | None | Standing (Equipped: Uzi (#45)) | Walk to position (88, 36, 5); Proximity Ambush Trigger at (86, 40, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (90, 38, 2) |
| Ped #22 | Guard | 16 HP | (90, 49, 0) | None | Standing (Equipped: Uzi (#46)) | Walk to position (90, 49, 2); Proximity Ambush Trigger at (84, 41, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (87, 45, 2) |
| Ped #23 | Guard | 16 HP | (76, 33, 0) | None | Standing (Equipped: Uzi (#47)) | Walk to position (76, 33, 6); Proximity Ambush Trigger at (78, 37, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 39, 2) |
| Ped #24 | Guard | 16 HP | (76, 33, 0) | None | Standing (Equipped: Uzi (#48)) | Walk to position (76, 33, 6); Proximity Ambush Trigger at (75, 37, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (79, 37, 2) |
| Ped #25 | Guard | 7 HP | (80, 48, 0) | None | Standing (Equipped: Uzi (#51)) | Walk to position (90, 27, 2); Walk to position (79, 27, 2); Walk to position (79, 47, 2) (+5 more) |
| Ped #26 | Guard | 8 HP | (80, 49, 0) | None | Standing (Equipped: Uzi (#49)) | Walk to position (90, 27, 2); Walk to position (79, 27, 2); Walk to position (79, 47, 2) (+1 more) |
| Ped #27 | Guard | 8 HP | (89, 27, 0) | None | Standing (Equipped: Uzi (#50)) | Walk to position (89, 27, 2); Walk to position (89, 40, 2); Walk to position (79, 40, 2) (+1 more) |
| Ped #28 | Guard | 8 HP | (79, 49, 0) | None | Standing (Equipped: Uzi (#52)) | Walk to position (90, 27, 2); Walk to position (79, 27, 2); Walk to position (79, 47, 2) (+1 more) |
| Ped #30 | Police Officer | 6 HP (Default) | (51, 55, 0) | None | Standing (Equipped: Uzi (#54)) | None (Stationary) |
| Ped #32 | Police Officer | 6 HP (Default) | (79, 42, 0) | None | Walking (Equipped: Uzi (#55)) | None (Wandering) |
| Ped #33 | Police Officer | 6 HP (Default) | (80, 76, 0) | None | Walking (Equipped: Uzi (#57)) | None (Wandering) |
| Ped #34 | Police Officer | 6 HP (Default) | (92, 46, 0) | None | Walking (Equipped: Uzi (#58)) | None (Wandering) |
| Ped #35 | Police Officer | 6 HP (Default) | (39, 62, 0) | None | Walking (Equipped: Uzi (#59)) | None (Wandering) |
| Ped #36 | Police Officer | 6 HP (Default) | (90, 47, 0) | None | Walking (Equipped: Uzi (#60)) | None (Wandering) |
| Ped #37 | Police Officer | 6 HP (Default) | (65, 30, 0) | None | Walking (Equipped: Uzi (#61)) | None (Wandering) |
| Ped #38 | Police Officer | 6 HP (Default) | (38, 60, 0) | None | Walking (Equipped: Uzi (#62)) | None (Wandering) |
| Ped #39 | Police Officer | 6 HP (Default) | (76, 73, 0) | None | Walking (Equipped: Uzi (#63)) | None (Wandering) |
| Ped #40 | Police Officer | 6 HP (Default) | (78, 55, 0) | None | Walking (Equipped: Uzi (#64)) | None (Wandering) |
| Ped #41 | Police Officer | 6 HP (Default) | (38, 61, 0) | None | Walking (Equipped: Uzi (#65)) | None (Wandering) |
| Ped #42 | Police Officer | 6 HP (Default) | (80, 39, 0) | None | Walking (Equipped: Uzi (#66)) | None (Wandering) |
| Ped #43 | Police Officer | 6 HP (Default) | (48, 26, 0) | None | Walking (Equipped: Uzi (#67)) | None (Wandering) |
| Ped #44 | Police Officer | 6 HP (Default) | (70, 60, 0) | None | Walking (Equipped: Uzi (#68)) | None (Wandering) |
| Ped #45 | Police Officer | 6 HP (Default) | (85, 35, 0) | None | Walking (Equipped: Uzi (#69)) | None (Wandering) |
| Ped #46 | Police Officer | 6 HP (Default) | (80, 36, 0) | None | Walking (Equipped: Uzi (#70)) | None (Wandering) |
| Ped #47 | Police Officer | 6 HP (Default) | (45, 39, 0) | None | Walking (Equipped: Uzi (#71)) | None (Wandering) |
| Ped #48 | Police Officer | 6 HP (Default) | (64, 35, 0) | None | Walking (Equipped: Uzi (#72)) | None (Wandering) |
| Ped #146 | Police Officer | 6 HP (Default) | (55, 44, 0) | Car #0 (Police Cruiser) | In Vehicle (Car #0 (Police Cruiser)) (Equipped: Uzi (#2)) | Enter vehicle #0; Drive vehicle #0 to (57, 57, 2); Drive vehicle #0 to (74, 54, 2) |
| Ped #147 | Guard | 8 HP (Default) | (53, 66, 0) | None | Standing (Equipped: Uzi (#0)) | Walk to position (53, 66, 7); Proximity Ambush Trigger at (57, 63, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (55, 67, 2) (+1 more) |
| Ped #148 | Guard | 8 HP (Default) | (54, 66, 0) | None | Standing (Equipped: Uzi (#1), Uzi (#3)) | Walk to position (54, 66, 7); Proximity Ambush Trigger at (57, 65, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (55, 67, 2) (+1 more) |
| Ped #149 | Guard | 8 HP (Default) | (54, 67, 0) | None | Standing (Unarmed) | Walk to position (54, 67, 7); Proximity Ambush Trigger at (56, 70, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (56, 67, 2) (+1 more) |
| Ped #150 | Guard | 8 HP (Default) | (54, 67, 0) | None | Standing (Equipped: Uzi (#4)) | Walk to position (54, 67, 7); Proximity Ambush Trigger at (57, 61, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (56, 67, 2) (+1 more) |
| Ped #151 | Guard | 8 HP (Default) | (54, 67, 0) | None | Standing (Equipped: Uzi (#5)) | Walk to position (54, 67, 7); Proximity Ambush Trigger at (56, 63, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (56, 67, 2) (+1 more) |

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

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |
| #1 | Root (Active at Start) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #17 (Guard)

* **Script Entry**: Scenario Script #22

**Scenario Chain**:
1. Walk to position (82, 37, 8)
1. Proximity Ambush Trigger at (79, 38, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (81, 39, 2)

### Ped #18 (Guard)

* **Script Entry**: Scenario Script #25

**Scenario Chain**:
1. Walk to position (84, 37, 8)
1. Proximity Ambush Trigger at (81, 39, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (84, 39, 2)

### Ped #19 (Guard)

* **Script Entry**: Scenario Script #31

**Scenario Chain**:
1. Walk to position (84, 36, 8)
1. Proximity Ambush Trigger at (81, 40, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (81, 41, 2)

### Ped #20 (Guard)

* **Script Entry**: Scenario Script #28

**Scenario Chain**:
1. Walk to position (82, 36, 8)
1. Proximity Ambush Trigger at (81, 39, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (81, 41, 2)

### Ped #21 (Guard)

* **Script Entry**: Scenario Script #40

**Scenario Chain**:
1. Walk to position (88, 36, 5)
1. Proximity Ambush Trigger at (86, 40, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (90, 38, 2)

### Ped #22 (Guard)

* **Script Entry**: Scenario Script #43

**Scenario Chain**:
1. Walk to position (90, 49, 2)
1. Proximity Ambush Trigger at (84, 41, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (87, 45, 2)

### Ped #23 (Guard)

* **Script Entry**: Scenario Script #37

**Scenario Chain**:
1. Walk to position (76, 33, 6)
1. Proximity Ambush Trigger at (78, 37, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 39, 2)

### Ped #24 (Guard)

* **Script Entry**: Scenario Script #34

**Scenario Chain**:
1. Walk to position (76, 33, 6)
1. Proximity Ambush Trigger at (75, 37, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (79, 37, 2)

### Ped #25 (Guard)

* **Script Entry**: Scenario Script #54

**Scenario Chain**:
1. Walk to position (90, 27, 2)
1. Walk to position (79, 27, 2)
1. Walk to position (79, 47, 2)
1. Walk to position (88, 40, 2)
1. Walk to position (88, 36, 5)
1. Timed Pause / Delay (5,000ms)
1. Walk to position (88, 40, 2)
1. Walk to position (90, 40, 2)

### Ped #26 (Guard)

* **Script Entry**: Scenario Script #50

**Scenario Chain**:
1. Walk to position (90, 27, 2)
1. Walk to position (79, 27, 2)
1. Walk to position (79, 47, 2)
1. Walk to position (90, 45, 2)

### Ped #27 (Guard)

* **Script Entry**: Scenario Script #62

**Scenario Chain**:
1. Walk to position (89, 27, 2)
1. Walk to position (89, 40, 2)
1. Walk to position (79, 40, 2)
1. Walk to position (80, 27, 2)

### Ped #28 (Guard)

* **Script Entry**: Scenario Script #46

**Scenario Chain**:
1. Walk to position (90, 27, 2)
1. Walk to position (79, 27, 2)
1. Walk to position (79, 47, 2)
1. Walk to position (90, 45, 2)

### Ped #146 (Police Officer)

* **Script Entry**: Scenario Script #72

**Scenario Chain**:
1. Enter vehicle #0
1. Drive vehicle #0 to (57, 57, 2)
1. Drive vehicle #0 to (74, 54, 2)

### Ped #147 (Guard)

* **Script Entry**: Scenario Script #91

**Scenario Chain**:
1. Walk to position (53, 66, 7)
1. Proximity Ambush Trigger at (57, 63, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (55, 67, 2)
1. Reset / Loop Scripted Action Queue

### Ped #148 (Guard)

* **Script Entry**: Scenario Script #87

**Scenario Chain**:
1. Walk to position (54, 66, 7)
1. Proximity Ambush Trigger at (57, 65, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (55, 67, 2)
1. Reset / Loop Scripted Action Queue

### Ped #149 (Guard)

* **Script Entry**: Scenario Script #83

**Scenario Chain**:
1. Walk to position (54, 67, 7)
1. Proximity Ambush Trigger at (56, 70, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (56, 67, 2)
1. Reset / Loop Scripted Action Queue

### Ped #150 (Guard)

* **Script Entry**: Scenario Script #79

**Scenario Chain**:
1. Walk to position (54, 67, 7)
1. Proximity Ambush Trigger at (57, 61, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (56, 67, 2)
1. Reset / Loop Scripted Action Queue

### Ped #151 (Guard)

* **Script Entry**: Scenario Script #75

**Scenario Chain**:
1. Walk to position (54, 67, 7)
1. Proximity Ambush Trigger at (56, 63, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (56, 67, 2)
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Police Cruiser | (55, 44, 0) | 1280 | Ped #146 |
| Car #1 | Civilian Sedan | (49, 68, 0) | 1024 | None |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 1 | Total Inventory Items: 45

#### Ground Pickups:

| Weapon Index | Weapon Subtype | Ground Tile (X, Y, Z) |
| :--- | :--- | :--- |
| Weapon #56 | Uzi | (81, 50, 0) |

---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (57, 26, 0) and establish perimeter control.
* **Enemy Composition**: 8 enemy syndicate agents across 1 active faction(s) plus 37 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
