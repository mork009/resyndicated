# ReSyndicated - Character Stats & Behaviors

This document provides a comprehensive overview of character attributes, health metrics, movement mechanics, cybernetic modifications, the IPA (Intelligence, Perception, Adrenaline) stimulation system, and AI behavior rules for all characters (Player Agents, Enemy Agents, Guards, Police, Civilians, Criminals, and Persuaded Followers) in **ReSyndicated**.

All base character attributes are dynamically loaded from configuration data (`data/ref/characters.dat`) at runtime, allowing gameplay balancing without code changes.

---

## 1. Character Categories & Base Attributes

Every character belongs to one of six categories, each configured with specific baseline attributes:

| Character Type   | Base Speed | Default Health | Sight Range              | Base Accuracy | AI Check Interval | Persuasion Cost |
|:---------------- |:----------:|:--------------:|:------------------------:|:-------------:|:-----------------:|:---------------:|
| **Player Agent** | 178        | 16 HP          | 1,792 units (7.0 tiles)  | 50%           | 400 ms            | 8 points        |
| **Enemy Agent**  | 178        | 16 HP          | 1,500 units (5.86 tiles) | 50%           | 400 ms            | 8 points        |
| **Guard**        | 128        | 1 HP           | 1,500 units (5.86 tiles) | 45%           | 300 ms            | 2 points        |
| **Police**       | 128        | 1 HP           | 1,500 units (5.86 tiles) | 40%           | 400 ms            | 4 points        |
| **Civilian**     | 128        | 1 HP           | 1,500 units (5.86 tiles) | 20%           | 600 ms            | 1 point         |
| **Criminal**     | 128        | 1 HP           | 1,500 units (5.86 tiles) | 20%           | 500 ms            | 1 point         |

---

## 2. Health, Armor Mitigation & Healing Systems

### Health Pools

* **Player & Enemy Agents**: Standardized at **16 HP**.
* **NPCs (Guards, Police, Civilians, Criminals)**: Default to **1 HP** when mission data does not specify a custom health pool (specific mission entities, such as heavy platform guards in certain campaigns, can carry elevated HP like **8 HP**).

### Cybernetic Armor Mitigation

Incoming damage is mitigated according to weapon damage type and the target's equipped **Chest Mod**:

* **Bullet Weapons**: Chest cybernetics provide a probabilistic chance to deflect incoming rounds:
  - **Unaugmented**: 100% hit chance (no deflection).
  - **Chest Mod V1**: 50% hit chance (50% deflected).
  - **Chest Mod V2**: 25% hit chance (75% deflected).
  - **Chest Mod V3**: 16.7% hit chance (83.3% deflected).
* **Laser, Explosions & Vehicle Collisions**:
  - **Unaugmented & Chest V1**: Instant kill on hit.
  - **Chest Mod V2**: Reduced to **8 HP** per hit.
  - **Chest Mod V3**: Reduced to **4 HP** per hit.
* **Flamer (Burn)**:
  - **Unaugmented**: Instant kill on hit.
  - **Chest Mod V1**: Reduced to **8 HP** per hit.
  - **Chest Mod V2**: Reduced to **4 HP** per hit.
  - **Chest Mod V3**: Reduced to **2 HP** per hit.

### Energy Shields

Active **Energy Shields** completely deflect and absorb incoming bullet and laser projectiles. Shields consume battery energy while active. If unequipped before draining completely, the battery gradually recharges over time. If allowed to drain to zero, the shield battery is permanently depleted.

### Health Regeneration & Healing

* **Chest Mod Regeneration**: Agents equipped with Chest Mod V1 or higher regenerate 1 HP periodically after taking damage:
  - **Normal Adrenaline ($\ge 4\%$)**: Fixed per-tier rate independent of Adrenaline boost (**12.8s** for Chest V1, **6.4s** for Chest V2, **3.2s** for Chest V3).
  - **Very Low Adrenaline ($< 4\%$)**: All Chest tiers (V1, V2, and V3) accelerate to regenerate **1 HP every 0.8 seconds**.
