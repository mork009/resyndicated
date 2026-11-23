# Mission 36: Venezuela

**Country**: Venezuela (`#CNTRY_36`)

**Map ID**: Map 31 (`map31.dat`)

**Map Tile Bounding Box**: Tile X [0 – 101], Tile Y [8 – 77]

**Total Entities**: 165 Active Peds (4 Player Agents, 45 Key/Scripted NPCs, 116 Generic Civilians), 0 Vehicles, 36 Weapons.

---

## 1. Briefing Summary

BEHIND THE WALL.

ASSASSINATION.

ONE OF OUR EARLY MODEL AGENTS HAS TURNED RENEGADE AFTER TOO MUCH ADRENALINE ABUSE. SHE HAS BEEN PICKED UP BY THE POLICE FORCE AND IS CURRENTLY UNDERGOING INTERROGATION IN A SAFE HOUSE.

AS SHE USED TO WORK FOR US SHE HAS VITAL INFORMATION ON OUR TACTICS AND WEAPONS RESEARCH PROGRAMS. OBVIOUSLY THIS WOULD BE DISASTROUS. SHE MUST BE TERMINATED BEFORE SHE IS ABLE TO DIVULGE THIS INFORMATION.

**TARGET UPDATE:**

THE SAFE HOUSE IS LOCATED IN THE MIDDLE SECTOR OF THE CITY. OUR PRELIMINARY SCOUT REPORTS SHOW THAT IT IS FLANKED BY TWO LARGER BUILDINGS AND THAT THE ONLY WAY INTO THE ENTRANCE COURTYARD IS BY TRAVELING OVER THE ROOFTOPS AND DOWN THE ACCESS RAMP.

**DEFENCE UPDATE:**

AS A CAPTURED SYNDICATE AGENT IS NOT AN EVERYDAY HAPPENING YOU CAN EXPECT THERE TO BE SEVERAL WELL EQUIPPED GUARDS STATIONED AROUND THE TARGET. EVEN SO THEY SHOULD STILL BE NO MATCH FOR A WELL EQUIPPED ASSAULT SQUAD.

**SYNDICATE UPDATE:**

WITHOUT ANY DOUBT THIS AGENT WILL ALSO BE OF GREAT VALUE TO ENEMY SYNDICATES AND YOU CAN EXPECT SEVERAL ENEMY SQUADS TO BE ACTIVE IN THE SURROUNDING AREA. THEY MUST NOT BE ALLOWED TO GET THEIR HANDS ON THE TARGET. YOU MUST NOT ALLOW THEM CLOSE ENOUGH TO <PERSUADE> THE TARGET AS THEY WILL THEN HAVE ACCESS TO HER HIGHER MEMORY FUNCTIONS.

**TACTICS:**

A SMALL SQUAD OF ONE OR TWO AGENTS SHOULD PROCEED WITH ALL HASTE TO THE SAFE HOUSE AND SECURE THE ROOF TOPS. WHEN THIS IS ACHIEVED THEN ANOTHER AGENT CAN ENTER THE COURTYARD AND SAFE HOUSE TO CARRY OUT THE MISSION. THE OTHER TWO AGENTS SHOULD PROVIDE COVER FIRE WHERE NECESSARY AND ABOVE ALL PREVENT THE ENEMY SYNDICATE SQUADS FROM REACHING THE HOUSE. LONG RANGE WEAPONRY SHOULD BE EMPLOYED BY THE ROOFTOP DEFENDERS AND LIGHT AUTOMATICS SHOULD TAKE CARE OF THE GUARDS AND TARGET.

---

## 2. Mission Objectives

