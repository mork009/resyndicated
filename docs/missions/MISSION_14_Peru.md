# Mission 14: Peru

**Country**: Peru (`#CNTRY_14`)

**Map ID**: Map 20 (`map20.dat`)

**Map Tile Bounding Box**: Tile X [17 – 86], Tile Y [13 – 69]

**Total Entities**: 98 Active Peds (4 Player Agents, 48 Key/Scripted NPCs, 46 Generic Civilians), 12 Vehicles, 35 Weapons.

---

## 1. Briefing Summary

THE RUNNER.

ASSASSINATION.

A MILITARY ADVISER TO AN ENEMY SYNDICATE IS HOLED UP IN A BUNKER IN THE CITY. HE IS A VALUED ASSET TO THE ENEMY BUT WOULDNT BE MUCH USE TO US AND SO HE MUST BE TERMINATED.

THE ENEMY SYNDICATE HAS SEEN FIT TO DEPLOY A NUMBER OF DEFENCE SQUADS IN THE CITY SO YOUR STRIKE TEAM WILL BE DEPLOYED JUST OUTSIDE THE AREA CLOSE TO A TRANSPORT SYSTEM.

YOU MUST INFILTRATE THE BUNKER AND ASSASSINATE THE TARGET BEFORE HE CAN LEAVE.

**CITY UPDATE:**

THE CITY ITSELF IS RAISED ABOVE THE VEGETATION LEVEL ON A SERIES OF SUPPORTS. THERE ARE ONLY A FEW ACCESS WAYS TO THE LOWER LEVEL SO ANYBODY FALLING OFF THE MAIN LEVEL WILL HAVE A HARD TIME TRYING TO GET BACK UP AGAIN.

**TARGET UPDATE:**

THE BUNKER ITSELF IS ACCESSIBLE BY VEHICLE OR A LONG TUNNEL. AS WE CANNOT PROVIDE A VEHICLE FOR THIS MISSION YOUR SQUAD WILL HAVE TO PROCEED ON FOOT TO THE TUNNEL ENTRANCE. WE BELIEVE THE ENTRANCE TO THE TUNNEL IS LOCATED NEAR THE DOCKS AREA TO THE SOUTH OF THE CITY.

**TACTICS:**

THE ADVISER ISNT STUPID AND IF HE SEES YOU COMING HE WILL EITHER MOBILISE FURTHER ENEMY INTERCEPT SQUADS OR ATTEMPT TO GET AWAY. YOU SHOULD POST A GUARD AT THE REAR ENTRANCE IN CASE HE TRIES TO MAKE A BREAK FOR IT.

---

## 2. Mission Objectives

