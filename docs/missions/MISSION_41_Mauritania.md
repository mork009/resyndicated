# Mission 41: Mauritania

**Country**: Mauritania (`#CNTRY_41`)

**Map ID**: Map 64 (`map64.dat`)

**Map Tile Bounding Box**: Tile X [0 – 92], Tile Y [8 – 75]

**Total Entities**: 109 Active Peds (4 Player Agents, 33 Key/Scripted NPCs, 72 Generic Civilians), 7 Vehicles, 17 Weapons.

---

## 1. Briefing Summary

UNDER GUARD.

PERSONNEL ACQUISITION.

A RENOWNED RESEARCH SCIENTIST IS WORKING FOR THE MILITARY WEAPONS PROGRAM IN THE AREA ALTHOUGH SHE HAS NO IDEA OF THE IMPORTANCE OF HER WORK. THE MILITARY ARE ANXIOUS THAT SHE COMPLETES HER RESEARCH AS SOON AS POSSIBLE AND CONSEQUENTLY SHE IS UNDER HEAVY GUARD.

OUR RESEARCH AND DESIGN BOYS WOULD DEARLY LIKE TO HAVE A CHAT WITH HER AND FIND OUT WHAT SHE KNOWS.

YOUR SQUAD MUST <PERSUADE> THE NOTABLE SCIENTIST TO COME AND SEE THE R AND D LADS.

**DEFENCE UPDATE:**

WHILST SHE IS WORKING IN THE RESEARCH BUILDING THERE IS A VERY STRONG SECURITY PRESENCE. THESE GUARDS ARE WELL EQUIPPED AND VERY WELL TRAINED ALTHOUGH OBVIOUSLY NOT TO OUR STANDARDS.

**TARGET UPDATE:**

RATHER THAN KEEP HER LOCKED AWAY FOR ALL HOURS THE TARGET IS OCCASIONALLY ALLOWED OUT FOR A WALK NOW AND AGAIN. THIS IS DEFINITELY WHEN SHE IS AT HER MOST VULNERABLE. EVEN SO THERE ARE STILL GUARDS ALONG THE WAY.

**TACTICS:**

IF EVER THERE WAS A JOB FOR A SNIPER THEN THIS IS IT. TWO AGENTS SHOULD BE DEPLOYED. ONE WITH A PERSUADERTRON AND THE OTHER WITH A LONG RANGE RIFLE. MAKE A DETAILED STUDY OF THE AREA AND SPECIFICALLY THE GUARD POINTS ALONG THE WAY BEFORE MAKING YOUR MOVE. THE AGENT WITH THE LONG RANGE RIFLE SHOULD PICK OFF THE GUARDS FROM MAXIMUM RANGE WHILE THE OTHER AGENT MOVES IN AND <PERSUADES> THE TARGET. TAKE CARE NOT TO ALERT THE GUARDS AS THEY MAY ACCIDENTALLY CATCH THE SCIENTIST WITH A STRAY SHOT.

---

## 2. Mission Objectives

### Objective #1: Persuade Target