### Objective #1: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #16 (Civilian) at Tile (63, 69, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (10, 82, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (10, 82, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (10, 81, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (9, 81, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP | (80, 34, 3) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Evacuate) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP | (42, 27, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Evacuate) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP | (86, 29, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Evacuate) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP | (72, 29, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Evacuate) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 16 HP | (80, 23, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Evacuate) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 16 HP | (86, 39, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Evacuate) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 16 HP | (67, 36, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Evacuate) |
| Ped #15 | Enemy Agent (Faction 1: Blue) | 16 HP | (53, 24, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Evacuate) |
| Ped #16 | Civilian | 2 HP (Default) | (63, 69, 0) | None | Standing (Unarmed) | Walk to position (63, 69, 8) |
| Ped #17 | Guard | 8 HP | (62, 68, 0) | None | Standing (Equipped: Uzi (#0)) | Walk to position (62, 68, 7); Proximity Ambush Trigger at (67, 72, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (63, 71, 2) (+1 more) |
| Ped #18 | Guard | 8 HP | (64, 69, 0) | None | Standing (Equipped: Uzi (#1)) | Walk to position (64, 69, 8); Proximity Ambush Trigger at (67, 72, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (63, 71, 2) (+1 more) |
| Ped #19 | Guard | 8 HP | (62, 68, 0) | None | Standing (Equipped: Uzi (#2)) | Walk to position (62, 68, 7); Proximity Ambush Trigger at (64, 71, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (62, 70, 2) (+1 more) |
| Ped #20 | Guard | 8 HP | (64, 69, 0) | None | Standing (Equipped: Uzi (#3)) | Walk to position (64, 69, 7); Proximity Ambush Trigger at (65, 71, 2) [Triggers when player agent enters 6-tile radius]; Walk to position (62, 71, 2) (+1 more) |
| Ped #21 | Guard | 8 HP | (64, 68, 0) | None | Standing (Equipped: Uzi (#4)) | Walk to position (64, 68, 7); Proximity Ambush Trigger at (66, 72, 2) [Triggers when player agent enters 6-tile radius]; Reset / Loop Scripted Action Queue |
| Ped #22 | Guard | 8 HP | (67, 67, 4) | None | Standing (Equipped: Long Range Rifle (#6)) | Proximity Ambush Trigger at (77, 69, 9) [Triggers when player agent enters 6-tile radius]; Walk to position (70, 67, 8); Reset / Loop Scripted Action Queue |
| Ped #23 | Police Officer | 6 HP (Default) | (27, 35, 0) | None | Walking (Equipped: Uzi (#5)) | None (Wandering) |
| Ped #24 | Police Officer | 6 HP (Default) | (97, 72, 0) | None | Walking (Equipped: Uzi (#7)) | None (Wandering) |
| Ped #25 | Police Officer | 6 HP (Default) | (73, 64, 0) | None | Walking (Equipped: Uzi (#8)) | None (Wandering) |
| Ped #26 | Police Officer | 6 HP (Default) | (49, 80, 0) | None | Walking (Equipped: Uzi (#9)) | None (Wandering) |
| Ped #27 | Police Officer | 6 HP (Default) | (112, 50, 1) | None | Walking (Equipped: Uzi (#10)) | None (Wandering) |
| Ped #28 | Police Officer | 6 HP (Default) | (60, 2, 0) | None | Walking (Equipped: Uzi (#11)) | None (Wandering) |
| Ped #29 | Police Officer | 6 HP (Default) | (5, 77, 0) | None | Walking (Equipped: Uzi (#12)) | None (Wandering) |
| Ped #30 | Police Officer | 6 HP (Default) | (66, 62, 0) | None | Walking (Equipped: Uzi (#13)) | None (Wandering) |
| Ped #31 | Police Officer | 6 HP (Default) | (117, 49, 0) | None | Walking (Equipped: Uzi (#14)) | None (Wandering) |
| Ped #32 | Police Officer | 6 HP (Default) | (34, 25, 0) | None | Walking (Equipped: Uzi (#15)) | None (Wandering) |
| Ped #33 | Police Officer | 6 HP (Default) | (72, 79, 0) | None | Walking (Equipped: Uzi (#16)) | None (Wandering) |
| Ped #34 | Police Officer | 6 HP (Default) | (67, 34, 0) | None | Walking (Equipped: Uzi (#17)) | None (Wandering) |
| Ped #35 | Police Officer | 6 HP (Default) | (112, 55, 0) | None | Walking (Equipped: Uzi (#18)) | None (Wandering) |
| Ped #36 | Police Officer | 6 HP (Default) | (116, 72, 0) | None | Walking (Equipped: Uzi (#19)) | None (Wandering) |
| Ped #37 | Police Officer | 6 HP (Default) | (45, 13, 0) | None | Walking (Equipped: Uzi (#20)) | None (Wandering) |
| Ped #38 | Police Officer | 6 HP (Default) | (82, 79, 5) | None | Walking (Equipped: Uzi (#21)) | None (Wandering) |
| Ped #39 | Police Officer | 6 HP (Default) | (5, 77, 0) | None | Walking (Equipped: Uzi (#22)) | None (Wandering) |
| Ped #40 | Police Officer | 6 HP (Default) | (44, 77, 5) | None | Walking (Equipped: Uzi (#23)) | None (Wandering) |
| Ped #41 | Police Officer | 6 HP (Default) | (49, 8, 0) | None | Walking (Equipped: Uzi (#24)) | None (Wandering) |
| Ped #42 | Police Officer | 6 HP (Default) | (40, 61, 0) | None | Walking (Equipped: Uzi (#25)) | None (Wandering) |
| Ped #43 | Police Officer | 6 HP (Default) | (78, 60, 0) | None | Walking (Equipped: Uzi (#26)) | None (Wandering) |
| Ped #44 | Police Officer | 6 HP (Default) | (47, 67, 5) | None | Walking (Equipped: Uzi (#27)) | None (Wandering) |
| Ped #45 | Police Officer | 6 HP (Default) | (9, 67, 0) | None | Walking (Equipped: Uzi (#28)) | None (Wandering) |
| Ped #46 | Police Officer | 6 HP (Default) | (55, 9, 0) | None | Walking (Equipped: Uzi (#29)) | None (Wandering) |
| Ped #47 | Police Officer | 6 HP (Default) | (114, 91, 0) | None | Walking (Equipped: Uzi (#30)) | None (Wandering) |
| Ped #48 | Police Officer | 6 HP (Default) | (102, 62, 0) | None | Walking (Equipped: Uzi (#31)) | None (Wandering) |
| Ped #49 | Police Officer | 6 HP (Default) | (75, 10, 0) | None | Walking (Equipped: Uzi (#32)) | None (Wandering) |
| Ped #50 | Police Officer | 6 HP (Default) | (27, 38, 0) | None | Walking (Equipped: Uzi (#33)) | None (Wandering) |
| Ped #51 | Police Officer | 6 HP (Default) | (37, 25, 0) | None | Walking (Equipped: Uzi (#34)) | None (Wandering) |
| Ped #52 | Police Officer | 6 HP (Default) | (104, 73, 0) | None | Walking (Equipped: Uzi (#35)) | None (Wandering) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 8`, `max_rush_factions = 9`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (8 agents: Peds #8–#15)

