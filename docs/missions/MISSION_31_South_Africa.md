# Mission 31: South Africa

**Country**: South Africa (`#CNTRY_31`)

**Map ID**: Map 93 (`map93.dat`)

**Map Tile Bounding Box**: Tile X [0 – 100], Tile Y [8 – 75]

**Total Entities**: 57 Active Peds (4 Player Agents, 53 Key/Scripted NPCs, 0 Generic Civilians), 4 Vehicles, 56 Weapons.

---

## 1. Briefing Summary

EVERY FORMATION A PARADE.

ASSASSINATION.

A SEVERE INFORMATION LEAK FROM A MOLE IN OUR INTELLIGENCE DEPARTMENT HAS FOUND ITS WAY TO A LOCAL OFFICIAL IN CHARGE OF THE MILITARY ACTIVITIES IN THE AREA. SINCE THEN THE MOLE HAS BEEN DEALT WITH BUT THAT STILL LEAVES THE PROBLEM OF THIS INFORMATION FLOATING AROUND.

WE HAVE LEARNED THAT THE SAME OFFICIAL IS TO ATTEND AN INSPECTION AT THE LOCAL BARRACKS. SECURITY AROUND THE BARRACKS IS TOO HIGH FOR US TO DROP YOU INTO THE COMPLEX ITSELF SO YOUR SQUAD WILL BE DEPLOYED SOMEWHERE AROUND THE PERIMETER AND YOU WILL HAVE TO FIND YOUR OWN WAY INSIDE.

ONCE INSIDE THE OFFICIAL MUST BE ASSASSINATED.

**TARGET UPDATE:**

THE TARGET IS BEING GREETED BY THE BASE COMMANDER AT THE NORTH SIDE TRANSPORT DECK. AS HE HAS JUST ARRIVED BY CAR HE MAY ATTEMPT TO LEAVE IF HE SEES YOU.

**DEFENCE UPDATE:**

BEING A MILITARY INSTALLATION YOU CAN EXPECT HEAVY DEFENCES. MOST OF THE FOOT SOLDIERS ARE EQUIPPED WITH LIGHT AUTOMATICS BUT THERE ARE ALSO SOME HEAVY WEAPONS SPECIALISTS. IT IS NOT KNOWN EXACTLY HOW MANY MEN ARE PRESENT AT THIS TIME BUT THE FIGURE IS DEFINITELY IN THE THIRTIES.

**TACTICS:**

THE FIRST TASK WILL BE GETTING INTO THE COMPLEX ITSELF. SOME FORM OF TRANSPORT IS DEFINITELY REQUIRED. ONCE THERE KEEP YOUR TEAM TIGHT AND KEEP YOUR EYES OPEN FOR AMBUSHES. CONCENTRATE FIRE ON THE HEAVY WEAPONS SPECIALISTS AS THEY WILL PROVIDE THE MOST DANGER TO THE SQUAD. ONCE THE TARGET IS ELIMINATED AN ARMOURED TRANSPORT WILL ARRIVE TO TAKE YOU TO THE PICKUP POINT. IT WILL NOT HANG AROUND SO YOU HAVE TO BE ON YOUR TOES.

---

## 2. Mission Objectives

