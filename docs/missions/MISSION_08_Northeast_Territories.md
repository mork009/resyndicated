# Mission 08: Northeast Territories

**Country**: Northeast Territories (`#CNTRY_08`)

**Map ID**: Map 08 (`map08.dat`)

**Map Tile Bounding Box**: Tile X [19 – 81], Tile Y [6 – 77]

**Total Entities**: 82 Active Peds (4 Player Agents, 47 Key/Scripted NPCs, 31 Generic Civilians), 8 Vehicles, 53 Weapons.

---

## 1. Briefing Summary

EX750 CAPTURE.

EQUIPMENT ACQUISITION.

THE ARMY HAVE DEVELOPED A NEW ANTI TANK WEAPON AND OUR RESEARCH BOYS WANT IT.

IT HAS BEEN DESIGNED AND BUILT IN THE SARACEN MILITARY BASE IN THE SOUTH EAST OF THE CITY AND WE BELIEVE IT IS STILL STORED ON THE BASE.

GENERAL UPDATE:

THE FIRST TASK IS TO REACH THE BASE. THIS IS HARDER THAN IT SOUNDS AS THE ARMY RECEIVED SOME BACKING FROM A RIVAL SYNDICATE. AS THE SYNDICATE DOESNT WANT ITS INVESTMENT TO BE LOST BE ON THE LOOK OUT FOR ENEMY AGENTS.

**DEFENCE UPDATE:**

BEING A MILITARY ESTABLISHMENT THE BASE IS OBVIOUSLY WELL DEFENDED. THE DEFENDERS ARE BELIEVED TO HAVE ACCESS TO ALL THE LATEST MILITARY HARDWARE SO EXPECT SOME SERIOUS RESISTANCE.

**TARGET UPDATE:**

WE BELIEVE THE WEAPON ITSELF IS HELD IN A LOCKED BUILDING IN THE SOUTH OF THE BASE.

**TACTICS:**

A WHOLE SQUAD SHOULD BE DEPLOYED. SPLIT THE SQUAD INTO 2 GROUPS FOR THE MOVE ON THE CAMP. THAT WAY IF ONE OF THEM IS WIPED OUT YOU WILL STILL HAVE A CHANCE WITH THE SECOND.

LONG RANGE GUNS OR HEAVY WEAPONS SHOULD BE USED TO TAKE OUT THE PERIMETER GUARDS AND MINI GUNS SHOULD TAKE CARE OF THE REST.

---

## 2. Mission Objectives

### Objective #1: Acquire Equipment / Take Weapon

