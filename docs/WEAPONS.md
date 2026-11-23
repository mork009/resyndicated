# ReSyndicated - Weapon Data & Property Reference (`weapons.dat`)

This document provides a comprehensive overview of the weapon configuration system in **ReSyndicated**. All weapon characteristics, economic costs, ballistic attributes, area-of-effect parameters, and encumbrance metrics are defined in [`data/ref/weapons.dat`](../data/ref/weapons.dat).

---

## 1. Core Mechanics & Units of Measurement

* **World & Distance Units**: 
  - **1 Map Grid Tile = 256 World Units**.
  - Target ranges, movement speeds, and explosion radii are specified in world units. For example, a range of `1280` corresponds to `5` map tiles (`1280 / 256`), while `4096` equals `16` tiles.
* **Time Units**: All durations (wind-up delays, cooldowns, and automatic fire repeat rates) are measured in **milliseconds (ms)**.
* **Damage & Cybernetic Defense**:
  - **Bullet Weapons**: Inflict 1 HP per round (or per pellet for multi-impact weapons). Equipped **Chest Mods** deflect incoming bullets probabilistically: V1 deflects 50% (50% hit chance), V2 deflects 75% (25% hit chance), and V3 deflects 83.3% (16.7% hit chance).
  - **Laser, Explosions & Vehicle Collisions**: Instant kill against unaugmented agents and Chest V1; reduced to 8 HP per hit on Chest V2 and 4 HP per hit on Chest V3.
  - **Flamer (Burn)**: Instant kill against unaugmented agents; reduced to 8 HP per hit on Chest V1, 4 HP on Chest V2, and 2 HP on Chest V3.
  - Active **Energy Shields** completely absorb bullet and laser damage.
* **Accuracy, Spread & Perception**:
  - Dispersion cone angle is defined in degrees.
  - Bullet spread scales dynamically with the shooter's **Perception** cybernetic level. Higher Perception tightens the spray pattern into a dense stream of fire.
* **Encumbrance**:
  - Each weapon has a weight rating (ranging from 20 for a Pistol up to 3,500 for a Flamer). Carrying equipment beyond an agent's Arm strength capacity (1,000 unaugmented; +10,000 per Arm Mod tier) progressively reduces movement speed down to a minimum crawl.

---

## 2. Weapon Property Dictionary

The following table details every configuration key present in `weapons.dat`:

| Configuration Key | Data Type | Units | Description & Gameplay Effect |
| :--- | :--- | :--- | :--- |
| `weapon.<ID>.name` | String | Key | Localization message key used for displaying weapon names in UI menus, tooltips, and mission briefings. |
| `weapon.<ID>.icon.small` | Integer | Sprite Index | Index of the 24x24 pixel icon sprite displayed in the in-game squad inventory bar and HUD. |
| `weapon.<ID>.icon.big` | Integer | Sprite Index | Index of the large weapon sprite rendered on the Armory / Equipment Purchase screen. |
| `weapon.<ID>.cost` | Integer | Credits | Purchase price in credits required to buy 1 unit of the weapon in the pre-mission Armory. |
| `weapon.<ID>.ammo.nb` | Integer | Rounds | Maximum ammunition capacity (rounds per magazine/battery). |
| `weapon.<ID>.ammo.price` | Integer | Credits | Cost in credits per single round of ammo when re-arming weapons between missions. Total reload cost = `(capacity - remaining) * price`. |
| `weapon.<ID>.range` | Integer | World Units | Maximum effective attack range (`256 units = 1 tile`). Targets beyond this range cannot be engaged. |
| `weapon.<ID>.rank` | Integer | Index | Power/technology rank used for inventory auto-sorting and AI priority selection (0 = Gauss Gun, 1 = Pistol, 6 = Minigun, 7 = Laser). |
| `weapon.<ID>.anim` | Integer | Sprite ID | Base sprite animation ID used when the weapon is lying dropped on the ground as a pick-up item. |
| `weapon.<ID>.ammopershot` | Integer | Rounds | Ammunition consumed per single attack trigger/cycle (e.g., Flamer uses 10, Minigun uses 5 per burst). |
| `weapon.<ID>.timeforshot` | Integer | Milliseconds | Optional firing execution delay / wind-up time. |
| `weapon.<ID>.bomb.explosiondelay` | Integer | Milliseconds | Detonation countdown delay timer for Time Bomb (20,000 ms = 20s). |
| `weapon.<ID>.reloadtime` | Integer | Milliseconds | Cooldown delay between consecutive shots. For Time Bomb, specifies interval between audio tick sounds. |
| `weapon.<ID>.damagerange` | Integer | World Units | Area-of-effect (AOE) explosion blast radius. `0` for single-target direct impact weapons; `512` (2 tiles) for Gauss Gun, Laser blast, and Time Bomb. |
| `weapon.<ID>.shotanglemin` | Double | Degrees | Minimum angular dispersion cone at max Perception (100%) and full cybernetic mods. |
| `weapon.<ID>.shotanglemax` | Double | Degrees | Maximum angular dispersion cone at zero Perception (0%) and no cybernetic mods. |
| `weapon.<ID>.shotdepthmin` | Double | World Units | Minimum front-to-back longitudinal scatter at max Perception (100%) and full mods (typically `0`). |
| `weapon.<ID>.shotdepthmax` | Double | World Units | Maximum front-to-back longitudinal scatter at zero Perception (0%) and no mods. |
| `weapon.<ID>.shotspeed` | Integer | Units/Sec | Velocity of physical moving projectile shots (e.g., Gauss rocket = 6000, Flamer stream = 1000). Set to `0` for instant raycast hits. |
| `weapon.<ID>.dmg_per_shot` | Integer | HP | Base hit-point damage inflicted per impact/pellet prior to armor mitigation. |
| `weapon.<ID>.ammo.impactNb` | Integer | Count | Number of individual impact damage hits created per fired round (e.g., Shotgun = 6 pellets, Minigun = 5 impacts, Gauss = 10 secondary cluster impacts). |
| `weapon.<ID>.weight` | Integer | Weight Units | Equipment weight added to agent payload encumbrance. |
| `weapon.<ID>.auto.fire_rate` | Integer | Milliseconds | Optional repeat interval when holding down continuous automatic fire (Uzi = 70ms, Minigun = 50ms, Flamer = 150ms). |