### Objective #1: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #9 (Enemy Agent (Faction 1: Blue)) at Tile (88, 35, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (98, 20, 8) | On Map | 16 HP |
| Agent #2 | Ped #1 | (99, 20, 8) | On Map | 16 HP |
| Agent #3 | Ped #2 | (100, 20, 8) | On Map | 16 HP |
| Agent #4 | Ped #3 | (100, 20, 8) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Civilian | 2 HP (Default) | (43, 17, 8) | Car #0 (Train Head (Locomotive)) | In Vehicle (Car #0 (Train Head (Locomotive))) (Unarmed) | Enter vehicle #0; Train Station Stop & Wait (10,000ms dwell for passenger boarding/alighting); Drive vehicle #0 to (39, 17, 9) (+2 more) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (88, 35, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP | (44, 66, 3) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP | (42, 66, 4) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 16 HP | (44, 67, 3) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 16 HP | (28, 56, 4) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 16 HP | (28, 37, 4) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #15 | Enemy Agent (Faction 1: Blue) | 16 HP | (28, 56, 4) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #16 | Enemy Agent (Faction 2: Orange) | 16 HP | (28, 37, 4) | None | Stationary Sentry (Default: Pistol) | Faction 2 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #17 | Enemy Agent (Faction 2: Orange) | 16 HP | (28, 37, 4) | None | Stationary Sentry (Default: Pistol) | Faction 2 AI Tactics: Tactic #0 (Objective Sweep -> Player Squad) |
| Ped #18 | Guard | 8 HP | (26, 77, 9) | None | Standing (Equipped: Long Range Rifle (#0)) | None (Stationary) |
| Ped #19 | Guard | 8 HP | (36, 76, 9) | None | Standing (Equipped: Long Range Rifle (#1)) | None (Stationary) |
| Ped #20 | Guard | 8 HP | (78, 26, 9) | None | Standing (Equipped: Long Range Rifle (#2)) | None (Stationary) |
| Ped #21 | Guard | 8 HP | (88, 26, 9) | None | Standing (Equipped: Long Range Rifle (#3)) | None (Stationary) |
| Ped #68 | Police Officer | 6 HP (Default) | (82, 56, 4) | None | Walking (Equipped: Uzi (#4)) | None (Wandering) |
| Ped #69 | Police Officer | 6 HP (Default) | (56, 45, 5) | None | Walking (Equipped: Uzi (#5)) | None (Wandering) |
| Ped #70 | Police Officer | 6 HP (Default) | (50, 46, 4) | None | Walking (Equipped: Uzi (#6)) | None (Wandering) |
| Ped #71 | Police Officer | 6 HP (Default) | (34, 46, 3) | None | Walking (Equipped: Uzi (#7)) | None (Wandering) |
| Ped #72 | Police Officer | 6 HP (Default) | (40, 74, 3) | None | Walking (Equipped: Uzi (#8)) | None (Wandering) |
| Ped #73 | Police Officer | 6 HP (Default) | (65, 52, 5) | None | Walking (Equipped: Uzi (#9)) | None (Wandering) |
| Ped #74 | Police Officer | 6 HP (Default) | (66, 62, 3) | None | Walking (Equipped: Uzi (#10)) | None (Wandering) |
| Ped #75 | Police Officer | 6 HP (Default) | (72, 45, 5) | None | Walking (Equipped: Uzi (#11)) | None (Wandering) |
| Ped #76 | Police Officer | 6 HP (Default) | (46, 18, 7) | None | Walking (Equipped: Uzi (#12)) | None (Wandering) |
| Ped #77 | Police Officer | 6 HP (Default) | (65, 55, 5) | None | Walking (Equipped: Uzi (#13)) | None (Wandering) |
| Ped #78 | Police Officer | 6 HP (Default) | (60, 55, 3) | None | Walking (Equipped: Uzi (#14)) | None (Wandering) |
| Ped #79 | Police Officer | 6 HP (Default) | (60, 64, 3) | None | Walking (Equipped: Uzi (#15)) | None (Wandering) |
| Ped #80 | Police Officer | 6 HP (Default) | (71, 57, 5) | None | Walking (Equipped: Uzi (#16)) | None (Wandering) |
| Ped #81 | Police Officer | 6 HP (Default) | (77, 57, 5) | None | Walking (Equipped: Uzi (#17)) | None (Wandering) |
| Ped #82 | Police Officer | 6 HP (Default) | (51, 46, 4) | None | Walking (Equipped: Uzi (#18)) | None (Wandering) |
| Ped #83 | Police Officer | 6 HP (Default) | (65, 58, 4) | None | Walking (Equipped: Uzi (#19)) | None (Wandering) |
| Ped #84 | Police Officer | 6 HP (Default) | (65, 62, 3) | None | Walking (Equipped: Uzi (#20)) | None (Wandering) |
| Ped #85 | Police Officer | 6 HP (Default) | (73, 58, 4) | None | Walking (Equipped: Uzi (#21)) | None (Wandering) |
| Ped #86 | Police Officer | 6 HP (Default) | (32, 67, 3) | None | Walking (Equipped: Uzi (#22)) | None (Wandering) |
| Ped #87 | Police Officer | 6 HP (Default) | (77, 62, 3) | None | Walking (Equipped: Uzi (#23)) | None (Wandering) |
| Ped #88 | Police Officer | 6 HP (Default) | (78, 27, 3) | None | Walking (Equipped: Uzi (#24)) | None (Wandering) |
| Ped #89 | Police Officer | 6 HP (Default) | (69, 50, 4) | None | Walking (Equipped: Uzi (#25)) | None (Wandering) |
| Ped #90 | Police Officer | 6 HP (Default) | (65, 56, 5) | None | Walking (Equipped: Uzi (#26)) | None (Wandering) |
| Ped #91 | Police Officer | 6 HP (Default) | (72, 45, 5) | None | Walking (Equipped: Uzi (#27)) | None (Wandering) |
| Ped #92 | Police Officer | 6 HP (Default) | (61, 46, 4) | None | Walking (Equipped: Uzi (#28)) | None (Wandering) |
| Ped #93 | Police Officer | 6 HP (Default) | (37, 50, 3) | None | Walking (Equipped: Uzi (#29)) | None (Wandering) |
| Ped #94 | Police Officer | 6 HP (Default) | (47, 41, 5) | None | Walking (Equipped: Uzi (#30)) | None (Wandering) |
| Ped #95 | Police Officer | 6 HP (Default) | (69, 62, 3) | None | Walking (Equipped: Uzi (#31)) | None (Wandering) |
| Ped #96 | Police Officer | 6 HP (Default) | (61, 63, 4) | None | Walking (Equipped: Uzi (#32)) | None (Wandering) |
| Ped #97 | Police Officer | 6 HP (Default) | (65, 46, 5) | None | Walking (Equipped: Uzi (#33)) | None (Wandering) |
| Ped #98 | Police Officer | 6 HP (Default) | (78, 45, 4) | None | Walking (Equipped: Uzi (#34)) | None (Wandering) |
| Ped #99 | Civilian | 2 HP (Default) | (61, 70, 4) | Car #9 (Civilian Sedan) | In Vehicle (Car #9 (Civilian Sedan)) (Unarmed) | Enter vehicle #9; Drive vehicle #9 to (49, 65, 2) |
| Ped #100 | Civilian | 2 HP (Default) | (43, 47, 4) | Car #10 (Civilian Sedan) | In Vehicle (Car #10 (Civilian Sedan)) (Unarmed) | Enter vehicle #10; Drive vehicle #10 to (73, 54, 12) |
| Ped #101 | Civilian | 2 HP (Default) | (38, 25, 4) | Car #11 (Civilian Sedan) | In Vehicle (Car #11 (Civilian Sedan)) (Unarmed) | Enter vehicle #11; Drive vehicle #11 to (30, 38, 2) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 8`, `max_rush_factions = 0`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (7 agents: Peds #9–#15)

