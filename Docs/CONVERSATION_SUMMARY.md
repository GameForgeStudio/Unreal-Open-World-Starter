# Conversation and Project Summary

## Purpose

Aurora began with a custom game engine and two earlier Neo Sakura prototypes: a browser/custom-runtime prototype and a Unity prototype. Those projects demonstrated the intended game and exposed the strengths and weaknesses of their respective engines. They are reference implementations, not Unreal projects.

The Unreal work began as an evaluation of whether Unreal Engine could supply the mature systems, tooling, physics, open-world support, and production foundation that the custom engine did not yet provide. The goal then became a runnable Sakura Tactics capability prototype—not merely a list of features.

The active Unreal project is:

- Folder: `C:\Projects\SakuraUnrealPrototype`
- Descriptor: `SakuraTactics.uproject`
- Engine: Unreal Engine 5.8.1 at `C:\UE_5.8`
- Default map: `/Game/SakuraPrototype/Maps/L_SakuraTestLab_WP`

The shorter descriptor name was chosen because Unreal repeatedly warns when a project filename exceeds 20 characters. Runtime module names were deliberately left unchanged to avoid breaking serialized `/Script/SakuraUnrealPrototype` class identities.

## Research and engine selection

The original evaluation compared the custom engine with Unreal in the same broad categories: rendering, world building, physics, animation, networking, editor tooling, content workflows, vehicles, movement, inventory, saving, extensibility, and production risk.

Unreal 5.8 was selected over 5.7 because Aurora had already started installing it and because the prototype could target the current engine while accepting that some important systems—especially Chaos Modular Vehicles and Mover—remain experimental.

Four mature paid-package categories were discussed as a way to buy years of authored behavior and tooling rather than recreate everything immediately:

- R-Tune Vehicle Physics Pro for vehicle physics and product-level vehicle tooling.
- Action Adventure Movement System V2 for broad, integrated traversal.
- Inventory Framework Plugin for a mature inventory/equipment/content workflow.
- Save Extension for persistence.

The rough package total was discussed as an investment in integrated features, examples, authoring tools, documentation, support, and edge-case testing—not merely source-code volume. Save Extension was acquired. The other areas were evaluated against free alternatives.

## Free alternatives evaluated

### Vehicles

- Chaos Modular Vehicles is included with Unreal 5.8 and provides Epic's modular vehicle simulation and network-physics foundation, but it is experimental.
- KinetiForge is an MIT-licensed source vehicle system with detailed suspension, tires, and drivetrain behavior. Its stock red sports car is now the preferred-feeling baseline based on Aurora's hands-on driving evaluation.
- An earlier plan proposed using Chaos as the sole runtime/network authority and porting selected KinetiForge algorithms into new Chaos modules. That is a legitimate future architecture, but it was attempted before a trustworthy stock baseline existed and was therefore stopped.
- Aurora explicitly reset the requirement: first prove one completely stock Chaos vehicle and one completely stock KinetiForge vehicle, with neither physics solver containing any part of the other.

### Movement

- ALS-Refactored was identified as a strong free Character Movement Component locomotion base.
- Epic's Game Animation Sample supplies high-quality motion-matching animations and examples, not a complete action-adventure traversal game system.
- Lyra supplies useful multiplayer, Gameplay Ability System, initialization, and animation-layer patterns, but is not a general traversal package.
- Mover is built into UE 5.8, but is experimental and intentionally isolated in this prototype.
- No free package was found that matched the breadth, integration, animation coverage, replication claims, and out-of-box usability of Action Adventure Movement System V2.

### Inventory

- No free, mature, exact-UE-5.8 package met the full Inventory Framework Plugin standard.
- Sigil Inventory 0.1.0 was selected as the closest permissively licensed UE 5.8 foundation. It is server-authoritative and broad, but very young, has no finished UI/content, and does not provide built-in GAS integration.
- Lyra remains an architecture reference, not a second inventory backend. Sakura uses one Sigil-owned inventory truth and project-side adapters.

### Saving

- Save Extension 1.5a is installed for UE 5.8.
- It is used as a file/slot service behind Sakura-owned persistence seams rather than being treated as automatic persistence for arbitrary third-party runtime state.

## Installed and enabled components

### Engine-wide or Marketplace components

- Unreal Engine 5.8.1.
- Save Extension 1.5a.
- Sigil Inventory 0.1.0.
- KinetiForge Vehicle System, built from source for UE 5.8.
- Built-in Chaos Modular Vehicle and its example content.
- Built-in Mover and Mover Examples.
- Built-in Gameplay Abilities, Enhanced Input, Modular Gameplay, and related dependencies.

### Project component

- VibeUE is installed under `Plugins/VibeUE` and provides an MCP bridge into the Unreal Editor.
- VibeUE improves editor inspection and automation access; it does not improve vehicle physics, terrain quality, gameplay output, or engineering judgment by itself.

