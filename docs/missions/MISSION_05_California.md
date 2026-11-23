# Mission 05: California

**Country**: California (`#CNTRY_05`)

**Map ID**: Map 05 (`map05.dat`)

**Map Tile Bounding Box**: Tile X [17 – 91], Tile Y [10 – 76]

**Total Entities**: 151 Active Peds (4 Player Agents, 81 Key/Scripted NPCs, 66 Generic Civilians), 8 Vehicles, 84 Weapons.

---

## 1. Briefing Summary

CHEM CORP PLANT.

PERSONNEL ACQUISITION.

DOCTOR CAULDWELL HAS MADE SIGNIFICANT ADVANCES IN THE FIELD OF CHEMICAL WARFARE AND WOULD THEREFORE BE AN IMMENSE ASSET TO THE COMPANY. HOWEVER HIS CURRENT WORKPLACE HAS COME UNDER SCRUTINY FROM ENEMY SYNDICATES WHO ARE ANXIOUS TO GRAB HIM FOR THEIR OWN NEEDS. OBVIOUSLY WE CANNOT ALLOW THIS TO HAPPEN.

YOUR MISSION IS TO <PERSUADE> THE GOOD DOCTOR TO MOVE OFFICES.

**CITY UPDATE:**

THE CHEMICAL PLANT THAT THE DOCTOR IS WORKING AT IS LOCATED IN THE SOUTHEASTERN CORNER OF THE CITY.

**DEFENCE UPDATE:**

IT IS BELIEVED THAT THE DOCTOR HAS BEEN AWARE OF A LOT OF SYNDICATE ACTIVITY AND INTEREST FOR A LONG TIME. IT IS THEREFORE REASONABLE TO ASSUME THAT HE WILL HAVE HIRED BODYGUARDS. IF THIS IS THE CASE THEN YOU CAN EXPECT THE COMPLEX TO BE WELL DEFENDED.

**TACTICS:**

DUE TO THE PROXIMITY AND AGGRESSION OF ENEMY SYNDICATES YOU WOULD BE WELL ADVISED TO DEPLOY A FULL SQUAD WITH AUTOMATIC WEAPONS. IF THE CHEMICAL PLANT IS DEFENDED THEN LONG RANGE WEAPONRY SHOULD BE USED TO ACCURATELY PICK OFF ANY GUARDS THAT HAPPEN TO BE AROUND WITHOUT ENDANGERING THE DOCTOR.

---

## 2. Mission Objectives

### Objective #1: Persuade Target