---

## 3. Weapon Catalogue Overview

| ID | Weapon Name | Cost (Credits) | Damage / Shot | Range (Tiles) | Ammo Cap | Weight | Fire Rate | Primary Use / Characteristics |
| :---: | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **1** | **Pistol** | Free ($0) | 1 HP | 1,280 (5.0) | 13 | 20 | Single | Starting sidearm. Lightweight, reliable accuracy. |
| **2** | **Gauss Gun** | $50,000 | 64 HP × 10 | 5,120 (20.0) | 3 | 3,000 | Single | Heavy rocket launcher. Cluster impacts, 2-tile AOE blast. |
| **3** | **Shotgun** | $250 | 1 HP × 7 | 1,024 (4.0) | 12 | 90 | Single | Wide 45° spread cone producing 7-pellet shells. |
| **4** | **Uzi** | $750 | 1 HP | 1,792 (7.0) | 50 | 150 | 70ms | Rapid submachine gun. Good automatic suppressive fire. |
| **5** | **MiniGun** | $10,000 | 1 HP × 5 | 2,816 (11.0) | 500 | 3,000 | 50ms | Heavy rotary cannon. High rate of fire, high DPS, heavy weight. |
| **6** | **Laser** | $35,000 | 32 HP | 4,096 (16.0) | 5 | 2,500 | Single | High-energy beam rifle. Vaporizes standard targets, 2-tile blast. |
| **7** | **Flamer** | $1,500 | 8 HP | 512 (2.0) | 1000 | 3,500 | 150ms | Projectile flame stream. Short-range close assault weapon, ignores energy shields. |
| **8** | **Long Range** | $1,000 | 1 HP | 6,144 (24.0) | 30 | 100 | Single | Sniper rifle. Extreme range (24 tiles), pinpoint accuracy. |
| **9** | **Scanner** | $500 | N/A | 4,096 (16.0) | ∞ | 30 | Single | Tactical radar utility. Reveals peds, pings hostiles & objectives. |
| **10** | **Medikit** | $500 | Heal | 256 (1.0) | 1 | 50 | Single | Single-use field medical kit. Fully restores agent health. |
| **11** | **TimeBomb** | $25,000 | 64 HP | 1,000 (3.9) | ∞ | 500 | Single | Deployable explosive charge with a 20-second countdown timer. |
| **12** | **AccessCard** | $1,000 | Key | 256 (1.0) | ∞ | 20 | Single | Security pass key required to unlock electronic security gates. |
| **13** | **Energy Shield** | $8,000 | Defend | 768 (3.0) | 200 | 180 | 75ms | Personal barrier. Consumes battery power to block bullets/lasers. |
| **14** | **Persuadatron** | $5,000 | Control | 256 (1.0) | ∞ | 100 | Single | Mind-control device. Subjugates civilians, guards, police, & enemy peds. |
| **15** | **Air Raid Com** *(AR)* | $5,000 | 800 HP | 5,120 (20.0) | ∞ | 100 | 1000ms | American Revolt air raid communicator. 5-plane V-formation minigun strafe followed by carpet bombing. $50,000 per call. |
| **16** | **Clone Shield** *(AR)* | $9,000 | Disguise | 768 (3.0) | 200 | 180 | 180ms | American Revolt holographic civilian disguise. Auto-activates when selected. |

