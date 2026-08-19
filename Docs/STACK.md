# OWS Full-Stack Prototype

## Authority boundaries

| Domain | Sole runtime authority | Donors or adapters | Explicitly forbidden combination |
|---|---|---|---|
| Vehicle A/B test | One Epic stock Chaos Modular Vehicle and one stock KinetiForge RWD sports car | Shared OWS door/seat, parked/off lifecycle, and Forza-style input adapter only | Mixing either vehicle's solver, suspension, wheels, or drivetrain with the other |
| Character locomotion | Standard Character Movement in the main prototype | Mover is an isolated comparison | Character Movement and Mover controlling the same pawn |
| Inventory ownership | Sigil Inventory | OWS validation, UI, GAS, and save bridges | A second item/equipment backend holding duplicate truth |
| Abilities and attributes | Unreal Gameplay Ability System | Sigil equipment applies/removes exact effect handles | Client-side equipment callbacks granting authoritative effects |
| Persistence | Save Extension file/slot service | OWS domain adapters | Raw save restoration writing over Mover simulation state |

## Current stock-vehicle baseline

The Chaos garage car instantiates Epic's exact `BP_ModularVehicleSimplifiedSkeletalSetup`. The other instantiates KinetiForge's exact shipped `BP_SportsCar`. Their physics systems remain independent and unmodified.

OWS currently adds only the project-level behavior needed to test that stock vehicle in the game: door and seat selection, possession, a locked parked/off state, collision-safe exit placement, and direct keyboard/gamepad input submission to the stock `UModularVehicleBaseComponent`. The parking lock is released by the first accelerator request; exiting parks the car again. `bKeepVehicleAwake` is false while parked and true while driving.

KinetiForge is enabled for its stock comparison car. The earlier OWS Kineti-derived Chaos modifier and wheel component are not instantiated by either car.

The startup test lab places both stock cars in the garages. On foot, face an in-range configured door and use keyboard `F` or gamepad `X` / `Square` to Activate. Vehicle controls use the approved core layout: `RT` accelerates, `LT` brakes while moving forward and reverses from a stop, the left stick steers, the right stick controls the chase camera, and `A` / `Cross` applies the handbrake. Keyboard equivalents are `W`, `S`, `A`/`D`, and `Space`. The engine remains logically off after entry and starts only when the driver presses gamepad `X` / `Square` or first uses a forward/reverse accelerator. Gamepad `B` / `Circle` exits when the vehicle is stopped; keyboard `F` remains the lab exit key.

Door and seat layouts are data rather than a four-seat assumption. Runtime occupancy stores a generic replicated `AActor`, so players, NPCs, animals, or cargo can own seats. Physical clearance also rejects unregistered query-colliding actors/objects occupying the seat volume; no-collision occupants and reserved cargo must call the generic `OccupySeat` seam. With auto-placement enabled and no current driver, an available control seat is preferred even when another door was faced. Once a driver exists, the faced door's ordered seats are considered in nearest-first order. With auto-placement disabled, only that door's primary configured seat is considered; if it is occupied, blocked, missing, or otherwise invalid, entry is rejected without fallback or repositioning. `UOWSUserSettings` persists this preference now; a future settings-menu checkbox can call its Blueprint setter without changing the seat-selection architecture.

## Inventory and GAS

GAS is Unreal’s Gameplay Ability System. It owns replicated abilities, attributes, gameplay effects, costs, cooldowns, and activation rules. In this prototype, Sigil owns the item and equipment lifecycle; OWS validates player requests; equipment applies one exact infinite Gameplay Effect; that effect modifies an attribute and grants a demo ability; unequipping removes the exact effect and granted ability.

The prototype uses a Pawn-owned Ability System Component because it is the smallest runnable vertical slice. A production decision between Pawn and PlayerState ownership remains deliberately open.

The deterministic lab sequence is:

| Step | Health | Attack Power | Main quantity | Equipped quantity | Expected result |
|---|---:|---:|---:|---:|---|
| Initial | 50 / 100 | 10 | 4 | 0 | One sword and three potions are in Main. |
| `E` | 50 / 100 | 20 | 3 | 1 | Sword equipment applies its infinite effect and grants the pulse ability. |
| `P` | 50 / 100 | 25, then 20 | 3 | 1 | Granted pulse adds 5 Attack Power for two seconds. |
| `Q` | 50 / 100 | 10 | 4 | 0 | Unequip removes the exact effect and granted ability. |
| First `U` | 75 / 100 | 10 | 3 | 0 | One potion is consumed for 25 Health. |
| Second `U` | 100 / 100 | 10 | 2 | 0 | A second potion is consumed. |
| Another `U` | 100 / 100 | 10 | 2 | 0 | Full Health rejects use without consuming the remaining potion. |

This is a server-authoritative lifecycle and attribute proof. It is not a finished inventory experience: there is no inventory window, item iconography, drag/drop, world pickup, visible equipped sword actor, target dummy, damage model, or ability VFX.

## Save Extension

Save Extension is integrated behind OWS-facing save/checkpoint code. General actor state uses its save-slot service. Mover restoration uses Mover’s queued teleport and velocity effects so the simulation state, rather than a raw actor transform, owns the resulting position.

The first Mover save proof is grounded-only. It does not claim to serialize rollback buffers, layered moves, midair state, vaults, ziplines, or World Partition streaming state.

The standard Character proof persists its grounded transform and one scalar. The Mover proof persists its bridge component and restores through Mover effects. Sigil inventory, equipped state, GAS effects/attributes, and online account persistence are deliberately not claimed by this slice; those need a separate OWS persistence contract rather than assuming that Save Extension understands third-party runtime state automatically.