* **Type**: Persuade Target
* **Target**: Ped #12 (Civilian) at Tile (99, 84, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (21, 46, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (26, 22, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (27, 22, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (28, 22, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (28, 22, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Guard | 8 HP (Default) | (104, 81, 0) | None | Standing (Equipped: Long Range Rifle (#9)) | None (Stationary) |
| Ped #10 | Guard | 8 HP | (94, 84, 0) | None | Standing (Equipped: Minigun (#13)) | None (Stationary) |
| Ped #11 | Guard | 8 HP | (90, 74, 0) | None | Standing (Equipped: Uzi (#1)) | None (Stationary) |
| Ped #12 | Civilian | 8 HP | (99, 84, 0) | None | Standing (Unarmed) | Walk to position (96, 88, 6); Timed Pause / Delay (50,000ms); Walk to position (102, 72, 10) (+1 more) |
| Ped #13 | Civilian | 2 HP (Default) | (79, 57, 0) | Car #5 (Civilian Sedan) | In Vehicle (Car #5 (Civilian Sedan)) (Unarmed) | Enter vehicle #5; Drive vehicle #5 to (83, 60, 2) |
| Ped #15 | Civilian | 2 HP (Default) | (65, 64, 0) | Car #1 (Civilian Sedan) | In Vehicle (Car #1 (Civilian Sedan)) (Unarmed) | Enter vehicle #1; Drive vehicle #1 to (84, 64, 2); Drive vehicle #1 to (59, 53, 2) (+1 more) |
| Ped #16 | Enemy Agent (Faction 2: Orange) | 20 HP | (21, 71, 0) | None | Stationary Sentry (Equipped: Uzi (#17)) | Faction 2 AI Tactics: Tactic #2 (Objective Sweep -> Player Squad) |
| Ped #17 | Enemy Agent (Faction 2: Orange) | 20 HP | (43, 83, 0) | None | Stationary Sentry (Equipped: Uzi (#19)) | Faction 2 AI Tactics: Tactic #2 (Objective Sweep -> Player Squad) |
| Ped #18 | Enemy Agent (Faction 2: Orange) | 20 HP | (29, 87, 0) | None | Stationary Sentry (Equipped: Uzi (#20)) | Faction 2 AI Tactics: Tactic #2 (Objective Sweep -> Player Squad) |
| Ped #19 | Enemy Agent (Faction 2: Orange) | 20 HP | (65, 81, 0) | None | Stationary Sentry (Equipped: Uzi (#21)) | Faction 2 AI Tactics: Tactic #2 (Objective Sweep -> Player Squad) |
| Ped #20 | Enemy Agent (Faction 2: Orange) | 20 HP | (53, 69, 0) | None | Stationary Sentry (Equipped: Uzi (#22)) | Faction 2 AI Tactics: Tactic #2 (Objective Sweep -> Player Squad) |
| Ped #21 | Enemy Agent (Faction 2: Orange) | 20 HP | (38, 87, 0) | None | Stationary Sentry (Equipped: Uzi (#23)) | Faction 2 AI Tactics: Tactic #2 (Objective Sweep -> Player Squad) |
| Ped #22 | Enemy Agent (Faction 2: Orange) | 20 HP | (54, 80, 0) | None | Stationary Sentry (Equipped: Uzi (#24)) | Faction 2 AI Tactics: Tactic #2 (Objective Sweep -> Player Squad) |
| Ped #23 | Enemy Agent (Faction 2: Orange) | 20 HP | (34, 85, 0) | None | Stationary Sentry (Equipped: Uzi (#25)) | Faction 2 AI Tactics: Tactic #2 (Objective Sweep -> Player Squad) |
| Ped #24 | Enemy Agent (Faction 3: Yellow A) | 20 HP | (97, 30, 0) | None | Stationary Sentry (Equipped: Uzi (#18)) | Faction 3 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #25 | Enemy Agent (Faction 3: Yellow A) | 20 HP | (105, 21, 0) | None | Stationary Sentry (Equipped: Uzi (#26)) | Faction 3 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #26 | Enemy Agent (Faction 3: Yellow A) | 20 HP | (101, 15, 0) | None | Stationary Sentry (Equipped: Uzi (#27)) | Faction 3 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #27 | Enemy Agent (Faction 3: Yellow A) | 20 HP | (91, 20, 1) | None | Stationary Sentry (Equipped: Uzi (#29)) | Faction 3 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #28 | Enemy Agent (Faction 3: Yellow A) | 20 HP | (92, 24, 0) | None | Stationary Sentry (Equipped: Uzi (#30)) | Faction 3 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #29 | Enemy Agent (Faction 3: Yellow A) | 20 HP | (75, 9, 0) | None | Stationary Sentry (Equipped: Uzi (#31)) | Faction 3 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #30 | Enemy Agent (Faction 3: Yellow A) | 20 HP | (83, 9, 0) | None | Stationary Sentry (Equipped: Uzi (#32)) | Faction 3 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #31 | Enemy Agent (Faction 3: Yellow A) | 20 HP | (105, 9, 0) | None | Stationary Sentry (Equipped: Uzi (#33)) | Faction 3 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #32 | Police Officer | 6 HP (Default) | (89, 15, 1) | None | Walking (Equipped: Long Range Rifle (#28)) | None (Wandering) |
| Ped #34 | Police Officer | 6 HP (Default) | (75, 12, 0) | None | Walking (Equipped: Long Range Rifle (#35)) | None (Wandering) |
| Ped #35 | Police Officer | 6 HP (Default) | (47, 9, 0) | None | Walking (Equipped: Long Range Rifle (#36)) | None (Wandering) |
| Ped #36 | Police Officer | 6 HP (Default) | (51, 21, 0) | None | Walking (Equipped: Long Range Rifle (#37)) | None (Wandering) |
| Ped #37 | Police Officer | 6 HP (Default) | (43, 40, 0) | None | Walking (Equipped: Long Range Rifle (#38)) | None (Wandering) |
| Ped #38 | Police Officer | 6 HP (Default) | (27, 87, 0) | None | Walking (Equipped: Long Range Rifle (#39)) | None (Wandering) |
| Ped #39 | Police Officer | 6 HP (Default) | (50, 66, 1) | None | Walking (Equipped: Long Range Rifle (#40)) | None (Wandering) |
| Ped #40 | Police Officer | 6 HP (Default) | (30, 44, 0) | None | Walking (Equipped: Long Range Rifle (#41)) | None (Wandering) |
| Ped #41 | Civilian | 2 HP (Default) | (57, 47, 0) | Car #3 (Civilian Sedan) | In Vehicle (Car #3 (Civilian Sedan)) (Unarmed) | Enter vehicle #3; Drive vehicle #3 to (85, 62, 2); Drive vehicle #3 to (72, 23, 2) (+1 more) |
| Ped #42 | Police Officer | 6 HP (Default) | (35, 16, 0) | None | Walking (Equipped: Long Range Rifle (#43)) | None (Wandering) |
| Ped #43 | Police Officer | 6 HP (Default) | (20, 10, 0) | None | Walking (Equipped: Long Range Rifle (#44)) | None (Wandering) |
| Ped #44 | Civilian | 2 HP (Default) | (47, 45, 0) | Car #4 (Civilian Sedan) | In Vehicle (Car #4 (Civilian Sedan)) (Unarmed) | Enter vehicle #4; Drive vehicle #4 to (40, 55, 2); Drive vehicle #4 to (76, 23, 2) |
| Ped #45 | Police Officer | 6 HP (Default) | (96, 9, 0) | None | Walking (Equipped: Long Range Rifle (#46)) | None (Wandering) |
| Ped #46 | Police Officer | 6 HP (Default) | (20, 65, 0) | None | Walking (Equipped: Long Range Rifle (#47)) | None (Wandering) |
| Ped #47 | Police Officer | 6 HP (Default) | (26, 32, 0) | None | Walking (Equipped: Long Range Rifle (#48)) | None (Wandering) |
| Ped #48 | Police Officer | 6 HP (Default) | (43, 18, 0) | None | Walking (Equipped: Long Range Rifle (#49)) | None (Wandering) |
| Ped #49 | Police Officer | 6 HP (Default) | (34, 70, 0) | None | Walking (Equipped: Long Range Rifle (#50)) | None (Wandering) |
| Ped #50 | Police Officer | 6 HP (Default) | (47, 48, 0) | None | Walking (Equipped: Long Range Rifle (#51)) | None (Wandering) |
| Ped #51 | Police Officer | 6 HP (Default) | (28, 63, 0) | None | Walking (Equipped: Long Range Rifle (#52)) | None (Wandering) |
| Ped #52 | Police Officer | 6 HP (Default) | (28, 48, 0) | None | Walking (Equipped: Long Range Rifle (#53)) | None (Wandering) |
| Ped #53 | Police Officer | 6 HP (Default) | (86, 74, 0) | None | Walking (Equipped: Long Range Rifle (#54)) | None (Wandering) |
| Ped #54 | Police Officer | 6 HP (Default) | (24, 51, 0) | None | Walking (Equipped: Long Range Rifle (#55)) | None (Wandering) |
| Ped #55 | Police Officer | 6 HP (Default) | (66, 26, 0) | None | Walking (Equipped: Long Range Rifle (#56)) | None (Wandering) |
| Ped #56 | Police Officer | 6 HP (Default) | (46, 82, 0) | None | Walking (Equipped: Long Range Rifle (#57)) | None (Wandering) |
| Ped #57 | Police Officer | 6 HP (Default) | (53, 44, 0) | None | Walking (Equipped: Long Range Rifle (#58)) | None (Wandering) |
| Ped #58 | Police Officer | 6 HP (Default) | (37, 44, 0) | None | Walking (Equipped: Uzi (#59)) | None (Wandering) |
| Ped #59 | Police Officer | 6 HP (Default) | (28, 89, 0) | None | Walking (Equipped: Uzi (#60)) | None (Wandering) |
| Ped #60 | Police Officer | 6 HP (Default) | (51, 40, 0) | None | Walking (Equipped: Uzi (#61)) | None (Wandering) |
| Ped #61 | Police Officer | 6 HP (Default) | (20, 31, 0) | None | Walking (Equipped: Uzi (#62)) | None (Wandering) |
| Ped #62 | Police Officer | 6 HP (Default) | (30, 57, 0) | None | Walking (Equipped: Uzi (#63)) | None (Wandering) |
| Ped #63 | Police Officer | 6 HP (Default) | (67, 26, 0) | None | Walking (Equipped: Uzi (#64)) | None (Wandering) |
| Ped #64 | Police Officer | 6 HP (Default) | (38, 44, 0) | None | Walking (Equipped: Uzi (#65)) | None (Wandering) |
| Ped #65 | Police Officer | 6 HP (Default) | (32, 4, 0) | None | Walking (Equipped: Uzi (#66)) | None (Wandering) |
| Ped #66 | Police Officer | 6 HP (Default) | (81, 25, 9) | None | Walking (Equipped: Uzi (#67)) | None (Wandering) |
| Ped #67 | Police Officer | 6 HP (Default) | (101, 34, 0) | None | Walking (Equipped: Uzi (#68)) | None (Wandering) |
| Ped #68 | Police Officer | 6 HP (Default) | (64, 44, 0) | None | Walking (Equipped: Uzi (#69)) | None (Wandering) |
| Ped #69 | Police Officer | 6 HP (Default) | (28, 57, 0) | None | Walking (Equipped: Uzi (#70)) | None (Wandering) |
| Ped #70 | Police Officer | 6 HP (Default) | (38, 8, 0) | None | Walking (Equipped: Uzi (#71)) | None (Wandering) |
| Ped #71 | Police Officer | 6 HP (Default) | (47, 83, 2) | None | Walking (Equipped: Uzi (#72)) | None (Wandering) |
| Ped #72 | Police Officer | 6 HP (Default) | (51, 40, 0) | None | Walking (Equipped: Uzi (#73)) | None (Wandering) |
| Ped #73 | Police Officer | 6 HP (Default) | (60, 73, 0) | None | Walking (Equipped: Uzi (#74)) | None (Wandering) |
| Ped #74 | Police Officer | 6 HP (Default) | (39, 26, 0) | None | Walking (Equipped: Uzi (#75)) | None (Wandering) |
| Ped #75 | Police Officer | 6 HP (Default) | (23, 3, 0) | None | Walking (Equipped: Uzi (#76)) | None (Wandering) |
| Ped #76 | Police Officer | 6 HP (Default) | (52, 26, 0) | None | Walking (Equipped: Uzi (#77)) | None (Wandering) |
| Ped #77 | Police Officer | 6 HP (Default) | (38, 4, 0) | None | Walking (Equipped: Uzi (#78)) | None (Wandering) |
| Ped #78 | Police Officer | 6 HP (Default) | (69, 20, 5) | None | Walking (Equipped: Uzi (#79)) | None (Wandering) |
| Ped #79 | Police Officer | 6 HP (Default) | (44, 71, 1) | None | Walking (Equipped: Uzi (#80)) | None (Wandering) |
| Ped #80 | Police Officer | 6 HP (Default) | (43, 59, 0) | None | Walking (Equipped: Uzi (#81)) | None (Wandering) |
| Ped #81 | Police Officer | 6 HP (Default) | (101, 71, 0) | None | Walking (Equipped: Laser (#82)) | None (Wandering) |
| Ped #82 | Police Officer | 6 HP (Default) | (88, 32, 0) | None | Walking (Equipped: Laser (#83)) | None (Wandering) |
| Ped #83 | Police Officer | 6 HP (Default) | (47, 31, 0) | None | Walking (Equipped: Laser (#84)) | None (Wandering) |
| Ped #84 | Police Officer | 6 HP (Default) | (101, 34, 0) | None | Walking (Equipped: Laser (#85)) | None (Wandering) |
| Ped #85 | Police Officer | 6 HP (Default) | (29, 50, 0) | None | Walking (Equipped: Laser (#86)) | None (Wandering) |
| Ped #86 | Civilian | 2 HP (Default) | (61, 46, 0) | Car #2 (Civilian Sedan) | In Vehicle (Car #2 (Civilian Sedan)) (Unarmed) | Enter vehicle #2; Drive vehicle #2 to (90, 62, 2); Drive vehicle #2 to (89, 47, 2) (+2 more) |
| Ped #91 | Police Officer | 6 HP (Default) | (77, 48, 0) | Car #7 (Police Cruiser) | In Vehicle (Car #7 (Police Cruiser)) (Equipped: Uzi (#4)) | Enter vehicle #7; Drive vehicle #7 to (66, 54, 3) |
| Ped #95 | Police Officer | 6 HP (Default) | (42, 61, 0) | Car #6 (Police Cruiser) | In Vehicle (Car #6 (Police Cruiser)) (Equipped: Uzi (#10)) | Enter vehicle #6; Drive vehicle #6 to (55, 53, 2) |
| Ped #161 | Guard | 8 HP (Default) | (99, 75, 0) | None | Standing (Equipped: Long Range Rifle (#87)) | None (Stationary) |
| Ped #164 | Guard | 8 HP (Default) | (98, 88, 0) | None | Standing (Equipped: Long Range Rifle (#90)) | None (Stationary) |
| Ped #166 | Guard | 8 HP (Default) | (96, 87, 0) | None | Standing (Equipped: Long Range Rifle (#92)) | None (Stationary) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 4`, `peds_per_squad = 8`, `max_rush_factions = 0`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 2 (Orange Syndicate) (8 agents: Peds #16–#23)