* **Medikit**: Activating a field Medikit instantly restores the agent's health back to their maximum capacity (16 HP).

### Combat Hit Interruption & Recoil Pushback

* **Firing Interruption**: When any character takes damage from bullets, lasers, explosions, or collisions, active weapon fire is immediately interrupted. Hit animations temporarily prevent characters from firing or acquiring new targets until the recoil finishes.
* **Recoil Pushback**: Impacts push the target directly backward away from the shooter or explosion center. Continuous automatic weapon fire (such as from a Minigun) continuously pushes the target back. If a target is pushed into an impassable wall or barrier, the backward displacement stops at the obstacle.
* **Weapon Lockout During Death**: When health drops to 0, weapon firing immediately ceases, equipped weapons are dropped to the ground, and death animations play. Characters cannot shoot while falling or dying.

---

## 3. Movement Speeds & Physics

Movement speed in world units per frame is calculated based on base speed, current Adrenaline stimulation, equipped Leg cybernetics, and inventory encumbrance:

* **Base Walking Speed (Neutral Adrenaline at 50%)**: At neutral Adrenaline (50%), all agents walk at the exact same base pace (178 units/frame, taking **13.0 seconds to cross 9 tiles**). Leg modifications do not alter this neutral walking speed.
* **Adrenaline Sprint Boost**: When Adrenaline is increased above 50%, movement speed accelerates based on the agent's equipped Leg modification:
  - **Unaugmented**: Up to 1.36x sprint speed at 100% Adrenaline (5.5 seconds across 9 tiles).
  - **Legs V1**: Up to 1.90x sprint speed at 100% Adrenaline (4.48 seconds across 9 tiles).
  - **Legs V2**: Up to 2.45x sprint speed at 100% Adrenaline (3.76 seconds across 9 tiles).
  - **Legs V3**: Up to 3.00x sprint speed at 100% Adrenaline (3.25 seconds across 9 tiles).
* **Encumbrance Penalty**: Carrying weapons exceeding an agent's Arm carrying capacity (1,000 weight units unaugmented; +10,000 per Arm cybernetic tier up to 31,000 at V3) progressively slows movement speed down to a minimum crawl when heavily overloaded.
* **Persuaded Follower Pacing**: Persuaded followers dynamically match the pace of their controlling agent. When the agent boosts Adrenaline to sprint, followers accelerate up to 2.30x speed, keeping pace with the squad. When calmed below neutral, followers slow down accordingly.
* **Vector Path Smoothing**: When characters travel toward distant destinations across open, unobstructed terrain, pathfinding smooths out jagged grid steps into direct straight-line vectors, moving at natural continuous angles. Waypoints on stairs, bridges, and elevation changes are preserved.

---

## 4. The IPA Stimulation System (Intelligence, Perception, Adrenaline)

Every agent possesses three bio-chemical stimulation meters that can be raised or lowered between 0% and 100%:

The stimulation multiplier ranges from **0.5x** (at 0% stimulation) to **1.0x** (at 50% neutral midpoint) up to **2.0x** (at 100% maximum stimulation):

* **Adrenaline**:
  - Scales movement speed via Leg cybernetics (up to 3.0x sprint speed).
  - High Adrenaline (>50%) introduces a slight aiming tremor at extreme levels.
  - Enemy agents automatically boost Adrenaline to 100% upon detecting player targets.
* **Perception**:
  - Extends visual scouting distance: expands awareness range from 6.25 tiles up to 25.0 tiles.
  - Improves weapon accuracy and tightens projectile spread, focusing bullet patterns into a dense line of fire.
* **Intelligence**:
  - Dictates AI decision-making speed and automatic target prioritization.
  - **Autonomous Combat Reaction** (active when Intelligence exceeds 55%):
    - **Reaction Time**: Reaction delay scales from 3.0 seconds at 60% Intelligence down to 0 seconds (instantaneous) at 100% Intelligence.
    - **Burst Fire Control**: Agents cease firing while their target is recoiling from bullet impact, conserving ammo and resuming the instant the target recovers.
    - **Target Switching**: If an engaged target enters recoil while another hostile is nearby, the agent instantly switches aim to the active threat, providing continuous crowd suppression.
    - **Police Engagement Restraint**: Agents holding fire avoid engaging police unless an officer unholsters a weapon or directly attacks.
    - **Player Manual Override**: Respects manual weapon selection until the selected weapon runs out of ammunition.

