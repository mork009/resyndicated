# Mission 18: Ural

**Country**: Ural (`#CNTRY_18`)

**Map ID**: Map 38 (`map38.dat`)

**Map Tile Bounding Box**: Tile X [30 – 96], Tile Y [11 – 68]

**Total Entities**: 74 Active Peds (4 Player Agents, 8 Key/Scripted NPCs, 62 Generic Civilians), 0 Vehicles, 0 Weapons.

---

## 1. Briefing Summary

CLEAN SWEEP.

COMBAT SWEEP.

AN ENEMY SYNDICATE HAS BEEN EXPERIENCING PROBLEMS WITH THE POPULATION OF A NEARBY CITY. THIS IS MAINLY DUE TO SUBVERSIVE TECHNIQUES FROM OUR ESPIONAGE DEPARTMENT.

AS A RESULT OF THIS THE ENEMY SYNDICATE HAS PULLED OUT MOST OF ITS FORCES FROM THE SURROUNDING AREA AND THE CITY HAS FALLEN OPEN FOR US TO TAKE.

YOUR SQUAD IS TO CONDUCT A RECONNAISSANCE SWEEP OF THE AREA TO CLEAR OUT ANY ENEMY SYNDICATE AGENTS REMAINING INSIDE CITY LIMITS BEFORE RETURNING TO THE PICK UP POINT.

**SYNDICATE UPDATE:**

THE ENEMY SQUAD IS SERIOUSLY UNDER STRENGTH AND POORLY EQUIPPED. THE POLICE FORCE IS ALMOST NON EXISTENT AND SHOULDNT PROVIDE ANY MORE PROBLEM FOR YOU THAN IT DOES FOR THEM.

**TACTICS:**

SMALL ARMS WITH AUTOMATICS FOR SUPPORT ARE ALL THAT ARE REALLY NEEDED FOR THIS MISSION. A SQUAD OF TWO AGENTS COULD COVER THE CITY IN ALMOST NO TIME AT ALL. EITHER KEEP BOTH AGENTS TIGHT TO EACH OTHER AND CONCENTRATE FIREPOWER ON INDIVIDUAL TARGETS OR DRAG ONE AGENT FURTHER BEHIND WITH LONGER RANGED EQUIPMENT TO PROVIDE COVER FOR THE FIRST.

---

## 2. Mission Objectives

### Objective #1: Combat Sweep (Eliminate All Enemy Agents)

* **Type**: Combat Sweep (Eliminate All Enemy Agents)
* **Target**: None
* **Status**: Inactive / Pending (`0`)

### Objective #2: Evacuate to Extraction Zone

* **Type**: Evacuate to Extraction Zone
* **Target**: Extraction Zone at Tile (49, 24, 7)
* **Status**: Inactive / Pending (`0`)

---

## 3. Player Agents Deployment

| Agent Slot | Ped Index | Spawn Tile (X, Y, Z) | Location Status | Initial Health |
| :--- | :--- | :--- | :--- | :--- |
| Agent #1 | Ped #0 | (50, 21, 7) | On Map | 16 HP |
| Agent #2 | Ped #1 | (49, 21, 7) | On Map | 16 HP |
| Agent #3 | Ped #2 | (49, 22, 7) | On Map | 16 HP |
| Agent #4 | Ped #3 | (50, 21, 7) | On Map | 16 HP |

---

## 4. Key & Non-Generic NPCs

| Ped Index | Type | Health | Spawn Tile (X, Y, Z) | Assigned Vehicle | Initial State & Weapons | Scripted Actions / AI |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Ped #8 | Enemy Agent (Faction 1: Blue) | 16 HP | (79, 42, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #9 | Enemy Agent (Faction 1: Blue) | 16 HP | (83, 32, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #10 | Enemy Agent (Faction 1: Blue) | 16 HP | (99, 42, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #11 | Enemy Agent (Faction 1: Blue) | 16 HP | (99, 41, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #12 | Enemy Agent (Faction 1: Blue) | 16 HP | (100, 13, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #13 | Enemy Agent (Faction 1: Blue) | 16 HP | (101, 13, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #14 | Enemy Agent (Faction 1: Blue) | 16 HP | (63, 57, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |
| Ped #15 | Enemy Agent (Faction 1: Blue) | 16 HP | (64, 57, 0) | None | Stationary Sentry (Default: Pistol) | Faction 1 AI Tactics: Tactic #1 (Proximity Tripwire -> Player Squad) |