* **Idle Movement State**: 8 Stationary Sentry
* **Assigned Root Tactic**: Tactic #2 *(Wildcard (Generic Pool - Sequential Claim))*
* **Execution Chain**:
  1. **Tactic #2** [Wildcard / Objective Sweep]: Target: Player Squad

#### Faction 3 (Yellow Syndicate A) (8 agents: Peds #24–#31)

* **Idle Movement State**: 8 Stationary Sentry
* **Assigned Root Tactic**: Tactic #1 *(Wildcard (Generic Pool - Wrap-Around Reuse))*
* **Execution Chain**:
  1. **Tactic #1** [Wildcard / Proximity Tripwire]: Watch Player Squad within 12 tiles (each sentry) OR 50 stages (~67.5s timer)
  2. **Tactic #3** [Wildcard / Objective Sweep]: Target: Player Squad

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |
| #1 | Root (Active at Start) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 50 stages (~67.5s timer) | Tactic #3 | Active |
| #2 | Root (Active at Start) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #3 | Child (after Tactic #1) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #12 (Civilian)

* **Script Entry**: Scenario Script #14

**Scenario Chain**:
1. Walk to position (96, 88, 6)
1. Timed Pause / Delay (50,000ms)
1. Walk to position (102, 72, 10)
1. Timed Pause / Delay (50,000ms)