### Stimulation Decay & Cybernetic Retention

Stimulation levels naturally wear off toward the dependency baseline over time, while dependency creeps slowly toward the current drug dose. Higher versions of Heart, Eye, and Brain modifications extend drug retention on two distinct curves:

* **Effect Duration**: **1.0x** (Unaugmented), **2.0x** (Mod V1), **4.0x** (Mod V2), and **9.14x** (Mod V3).
* **Dependency Adaptation**: **1.0x** (Unaugmented), **1.30x** (Mod V1), **1.86x** (Mod V2), and **6.50x** (Mod V3).

### Initial NPC Stimulation Levels

Non-player characters (Police, Guards, Civilians, and Rival Agents) initialize their starting **Adrenaline**, **Intelligence**, and **Perception** levels from their mission record's dependency baseline and active effect (or preset lock value when assigned), rather than defaulting all NPCs to neutral 50%. Higher-tier mission NPCs therefore begin missions with elevated effective Perception and expanded detection radii.

---

## 5. Cybernetic Modifications Summary

| Body Slot | Modification Version | Primary Gameplay Effects                                                                                                                                                               |
|:--------- |:--------------------:|:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Legs**  | V1 / V2 / V3         | Scales Adrenaline sprint speed (1.90x, 2.45x, 3.00x) while preserving standard base walking speed at neutral Adrenaline. Adds a small accuracy bonus.                                  |
| **Arms**  | V1 / V2 / V3         | Increases payload carrying capacity (11,000, 21,000, 31,000 weight units), preventing speed penalties from heavy weapons. Adds a small accuracy bonus.                                |
| **Chest** | V1 / V2 / V3         | Deflects bullets probabilistically (50%, 75%, 83.3% deflected), mitigates Laser/Explosion/Collision/Burn damage by tier, and enables health regeneration (12.8s, 6.4s, 3.2s; 0.8s below 4% Adrenaline). |
| **Heart** | V1 / V2 / V3         | Extends Adrenaline drug effect retention (2.0x, 4.0x, 9.14x) and slows dependency adaptation (1.30x, 1.86x, 6.50x), plus adds a small accuracy bonus.                                    |
| **Eyes**  | V1 / V2 / V3         | Increases base shooting accuracy and extends Intelligence drug retention.                                                                                                              |
| **Brain** | V1 / V2 / V3         | Extends Persuadotron control range, reduces required persuasion points, enables Access Card authorization (V3), sharpens combat reaction times, and extends Perception drug retention. |

### Cyberware Tech Level Tiers

Enemy agents are equipped with cyberware according to predefined tech levels:

* **Tech Level 0 (Unaugmented)**: Standard human body with no cybernetics installed.
* **Tech Level 1**: Equipped with Version 1 Legs, Arms, Chest, Heart, Eyes, and Brain.
* **Tech Level 2**: Equipped with Version 2 Legs, Arms, Chest, Heart, Eyes, and Brain.
* **Tech Level 3**: Equipped with Version 3 Legs, Arms, Chest, Heart, Eyes, and Brain.

### Cyberware Resolution Hierarchy

Hostile agent equipment is resolved through a 3-tier hierarchy:

1. **Individual Agent Override**: Specific enemy agents can be configured with customized cyberware loadouts in mission data. This allows mission authors to design unique boss agents, heavily armored commandos, or precision snipers.
2. **Mission Default Tech Level**: Any agent without an individual override inherits the mission's default tech level (Tech Levels 0 through 3), configurable in the mission editor.
3. **Campaign Territory Progression**: When playing standard campaign missions without custom edits, the game automatically applies tech levels matching campaign difficulty progression (early territories feature unaugmented enemies, mid-campaign territories use Tech Level 1 and 2, and late-game strongholds deploy Tech Level 3 agents).