* **Type**: Acquire Equipment / Take Weapon
* **Target**: Weapon #56 (Laser) at Tile (84, 84, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (48, 18, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (47, 19, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (48, 19, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (48, 19, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (49, 19, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 20 HP | (41, 78, 0) | None | Stationary Sentry (Equipped: Minigun (#9)) | Faction 1 AI Tactics: Tactic #1 (Seek & Destroy -> Faction 3: Yellow A) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 20 HP | (42, 78, 0) | None | Stationary Sentry (Equipped: Uzi (#10)) | Faction 1 AI Tactics: Tactic #1 (Seek & Destroy -> Faction 3: Yellow A) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 20 HP | (43, 78, 0) | None | Stationary Sentry (Equipped: Shotgun (#13)) | Faction 1 AI Tactics: Tactic #1 (Seek & Destroy -> Faction 3: Yellow A) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 20 HP | (43, 78, 0) | None | Stationary Sentry (Equipped: Pistol (#14)) | Faction 1 AI Tactics: Tactic #1 (Seek & Destroy -> Faction 3: Yellow A) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 20 HP | (44, 78, 0) | None | Stationary Sentry (Equipped: Persuadertron (#15)) | Faction 1 AI Tactics: Tactic #1 (Seek & Destroy -> Faction 3: Yellow A) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 20 HP | (44, 78, 0) | None | Stationary Sentry (Equipped: Laser (#17)) | Faction 1 AI Tactics: Tactic #1 (Seek & Destroy -> Faction 3: Yellow A) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 20 HP | (45, 78, 0) | None | Stationary Sentry (Equipped: Flamer (#18)) | Faction 1 AI Tactics: Tactic #1 (Seek & Destroy -> Faction 3: Yellow A) |
| Ped #15 | Enemy Agent (Faction 1: Blue) | 20 HP | (45, 78, 0) | None | Stationary Sentry (Equipped: Long Range Rifle (#19)) | Faction 1 AI Tactics: Tactic #1 (Seek & Destroy -> Faction 3: Yellow A) |
| Ped #16 | Police Officer | 6 HP (Default) | (35, 34, 0) | None | Walking (Equipped: Uzi (#22)) | None (Wandering) |
| Ped #17 | Police Officer | 6 HP (Default) | (41, 81, 0) | None | Walking (Equipped: Uzi (#23)) | None (Wandering) |
| Ped #18 | Police Officer | 6 HP (Default) | (31, 41, 0) | None | Walking (Equipped: Uzi (#24)) | None (Wandering) |
| Ped #19 | Police Officer | 6 HP (Default) | (74, 54, 0) | None | Walking (Equipped: Uzi (#25)) | None (Wandering) |
| Ped #20 | Police Officer | 6 HP (Default) | (73, 54, 0) | None | Walking (Equipped: Uzi (#26)) | None (Wandering) |
| Ped #21 | Police Officer | 6 HP (Default) | (41, 33, 0) | None | Walking (Equipped: Uzi (#27)) | None (Wandering) |
| Ped #22 | Police Officer | 6 HP (Default) | (41, 31, 0) | None | Walking (Equipped: Uzi (#28)) | None (Wandering) |
| Ped #23 | Police Officer | 6 HP (Default) | (37, 60, 1) | None | Walking (Equipped: Uzi (#29)) | None (Wandering) |
| Ped #24 | Police Officer | 6 HP (Default) | (67, 59, 0) | None | Walking (Equipped: Uzi (#30)) | None (Wandering) |
| Ped #26 | Police Officer | 6 HP (Default) | (35, 23, 0) | None | Walking (Equipped: Uzi (#32)) | None (Wandering) |
| Ped #27 | Police Officer | 6 HP (Default) | (41, 49, 0) | None | Walking (Equipped: Uzi (#33)) | None (Wandering) |
| Ped #28 | Police Officer | 6 HP (Default) | (62, 37, 0) | None | Walking (Equipped: Uzi (#34)) | None (Wandering) |
| Ped #29 | Police Officer | 6 HP (Default) | (67, 64, 0) | None | Walking (Equipped: Uzi (#35)) | None (Wandering) |
| Ped #30 | Police Officer | 6 HP (Default) | (46, 54, 0) | None | Walking (Equipped: Uzi (#36)) | None (Wandering) |
| Ped #31 | Police Officer | 6 HP (Default) | (37, 49, 4) | None | Walking (Equipped: Uzi (#37)) | None (Wandering) |
| Ped #40 | Civilian | 2 HP (Default) | (34, 51, 0) | Car #7 (Ambulance) | In Vehicle (Car #7 (Ambulance)) (Unarmed) | Enter vehicle #7; Drive vehicle #7 to (83, 41, 2); Drive vehicle #7 to (32, 37, 2) (+1 more) |
| Ped #50 | Police Officer | 6 HP (Default) | (61, 51, 0) | Car #5 (Police Cruiser) | In Vehicle (Car #5 (Police Cruiser)) (Equipped: Minigun (#68)) | Enter vehicle #5; Drive vehicle #5 to (57, 46, 12) |
| Ped #70 | Guard | 8 HP | (92, 84, 7) | None | Standing (Equipped: Uzi (#3), Gauss Gun (#70)) | Walk to position (86, 74, 2) |
| Ped #71 | Guard | 8 HP | (75, 82, 0) | None | Standing (Equipped: Minigun (#1)) | Walk to position (75, 82, 7); Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (81, 70, 2) |
| Ped #72 | Guard | 8 HP | (92, 85, 0) | None | Standing (Equipped: Minigun (#5)) | Walk to position (92, 85, 2); Proximity Ambush Trigger at (82, 67, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (84, 73, 2) |
| Ped #73 | Guard | 8 HP | (97, 80, 0) | None | Standing (Equipped: Minigun (#16)) | Walk to position (97, 80, 2); Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (87, 70, 2) |
| Ped #74 | Guard | 8 HP | (73, 60, 0) | None | Standing (Equipped: Uzi (#20)) | Walk to position (73, 60, 9); Proximity Ambush Trigger at (82, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 57, 2) |
| Ped #75 | Guard | 8 HP | (73, 60, 0) | None | Standing (Equipped: Uzi (#21)) | Walk to position (73, 60, 9); Proximity Ambush Trigger at (82, 55, 2) [Triggers when player agent enters 6-tile radius]; Timed Pause / Delay (10,000ms) (+1 more) |
| Ped #76 | Guard | 8 HP | (73, 60, 0) | None | Standing (Equipped: Uzi (#43)) | Walk to position (73, 60, 9); Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 61, 2) |
| Ped #77 | Guard | 8 HP | (73, 60, 0) | None | Standing (Equipped: Uzi (#44)) | Walk to position (73, 60, 9); Proximity Ambush Trigger at (82, 55, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 61, 2) |
| Ped #78 | Guard | 8 HP | (73, 60, 0) | None | Standing (Equipped: Uzi (#45)) | Walk to position (73, 60, 9); Proximity Ambush Trigger at (82, 67, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (78, 66, 2) |
| Ped #79 | Guard | 8 HP | (73, 60, 0) | None | Standing (Equipped: Minigun (#46)) | Walk to position (73, 60, 9); Proximity Ambush Trigger at (82, 55, 2) [Triggers when player agent enters 6-tile radius]; Timed Pause / Delay (50,000ms) (+1 more) |
| Ped #80 | Guard | 8 HP | (76, 70, 0) | None | Standing (Equipped: Uzi (#47)) | Walk to position (76, 70, 8); Proximity Ambush Trigger at (82, 62, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 65, 2) |
| Ped #81 | Guard | 8 HP | (75, 70, 0) | None | Standing (Equipped: Uzi (#48)) | Walk to position (75, 70, 8); Proximity Ambush Trigger at (82, 62, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 64, 2) |
| Ped #82 | Guard | 8 HP | (74, 70, 0) | None | Standing (Equipped: Uzi (#49)) | Walk to position (74, 70, 8); Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 66, 2) |
| Ped #83 | Guard | 8 HP | (73, 70, 0) | None | Standing (Equipped: Uzi (#50)) | Walk to position (73, 70, 8); Proximity Ambush Trigger at (82, 60, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 64, 2) |
| Ped #84 | Guard | 8 HP | (73, 70, 0) | None | Standing (Equipped: Uzi (#51)) | Walk to position (73, 70, 8); Proximity Ambush Trigger at (82, 64, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 66, 2) |
| Ped #85 | Guard | 8 HP | (72, 70, 0) | None | Standing (Equipped: Uzi (#52)) | Walk to position (72, 70, 8); Proximity Ambush Trigger at (82, 65, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 68, 2) |
| Ped #86 | Guard | 8 HP | (71, 70, 0) | None | Standing (Equipped: Uzi (#53)) | Walk to position (71, 70, 8); Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 68, 2) |
| Ped #87 | Guard | 8 HP | (76, 70, 0) | None | Standing (Equipped: Uzi (#54)) | Walk to position (76, 70, 8); Proximity Ambush Trigger at (82, 60, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (80, 63, 2) |
| Ped #88 | Guard | 8 HP | (81, 56, 0) | None | Standing (Equipped: Uzi (#55)) | None (Stationary) |
| Ped #91 | Civilian | 2 HP (Default) | (51, 22, 0) | Car #2 (Civilian Sedan) | In Vehicle (Car #2 (Civilian Sedan)) (Unarmed) | Enter vehicle #2; Drive vehicle #2 to (74, 48, 2); Drive vehicle #2 to (57, 53, 2) (+1 more) |
| Ped #93 | Police Officer | 6 HP (Default) | (37, 18, 0) | Car #4 (Police Cruiser) | In Vehicle (Car #4 (Police Cruiser)) (Equipped: Minigun (#67)) | Enter vehicle #4; Leave / Alight from Vehicle |
| Ped #94 | Civilian | 2 HP (Default) | (48, 53, 0) | Car #6 (Civilian Sedan) | In Vehicle (Car #6 (Civilian Sedan)) (Unarmed) | Enter vehicle #6; Drive vehicle #6 to (64, 58, 2); Drive vehicle #6 to (63, 34, 2) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 8`, `max_rush_factions = 0`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (8 agents: Peds #8–#15)

