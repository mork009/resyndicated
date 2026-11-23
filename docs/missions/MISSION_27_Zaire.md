# Mission 27: Zaire

**Country**: Zaire (`#CNTRY_27`)

**Map ID**: Map 71 (`map71.dat`)

**Map Tile Bounding Box**: Tile X [0 – 100], Tile Y [8 – 76]

**Total Entities**: 189 Active Peds (4 Player Agents, 42 Key/Scripted NPCs, 143 Generic Civilians), 0 Vehicles, 35 Weapons.

---

## 1. Briefing Summary

THE POLICEMANS BALL.

PERSONNEL ACQUISITION.

THE CAPTAIN OF THE LOCAL POLICE FORCE IS PLANNING TO RETIRE AFTER A LONG AND GLORIOUS CAREER IN WHICH HE RARELY STRAYED FROM THE PATH OF JUSTICE UNLESS WE ASKED HIM TO. HIS REPLACEMENT IS REPUTED TO BE IN THE POCKET OF ANOTHER SYNDICATE AND WILL OBVIOUSLY NOT BE AS HELPFUL.

YOUR SQUAD IS TO <PERSUADE> THE NEW CAPTAIN AND BRING HIM BACK TO THE LANDING ZONE SO HE CAN BE TAKEN TO HEADQUARTERS FOR A CHAT.

**SYNDICATE UPDATE:**

THE ENEMY SYNDICATE HAS GOTTEN WIND OF THIS PLAN AND HAS DESPATCHED A DEFENCE SQUAD FOR THE PROTECTION OF THE CAPTAIN.

**TARGET UPDATE:**

THE TARGET IS IN THE COMPLEX IN THE NORTHWEST SECTOR OF THE CITY. HE MAY BE CONSIDERABLY HARDER TO <PERSUADE> THAN A NORMAL CIVILIAN DUE TO HIS POLICE BACKGROUND. WE ALSO BELIEVE HIM TO BE ARMED.

**TACTICS:**

ONE OR TWO AGENTS WITH LIGHT AUTOMATICS AND AT LEAST ONE PERSUADERTRON SHOULD HAVE NO TROUBLE WITH THE CAPTAIN. THE ENEMY SQUAD SHOULD TAKE MORE CONSIDERATION. IF YOU DEPLOY A FEW AGENTS THEN KEEP THEM TOGETHER FOR THE BEST DEFENCE AND LET THEM WATCH EACH OTHERS BACKS. IF YOU DEPLOY A LARGER AMOUNT THEN TRY TO LURE THE ENEMY SQUAD INTO AN AMBUSH SITUATION. DO NOT ENGAGE ANYBODY NEAR THE TARGET ZONE AS THIS MAY ENDANGER THE TARGET.

---

## 2. Mission Objectives

### Objective #1: Persuade Target