### Objective #1: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #12 (Civilian) at Tile (64, 17, 3)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (5, 13, 4)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (16, 77, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (17, 76, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (16, 75, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (15, 76, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP | (79, 62, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP | (82, 63, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP | (87, 75, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP | (88, 80, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #12 | Civilian | 8 HP | (64, 17, 3) | None | Standing (Unarmed) | Proximity Ambush Trigger at (63, 20, 5) [Triggers when player agent enters 6-tile radius]; Enter vehicle #1; Drive vehicle #1 to (120, 12, 6) (+1 more) |
| Ped #13 | Guard | 8 HP | (63, 29, 3) | None | Standing (Equipped: Uzi (#0)) | None (Stationary) |
| Ped #14 | Guard | 8 HP | (66, 29, 3) | None | Standing (Equipped: Uzi (#1)) | None (Stationary) |
| Ped #15 | Guard | 8 HP (Default) | (95, 22, 5) | None | Standing (Equipped: Uzi (#4)) | Proximity Ambush Trigger at (63, 22, 5) [Triggers when player agent enters 6-tile radius]; Walk to position (66, 20, 5); Reset / Loop Scripted Action Queue |
| Ped #16 | Guard | 8 HP (Default) | (96, 22, 5) | None | Standing (Equipped: Uzi (#5)) | Proximity Ambush Trigger at (64, 27, 5) [Triggers when player agent enters 6-tile radius]; Walk to position (66, 20, 5); Reset / Loop Scripted Action Queue |
| Ped #17 | Guard | 8 HP (Default) | (70, 34, 0) | None | Standing (Equipped: Flamer (#6)) | Walk to position (70, 33, 5); Walk to position (70, 35, 5) |
| Ped #18 | Guard | 8 HP (Default) | (94, 22, 5) | None | Standing (Equipped: Uzi (#3)) | Proximity Ambush Trigger at (62, 29, 5) [Triggers when player agent enters 6-tile radius]; Walk to position (72, 22, 7); Reset / Loop Scripted Action Queue |
| Ped #19 | Guard | 8 HP (Default) | (30, 22, 5) | None | Standing (Equipped: Uzi (#7)) | Proximity Ambush Trigger at (63, 20, 5) [Triggers when player agent enters 6-tile radius]; Walk to position (62, 21, 5); Reset / Loop Scripted Action Queue |
| Ped #20 | Guard | 8 HP (Default) | (31, 22, 4) | None | Standing (Equipped: Uzi (#8)) | Proximity Ambush Trigger at (62, 19, 5) [Triggers when player agent enters 6-tile radius]; Walk to position (63, 22, 5); Reset / Loop Scripted Action Queue |
| Ped #21 | Guard | 8 HP (Default) | (32, 22, 5) | None | Standing (Equipped: Uzi (#9)) | Proximity Ambush Trigger at (64, 27, 5) [Triggers when player agent enters 6-tile radius]; Walk to position (58, 22, 7); Reset / Loop Scripted Action Queue |
| Ped #22 | Guard | 8 HP (Default) | (33, 22, 5) | None | Standing (Equipped: Uzi (#10)) | Proximity Ambush Trigger at (63, 29, 5) [Triggers when player agent enters 6-tile radius]; Walk to position (57, 22, 7); Reset / Loop Scripted Action Queue |
| Ped #23 | Guard | 8 HP (Default) | (70, 32, 0) | None | Standing (Equipped: Uzi (#12)) | Walk to position (70, 32, 5); Proximity Ambush Trigger at (66, 36, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (68, 36, 2) (+1 more) |
| Ped #24 | Guard | 8 HP (Default) | (70, 30, 0) | None | Standing (Equipped: Minigun (#13)) | Walk to position (70, 30, 5); Proximity Ambush Trigger at (63, 29, 5) [Triggers when player agent enters 6-tile radius]; Walk to position (64, 32, 5) (+1 more) |
| Ped #25 | Guard | 8 HP (Default) | (70, 34, 0) | None | Standing (Equipped: Uzi (#14)) | Walk to position (70, 34, 5) |
| Ped #26 | Guard | 8 HP (Default) | (70, 29, 0) | None | Standing (Equipped: Uzi (#15)) | Walk to position (70, 29, 5); Proximity Ambush Trigger at (64, 36, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (67, 36, 2) (+1 more) |
| Ped #27 | Guard | 8 HP (Default) | (42, 30, 0) | None | Standing (Equipped: Uzi (#16)) | Walk to position (42, 30, 5) |
| Ped #28 | Guard | 8 HP (Default) | (50, 30, 0) | None | Standing (Equipped: Uzi (#17)) | Walk to position (50, 30, 5) |
| Ped #29 | Guard | 8 HP (Default) | (59, 35, 0) | None | Standing (Equipped: Minigun (#18)) | Walk to position (59, 35, 5); Proximity Ambush Trigger at (63, 36, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (61, 36, 2) (+1 more) |
| Ped #30 | Guard | 8 HP (Default) | (50, 35, 0) | None | Standing (Equipped: Flamer (#19)) | Walk to position (50, 35, 5) |
| Ped #31 | Guard | 8 HP (Default) | (39, 40, 0) | None | Standing (Equipped: Uzi (#20)) | Walk to position (39, 40, 5) |
| Ped #32 | Guard | 8 HP (Default) | (55, 40, 0) | None | Standing (Equipped: Uzi (#21)) | Walk to position (55, 40, 5) |
| Ped #33 | Guard | 8 HP (Default) | (42, 39, 0) | None | Standing (Equipped: Uzi (#22)) | Walk to position (42, 39, 5) |
| Ped #34 | Guard | 8 HP (Default) | (52, 40, 0) | None | Standing (Equipped: Long Range Rifle (#23)) | Walk to position (52, 40, 5) |
| Ped #35 | Guard | 8 HP (Default) | (40, 35, 0) | None | Standing (Equipped: Long Range Rifle (#24)) | Walk to position (40, 35, 5) |
| Ped #36 | Guard | 8 HP (Default) | (41, 29, 0) | None | Standing (Equipped: Long Range Rifle (#25)) | Walk to position (41, 29, 5) |
| Ped #37 | Guard | 8 HP (Default) | (50, 30, 0) | None | Standing (Equipped: Flamer (#26)) | Walk to position (50, 30, 5) |
| Ped #38 | Guard | 8 HP (Default) | (50, 35, 0) | None | Standing (Equipped: Minigun (#27)) | Walk to position (50, 35, 5) |
| Ped #39 | Guard | 8 HP (Default) | (80, 35, 0) | None | Standing (Equipped: Minigun (#28)) | Walk to position (80, 35, 5) |
| Ped #40 | Guard | 8 HP (Default) | (87, 35, 0) | None | Standing (Equipped: Minigun (#29)) | Walk to position (87, 35, 5) |
| Ped #41 | Guard | 8 HP (Default) | (90, 39, 0) | None | Standing (Equipped: Flamer (#30)) | Walk to position (90, 39, 5) |
| Ped #42 | Guard | 8 HP (Default) | (82, 29, 0) | None | Standing (Equipped: Long Range Rifle (#31)) | Walk to position (82, 29, 5) |
| Ped #43 | Guard | 8 HP (Default) | (81, 40, 0) | None | Standing (Equipped: Long Range Rifle (#32)) | Walk to position (81, 40, 5) |
| Ped #44 | Guard | 8 HP (Default) | (76, 39, 0) | None | Standing (Equipped: Long Range Rifle (#33)) | Walk to position (76, 39, 5) |
| Ped #45 | Guard | 8 HP (Default) | (90, 39, 0) | None | Standing (Equipped: Uzi (#34)) | Walk to position (90, 39, 5) |
| Ped #46 | Guard | 8 HP (Default) | (90, 30, 0) | None | Standing (Equipped: Uzi (#35)) | Walk to position (90, 30, 5) |
| Ped #47 | Guard | 8 HP (Default) | (89, 34, 0) | None | Standing (Equipped: Uzi (#36)) | Walk to position (89, 34, 5) |
| Ped #48 | Guard | 8 HP (Default) | (88, 30, 0) | None | Standing (Equipped: Uzi (#37)) | Walk to position (88, 30, 5) |
| Ped #49 | Guard | 8 HP (Default) | (80, 34, 0) | None | Standing (Equipped: Uzi (#38)) | Walk to position (80, 34, 5) |
| Ped #50 | Guard | 8 HP (Default) | (81, 29, 0) | None | Standing (Equipped: Uzi (#39)) | Walk to position (81, 29, 5) |
| Ped #51 | Guard | 16 HP | (63, 17, 3) | None | Standing (Equipped: Pistol (#40)) | None (Stationary) |
| Ped #52 | Guard | 16 HP | (61, 19, 3) | None | Standing (Equipped: Uzi (#41), Uzi (#42)) | None (Stationary) |
| Ped #53 | Guard | 16 HP | (61, 19, 3) | None | Standing (Equipped: Uzi (#45)) | None (Stationary) |
| Ped #54 | Guard | 16 HP | (61, 18, 3) | None | Standing (Equipped: Uzi (#43)) | None (Stationary) |
| Ped #55 | Guard | 16 HP | (61, 18, 3) | None | Standing (Equipped: Uzi (#44)) | None (Stationary) |
| Ped #56 | Guard | 16 HP | (61, 17, 3) | None | Standing (Equipped: Uzi (#46)) | None (Stationary) |
| Ped #57 | Guard | 16 HP | (61, 17, 3) | None | Standing (Equipped: Uzi (#47)) | None (Stationary) |
| Ped #58 | Civilian | 2 HP (Default) | (118, 12, 4) | Car #2 (Civilian Sedan) | In Vehicle (Car #2 (Civilian Sedan)) (Unarmed) | Enter vehicle #2; Proximity Ambush Trigger at (62, 19, 5) [Triggers when player agent enters 6-tile radius]; Drive vehicle #2 to (64, 12, 6) (+4 more) |
| Ped #59 | Guard | 8 HP | (61, 14, 4) | None | Standing (Equipped: Long Range Rifle (#49)) | None (Stationary) |
| Ped #60 | Guard | 8 HP | (4, 12, 4) | Car #3 (Large Armored Car) | In Vehicle (Car #3 (Large Armored Car)) (Equipped: Minigun (#66)) | Enter vehicle #3; Proximity Ambush Trigger at (64, 27, 5) [Triggers when player agent enters 6-tile radius]; Proximity Ambush Trigger at (63, 22, 5) [Triggers when player agent enters 6-tile radius] (+6 more) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 4`, `max_rush_factions = 0`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (4 agents: Peds #8–#11)