* **Idle Movement State**: 8 Stationary Sentry
* **Assigned Root Tactic**: Tactic #1 *(Wildcard (Generic Pool - Sequential Claim))*
* **Execution Chain**:
  1. **Tactic #1** [Wildcard / Seek & Destroy]: Target: Faction 3: Yellow A
  2. **Tactic #2** [Wildcard / Proximity Tripwire]: Watch Player Squad within 12 tiles (each sentry) OR 50 stages (~67.5s timer)
  3. **Tactic #3** [Wildcard / Weapon Ambush]: Ambush hold range: equipped weapon range (advances on engagement)
  4. **Tactic #0** [Wildcard / Objective Sweep]: Target: Player Squad

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |
| #1 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #2 | Active |
| #2 | Child (after Tactic #1) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 50 stages (~67.5s timer) | Tactic #3 | Active |
| #3 | Child (after Tactic #2) | Wildcard (Global) | Weapon Ambush | Ambush hold range: equipped weapon range (advances on engagement) | Tactic #0 (Fallback) | Active |
| #4 | Root (Active at Start) | Wildcard (Global) | Delay Timer | Wait 23680 ticks (2368.0s) then advance | Tactic #5 | Active |
| #5 | Child (after Tactic #4) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #6 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Player Squad | Tactic #7 | Active |
| #7 | Child (after Tactic #6) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #8 | Active |
| #8 | Child (after Tactic #7) | Wildcard (Global) | Seek & Destroy | Target: Player Squad | Tactic #9 | Active |
| #9 | Child (after Tactic #8) | Wildcard (Global) | Rival Sweep | Target: Player Squad | Tactic #10 | Active |
| #10 | Child (after Tactic #9) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #11 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #12 | Active |
| #12 | Child (after Tactic #11) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 50 stages (~67.5s timer) | Tactic #13 | Active |
| #13 | Child (after Tactic #12) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #14 | Active |
| #14 | Child (after Tactic #13) | Wildcard (Global) | Rival Sweep | Target: Player Squad | Tactic #15 | Active |
| #15 | Child (after Tactic #14) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #16 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #17 | Active |
| #17 | Child (after Tactic #16) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 100 stages (~135.0s timer) | Tactic #18 | Active |
| #18 | Child (after Tactic #17) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #19 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #20 | Active |
| #20 | Child (after Tactic #19) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #21 | Root (Active at Start) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #40 (Civilian)

