# Mission 15: Central Europe

**Country**: Central Europe (`#CNTRY_15`)

**Map ID**: Map 18 (`map18.dat`)

**Map Tile Bounding Box**: Tile X [28 – 74], Tile Y [8 – 65]

**Total Entities**: 57 Active Peds (4 Player Agents, 11 Key/Scripted NPCs, 42 Generic Civilians), 3 Vehicles, 17 Weapons.

---

## 1. Briefing Summary

URBAN ASSAULT.

COMBAT SWEEP.

A SMALL TOWN IS HOLDING OUT AGAINST ALL OUR ADVERTISING CAMPAIGNS THANKS TO ENEMY SYNDICATE INFLUENCE. THIS IS HAVING A DETRIMENTAL EFFECT ON OUR PROFITS AND SO WE HAVE DECIDED TO STEP UP THE CAMPAIGN.

AN ASSAULT SQUAD SHOULD ENTER THE TOWN AND ELIMINATE ALL ENEMY SYNDICATE ACTIVITY IN THE AREA. THIS WILL LEAVE THE TOWN OPEN FOR OUR MARKETING BOYS.

**CITY UPDATE:**

THE TOWN IS VERY SMALL AND THE MAIN ENTRANCE IS ONLY ACCESSIBLE BY VEHICLE. THE TRANSPORT DIVISION HAS PROVIDED A GROUND CAR FOR THIS MISSION. PLEASE TRY TO BRING IT BACK.

**DEFENCE UPDATE:**

THERE IS LITTLE OR NO POLICE INFLUENCE IN THE TOWN AND THE ENEMY SYNDICATE FORCES ARE SERIOUSLY UNDER STRENGTH. NOW IS THE IDEAL TIME TO STRIKE.

**TACTICS:**

A WELL EQUIPPED SQUAD SHOULD HAVE NO PROBLEMS WITH THIS ONE. STICK TOGETHER AND THE ENEMY SQUAD SHOULD FALL BEFORE YOU. SHOTGUNS AND A FEW MEDIKITS JUST IN CASE ARE ALL THAT IS REALLY NEEDED.

---

## 2. Mission Objectives

### Objective #1: Combat Sweep (Eliminate All Enemy Agents)

* **Type**: Combat Sweep (Eliminate All Enemy Agents)
* **Target**: None
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (63, 7, 0)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (61, 8, 0) | On Map | 16 HP |
| Agent #2 | Ped #1 | (61, 7, 0) | On Map | 16 HP |
| Agent #3 | Ped #2 | (61, 7, 0) | On Map | 16 HP |
| Agent #4 | Ped #3 | (61, 6, 0) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (53, 62, 1) | None | Stationary Sentry (Equipped: Shotgun (#0)) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (58, 54, 1) | None | Stationary Sentry (Equipped: Shotgun (#1)) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (76, 60, 1) | None | Stationary Sentry (Equipped: Pistol (#2)) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP (Default) | (75, 40, 0) | None | Stationary Sentry (Equipped: Uzi (#3)) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #12 | Police Officer | 6 HP (Default) | (63, 47, 0) | Car #1 (Police Cruiser) | In Vehicle (Car #1 (Police Cruiser)) (Equipped: Uzi (#5)) | Enter vehicle #1; Drive vehicle #1 to (46, 68, 2); Drive vehicle #1 to (83, 33, 2) |
| Ped #55 | Police Officer | 6 HP (Default) | (87, 29, 0) | None | Walking (Equipped: Pistol (#6), Pistol (#7)) | None (Wandering) |
| Ped #56 | Police Officer | 6 HP (Default) | (81, 34, 0) | None | Walking (Equipped: Pistol (#8), Pistol (#9)) | None (Wandering) |
| Ped #57 | Police Officer | 6 HP (Default) | (61, 40, 0) | None | Walking (Equipped: Pistol (#10), Pistol (#11)) | None (Wandering) |
| Ped #58 | Police Officer | 6 HP (Default) | (89, 51, 0) | None | Walking (Equipped: Pistol (#12), Pistol (#13)) | None (Wandering) |
| Ped #59 | Police Officer | 6 HP (Default) | (88, 64, 0) | None | Walking (Equipped: Pistol (#14), Pistol (#15)) | None (Wandering) |
| Ped #60 | Police Officer | 6 HP (Default) | (60, 45, 0) | None | Walking (Equipped: Pistol (#16), Pistol (#17)) | None (Wandering) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 4`, `max_rush_factions = 4`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (4 agents: Peds #8–#11)

* **Idle Movement State**: 4 Stationary Sentry
* **Assigned Root Tactic**: Tactic #1 *(Wildcard (Generic Pool - Sequential Claim))*
* **Execution Chain**:
  1. **Tactic #1** [Wildcard / Proximity Tripwire]: Watch Player Squad within 12 tiles (each sentry) OR 200 stages (~270.0s timer)
  2. **Tactic #2** [Wildcard / Objective Sweep]: Target: Player Squad

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Objective Sweep | Target: Player Squad | Terminal | Active |
| #1 | Root (Active at Start) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 200 stages (~270.0s timer) | Tactic #2 | Active |
| #2 | Child (after Tactic #1) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #12 (Police Officer)

* **Script Entry**: Scenario Script #12

**Scenario Chain**:
1. Enter vehicle #1
1. Drive vehicle #1 to (46, 68, 2)
1. Drive vehicle #1 to (83, 33, 2)

---

## 7. Significant Objects

### 7.1 Key Vehicles

| Vehicle Index | Vehicle Type | Spawn Tile (X, Y, Z) | Speed | Driver Ped |
| :--- | :--- | :--- | :--- | :--- |
| Car #0 | Civilian Sedan | (62, 7, 0) | 1024 | None |
| Car #1 | Police Cruiser | (63, 47, 0) | 1280 | Ped #12 |
| Car #2 | Civilian Sedan | (53, 66, 0) | 1024 | None |

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

* **Squad Positioning**: Spawn squad members at Tile (61, 8, 0) and establish perimeter control.
* **Enemy Composition**: 4 enemy syndicate agents across 1 active faction(s) plus 7 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