* **Idle Movement State**: 4 Stationary Sentry
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
| #3 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Player Squad | Tactic #4 | Active |
| #4 | Child (after Tactic #3) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 50 stages (~67.5s timer) | Tactic #5 | Active |
| #5 | Child (after Tactic #4) | Wildcard (Global) | Seek & Destroy | Target: Player Squad | Tactic #6 | Active |
| #6 | Child (after Tactic #5) | Wildcard (Global) | Rival Sweep | Target: Player Squad | Tactic #7 | Active |
| #7 | Child (after Tactic #6) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #14 | Active |
| #8 | Child (after Tactic #15) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #9 | Active |
| #9 | Child (after Tactic #8) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #10 | Child (after Tactic #16) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #11 | Active |
| #11 | Child (after Tactic #10) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #12 | Child (after Tactic #17) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer) | Tactic #13 | Active |
| #13 | Child (after Tactic #12) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #14 | Child (after Tactic #7) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #15 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Player Squad | Tactic #8 | Active |
| #16 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Player Squad | Tactic #10 | Active |
| #17 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Player Squad | Tactic #12 | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #12 (Civilian)

* **Script Entry**: Scenario Script #120

**Scenario Chain**:
1. Proximity Ambush Trigger at (63, 20, 5) [Triggers when player agent enters 6-tile radius]
1. Enter vehicle #1
1. Drive vehicle #1 to (120, 12, 6)
1. Target Escaped (Mission Failure Condition)