---

## 6. AI Behaviors & State Transitions

### A. Civilian Panic Behavior

* **Target Audience**: Civilians and unarmed bystanders.
* **Triggers**: Gunfire nearby, explosions, or armed hostiles drawing weapons within a detection radius scaled by the civilian's **Perception** level (up to 10.0 tiles / 2,560 units at maximum Perception, with a neutral baseline of 5.86 tiles / 1,500 units).
* **Behavior**: Unscripted civilians scream, turn away from the threat, and sprint 500 units in the opposite direction.
* **Panic Immunity & Script Exceptions**:
  - **Scripted Path Actors**: Civilians executing a mission script (such as a target heading to an escape car, or an escort VIP walking a planned route) remain focused on their objective and do not panic from nearby gunfire or drawn weapons.
  - **Vehicle Boarding & Passengers**: Civilians actively walking to board a vehicle, or seated inside a car or train, do not panic.
  - **Persuaded Followers**: Persuaded civilians obey squad commands and ignore civilian panic routines.
  - **Vehicle Hijacking / Attack Bailout**: If an escape vehicle carrying a scripted civilian driver or passenger is attacked and damaged, or carjacked by player agents, the passenger's script aborts, they exit the vehicle in panic, and they flee on foot.

### B. Police Behavior

* **Target Audience**: City police officers.
* **Triggers**:
  - **Warning State**: Drawing weapons in public alerts nearby police within a detection radius scaled by the officer's **Perception** level (up to 10 tiles at max Perception, varying across mission territories). Officers approach and monitor the suspect.
  - **Hostility & Engagement**: Firing weapons, injuring bystanders, or ignoring warnings triggers active combat engagement.
  - **Incoming Fire Retaliation**: Taking damage or being shot at triggers immediate return fire against the attacker.
* **Behavior**: Police unholster pistols or submachine guns, pursue suspects within range (scaled up to 10.0 tiles / 2,560 units by **Perception**, matching a 1,500-unit neutral baseline), and open fire if warnings expire or shots are exchanged.
* **Variable Disengagement Cooldown**: If an engaged suspect holsters or lowers their weapon while still alive, police do not disengage instantaneously. Instead, the officer enters a variable disengagement cooldown (~0.75 to 2.6 seconds, shorter or longer depending on **Adrenaline** level and encounter timing), during which the officer remains locked on, continues pursuit, and may still fire shots before standing down. If the suspect re-draws a weapon before the cooldown expires, disengagement cancels and combat resumes seamlessly.
* **De-escalation & Patrol Resumption**: If an active suspect is eliminated (killed or mortally hit), police cease fire immediately; otherwise they stand down once the disengagement cooldown completes. After a brief pause, if no other armed threats remain in the area, officers holster their weapons and resume their regular patrol routes.

### C. Rival Syndicate Agent & Guard Behavior