* **Type**: Persuade Target
* **Target**: Ped #16 (Civilian) at Tile (94, 57, 1)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (69, 59, 9)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (82, 9, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (82, 9, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (81, 9, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (80, 9, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (72, 61, 5) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (106, 51, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (81, 44, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (70, 54, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (80, 64, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (69, 59, 9) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (68, 67, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #15 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (59, 54, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #16 | Civilian | 2 HP (Default) | (94, 57, 1) | None | Standing (Unarmed) | Walk to position (80, 53, 2); Walk to position (77, 48, 2); Walk to position (77, 44, 2) (+20 more) |
| Ped #17 | Guard | 8 HP | (69, 17, 0) | None | Standing (Equipped: Minigun (#7)) | None (Stationary) |
| Ped #18 | Guard | 8 HP | (67, 24, 0) | None | Standing (Equipped: Minigun (#6)) | None (Stationary) |
| Ped #19 | Guard | 8 HP | (66, 28, 0) | None | Standing (Equipped: Minigun (#5)) | None (Stationary) |
| Ped #20 | Guard | 8 HP | (71, 35, 0) | None | Standing (Equipped: Minigun (#4)) | None (Stationary) |
| Ped #21 | Guard | 8 HP | (94, 56, 1) | None | Standing (Equipped: Long Range Rifle (#13)) | Walk to position (94, 56, 9); Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (98, 56, 2) (+1 more) |
| Ped #22 | Guard | 8 HP | (97, 60, 0) | None | Standing (Unarmed) | Walk to position (94, 57, 9); Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (97, 58, 2) (+1 more) |
| Ped #23 | Guard | 8 HP | (94, 56, 1) | None | Standing (Equipped: Long Range Rifle (#14), Long Range Rifle (#15)) | Walk to position (94, 56, 9); Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (98, 58, 2) (+1 more) |
| Ped #24 | Guard | 8 HP | (94, 57, 1) | None | Standing (Equipped: Long Range Rifle (#16)) | Walk to position (94, 57, 9); Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (97, 55, 2) (+1 more) |
| Ped #25 | Guard | 8 HP | (94, 57, 1) | None | Standing (Equipped: Long Range Rifle (#18)) | Walk to position (94, 57, 9); Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (99, 59, 2) (+1 more) |
| Ped #26 | Guard | 8 HP | (94, 56, 1) | None | Standing (Equipped: Long Range Rifle (#17)) | Walk to position (94, 56, 9); Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (98, 58, 2) (+1 more) |
| Ped #27 | Guard | 8 HP | (94, 56, 1) | None | Standing (Equipped: Long Range Rifle (#19)) | Walk to position (94, 56, 9); Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (100, 58, 2) (+1 more) |
| Ped #28 | Guard | 8 HP | (94, 57, 1) | None | Standing (Equipped: Long Range Rifle (#20)) | Walk to position (94, 57, 9); Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (100, 58, 2) (+1 more) |
| Ped #29 | Guard | 8 HP | (94, 57, 1) | None | Standing (Equipped: Minigun (#8)) | Walk to position (94, 57, 9) |
| Ped #30 | Guard | 8 HP | (94, 57, 1) | None | Standing (Equipped: Minigun (#9)) | Walk to position (94, 57, 9) |
| Ped #31 | Guard | 8 HP | (94, 56, 1) | None | Standing (Equipped: Minigun (#10)) | Walk to position (94, 56, 9) |
| Ped #32 | Guard | 8 HP | (94, 56, 1) | None | Standing (Equipped: Minigun (#11)) | Walk to position (94, 56, 9) |
| Ped #33 | Guard | 8 HP | (94, 56, 1) | None | Standing (Equipped: Minigun (#12)) | Walk to position (94, 56, 9) |
| Ped #34 | Civilian | 2 HP (Default) | (63, 51, 0) | Car #0 (Civilian Sedan) | In Vehicle (Car #0 (Civilian Sedan)) (Unarmed) | Enter vehicle #0; Drive vehicle #0 to (45, 40, 2) |
| Ped #35 | Civilian | 2 HP (Default) | (61, 54, 0) | Car #1 (Civilian Sedan) | In Vehicle (Car #1 (Civilian Sedan)) (Unarmed) | Enter vehicle #1; Drive vehicle #1 to (44, 37, 2); Enter vehicle #0 |
| Ped #36 | Civilian | 2 HP (Default) | (63, 54, 0) | Car #2 (Civilian Sedan) | In Vehicle (Car #2 (Civilian Sedan)) (Unarmed) | Enter vehicle #2; Drive vehicle #2 to (44, 34, 2) |
| Ped #37 | Civilian | 2 HP (Default) | (63, 31, 0) | Car #3 (Fire Engine) | In Vehicle (Car #3 (Fire Engine)) (Unarmed) | Enter vehicle #3; Drive vehicle #3 to (56, 14, 2) |
| Ped #38 | Civilian | 2 HP (Default) | (48, 45, 0) | Car #4 (Fire Engine) | In Vehicle (Car #4 (Fire Engine)) (Unarmed) | Enter vehicle #4; Drive vehicle #4 to (58, 14, 2) |
| Ped #39 | Civilian | 2 HP (Default) | (89, 45, 0) | Car #5 (Police Cruiser) | In Vehicle (Car #5 (Police Cruiser)) (Unarmed) | Enter vehicle #5; Drive vehicle #5 to (95, 44, 2) |
| Ped #40 | Civilian | 2 HP (Default) | (85, 24, 0) | Car #6 (Police Cruiser) | In Vehicle (Car #6 (Police Cruiser)) (Unarmed) | Enter vehicle #6; Drive vehicle #6 to (96, 48, 2) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 8`, `max_rush_factions = 4`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (8 agents: Peds #8–#15)