### Ped #15 (Guard)

* **Script Entry**: Scenario Script #22

**Scenario Chain**:
1. Proximity Ambush Trigger at (63, 22, 5) [Triggers when player agent enters 6-tile radius]
1. Walk to position (66, 20, 5)
1. Reset / Loop Scripted Action Queue

### Ped #16 (Guard)

* **Script Entry**: Scenario Script #25

**Scenario Chain**:
1. Proximity Ambush Trigger at (64, 27, 5) [Triggers when player agent enters 6-tile radius]
1. Walk to position (66, 20, 5)
1. Reset / Loop Scripted Action Queue

### Ped #17 (Guard)

* **Script Entry**: Scenario Script #29

**Scenario Chain**:
1. Walk to position (70, 33, 5)
1. Walk to position (70, 35, 5)

### Ped #18 (Guard)

* **Script Entry**: Scenario Script #19

**Scenario Chain**:
1. Proximity Ambush Trigger at (62, 29, 5) [Triggers when player agent enters 6-tile radius]
1. Walk to position (72, 22, 7)
1. Reset / Loop Scripted Action Queue

### Ped #19 (Guard)

* **Script Entry**: Scenario Script #16

**Scenario Chain**:
1. Proximity Ambush Trigger at (63, 20, 5) [Triggers when player agent enters 6-tile radius]
1. Walk to position (62, 21, 5)
1. Reset / Loop Scripted Action Queue