KinetiForge and Sigil were installed engine-wide so they can be used by every UE 5.8 project. KinetiForge required UE 5.8 include/category compatibility work to compile, but the installed stock assets and solver are not to be edited for this comparison.

## Security incident and operating rule

During VibeUE account setup, sensitive account information and an API credential were displayed in a browser surface that was visible on Aurora's Twitch stream. The exposed key was revoked and recreated. No credential value is recorded in this repository.

The resulting permanent operating rule is:

- Never display, repeat, screenshot, log, or document API keys, email addresses, phone verification codes, or other credentials in a stream-visible surface.
- Use the user-designated browser and session when browser context matters.
- Treat credentials as secrets even when a setup workflow makes them visually accessible.

## Project creation and discovery

The Unreal prototype was created separately from both earlier prototypes. Initially, Epic Launcher and the Unreal Project Browser showed no projects because `C:\Projects` was not in UE 5.8's `CreatedProjectPaths`. The durable discovery root was added and the descriptor's engine association was normalized to `5.8`.

The project now opens directly as `SakuraTactics.uproject` and targets the local UE 5.8.1 installation.

## Reusable architecture

Reusable integration code lives in `Plugins/SakuraFramework`. Project-specific lab hosting, world actors, and demonstrations live in `Source/SakuraUnrealPrototype`.

The core authority rules are:

- One vehicle solver per vehicle.
- Standard Character Movement owns the main character; Mover never co-controls that pawn.
- Sigil owns inventory and equipment truth.
- GAS owns abilities, effects, and attributes.
- Save Extension owns save-file/slot operations; Sakura adapters own domain semantics.
- The shared Sakura interaction layer may handle doors, seats, possession, parking lifecycle, settings, and input translation, but it must not blend the two vehicle physics systems.

## Inventory and Gameplay Ability System prototype

GAS means Unreal's Gameplay Ability System. It manages replicated abilities, attributes, effects, activation rules, costs, and cooldowns.

The prototype currently proves:

- A Sigil-owned sword can move between Main and Equipped collections.
- Equipping applies an exact infinite Gameplay Effect, raises Attack Power, and grants a demo ability.
- Unequipping removes the same effect and granted ability.
- Potions apply a health effect and are consumed only when use succeeds.
- Inventory requests are server-authoritative.
- The HUD exposes the numeric result so the integration can be tested without a finished inventory screen.

This is not a finished inventory experience. It does not yet include a proper inventory window, icons, drag-and-drop, visible equipped items, pickups, shops, crafting presentation, or production persistence.

## Save Extension prototype

The standard character proof stores a grounded checkpoint transform and one scalar. `F5` saves; `F9` loads. The scalar can be changed with `PageUp` and `PageDown` to prove that non-transform data also returns.

The Mover proof restores through queued Mover teleport and velocity effects rather than writing a raw actor transform over Mover's simulation state.

The prototype does not claim persistence of Sigil inventory, equipped items, GAS effects, attributes, rollback history, midair movement, or online account data.

## Character Movement and Mover

“Character Movement” refers to Unreal's established `UCharacterMovementComponent` path used by the normal playable character. “Mover” is a newer experimental movement framework intended eventually to replace or supersede parts of Character Movement. Mover is not the vehicle system.

The main character uses Character Movement because it is the reliable baseline Aurora already liked. Pressing `M` reloads the lab into a separate Mover comparison GameMode. This keeps the authorities isolated and makes the comparison honest.

## Test-lab evolution

### Initial lab

The first scene used Unreal's template map and primitive geometry. It was too small, had no saved project map at first, provided poor instructions, and could not demonstrate open-world driving or the reasons Unreal was chosen.

The lab then gained:

- A saved project map and persistent HUD.
- Character, inventory, GAS, and save test stations.
- Enterable vehicles and controller mappings.
- Ramps, slalom, bumps, rough surfaces, garage pads, signs, and recovery logic.
- Independent simulated cones, mass-comparison blocks, restitution examples, a block wall, and Manny ragdoll targets.

Several early geometry choices were wrong. The campus foundation hovered above terrain, curbs had no sensible driveway opening, acceleration distance was inadequate, and the course could launch vehicles beyond the playable floor. Those failures drove the larger proving-ground requirement.

### Open-world proving ground

Aurora clarified that the relevant references are large contemporary open worlds and driving games such as Forza Horizon, GTA, and Watch Dogs—not a tiny physics courtyard.

The generated proving ground is an 8.128 km square World Partition landscape with a retained central systems campus, streamed terrain, routes, districts, ramps, bridges, recovery anchors, and multiple Player Starts. It is an engineering proving ground, not final world art.

