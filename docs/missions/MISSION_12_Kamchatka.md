# Mission 12: Kamchatka

**Country**: Kamchatka (`#CNTRY_12`)

**Map ID**: Map 12 (`map12.dat`)

**Map Tile Bounding Box**: Tile X [15 – 68], Tile Y [14 – 71]

**Total Entities**: 74 Active Peds (4 Player Agents, 13 Key/Scripted NPCs, 57 Generic Civilians), 0 Vehicles, 4 Weapons.

---

## 1. Briefing Summary

WATERWORKS.

ASSASSINATION.

ONE OF OUR POLITICIAN FRIENDS HAS STARTED TO REFUSE US OUR NORMAL AND COURTEOUS REQUESTS. THIS IS EXTREMELY BAD FORM AND IS JUST NOT TOLERABLE.

IN ORDER TO RESTORE THE STATUS QUO AND MAKE AN EXAMPLE OF HIM WE HAVE DECIDED TO KILL HIS GOOD LADY WIFE.

YOUR SQUAD WILL BE DEPLOYED IN THE DOCKS OF A FLOATING CITY WHERE IT IS BELIEVED SHE SPENDS MOST OF HER TIME. WE HAVE BEEN UNABLE TO FIND HER LOCATION BUT WE DO KNOW THE WHEREABOUTS OF A LIKELY INFORMER. HE MAY REQUIRE SOME <PERSUASION> TO REVEAL HER LOCATION HOWEVER.

**DEFENCE UPDATE:**

THE LOCAL POLICE FORCE IS ALL THAT REALLY STANDS BETWEEN YOU AND THE MISSION OBJECTIVE. THEY ARE POORLY EQUIPPED AND BADLY TRAINED SO THEY SHOULD NOT BE A PROBLEM.

**SYNDICATE UPDATE:**

THERE IS NO REASON TO EXPECT ANY ENEMY SYNDICATE FORCES AS THIS DOESNT HAVE ANY DIRECT INFLUENCE ON THEIR BUSINESS. NEVERTHELESS YOU SHOULD STILL KEEP AN EYE OUT FOR ANY ENEMY SCOUT SQUADS IN THE AREA.

**TARGET UPDATE:**

IT IS BELIEVED THAT THE TARGET HAS BODYGUARDS. NO FURTHER INFORMATION IS AVAILABLE ON THEM HOWEVER.

**TACTICS:**

A SINGLE ASSASSIN COULD WALK RIGHT UP TO THE HOUSE ALMOST UNDETECTED BY THE LOCAL AUTHORITIES. WHEN YOU ARRIVE AT THE HOUSE PICK YOUR TIME WELL BEFORE STRIKING. IF THE BUILDING IS GUARDED IT WOULD BE ADVISABLE TO WAIT FOR THE TARGET TO LEAVE AND PICK HER OFF WITH A LONG RANGE SHOT.

---

## 2. Mission Objectives

### Objective #1: Persuade Target

* **Type**: Persuade Target
* **Target**: Ped #17 (Civilian) at Tile (34, 83, 1)
* **Status**: Inactive / Pending (`0`)

### Objective #2: Assassinate Target

* **Type**: Assassinate Target
* **Target**: Ped #16 (Civilian) at Tile (23, 16, 3)
* **Status**: Inactive / Pending (`0`)

### Objective #3: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (82, 70, 1)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (81, 70, 1) | On Map | 16 HP |
| Agent #2 | Ped #1 | (82, 71, 1) | On Map | 16 HP |
| Agent #3 | Ped #2 | (82, 69, 1) | On Map | 16 HP |
| Agent #4 | Ped #3 | (83, 70, 1) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP | (26, 40, 3) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP | (26, 39, 3) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP | (38, 14, 2) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP | (39, 14, 2) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 16 HP | (67, 30, 2) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 16 HP | (68, 30, 2) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 16 HP | (54, 58, 2) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #15 | Enemy Agent (Faction 1: Blue) | 16 HP | (54, 58, 2) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #16 | Civilian | 8 HP | (23, 16, 3) | None | Standing (Unarmed) | Walk to position (23, 16, 9) |
| Ped #17 | Civilian | 8 HP | (34, 83, 1) | None | Standing (Unarmed) | None (Stationary) |
| Ped #18 | Guard | 8 HP | (25, 14, 3) | None | Standing (Equipped: Uzi (#2), Shotgun (#3)) | Walk to position (25, 14, 9); Proximity Ambush Trigger at (29, 14, 4) [Triggers when player agent enters 6-tile radius]; Walk to position (26, 15, 5) (+1 more) |
| Ped #19 | Guard | 8 HP | (25, 16, 3) | None | Standing (Equipped: Shotgun (#4)) | Walk to position (25, 16, 10); Proximity Ambush Trigger at (29, 14, 4) [Triggers when player agent enters 6-tile radius]; Walk to position (26, 15, 5) (+1 more) |
| Ped #21 | Guard | 8 HP | (25, 16, 3) | None | Standing (Equipped: Flamer (#0)) | Walk to position (25, 16, 9); Proximity Ambush Trigger at (27, 14, 5) [Triggers when player agent enters 6-tile radius]; Walk to position (26, 15, 5) (+1 more) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 8`, `max_rush_factions = 0`

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
| #3 | Child (after Tactic #10) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #4 | Active |
| #4 | Child (after Tactic #3) | Wildcard (Global) | Board Vehicle | Target vehicle index: #1566 | Tactic #5 | Active |
| #5 | Child (after Tactic #4) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #6 | Root (Active at Start) | Wildcard (Global) | Seek & Destroy | Target: Faction 3: Yellow A | Tactic #7 | Active |
| #7 | Child (after Tactic #6) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 200 stages (~270.0s timer) | Tactic #8 | Active |
| #8 | Child (after Tactic #7) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #9 | Root (Active at Start) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |
| #10 | Root (Active at Start) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 12 tiles (each sentry) OR 50 stages (~67.5s timer) | Tactic #3 | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

### Ped #16 (Civilian)

* **Script Entry**: Scenario Script #25

**Scenario Chain**:
1. Walk to position (23, 16, 9)

### Ped #18 (Guard)

* **Script Entry**: Scenario Script #26

**Scenario Chain**:
1. Walk to position (25, 14, 9)
1. Proximity Ambush Trigger at (29, 14, 4) [Triggers when player agent enters 6-tile radius]
1. Walk to position (26, 15, 5)
1. Reset / Loop Scripted Action Queue

### Ped #19 (Guard)

* **Script Entry**: Scenario Script #30

**Scenario Chain**:
1. Walk to position (25, 16, 10)
1. Proximity Ambush Trigger at (29, 14, 4) [Triggers when player agent enters 6-tile radius]
1. Walk to position (26, 15, 5)
1. Reset / Loop Scripted Action Queue

### Ped #21 (Guard)

* **Script Entry**: Scenario Script #35

**Scenario Chain**:
1. Walk to position (25, 16, 9)
1. Proximity Ambush Trigger at (27, 14, 5) [Triggers when player agent enters 6-tile radius]
1. Walk to position (26, 15, 5)
1. Reset / Loop Scripted Action Queue

---

## 7. Significant Objects

### 7.1 Key Vehicles

No vehicles placed in map level.


### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 4


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (81, 70, 0) and establish perimeter control.
* **Enemy Composition**: 8 enemy syndicate agents across 1 active faction(s) plus 5 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