### Ped #13 (Civilian)

* **Script Entry**: Scenario Script #18

**Scenario Chain**:
1. Enter vehicle #5
1. Drive vehicle #5 to (83, 60, 2)

### Ped #15 (Civilian)

* **Script Entry**: Scenario Script #20

**Scenario Chain**:
1. Enter vehicle #1
1. Drive vehicle #1 to (84, 64, 2)
1. Drive vehicle #1 to (59, 53, 2)
1. Drive vehicle #1 to (40, 53, 2)

### Ped #41 (Civilian)

* **Script Entry**: Scenario Script #24

**Scenario Chain**:
1. Enter vehicle #3
1. Drive vehicle #3 to (85, 62, 2)
1. Drive vehicle #3 to (72, 23, 2)
1. Drive vehicle #3 to (48, 32, 2)

### Ped #44 (Civilian)

* **Script Entry**: Scenario Script #28

**Scenario Chain**:
1. Enter vehicle #4
1. Drive vehicle #4 to (40, 55, 2)
1. Drive vehicle #4 to (76, 23, 2)

### Ped #86 (Civilian)

* **Script Entry**: Scenario Script #31

**Scenario Chain**:
1. Enter vehicle #2
1. Drive vehicle #2 to (90, 62, 2)
1. Drive vehicle #2 to (89, 47, 2)
1. Drive vehicle #2 to (60, 53, 2)
1. Drive vehicle #2 to (54, 47, 2)