* **Idle Movement State**: 8 Stationary Sentry
* **Assigned Root Tactic**: Tactic #1 *(Wildcard (Generic Pool - Sequential Claim))*
* **Execution Chain**:
  1. **Tactic #1** [Wildcard / Proximity Tripwire]: Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer)
  2. **Tactic #2** [Wildcard / Weapon Ambush]: Ambush hold range: equipped weapon range (advances on engagement)
  3. **Tactic #0** [Wildcard / Objective Sweep]: Target: Player Squad

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |
| #1 | Root (Active at Start) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer) | Tactic #2 | Active |
| #2 | Child (after Tactic #1) | Wildcard (Global) | Weapon Ambush | Ambush hold range: equipped weapon range (advances on engagement) | Tactic #0 (Fallback) | Active |
| #3 | Root (Active at Start) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #4 | Root (Active at Start) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer) | Tactic #5 | Active |
| #5 | Child (after Tactic #4) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #6 | Root (Active at Start) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 50 stages (~67.5s timer) | Tactic #7 | Active |
| #7 | Child (after Tactic #6) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #8 | Root (Active at Start) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 50 stages (~67.5s timer) | Tactic #9 | Active |
| #9 | Child (after Tactic #8) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer) | Tactic #10 | Active |
| #10 | Child (after Tactic #9) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #11 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #12 | Active |
| #12 | Child (after Tactic #11) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 200 stages (~270.0s timer) | Tactic #13 | Active |
| #13 | Child (after Tactic #12) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 200 stages (~270.0s timer) | Tactic #14 | Active |
| #14 | Child (after Tactic #13) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #16 (Civilian)

* **Script Entry**: Scenario Script #1

**Scenario Chain**:
1. Walk to position (80, 53, 2)
1. Walk to position (77, 48, 2)
1. Walk to position (77, 44, 2)
1. Walk to position (81, 43, 2)
1. Walk to position (79, 35, 2)
1. Walk to position (78, 34, 2)
1. Walk to position (71, 34, 2)
1. Walk to position (71, 28, 2)
1. Walk to position (69, 27, 2)
1. Timed Pause / Delay (50,000ms)
1. Walk to position (69, 21, 2)
1. Walk to position (68, 9, 2)
1. Walk to position (83, 7, 2)
1. Walk to position (105, 10, 2)
1. Walk to position (106, 25, 2)
1. Walk to position (105, 58, 2)
1. Walk to position (94, 57, 9)
1. Timed Pause / Delay (50,000ms)
1. Timed Pause / Delay (50,000ms)
1. Timed Pause / Delay (50,000ms)
1. Timed Pause / Delay (50,000ms)
1. Walk to position (97, 64, 2)
1. Walk to position (81, 64, 2)

### Ped #21 (Guard)

* **Script Entry**: Scenario Script #36

**Scenario Chain**:
1. Walk to position (94, 56, 9)
1. Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (98, 56, 2)
1. Reset / Loop Scripted Action Queue

### Ped #22 (Guard)

* **Script Entry**: Scenario Script #35

**Scenario Chain**:
1. Walk to position (94, 57, 9)
1. Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (97, 58, 2)
1. Reset / Loop Scripted Action Queue

### Ped #23 (Guard)

* **Script Entry**: Scenario Script #34

**Scenario Chain**:
1. Walk to position (94, 56, 9)
1. Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (98, 58, 2)
1. Reset / Loop Scripted Action Queue

### Ped #24 (Guard)

* **Script Entry**: Scenario Script #33