### Ped #20 (Guard)

* **Script Entry**: Scenario Script #13

**Scenario Chain**:
1. Proximity Ambush Trigger at (62, 19, 5) [Triggers when player agent enters 6-tile radius]
1. Walk to position (63, 22, 5)
1. Reset / Loop Scripted Action Queue

### Ped #21 (Guard)

* **Script Entry**: Scenario Script #10

**Scenario Chain**:
1. Proximity Ambush Trigger at (64, 27, 5) [Triggers when player agent enters 6-tile radius]
1. Walk to position (58, 22, 7)
1. Reset / Loop Scripted Action Queue

### Ped #22 (Guard)

* **Script Entry**: Scenario Script #7

**Scenario Chain**:
1. Proximity Ambush Trigger at (63, 29, 5) [Triggers when player agent enters 6-tile radius]
1. Walk to position (57, 22, 7)
1. Reset / Loop Scripted Action Queue

### Ped #23 (Guard)

* **Script Entry**: Scenario Script #35

**Scenario Chain**:
1. Walk to position (70, 32, 5)
1. Proximity Ambush Trigger at (66, 36, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (68, 36, 2)
1. Reset / Loop Scripted Action Queue

### Ped #24 (Guard)

* **Script Entry**: Scenario Script #39

**Scenario Chain**:
1. Walk to position (70, 30, 5)
1. Proximity Ambush Trigger at (63, 29, 5) [Triggers when player agent enters 6-tile radius]
1. Walk to position (64, 32, 5)
1. Reset / Loop Scripted Action Queue

### Ped #25 (Guard)

* **Script Entry**: Scenario Script #43

**Scenario Chain**:
1. Walk to position (70, 34, 5)

### Ped #26 (Guard)

* **Script Entry**: Scenario Script #30

**Scenario Chain**:
1. Walk to position (70, 29, 5)
1. Proximity Ambush Trigger at (64, 36, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (67, 36, 2)
1. Reset / Loop Scripted Action Queue

### Ped #27 (Guard)

* **Script Entry**: Scenario Script #50

**Scenario Chain**:
1. Walk to position (42, 30, 5)

### Ped #28 (Guard)

* **Script Entry**: Scenario Script #49

**Scenario Chain**:
1. Walk to position (50, 30, 5)

### Ped #29 (Guard)

* **Script Entry**: Scenario Script #45

**Scenario Chain**:
1. Walk to position (59, 35, 5)
1. Proximity Ambush Trigger at (63, 36, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (61, 36, 2)
1. Reset / Loop Scripted Action Queue

### Ped #30 (Guard)

* **Script Entry**: Scenario Script #44

**Scenario Chain**:
1. Walk to position (50, 35, 5)

### Ped #31 (Guard)

* **Script Entry**: Scenario Script #58

**Scenario Chain**:
1. Walk to position (39, 40, 5)

### Ped #32 (Guard)

* **Script Entry**: Scenario Script #57

**Scenario Chain**:
1. Walk to position (55, 40, 5)

### Ped #33 (Guard)

* **Script Entry**: Scenario Script #56

**Scenario Chain**:
1. Walk to position (42, 39, 5)

### Ped #34 (Guard)

* **Script Entry**: Scenario Script #55

**Scenario Chain**:
1. Walk to position (52, 40, 5)

### Ped #35 (Guard)

* **Script Entry**: Scenario Script #54

**Scenario Chain**:
1. Walk to position (40, 35, 5)

### Ped #36 (Guard)

* **Script Entry**: Scenario Script #53

**Scenario Chain**:
1. Walk to position (41, 29, 5)

### Ped #37 (Guard)

* **Script Entry**: Scenario Script #52

**Scenario Chain**:
1. Walk to position (50, 30, 5)

### Ped #38 (Guard)

* **Script Entry**: Scenario Script #51

**Scenario Chain**:
1. Walk to position (50, 35, 5)

### Ped #39 (Guard)

* **Script Entry**: Scenario Script #63

**Scenario Chain**:
1. Walk to position (80, 35, 5)

### Ped #40 (Guard)

* **Script Entry**: Scenario Script #64

**Scenario Chain**:
1. Walk to position (87, 35, 5)

### Ped #41 (Guard)

* **Script Entry**: Scenario Script #66

**Scenario Chain**:
1. Walk to position (90, 39, 5)

### Ped #42 (Guard)

* **Script Entry**: Scenario Script #65

**Scenario Chain**:
1. Walk to position (82, 29, 5)

### Ped #43 (Guard)

* **Script Entry**: Scenario Script #67

**Scenario Chain**:
1. Walk to position (81, 40, 5)

### Ped #44 (Guard)

* **Script Entry**: Scenario Script #68

**Scenario Chain**:
1. Walk to position (76, 39, 5)

### Ped #45 (Guard)

* **Script Entry**: Scenario Script #69

**Scenario Chain**:
1. Walk to position (90, 39, 5)

### Ped #46 (Guard)

* **Script Entry**: Scenario Script #70

**Scenario Chain**:
1. Walk to position (90, 30, 5)

### Ped #47 (Guard)

* **Script Entry**: Scenario Script #62

**Scenario Chain**:
1. Walk to position (89, 34, 5)

### Ped #48 (Guard)

* **Script Entry**: Scenario Script #61

**Scenario Chain**:
1. Walk to position (88, 30, 5)

### Ped #49 (Guard)

* **Script Entry**: Scenario Script #60

**Scenario Chain**:
1. Walk to position (80, 34, 5)

### Ped #50 (Guard)

* **Script Entry**: Scenario Script #59

**Scenario Chain**:
1. Walk to position (81, 29, 5)

### Ped #58 (Civilian)

* **Script Entry**: Scenario Script #166

**Scenario Chain**:
1. Enter vehicle #2
1. Proximity Ambush Trigger at (62, 19, 5) [Triggers when player agent enters 6-tile radius]
1. Drive vehicle #2 to (64, 12, 6)
1. Timed Pause / Delay (10,000ms)
1. Drive vehicle #2 to (4, 12, 6)
1. Timed Pause / Delay (50,000ms)
1. Target Escaped (Mission Failure Condition)

### Ped #60 (Guard)

* **Script Entry**: Scenario Script #124

**Scenario Chain**:
1. Enter vehicle #3
1. Proximity Ambush Trigger at (64, 27, 5) [Triggers when player agent enters 6-tile radius]
1. Proximity Ambush Trigger at (63, 22, 5) [Triggers when player agent enters 6-tile radius]
1. Proximity Ambush Trigger at (63, 22, 5) [Triggers when player agent enters 6-tile radius]
1. Proximity Ambush Trigger at (63, 22, 5) [Triggers when player agent enters 6-tile radius]
1. Drive vehicle #3 to (63, 14, 6)
1. Stand Guard / Hold Position (legacy marker)
1. Drive vehicle #3 to (63, 15, 6)
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Large Armored Car | (104, 63, 0) | 640 | None |
| Car #1 | Civilian Sedan | (66, 14, 4) | 1024 | None |
| Car #2 | Civilian Sedan | (118, 12, 4) | 1024 | Ped #58 |
| Car #3 | Large Armored Car | (4, 12, 4) | 640 | Ped #60 |

### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 1 | Total Inventory Items: 55

#### Ground Pickups:

| Weapon Index | Weapon Subtype | Ground Tile (X, Y, Z) |
| :--- | :--- | :--- |
| Weapon #11 | Flamer | (80, 59, 0) |

---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (16, 77, 0) and establish perimeter control.
* **Enemy Composition**: 4 enemy syndicate agents across 1 active faction(s) plus 49 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