* **Idle Movement State**: 8 Stationary Sentry
* **Assigned Root Tactic**: Tactic #1 *(Wildcard (Generic Pool - Sequential Claim))*
* **Execution Chain**:
  1. **Tactic #1** [Wildcard / Evacuate]: Evacuation coordinates: (1474, 0, 0)
  2. **Tactic #2** [Wildcard / Objective Sweep]: Target: Player Squad

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |
| #1 | Root (Active at Start) | Wildcard (Global) | Evacuate | Evacuation coordinates: (1474, 0, 0) | Tactic #2 | Active |
| #2 | Child (after Tactic #1) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #16 (Civilian)

* **Script Entry**: Scenario Script #2

**Scenario Chain**:
1. Walk to position (63, 69, 8)

### Ped #17 (Guard)

* **Script Entry**: Scenario Script #3

**Scenario Chain**:
1. Walk to position (62, 68, 7)
1. Proximity Ambush Trigger at (67, 72, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (63, 71, 2)
1. Reset / Loop Scripted Action Queue

### Ped #18 (Guard)

* **Script Entry**: Scenario Script #7

**Scenario Chain**:
1. Walk to position (64, 69, 8)
1. Proximity Ambush Trigger at (67, 72, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (63, 71, 2)
1. Reset / Loop Scripted Action Queue

### Ped #19 (Guard)

* **Script Entry**: Scenario Script #11

**Scenario Chain**:
1. Walk to position (62, 68, 7)
1. Proximity Ambush Trigger at (64, 71, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (62, 70, 2)
1. Reset / Loop Scripted Action Queue

### Ped #20 (Guard)

* **Script Entry**: Scenario Script #15

**Scenario Chain**:
1. Walk to position (64, 69, 7)
1. Proximity Ambush Trigger at (65, 71, 2) [Triggers when player agent enters 6-tile radius]
1. Walk to position (62, 71, 2)
1. Reset / Loop Scripted Action Queue

### Ped #21 (Guard)

* **Script Entry**: Scenario Script #19

**Scenario Chain**:
1. Walk to position (64, 68, 7)
1. Proximity Ambush Trigger at (66, 72, 2) [Triggers when player agent enters 6-tile radius]
1. Reset / Loop Scripted Action Queue

### Ped #22 (Guard)

* **Script Entry**: Scenario Script #24

**Scenario Chain**:
1. Proximity Ambush Trigger at (77, 69, 9) [Triggers when player agent enters 6-tile radius]
1. Walk to position (70, 67, 8)
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

No vehicles placed in map level.


### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 36


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (10, 82, 0) and establish perimeter control.
* **Enemy Composition**: 8 enemy syndicate agents across 1 active faction(s) plus 37 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