* **Type**: Persuade Target
* **Target**: Ped #16 (Guard) at Tile (9, 10, 0)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (101, 40, 7)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (100, 43, 7) | On Map | 16 HP |
| Agent #2 | Ped #1 | (100, 43, 7) | On Map | 16 HP |
| Agent #3 | Ped #2 | (100, 42, 7) | On Map | 16 HP |
| Agent #4 | Ped #3 | (100, 42, 7) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP | (9, 50, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP | (19, 51, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP | (21, 34, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP | (9, 71, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 16 HP | (8, 27, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 16 HP | (26, 23, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 16 HP | (33, 26, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #15 | Enemy Agent (Faction 1: Blue) | 16 HP | (36, 59, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Objective Sweep -> Player Squad) |
| Ped #16 | Guard | 8 HP (Default) | (9, 10, 0) | None | Standing (Equipped: Uzi (#34), Flamer (#35)) | Proximity Ambush Trigger at (7, 15, 2) [Triggers when player agent enters 6-tile radius]; Reset / Loop Scripted Action Queue |
| Ped #17 | Police Officer | 6 HP (Default) | (17, 31, 0) | None | Walking (Equipped: Shotgun (#1)) | None (Wandering) |
| Ped #18 | Police Officer | 6 HP (Default) | (37, 30, 0) | None | Walking (Equipped: Shotgun (#2)) | None (Wandering) |
| Ped #19 | Police Officer | 6 HP (Default) | (37, 29, 0) | None | Walking (Equipped: Shotgun (#3)) | None (Wandering) |
| Ped #20 | Police Officer | 6 HP (Default) | (54, 21, 0) | None | Walking (Equipped: Shotgun (#4)) | None (Wandering) |
| Ped #21 | Police Officer | 6 HP (Default) | (27, 59, 0) | None | Walking (Equipped: Shotgun (#5)) | None (Wandering) |
| Ped #22 | Police Officer | 6 HP (Default) | (64, 76, 0) | None | Walking (Equipped: Shotgun (#6)) | None (Wandering) |
| Ped #23 | Police Officer | 6 HP (Default) | (64, 33, 0) | None | Walking (Equipped: Shotgun (#7)) | None (Wandering) |
| Ped #24 | Police Officer | 6 HP (Default) | (56, 62, 0) | None | Walking (Equipped: Shotgun (#8)) | None (Wandering) |
| Ped #25 | Police Officer | 6 HP (Default) | (42, 20, 0) | None | Walking (Equipped: Shotgun (#9)) | None (Wandering) |
| Ped #26 | Police Officer | 6 HP (Default) | (51, 25, 0) | None | Walking (Equipped: Shotgun (#10)) | None (Wandering) |
| Ped #27 | Police Officer | 6 HP (Default) | (43, 50, 0) | None | Walking (Equipped: Shotgun (#11)) | None (Wandering) |
| Ped #28 | Police Officer | 6 HP (Default) | (93, 66, 0) | None | Walking (Equipped: Shotgun (#12)) | None (Wandering) |
| Ped #29 | Police Officer | 6 HP (Default) | (5, 78, 1) | None | Walking (Equipped: Shotgun (#13)) | None (Wandering) |
| Ped #30 | Police Officer | 6 HP (Default) | (76, 65, 0) | None | Walking (Equipped: Shotgun (#14)) | None (Wandering) |
| Ped #31 | Police Officer | 6 HP (Default) | (14, 21, 0) | None | Walking (Equipped: Shotgun (#15)) | None (Wandering) |
| Ped #32 | Police Officer | 6 HP (Default) | (38, 17, 0) | None | Walking (Equipped: Shotgun (#16)) | None (Wandering) |
| Ped #33 | Police Officer | 6 HP (Default) | (26, 18, 0) | None | Walking (Equipped: Shotgun (#17)) | None (Wandering) |
| Ped #34 | Police Officer | 6 HP (Default) | (70, 38, 0) | None | Walking (Equipped: Shotgun (#18)) | None (Wandering) |
| Ped #35 | Police Officer | 6 HP (Default) | (87, 63, 0) | None | Walking (Equipped: Shotgun (#19)) | None (Wandering) |
| Ped #36 | Police Officer | 6 HP (Default) | (44, 36, 0) | None | Walking (Equipped: Shotgun (#20)) | None (Wandering) |
| Ped #37 | Police Officer | 6 HP (Default) | (105, 42, 0) | None | Walking (Equipped: Shotgun (#21)) | None (Wandering) |
| Ped #38 | Police Officer | 6 HP (Default) | (82, 30, 0) | None | Walking (Equipped: Shotgun (#22)) | None (Wandering) |
| Ped #39 | Police Officer | 6 HP (Default) | (94, 59, 0) | None | Walking (Equipped: Shotgun (#23)) | None (Wandering) |
| Ped #40 | Police Officer | 6 HP (Default) | (103, 52, 0) | None | Walking (Equipped: Shotgun (#24)) | None (Wandering) |
| Ped #41 | Police Officer | 6 HP (Default) | (71, 9, 0) | None | Walking (Equipped: Shotgun (#25)) | None (Wandering) |
| Ped #42 | Police Officer | 6 HP (Default) | (52, 12, 0) | None | Walking (Equipped: Shotgun (#26)) | None (Wandering) |
| Ped #43 | Police Officer | 6 HP (Default) | (98, 57, 0) | None | Walking (Equipped: Shotgun (#27)) | None (Wandering) |
| Ped #44 | Police Officer | 6 HP (Default) | (57, 28, 0) | None | Walking (Equipped: Shotgun (#28)) | None (Wandering) |
| Ped #45 | Police Officer | 6 HP (Default) | (15, 21, 0) | None | Walking (Equipped: Shotgun (#29)) | None (Wandering) |
| Ped #46 | Police Officer | 6 HP (Default) | (63, 32, 0) | None | Walking (Equipped: Shotgun (#30)) | None (Wandering) |
| Ped #47 | Police Officer | 6 HP (Default) | (91, 60, 7) | None | Walking (Equipped: Shotgun (#31)) | None (Wandering) |
| Ped #48 | Police Officer | 6 HP (Default) | (93, 28, 1) | None | Walking (Equipped: Shotgun (#32)) | None (Wandering) |
| Ped #49 | Police Officer | 6 HP (Default) | (62, 69, 0) | None | Walking (Equipped: Shotgun (#33)) | None (Wandering) |

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

### Ped #16 (Guard)

* **Script Entry**: Scenario Script #3

**Scenario Chain**:
1. Proximity Ambush Trigger at (7, 15, 2) [Triggers when player agent enters 6-tile radius]
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

No vehicles placed in map level.


### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 35


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (100, 43, 0) and establish perimeter control.
* **Enemy Composition**: 8 enemy syndicate agents across 1 active faction(s) plus 34 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