* **Script Entry**: Scenario Script #75

**Scenario Chain**:
1. Enter vehicle #7
1. Drive vehicle #7 to (83, 41, 2)
1. Drive vehicle #7 to (32, 37, 2)
1. Drive vehicle #7 to (63, 26, 2)

### Ped #50 (Police Officer)

* **Script Entry**: Scenario Script #70

**Scenario Chain**:
1. Enter vehicle #5
1. Drive vehicle #5 to (57, 46, 12)

### Ped #70 (Guard)

* **Script Entry**: Scenario Script #54

**Scenario Chain**:
1. Walk to position (86, 74, 2)

### Ped #71 (Guard)

* **Script Entry**: Scenario Script #1

**Scenario Chain**:
1. Walk to position (75, 82, 7)
1. Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (81, 70, 2)

### Ped #72 (Guard)

* **Script Entry**: Scenario Script #51

**Scenario Chain**:
1. Walk to position (92, 85, 2)
1. Proximity Ambush Trigger at (82, 67, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (84, 73, 2)

### Ped #73 (Guard)

* **Script Entry**: Scenario Script #4

**Scenario Chain**:
1. Walk to position (97, 80, 2)
1. Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (87, 70, 2)

### Ped #74 (Guard)

* **Script Entry**: Scenario Script #7

**Scenario Chain**:
1. Walk to position (73, 60, 9)
1. Proximity Ambush Trigger at (82, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 57, 2)

### Ped #75 (Guard)

* **Script Entry**: Scenario Script #10

**Scenario Chain**:
1. Walk to position (73, 60, 9)
1. Proximity Ambush Trigger at (82, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Timed Pause / Delay (10,000ms)
1. Walk to position (80, 60, 2)

### Ped #76 (Guard)

* **Script Entry**: Scenario Script #14

**Scenario Chain**:
1. Walk to position (73, 60, 9)
1. Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 61, 2)

### Ped #77 (Guard)

* **Script Entry**: Scenario Script #20

**Scenario Chain**:
1. Walk to position (73, 60, 9)
1. Proximity Ambush Trigger at (82, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 61, 2)

### Ped #78 (Guard)

* **Script Entry**: Scenario Script #17

**Scenario Chain**:
1. Walk to position (73, 60, 9)
1. Proximity Ambush Trigger at (82, 67, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (78, 66, 2)

### Ped #79 (Guard)

* **Script Entry**: Scenario Script #23

**Scenario Chain**:
1. Walk to position (73, 60, 9)
1. Proximity Ambush Trigger at (82, 55, 2) [Triggers when player agent enters 6-tile radius]
1. Timed Pause / Delay (50,000ms)
1. Walk to position (80, 62, 2)

### Ped #80 (Guard)

* **Script Entry**: Scenario Script #30

**Scenario Chain**:
1. Walk to position (76, 70, 8)
1. Proximity Ambush Trigger at (82, 62, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 65, 2)

### Ped #81 (Guard)

* **Script Entry**: Scenario Script #33

**Scenario Chain**:
1. Walk to position (75, 70, 8)
1. Proximity Ambush Trigger at (82, 62, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 64, 2)

### Ped #82 (Guard)

* **Script Entry**: Scenario Script #36

**Scenario Chain**:
1. Walk to position (74, 70, 8)
1. Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 66, 2)

### Ped #83 (Guard)

* **Script Entry**: Scenario Script #39

**Scenario Chain**:
1. Walk to position (73, 70, 8)
1. Proximity Ambush Trigger at (82, 60, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 64, 2)

### Ped #84 (Guard)

* **Script Entry**: Scenario Script #42

**Scenario Chain**:
1. Walk to position (73, 70, 8)
1. Proximity Ambush Trigger at (82, 64, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 66, 2)

### Ped #85 (Guard)

* **Script Entry**: Scenario Script #45

**Scenario Chain**:
1. Walk to position (72, 70, 8)
1. Proximity Ambush Trigger at (82, 65, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 68, 2)

### Ped #86 (Guard)

* **Script Entry**: Scenario Script #48

**Scenario Chain**:
1. Walk to position (71, 70, 8)
1. Proximity Ambush Trigger at (82, 66, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 68, 2)

### Ped #87 (Guard)

* **Script Entry**: Scenario Script #27

**Scenario Chain**:
1. Walk to position (76, 70, 8)
1. Proximity Ambush Trigger at (82, 60, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (80, 63, 2)

### Ped #91 (Civilian)

* **Script Entry**: Scenario Script #58

**Scenario Chain**:
1. Enter vehicle #2
1. Drive vehicle #2 to (74, 48, 2)
1. Drive vehicle #2 to (57, 53, 2)
1. Drive vehicle #2 to (39, 20, 2)

### Ped #93 (Police Officer)

* **Script Entry**: Scenario Script #68

**Scenario Chain**:
1. Enter vehicle #4
1. Leave / Alight from Vehicle

### Ped #94 (Civilian)

* **Script Entry**: Scenario Script #72

**Scenario Chain**:
1. Enter vehicle #6
1. Drive vehicle #6 to (64, 58, 2)
1. Drive vehicle #6 to (63, 34, 2)

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Large Armored Car | (88, 64, 0) | 640 | None |
| Car #1 | Civilian Sedan | (43, 80, 0) | 1024 | None |
| Car #2 | Civilian Sedan | (51, 22, 0) | 1024 | Ped #91 |
| Car #3 | Small Armored Transport | (58, 17, 0) | 768 | None |
| Car #4 | Police Cruiser | (37, 18, 0) | 1280 | Ped #93 |
| Car #5 | Police Cruiser | (61, 51, 0) | 1280 | Ped #50 |
| Car #6 | Civilian Sedan | (48, 53, 0) | 1024 | Ped #94 |
| Car #7 | Ambulance | (34, 51, 0) | 1150 | Ped #40 |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 1 | Total Inventory Items: 52

#### Ground Pickups:

| Weapon Index | Weapon Subtype | Ground Tile (X, Y, Z) |
| :--- | :--- | :--- |
| Weapon #56 | Laser | (84, 84, 0) |

---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (47, 19, 0) and establish perimeter control.
* **Enemy Composition**: 8 enemy syndicate agents across 1 active faction(s) plus 39 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