* **Corporate Factions & Visual Appearance**:
  - Rival agents are organized into corporate squads with distinctive uniform collar and head accents:
    - **Faction 0**: Player's Syndicate (accent based on company color selected).
    - **Faction 1**: Rival Syndicate Blue (Blue-grey accent).
    - **Faction 2**: Rival Syndicate Orange (Light red / orange accent).
    - **Faction 3 & 4**: Rival Syndicate Yellow (Yellow accent).
    - **Faction 5**: Rival Syndicate Purple (Purple accent).
    - **Faction 6**: Rival Syndicate Grey (Grey accent).
    - **Faction 7**: Rival Syndicate Cyan (Cyan / teal accent).
  - **Faction Agent Slots & Capacity (Max 8 per Faction)**: In the binary mission format (`gameXX.dat`), pedestrian faction identity is determined by slot index (`faction = ped_index / 8`). Each faction accommodates up to 8 agents (Slots 0..7 for Player's Syndicate, 8..15 for Blue, 16..23 for Orange, 24..31 for Yellow A, 32..39 for Yellow B, 40..47 for Purple, 48..55 for Grey, and 56..63 for Cyan). Slots 64..255 are designated for civilians, police, and platform guards. When spawning or reassigning agents in the editor, the engine automatically aligns agents into their designated faction slot blocks and relocates any occupying non-agents to free slots $\ge 64$ to preserve data integrity and prevent cross-faction slot contamination.
* **Hunter AI vs. Sentry AI**:
  - **Hunter AI**: Hunters actively track the player squad across the entire map from mission start, ignoring visual sight limits and bypassing normal reaction delays. In Combat Sweep missions, designated rival syndicate squads automatically act as Hunters (in American Revolt and wildcard sweep missions, all hostile rival factions participate in swarming the player squad; in vanilla Syndicate, initial hunter activation is governed by the mission's AI tactics header and faction limits). Tactical pursuit behaviors and staged tripwires are configured at the faction level in the AI Tactics Editor (accessible via the "Edit Faction AI Tactics..." button in the Entity Inspector or main menu).
  - **Sentry AI**: Sentry agents hold their assigned positions (or wander within their local sector if configured with unscripted Wandering Patrol `0x10`) until triggered. (In original Bullfrog Syndicate, enemy agents do not use scripted waypoint routes; their movement and tactical posture are governed strictly by Movement Mode and the Faction AI Tactics FSM). Sentries defend themselves and engage approaching hostiles (including player agents) who enter direct visual line of sight if they have an active combat or hunt tactic, or if attacked. In missions where squads have terminal tripwire tactics (such as in Mission 2 Far East for Factions 2, 3, and 4), the tripwire does not time out and advances to a combat sweep when the player squad breaches the proximity perimeter of any agent in that faction (every agent acts as an independent alert sensor without requiring a centralized squad leader). Approaching an outer sentry at street level detects the player and mobilizes the entire faction into combat sweep. If an unalerted sentry is attacked directly, they exhibit a 1.5–2 second reaction latency before returning fire. Line-of-sight checks take vertical elevation and torso height into account, ensuring elevated sentries on walkways and roofs have unobstructed downward sightlines.
  - **Weapon Wielding Distance**: When in tactical transit or pursuit, enemy agents keep their weapons holstered until approaching within weapon effective range plus a short 1-tile anticipation threshold (256 world units). This prevents them from prematurely drawing firearms and triggering police confrontations from across the city, allowing squads to run past outer sentries during cross-city tactical sweeps without premature skirmishing.
  - **Weapon Fallback**: If an enemy agent in mission data does not have an assigned weapon, they are automatically equipped with a loaded sidearm upon mission start, ensuring they can defend themselves immediately.
* **AI Tactics Finite State Machine (FSM)**:
  - **Multi-Stage Faction Behaviors**: Enemy squads can execute multi-stage tactical behavior chains rather than static behaviors. Missions support up to 127 linked tactic nodes across rival factions.
  - **Tactical Modes**:
    - **Seek & Destroy**: Continuous global pursuit of a target syndicate or player squad.
    - **Rival Sweep**: Tactical sweep toward rival syndicate encampments or designated map zones.
    - **Objective Sweep**: Coordinated movement toward mission key locations or objectives.
    - **Stationary Guard**: Disciplined defensive posture that holds position and engages only in immediate self-defense.
    - **Weapon Ambush**: Sentry ambusher that holds fire until a target moves into weapon lethal range with unobstructed line of sight.
    - **Patrol & Search**: Waypoint-based movement and searching across map coordinates.
    - **Proximity Tripwire**: Dual spatial perimeter sensor and sentry countdown timer. Evaluates exclusively against the downstream target faction in its tactical chain (ignoring third-party factions). Against the player squad, each sentry in the faction acts as an independent alert sensor (approaching any sentry within 12 tiles sounds the alarm and mobilizes the squad). For inter-faction rival AI encounters across the map, distance is anchored to the squad's central encampment. Triggers and advances the squad when the target faction breaches the proximity perimeter (either explicitly configured in tiles, or defaulting to an alert perimeter of 12 tiles), or when the stage countdown timer (where each tick represents approximately 1.35 seconds; e.g. 50 ticks is ~70s, 200 ticks is ~270s) expires. Distance checks are calculated with high numerical precision to prevent overflow at long ranges.
    - **Board Vehicle**: Directs squad members to seek out and enter vehicles.
    - **Evacuate**: Commands agents to retreat to designated extraction coordinates.
    - **Delay Timer**: Countdown delay before transitioning the faction to subsequent tactical nodes.
  - **Inter-Faction Warfare & Corporate Alliances**:
    - Rival corporate syndicates can be configured to target one another via tactical nodes. When sweeping a rival syndicate, agents march cohesively toward the rival squad leader anchor with firearms holstered until closing to weapon effective range.
    - Hostile agents prioritize active threats from their assigned tactical target faction. Tripwire and sentry agents from uninvolved factions (such as Faction 2 Orange while Factions 3 and 4 clash in Kazakhstan) ignore passing combatants unless directly threatened or their specific target is detected.
    - Chained tripwire nodes (such as Faction 2's staged tripwires in Kazakhstan) act as timed readiness stages: each stage counts down (~14 seconds), transitioning the faction in lockstep across all 8 agents (e.g. Faction 2 activates Combat Sweep at ~30s; Faction 1 activates at ~1m 10s).
    - Corporate alliances and rivalries are respected: squads belonging to the same faction will not fire upon one another, while distinct rival factions engage and eliminate one another according to their assigned tactical objectives.
    - Tactical nodes that specify a squad's own faction as the target act as defensive station-keeping orders, keeping the squad in a stationary sentry posture at key chokepoints rather than roaming.
    - Line-of-sight checks enforce that agents in inter-faction warfare do not target distant enemies through solid walls across the city. When all visible enemies from an assigned rival faction are neutralized or out of sight, surviving agents hold defensive positions as stationary sentries (clustering at their combat line) and engage approaching player agents only when they come into direct visual line of sight within normal perception range (~6 tiles).
* **Scripted Ambush Triggers**:
  - Guards placed in ambush positions monitor key map corridors using line of sight. When a player agent steps into view within range (15 tiles), the ambush activates: guards clear patrol routines, draw their best weapon, boost Adrenaline, and attack.
* **Priority Engagement**: Hostile agents and guards engage player agents on sight, whether the player's agents are actively brandishing weapons or walking unarmed.

### D. Persuaded Followers

* **Target Audience**: Any civilian, guard, police officer, or rival agent subjugated by the Persuadotron.
* **Required Persuasion Points**:
  - **Civilian / Criminal**: 1 point.
  - **Guard**: 2 points.
  - **Police Officer**: 4 points.
  - **Enemy Agent**: 8 points.
* **Behavior**: Subjugated followers discard their former routines, form a defensive escort cluster around the controlling squad leader, and automatically scan for and pick up dropped weapons on the ground to build up the player's armed force.
* **Weapon Synchronization**:
  - Followers only draw weapons when their squad leader equips a shooting firearm. When holding non-weapon utility devices (such as a Persuadotron, Scanner, or Medikit), followers keep their weapons holstered.
  - When the squad leader equips a combat weapon, all armed followers immediately draw their highest-ranked weapon. When the leader holsters or switches to a utility item, followers immediately holster their weapons.
  - If a follower's controlling agent is eliminated, followers remain stationary and holster their weapons.

### E. Traffic & Road Dynamics

* **Vehicle Yielding**: Civilian and NPC drivers continuously monitor the roadway ahead and stop smoothly if pedestrians are crossing their path. Vehicles wait until the roadway is clear before continuing forward.
* **Pedestrian Avoidance**: Pedestrians on foot scan for oncoming moving vehicles and take lateral sidesteps toward the sidewalk or open ground to clear the vehicle's path.
* **Player-Controlled Vehicles**: When a player agent takes the wheel of a vehicle, normal yielding rules are bypassed. The vehicle can plow through pedestrians in the roadway. Collisions with moving vehicles inflict lethal impact damage.