**Scenario Chain**:
1. Walk to position (94, 57, 9)
1. Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (97, 55, 2)
1. Reset / Loop Scripted Action Queue

### Ped #25 (Guard)

* **Script Entry**: Scenario Script #31

**Scenario Chain**:
1. Walk to position (94, 57, 9)
1. Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (99, 59, 2)
1. Reset / Loop Scripted Action Queue

### Ped #26 (Guard)

* **Script Entry**: Scenario Script #32

**Scenario Chain**:
1. Walk to position (94, 56, 9)
1. Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (98, 58, 2)
1. Reset / Loop Scripted Action Queue

### Ped #27 (Guard)

* **Script Entry**: Scenario Script #30

**Scenario Chain**:
1. Walk to position (94, 56, 9)
1. Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (100, 58, 2)
1. Reset / Loop Scripted Action Queue

### Ped #28 (Guard)

* **Script Entry**: Scenario Script #29

**Scenario Chain**:
1. Walk to position (94, 57, 9)
1. Proximity Ambush Trigger at (98, 56, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (100, 58, 2)
1. Reset / Loop Scripted Action Queue

### Ped #29 (Guard)

* **Script Entry**: Scenario Script #27

**Scenario Chain**:
1. Walk to position (94, 57, 9)

### Ped #30 (Guard)

* **Script Entry**: Scenario Script #25

**Scenario Chain**:
1. Walk to position (94, 57, 9)

### Ped #31 (Guard)

* **Script Entry**: Scenario Script #24

**Scenario Chain**:
1. Walk to position (94, 56, 9)

### Ped #32 (Guard)

* **Script Entry**: Scenario Script #26

**Scenario Chain**:
1. Walk to position (94, 56, 9)

### Ped #33 (Guard)

* **Script Entry**: Scenario Script #28

**Scenario Chain**:
1. Walk to position (94, 56, 9)

### Ped #34 (Civilian)

* **Script Entry**: Scenario Script #66

**Scenario Chain**:
1. Enter vehicle #0
1. Drive vehicle #0 to (45, 40, 2)

### Ped #35 (Civilian)

* **Script Entry**: Scenario Script #63

**Scenario Chain**:
1. Enter vehicle #1
1. Drive vehicle #1 to (44, 37, 2)
1. Enter vehicle #0

### Ped #36 (Civilian)

* **Script Entry**: Scenario Script #61

**Scenario Chain**:
1. Enter vehicle #2
1. Drive vehicle #2 to (44, 34, 2)

### Ped #37 (Civilian)

* **Script Entry**: Scenario Script #68

**Scenario Chain**:
1. Enter vehicle #3
1. Drive vehicle #3 to (56, 14, 2)

### Ped #38 (Civilian)

* **Script Entry**: Scenario Script #70

**Scenario Chain**:
1. Enter vehicle #4
1. Drive vehicle #4 to (58, 14, 2)

### Ped #39 (Civilian)

* **Script Entry**: Scenario Script #74

**Scenario Chain**:
1. Enter vehicle #5
1. Drive vehicle #5 to (95, 44, 2)

### Ped #40 (Civilian)

* **Script Entry**: Scenario Script #72

**Scenario Chain**:
1. Enter vehicle #6
1. Drive vehicle #6 to (96, 48, 2)

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (63, 51, 0) | 1024 | Ped #34 |
| Car #1 | Civilian Sedan | (61, 54, 0) | 1024 | Ped #35 |
| Car #2 | Civilian Sedan | (63, 54, 0) | 1024 | Ped #36 |
| Car #3 | Fire Engine | (63, 31, 0) | 1024 | Ped #37 |
| Car #4 | Fire Engine | (48, 45, 0) | 1024 | Ped #38 |
| Car #5 | Police Cruiser | (89, 45, 0) | 1280 | Ped #39 |
| Car #6 | Police Cruiser | (85, 24, 0) | 1280 | Ped #40 |

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

* **Squad Positioning**: Spawn squad members at Tile (82, 9, 0) and establish perimeter control.
* **Enemy Composition**: 8 enemy syndicate agents across 1 active faction(s) plus 25 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