The current terrain is synthetic. The requested future direction is to use real topographical/geospatial data for Minneapolis, St. Paul, and surrounding areas at an open-world scale. That real-world import has not yet been performed.

The project Game Bible and earlier prototypes still need a deliberate design pass before the proving ground becomes a representative Neo Sakura world rather than a generic capability test.

## Shader compilation and terrain-generation failures

Unreal spawned many Shader Compile Worker processes because it automatically parallelizes shader compilation for the available CPU. Numerous workers are normal, but their combined memory load became highly visible during terrain/material work.

The worker limit was temporarily reduced too aggressively after Aurora questioned the process count. Aurora had not requested a memory restriction. The important lesson is not to infer a resource policy from a question: retain Unreal's hardware-aware defaults unless a measured failure demonstrates the need for a controlled change.

Open-world generation also suffered several commandlet crashes and validation failures. Causes included:

- Loading skeletal vehicle actors during a headless World Partition validation path that had no render scene.
- Constructor-time physics calls before the engine was fully initialized.
- Validation treating directory timestamp drift as content mutation.
- An initially monolithic landscape-generation approach with excessive peak-memory risk.

The generator was reworked toward regional landscape writes, safer World Partition actor validation, stronger rollback, and collision/height/weight checks. The generated map ultimately loaded, but the first visible result did not meet Aurora's expectation because it looked too much like the old campus and initially lost the cars. That outcome is recorded as a product failure even where commandlets technically succeeded.

## Vehicle history

### Early hybrid attempt

The first vehicle plan used Chaos as the host and ported selected KinetiForge-derived input/tire behavior into project-side Chaos components. The intention was an algorithm port, not running two solvers simultaneously. However, the prototype exposed a modified/hybrid car before establishing that the exact stock systems behaved correctly.

The resulting vehicles suffered from visible and behavioral defects:

- Parked cars bounced, jittered, rolled, or could be launched by the player.
- Engines appeared to run while unoccupied.
- Vehicle entry and gamepad exit did not reliably work.
- Forward/reverse/brake input paths were inconsistent.
- Wheels appeared to separate, spin, smear, or visually glitch.
- An upside-down car continued flopping and spinning its wheels.
- Camera axes were inverted and accumulated rotation unwound rapidly when released.
- Steering and speed tuning were poor.
- Cars reacted unrealistically to curbs and standing ragdoll targets.

Aurora correctly rejected continuing the port on top of an untrusted baseline.

### Reset to independent stock baselines

The current explicit requirement is:

- One exact Epic stock Chaos Modular Vehicle example car.
- One exact KinetiForge stock RWD sports car.
- No KinetiForge solver, suspension, wheel, tire, or drivetrain code inside the Chaos car.
- No Chaos vehicle solver, suspension, wheel, tire, or drivetrain code inside the KinetiForge car.
- A shared project layer may provide interaction, seating, input mapping, parking/off lifecycle, settings, and HUD labels.

Aurora's current hands-on verdict is that the red KinetiForge car does not look as impressive but drives substantially better. That makes KinetiForge the current preferred driving baseline, while the Epic car remains an honest stock comparison.

The KinetiForge stock Blueprint contains an overhead migration billboard. Its important instruction was read: copy the plugin's entire `[CoreRedirects]` section into the project's `DefaultEngine.ini`, then resave migrated Blueprints. The 30 redirects were missing and have now been added to project configuration. The stock plugin Blueprint and its billboard were not edited or deleted.

### Shared vehicle behavior requirements

Aurora specified a data-driven seat system rather than a hard-coded four-seat assumption:

- On foot, `X` / `Square` is Activate.
- Face a vehicle door and activate to request the seat associated with that door.
- With auto-placement enabled and no driver, the system may choose the control seat and reposition the character.
- With auto-placement disabled, the faced door's primary seat is the only valid request. If occupied or blocked by a player, animal, NPC, cargo, or other reservation, entry is refused with no fallback.
- If a driver already exists, choose the closest available seat associated with the faced door, then the next configured seat only when auto-placement permits it.
- Seat definitions must support arbitrary future vehicle layouts.
- The auto-placement option must be a persisted player setting and later appear as a settings-menu checkbox.

Driving controls follow the requested Forza Horizon-style core layout:

- `RT`: accelerator.
- `LT`: brake while moving forward, then reverse from a stop.
- Left stick: steering.
- Right stick: chase camera.
- `A` / `Cross`: handbrake.
- `X` / `Square`: start the engine; accelerator input may also start it.
- `B` / `Circle`: exit at a full stop.
- Holding `B` / `Circle` for three seconds while moving is intended eventually to bail out, preserving momentum and transitioning the character into physics/ragdoll. That moving bailout is a requirement, not a currently production-proven feature.