---

## 4. Detailed Weapon & Equipment Profiles

### 1. Pistol
* **ID**: `1` | **Cost**: $0 | **Ammo Capacity**: 13 rounds ($0/round)
* **Stats**: Range: 1,280 (5.0 tiles) | Damage: 1 HP | Spread Angle: 1.0° – 15.0° | Spread Depth: 0 – 32 units | Cooldown: 100ms | Weight: 20
* **Mechanics**: Starting sidearm. Lightweight, reliable accuracy.

### 2. Gauss Gun
* **ID**: `2` | **Cost**: $50,000 | **Ammo Capacity**: 3 rounds ($15,000/round)
* **Stats**: Range: 5,120 (20.0 tiles) | Damage: 64 HP × 10 | AOE Radius: 512 (2.0 tiles) | Spread Angle: 0.5° – 15.0° | Spread Depth: 0 – 32 units | Projectile Speed: 6000 | Cooldown: 1500ms | Weight: 3,000
* **Mechanics**: Heavy rocket launcher. Cluster impacts, 2-tile AOE blast.

### 3. Shotgun
* **ID**: `3` | **Cost**: $250 | **Ammo Capacity**: 12 rounds ($2/round)
* **Stats**: Range: 1,024 (4.0 tiles) | Damage: 1 HP × 7 | Spread Angle: 10.0° – 20.0° | Spread Depth: 5 – 128 units | Cooldown: 200ms | Weight: 90
* **Mechanics**: Wide 45° spread cone producing 7-pellet shells.

### 4. Uzi
* **ID**: `4` | **Cost**: $750 | **Ammo Capacity**: 50 rounds ($2/round)
* **Stats**: Range: 1,792 (7.0 tiles) | Damage: 1 HP | Spread Angle: 5.0° – 20.0° | Spread Depth: 5 – 128 units | Fire Rate: 70ms | Weight: 150
* **Mechanics**: Rapid submachine gun. Good automatic suppressive fire.

### 5. MiniGun
* **ID**: `5` | **Cost**: $10,000 | **Ammo Capacity**: 500 rounds ($10/round)
* **Stats**: Range: 2,816 (11.0 tiles) | Damage: 1 HP × 5 | Spread Angle: 5.0° – 20.0° | Spread Depth: 5 – 128 units | Fire Rate: 50ms | Weight: 3,000
* **Mechanics**: Heavy rotary cannon. High rate of fire, high DPS, heavy weight.

### 6. Laser
* **ID**: `6` | **Cost**: $35,000 | **Ammo Capacity**: 5 rounds ($2,000/round)
* **Stats**: Range: 4,096 (16.0 tiles) | Damage: 32 HP | AOE Radius: 512 (2.0 tiles) | Spread Angle: 0.5° – 5.0° | Spread Depth: 0 – 32 units | Cooldown: 200ms | Weight: 2,500
* **Mechanics**: High-energy beam rifle. Vaporizes standard targets, 2-tile blast.

### 7. Flamer
* **ID**: `7` | **Cost**: $1,500 | **Ammo Capacity**: 1000 rounds ($1/round)
* **Stats**: Range: 512 (2.0 tiles; ~468–1,056 effective with muzzle offset & wind) | Damage: 8 HP | Spread Angle: 5.0° – 20.0° | Spread Depth: 0 units | Projectile Speed: 2400 | Fire Rate: 150ms | Weight: 3,500
* **Mechanics**: Projectile flame stream. Spawns slightly ahead of the barrel and travels in rapid wind-affected substeps, causing reach and curve to vary with ambient wind direction (~1.8–4.1 tiles); ignores energy shields.

### 8. Long Range
* **ID**: `8` | **Cost**: $1,000 | **Ammo Capacity**: 30 rounds ($2/round)
* **Stats**: Range: 6,144 (24.0 tiles) | Damage: 1 HP | Spread Angle: 0.5° – 10.0° | Spread Depth: 0 – 64 units | Cooldown: 400ms | Weight: 100
* **Mechanics**: Sniper rifle. Extreme range (24 tiles), pinpoint accuracy.