### Ped #91 (Police Officer)

* **Script Entry**: Scenario Script #36

**Scenario Chain**:
1. Enter vehicle #7
1. Drive vehicle #7 to (66, 54, 3)

### Ped #95 (Police Officer)

* **Script Entry**: Scenario Script #38

**Scenario Chain**:
1. Enter vehicle #6
1. Drive vehicle #6 to (55, 53, 2)

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (22, 66, 0) | 1024 | None |
| Car #1 | Civilian Sedan | (65, 64, 0) | 1024 | Ped #15 |
| Car #2 | Civilian Sedan | (61, 46, 0) | 1024 | Ped #86 |
| Car #3 | Civilian Sedan | (57, 47, 0) | 1024 | Ped #41 |
| Car #4 | Civilian Sedan | (47, 45, 0) | 1024 | Ped #44 |
| Car #5 | Civilian Sedan | (79, 57, 0) | 1024 | Ped #13 |
| Car #6 | Police Cruiser | (42, 61, 0) | 1280 | Ped #95 |
| Car #7 | Police Cruiser | (77, 48, 0) | 1280 | Ped #91 |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 1 | Total Inventory Items: 83

#### Ground Pickups:

| Weapon Index | Weapon Subtype | Ground Tile (X, Y, Z) |
| :--- | :--- | :--- |
| Weapon #15 | Uzi | (99, 71, 0) |

---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (26, 22, 0) and establish perimeter control.
* **Enemy Composition**: 16 enemy syndicate agents across 2 active faction(s) plus 65 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