* **Idle Movement State**: 7 Stationary Sentry
* **Assigned Root Tactic**: Tactic #0 *(Tactic #0 Fallback (Wildcard))*
* **Execution Chain**:
  1. **Tactic #0** [Wildcard / Objective Sweep]: Target: Player Squad

#### Faction 2 (Orange Syndicate) (2 agents: Peds #16–#17)

* **Idle Movement State**: 2 Stationary Sentry
* **Assigned Root Tactic**: Tactic #0 *(Tactic #0 Fallback (Wildcard))*
* **Execution Chain**:
  1. **Tactic #0** [Wildcard / Objective Sweep]: Target: Player Squad

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |

* **Fallback Resolution Note**: Any active hostile faction without a matching faction-specific root (`1..7`) in slots `1..127` falls back to `Tactic #0` (when enabled).

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #8 (Civilian)

* **Script Entry**: Scenario Script #1

**Scenario Chain**:
1. Enter vehicle #0
1. Train Station Stop & Wait (10,000ms dwell for passenger boarding/alighting)
1. Drive vehicle #0 to (39, 17, 9)
1. Train Station Stop & Wait (10,000ms dwell for passenger boarding/alighting)
1. Drive vehicle #0 to (97, 17, 9)

### Ped #99 (Civilian)

* **Script Entry**: Scenario Script #20

**Scenario Chain**:
1. Enter vehicle #9
1. Drive vehicle #9 to (49, 65, 2)

### Ped #100 (Civilian)

* **Script Entry**: Scenario Script #22

**Scenario Chain**:
1. Enter vehicle #10
1. Drive vehicle #10 to (73, 54, 12)

### Ped #101 (Civilian)

* **Script Entry**: Scenario Script #24

**Scenario Chain**:
1. Enter vehicle #11
1. Drive vehicle #11 to (30, 38, 2)

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Train Head (Locomotive) | (43, 17, 7) | 1280 | Ped #8 |
| Car #1 | Train Carriage | (43, 17, 7) | 0 | None |
| Car #2 | Train Carriage | (44, 17, 7) | 0 | None |
| Car #3 | Train Carriage | (45, 17, 7) | 0 | None |
| Car #4 | Train Carriage | (46, 17, 7) | 0 | None |
| Car #5 | Train Carriage | (47, 17, 7) | 0 | None |
| Car #6 | Train Head (Locomotive) | (48, 17, 7) | 1280 | None |
| Car #7 | Large Armored Car | (89, 26, 0) | 640 | None |
| Car #8 | Civilian Sedan | (31, 75, 4) | 1024 | None |
| Car #9 | Civilian Sedan | (61, 70, 4) | 1024 | Ped #99 |
| Car #10 | Civilian Sedan | (43, 47, 4) | 1024 | Ped #100 |
| Car #11 | Civilian Sedan | (38, 25, 4) | 1024 | Ped #101 |

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

* **Squad Positioning**: Spawn squad members at Tile (98, 20, 0) and establish perimeter control.
* **Enemy Composition**: 9 enemy syndicate agents across 2 active faction(s) plus 39 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