### 9. Scanner
* **ID**: `9` | **Cost**: $500 | **Ammo Capacity**: 1 rounds ($0/round)
* **Stats**: Range: 4,096 (16.0 tiles) | Weight: 30
* **Mechanics**: Tactical radar utility. Reveals peds, pings hostiles & objectives.

### 10. Medikit
* **ID**: `10` | **Cost**: $500 | **Ammo Capacity**: 1 rounds ($1/round)
* **Stats**: Range: 256 (1.0 tiles) | Weight: 50
* **Mechanics**: Single-use field medical kit. Fully restores agent health.

### 11. TimeBomb
* **ID**: `11` | **Cost**: $25,000 | **Ammo Capacity**: 1 rounds ($1/round)
* **Stats**: Range: 1,000 (3.9 tiles) | Damage: 64 HP | AOE Radius: 512 (2.0 tiles) | Cooldown: 800ms | Weight: 500
* **Mechanics**: Deployable explosive charge with a 20-second countdown timer.

### 12. AccessCard
* **ID**: `12` | **Cost**: $1,000 | **Ammo Capacity**: 1 rounds ($0/round)
* **Stats**: Range: 256 (1.0 tiles) | Weight: 20
* **Mechanics**: Security pass key required to unlock electronic security gates.

### 13. Energy Shield
* **ID**: `13` | **Cost**: $8,000 | **Ammo Capacity**: 200 rounds ($15/round)
* **Stats**: Range: 768 (3.0 tiles) | Fire Rate: 75ms | Weight: 180
* **Mechanics**: Personal barrier. Consumes battery power to block bullets/lasers.

### 14. Persuadatron
* **ID**: `14` | **Cost**: $5,000 | **Ammo Capacity**: 1 rounds ($0/round)
* **Stats**: Range: 256 (1.0 tiles) | Cooldown: 25ms | Weight: 100
* **Mechanics**: Mind-control device. Subjugates civilians, guards, police, & enemy peds.

### 15. Air Raid Com (American Revolt)
* **ID**: `15` | **Cost**: $5,000 | **Ammo Capacity**: Unlimited ($50,000 syndicate funds deducted per call)
* **Stats**: Range: 5,120 (20.0 tiles) | Bomb Damage: 800 HP (non-incendiary) | Bullet Strafe Damage: 35 HP | Blast Radius: 380 units | Coverage Tract: 2,800 x 2,600 units (~11 x 10 tiles) | Bombing Sweep Duration: 6–7 seconds (sprites last 4–5s each) | Cooldown: 1000ms | Weight: 100
* **Mechanics**: Tactical air strike transponder ("AIR RAID COM"). Verifies syndicate funds ($50,000 required, displaying "NEED 50,000" in yellow text on the status bar if insufficient) and begins a 10-second countdown ("AIR RAID T-9" down to "T-0"). Does not consume physical weapon ammo and does not auto-switch weapons, allowing repeated calls as long as syndicate funds remain. Spawns a 5-plane V-formation pass approaching from a randomized inward angle based on target quadrant. As the planes enter the target coverage tract (2,800 units length x 2,600 units width centered at the clicked coordinates), they lay down 5 distinct clusters of minigun bullet fire (with 3 parallel rows per cluster) in a sweeping V-formation. Staggered behind the bullet pass, as the bullet fire reaches the far end of the tract, high-explosive bomb detonations begin at the tract entrance and progressively roll through the area over 6–7 seconds in an advancing V-formation with random timing variations (some explosions a couple tiles ahead/behind starting concurrently, with 100% full coverage across every tile). Each explosion sprite lasts approximately 4–5 seconds before smoke dissipates, inflicting 800 HP blast damage without setting peds on fire. Requires American Revolt data assets.

### 16. Clone Shield (American Revolt)
* **ID**: `16` | **Cost**: $9,000 | **Ammo Capacity**: 200 units ($15/unit)
* **Stats**: Range: 768 (3.0 tiles) | Drain Rate: 1 unit / 180ms | Cooldown: 100ms | Weight: 180
* **Mechanics**: Holographic civilian disguise projector. Automatically activates when equipped in the agent's inventory without requiring weapon fire triggers, disguising the agent as an unarmed civilian, suppressing corporate collar recoloring, and preventing civilians, police, and guards from recognizing the agent as a hostile threat. Enemy syndicate agents are equipped with cybernetic optics that see through the clone disguise and will actively engage. Drains battery at 1 unit per 180ms while equipped and progressively recharges when unequipped. Requires American Revolt data assets.