The prototype save hotkeys are appropriate for standalone evaluation. Save Extension performs a server/world slot operation, so this is not yet a per-player online save service and should not be exposed to arbitrary multiplayer clients without authorization and rate limiting.

To test the standard Character proof, change the scalar with `PageUp` or `PageDown`, stand on the ground at a recognizable marker, and press `F5`. Move elsewhere and change the scalar again, then press `F9`; both position and the previously saved scalar should return. `F5` while airborne must be rejected. Loading does not roll back inventory, equipment, Health, Attack Power, or other GAS state.

Pressing `M` reloads the same map with the isolated Mover GameMode. In that mode, `F5` accepts only strict Walking/grounded state and `F9` restores by queueing a Mover teleport followed by zero velocity in Walking mode. Press `M` to reload the standard lab. The standard mode resets Save Extension's active slot before play so the Mover comparison slot policy cannot leak into the Character proof.

## Runnable modes

| Mode | Selection | Purpose |
|---|---|---|
| Open-world proving ground | Project default: `/Game/OWSPrototype/Maps/L_OWSTestLab_WP` with `/Script/OWS.OWSTestLabGameMode` | 8.128 km World Partition terrain containing the central Character Movement, Sigil, GAS, Save Extension, physics-showcase, and two enterable stock-vehicle baselines plus streamed driving routes and test districts |
| Isolated Mover lab | Press `M`; `/Script/OWS.OWSTestLabMoverGameMode` | Same course with the experimental Mover pawn and state-aware grounded restore; press `M` to return |

The lab HUD remains present during vehicle possession by reading the controller's preserved home Character. Entering a vehicle hides and disables the same Character instance rather than creating another inventory/GAS authority; exiting returns control to that instance beside the car.

The campus physics lane contains 26 independent 2 kg cones, equal-size 10/100/1000 kg mass cubes, equal-mass low/high-restitution spheres, 32 independently simulated wall blocks, and two finite-mass Manny impact targets that activate their physics-asset ragdolls. Dynamic props block vehicles and pawns but ignore the `ECC_WorldDynamic` suspension query used by the stock modular vehicle so a cone cannot masquerade as drivable ground.

## Test lab limitations

- The central systems campus is an asset-light capability harness made from built-in primitive geometry and Epic example vehicle/mannequin assets. The generated World Partition proving ground adds terrain and test districts, but it is still engineering geometry rather than representative final art.
- Door facing, arbitrary seat definitions, generic replicated occupancy, driver/passenger assignment, and collision-safe exit placement are current logic seams. Authored door motion, transition animations, visible seated poses, vehicle inventory, damage, fuel, and ownership gameplay remain future presentation/gameplay layers.
- The two current cars deliberately use different, isolated stock solvers: Epic Chaos and KinetiForge.
- Mover remains a separate pawn and GameMode. Character Movement and Mover are never attached to or controlling the same pawn.
- Mover save scope excludes async backend restore, velocity, non-Walking modes, layered moves, movement modifiers, rollback buffers, queued effects, animation/montage state, vaults, ziplines, and World Partition streaming state.
- Standard saves exclude inventory, equipment, GAS effects/attributes, and account persistence. The current hotkeys are not an authorized per-player online persistence service.
- Pressing `M` is a lab-wide server-travel operation. Remote clients cannot initiate it until an explicit host/admin permission model exists.
- Switching modes reloads the map and deliberately resets unsaved transient character, inventory, GAS, and vehicle state.
- Vehicle possession is a standalone-scoped seam. OWS now replicates a seated-presentation state and reapplies Character collision enablement on clients, but the revised enter/exit path and all multi-client relevancy, recovery, and contention behavior still require runtime validation.
- Ragdoll activation and its initial impact are replicated, but the evolving per-bone Chaos pose is not an authoritative networked-ragdoll implementation; late relevance and late-join pose convergence remain outside this showcase proof.
- The original central map has one Player Start. The generated World Partition proving ground adds four separated starts before multi-client occupancy testing.

## Verification performed

- `OWSEditor Win64 Development`: compiled
- `OWS Win64 Development`: compiled
- `OWS Win64 Shipping`: compiled
- `OWS.Movement.GroundedBaseline.CaptureValidation`: passed
- `OWS.Movement.GroundedBaseline.RestoreValidation`: passed
- `OWS.Vehicle.Input.KinetiThrottleAlgorithms`: passed
- `OWS.Vehicle.Wheel.KinetiLateralSlipSamples`: passed
- Standard, Mover, hybrid-vehicle, and stock-vehicle headless map loads: passed without OWS/runtime errors
- Rendered off-screen standard-lab smoke: character, persistent HUD, both sports-car meshes, garages, signs, sky, lighting, and lab geometry visible
- Windows map cook: passed; all 15 Chaos example Input packages and all 4 OWS prototype DataAssets retained

The Sigil schema warning emitted while generating item assets is an upstream validation-system warning when no optional item schema is configured. The installed Sigil code treats an entirely unconfigured schema system as valid; OWS performs its own explicit prototype definition checks before inventory initialization.

## Redistribution

- OWS-owned source may be shared under the license Aurora chooses later.
- KinetiForge-derived code must retain `Plugins/OWSFramework/ThirdPartyNotices/KinetiForge-LICENSE.txt`.
- Epic engine/example/template content remains governed by Epic’s Unreal/content terms and is not relicensed by this project.
- Save Extension is a separately licensed Marketplace dependency and is not bundled for standalone redistribution. Friends need their own lawful access to required Marketplace dependencies.
- Sigil Inventory is MIT, but this prototype currently consumes the engine-installed copy rather than vendoring it.

## Prototype status language

Passing this prototype means the integration seams compile, load, and behave correctly in their stated tests. It does not mean any experimental Epic subsystem or young third-party plugin has become production-proven, MMO-scaled, or a complete substitute for a mature paid package.