Vehicle behavior must be state-appropriate. An unoccupied ordinary car should be engine-off and parked. A vehicle under AI, remote, Matrix, hacked, autopilot, hover, or scripted control may legitimately continue operating without a physically seated driver. Exiting at speed must never erase momentum arbitrarily.

## Physics-showcase correction

The initial standing Manny target used a non-simulating, blocking capsule. On impact, the vehicle first struck that effectively infinite-mass proxy, rebounded, and only afterward activated the ragdoll. That is why both vehicle and person reacted incorrectly.

The latest compiled correction makes the target detect an approaching Sakura-managed vehicle and activate its actual skeletal ragdoll before first blocking contact. The standing capsule ignores the Vehicle channel, and no Chaos or KinetiForge solver code was changed. This correction compiled successfully but still requires hands-on runtime acceptance in the editor.

## Current controls

### On foot

- `WASD` / left stick: move.
- Mouse / right stick: look.
- `Space` / `A` / `Cross`: jump.
- `F` or `X` / `Square`: Activate vehicle entry.
- `E`: equip demo sword.
- `Q`: unequip demo sword.
- `U`: use potion.
- `P`: activate granted demo ability.
- `F5` / `F9`: save/load checkpoint.
- `PageUp` / `PageDown`: change save-test scalar.
- `M`: switch between standard Character Movement and isolated Mover mode.

### In a vehicle

- `W` / `RT`: accelerate.
- `S` / `LT`: brake and automatic reverse.
- `A`/`D` or left stick: steer.
- `Space` or `A` / `Cross`: handbrake.
- Right stick: camera.
- `X` / `Square`: ignition.
- `B` / `Circle`: stopped exit.
- `F`: keyboard exit.

## Current status

### Working or compiled

- UE 5.8.1 project discovery and launch.
- Reusable Sakura Framework modules.
- Standard Character Movement baseline.
- Isolated Mover comparison.
- Sigil inventory-to-GAS equipment and consumable proof.
- Save Extension grounded checkpoint/scalar proof.
- World Partition proving-ground map and central capability campus.
- One stock Chaos car and one stock KinetiForge car with independent solvers.
- Shared data-driven door/seat and input layer.
- KinetiForge `[CoreRedirects]` project configuration.
- Pre-contact ragdoll activation correction compiles.

### Not yet accepted as production behavior

- Final handling, speed, dead zones, steering, curb response, handbrake hold, parked stability, upside-down behavior, wheel presentation, and camera behavior across both vehicles.
- Full moving bailout and momentum-to-ragdoll transition.
- Door opening, interiors, seated character animation, and entry/exit animation.
- Vehicle-versus-pedestrian mass/impulse tuning after the latest collision correction.
- Multiplayer vehicle occupancy, recovery, relevancy, and authoritative ragdoll pose synchronization.
- Finished inventory UI and complete inventory persistence.
- Production Mover traversal stack.
- Real Minneapolis–St. Paul topographical terrain import and representative open-world content.
- A Game Bible-driven production slice.

## Decisions that must not silently regress

- Do not mix the stock Chaos and KinetiForge solvers.
- Do not resume a KinetiForge-to-Chaos port until both stock baselines are trusted and Aurora explicitly authorizes the next port slice.
- Do not modify stock plugin assets merely to hide migration notices or make the comparison look cleaner.
- Do not let Character Movement and Mover control the same pawn.
- Do not create a second inventory authority beside Sigil.
- Do not claim Save Extension automatically persists inventory, GAS, Mover internals, or online accounts.
- Do not treat a successful compile, cook, or commandlet sentinel as proof that the visible experience meets the requirement.
- Do not infer memory limits, worker counts, vehicle behavior, or architectural choices from a question; obtain authorization when behavior would change.
- Do not expose credentials or personal information through a stream-visible browser or tool surface.

## Recommended next sequence

1. Restart Unreal so the newly added KinetiForge redirects are loaded.
2. Runtime-test the latest pedestrian/ragdoll collision correction with the stock KinetiForge car first, then the stock Chaos car.
3. Record separate stock-baseline acceptance results for parking, ignition, forward/reverse/brake, stopped exit, camera, wheel presentation, curb response, rollover, and pedestrian impact.
4. Fix only shared interaction/presentation defects or defects proven to belong to the relevant stock integration. Do not blend solvers.
5. Read the Game Bible and earlier Neo Sakura prototypes deliberately before expanding the proving ground into representative game content.
6. Plan a real geospatial/topographical Minneapolis–St. Paul terrain pipeline as a distinct, sourced world-building task.
7. Only after the stock A/B report is accepted should Aurora choose whether to adopt KinetiForge, buy R-Tune, or authorize a precisely scoped algorithm port.

## Historical interpretation

This prototype has already served its most important purpose: it exposed the difference between a technically compiling integration and a convincing playable result. The durable direction is to preserve clean authorities, establish honest stock baselines, test visibly, and make one authorized change at a time.