* **Note on Enemy Agent Loadouts & Behavior**: NPC weapon assignments are loaded statically from mission level data and are not scaled or randomized by player research progress. Enemy agents without a pre-assigned weapon in level data automatically receive a default loaded Pistol at mission start. Enemy agents do not use scenario waypoint scripts; ambient movement follows their idle state (Stationary Sentry vs. Wandering Patrol), while combat mobilization is governed by their Faction AI Tactics FSM (Section 5).

---

## 5. Enemy Faction AI Tactics

**AI Header Config**: `num_squads = 2`, `peds_per_squad = 8`, `max_rush_factions = 5`

### 5.1 Active Faction Deployments & Effective Resolution

#### Faction 1 (Blue Syndicate) (8 agents: Peds #8–#15)

* **Idle Movement State**: 8 Stationary Sentry
* **Assigned Root Tactic**: Tactic #1 *(Wildcard (Generic Pool - Sequential Claim))*
* **Execution Chain**:
  1. **Tactic #1** [Wildcard / Proximity Tripwire]: Watch Player Squad within 10 tiles (terminal: no timer; advances to Tactic #0 on breach)
  2. **Tactic #0** [Wildcard / Proximity Tripwire]: Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer)

### 5.2 Mission AI Tactics Pool

| Tactic # | Chain Role | Faction Owner | Mode | Target / Trigger Details | Next Tactic | Flags |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- |
| #0 | Global Fallback (Slot 0) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer) | Tactic #1 | Active |
| #1 | Root (Active at Start) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 10 tiles (terminal: no timer; advances to Tactic #0 on breach) | Tactic #0 (Fallback) | Active |
| #2 | Root (Active at Start) | Wildcard (Global) | Proximity Tripwire | Watch Player Squad within 10 tiles (each sentry) OR 10 stages (~13.5s timer) | Tactic #3 | Active |
| #3 | Child (after Tactic #2) | Wildcard (Global) | Objective Sweep | Target: Player Squad | End | Active |

* **Wildcard Resolution Note**: Unclaimed faction-specific roots (`1..7`) are assigned first; any remaining active hostile faction sequentially claims unclaimed Wildcard (`255`) roots in slot order `1..127` (Pass 1), wrapping around to reuse earlier Wildcard roots (Pass 2) before ever falling back to `Tactic #0`.

---

## 6. Detailed Scripted Actions & Paths (Non-Agent NPCs & Vehicles)

No scripted waypoint paths defined for non-agent NPCs or vehicles in this mission.

---

## 7. Significant Objects

### 7.1 Key Vehicles

No vehicles placed in map level.


### 7.2 Key Weapons & Inventory Pickups

Total Ground Pickups: 0 | Total Inventory Items: 0


---

## 8. Level Logic & Exceptions

* **Grid Tile Index Alignment**: All tile coordinates $(X, Y, Z)$ are normalized to 0-indexed map tile grid coordinates to align 1:1 with in-game debug displays.

* **Reserved Ped Slots 4–7**: Ped slots 4 through 7 are reserved for persuaded agents during gameplay and are excluded from initial level entity spawning.

* **Scenario Script Command Support**: Executed scenario steps cover Walk / Drive (`1`), Enter Vehicle (`2`), Timed Pause (`5`, `ticks * 100ms` or `5,000ms` default), Leave Vehicle (`6`), Target Escape (`7`), Proximity Ambush Trigger (`8`, 6-tile radius), Loop / Reset (`9`), and Train Station Stop (`10`, `10,000ms` dwell). Scripted NPCs ignore ambient gunfire panic, but bail out and abandon their script if their vehicle is damaged or carjacked.

* **Enemy Agent Tactical AI**: Enemy syndicate operatives do not use waypoint routes; combat activation, target selection, and cross-faction warfare are governed by Section 5's AI Tactics FSM (`AITacticsBlock`).

Follows standard data-driven level loading.

---

## 9. Gameplay & Tactical Considerations

* **Squad Positioning**: Spawn squad members at Tile (50, 21, 0) and establish perimeter control.
* **Enemy Composition**: 8 enemy syndicate agents across 1 active faction(s) plus 0 key/scripted non-agent NPCs (police, guards, scripted targets).
* **Evacuation Protocol**: Assemble squad and target peds at extraction zone upon objective completion.
