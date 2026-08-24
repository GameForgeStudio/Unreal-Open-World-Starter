# OWS Automated Testing

## City Foundation conformance status

The accepted City Foundation target is documented in [CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md). Its importer, generators, World Partition cell orchestrator, and player cache are not yet implemented in `main`, so the repository does not yet claim executable City Foundation conformance.

Implementation work must add automated and manual coverage for source validation, four-corner alignment, deterministic generation, cross-cell seams, override preservation, cache hit/miss equivalence, partial invalidation, eviction, corruption recovery, high-speed traversal, teleport preloading, multiplayer authority, save restoration, and constrained storage. Player-facing coverage must verify Off, Automatic, Custom, and Clear Cache; recommendation from the volume that actually stores the cache; emergency free-space preservation; high-water-to-target eviction hysteresis; and that Clear Cache cannot touch saves or shipped prewarm data. The non-negotiable destructive-cache test deletes or corrupts all writable city-cache data, disables runtime PCG, joins multiplayer through every supported role, fast-travels to tested cells, and verifies that traversal, collision, navigation, vehicles, authority, and saves still function using the cooked core. [Issue #100](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/100) owns the complete City Foundation verification matrix.

## Clean-start smoke test

The clean-start smoke test detects Git LFS corruption, missing project modules, required-asset failures, canonical-map load failures, Map Check errors, and incorrect setup of the three OWS character instances.

### Prerequisites

- Windows with Unreal Engine 5.8.1.
- Git and Git LFS available on `PATH`.
- `OWSEditor Win64 Development` built for the checkout being tested.
- The repository cloned with the Windows setup documented in the root README.

### Run

From the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunOWSCleanStartSmoke.ps1 -EngineRoot C:\UE_5.8
```

You may omit `-EngineRoot` when Unreal is installed at `C:\UE_5.8`, under the standard Epic Games installation directory, or when `OWS_UNREAL_ENGINE_ROOT` points to the engine installation.

The successful result ends with:

```text
[OWS smoke] PASS: Git LFS, module startup, required assets, canonical maps, Map Check, and three-character setup passed.
```

Any failed assertion or Unreal process failure returns exit code `1`. The runner prints the failure reason and relevant Unreal log lines, then removes its temporary logs. This command is suitable for local contributor checks and as the canonical-map smoke step in future Windows CI.

### What it verifies

- Every checked-out Git LFS object passes `git lfs fsck`.
- Unreal starts the project and loads its required runtime and editor modules.
- Required OWS, GASPALS, and KinetiForge foundation assets load.
- `/Game/OWS/Levels/OWS_CombinedDemo` loads and passes Map Check without errors.
- `/Game/OWS/Levels/OWS_CourseSection` loads with exactly three OWS character instances.
- Every OWS character instance uses the shared experimental setup marker.
- No retired `OWSPrototype` or `OWSTestLab` dependency appears during startup or map loading.

## Character and vehicle functional tests

These editor automation tests exercise the actual OWS possession boundary in the configured canonical map. Build `OWSEditor Win64 Development`, then run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunOWSCharacterVehicleTests.ps1 -EngineRoot C:\UE_5.8
```

The successful result ends with:

```text
[OWS character/vehicle] PASS: repeated, representative, constrained, and moving bailout/recovery tests passed.
```

The suite verifies:

- Five consecutive enter/stopped-exit cycles using the same OWS character and a canonical vehicle.
- Stopped exits use the authored exit transform for the entered door across all 14 vehicles in `OWS_CombinedDemo`.
- The bus exposes separate front and rear curb-side door/exit transforms; the other included vehicles expose vehicle-sized left/right door/exit transforms.
- Blocking the entered door selects another authored door; blocking every authored door rejects the exit without changing possession or occupancy; removing blockers allows a retry.
- Possession, character identity, visibility, capsule collision, attachment, animation instance, movement state, control-seat occupancy, input-context activation, and camera target on every transition.
- Moving bailouts cover ordinary locomotion recovery at 600 cm/s and controlled-roll behavior at 1200 and 3000 cm/s. Each begins outside the vehicle body and releases seat state and vehicle input without entering ragdoll physics.
- At 5–15 mph, the bailout hands directly to ordinary locomotion without forcing a full-body recovery animation. Above 15 mph, the root-locked full-body roll inherits the vehicle's velocity without artificial sideways or upward launch, loops until grounded horizontal speed falls to the character's configured runnable speed (500 cm/s by default), permits up to 15 degrees/second of left-stick steering, then restores ordinary locomotion directly. Grounded roll slowdown models sliding resistance and discrete contact-energy losses separately from ordinary walking braking.
- While occupying a vehicle, the interaction prompt displays live speed in MPH so the bailout bands can be tested directly.
- When bailout telemetry is enabled, `[VIC][FullTrace]` samples the complete entry-to-stop motion state every 100 milliseconds, including character/vehicle mass, positions, velocity, observed acceleration, ground contact, movement mode, root motion, braking, friction, controlled-roll deceleration, steering, and separation distance.
- Vehicle re-entry remains blocked during controlled recovery and succeeds once recovery restores player control.

Before merging gameplay changes, manually smoke-test one stopped exit, a 5–15 mph bailout, and controlled rolls at both moderate and high speed in `/Game/OWS/Levels/OWS_CombinedDemo`. Confirm the lower-speed bailout does not force a landing or stumble animation; confirm the roll loops until runnable speed, left-stick steering is limited but useful, locomotion resumes directly, and the camera remains aligned. After each exit or recovery, verify vehicle re-entry, locomotion (including analog walk/shuffle and sprint), traversal, aiming, hotbar/equipment/abilities, camera input, and collision. Those feature-level actions require player input and are not claimed by the automation suite.

Multiplayer is a separate explicit test tier. Server-authoritative transitions are tracked by [issue #27](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/27), and seat-contention/recovery coverage is tracked by [issue #28](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/28). This local editor suite does not claim multiplayer acceptance.

## Selector and activation tests

Build `OWSEditor Win64 Development`, then run the selector automation filter:

```powershell
& C:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe `
  .\OWS.uproject `
  '-ExecCmds=Automation RunTests OWS.Selector' `
  '-TestExit=Automation Test Queue Empty' `
  -unattended -nop4 -nosplash -nosound -RenderOffscreen -log
```

The filter verifies the configurable default stack and shared-character setup,
proves precision targeting still detects another character and a ground menu
token with every passive range detector disabled, resolves an exact authored
vehicle-door interaction point, and proves that the shared Activate dispatch
transfers control to that vehicle.

Before merging selector changes, manually smoke-test
`/Game/OWS/Levels/OWS_CombinedDemo`:

1. Look across ordinary geometry, characters, and vehicles; confirm the
   bottom-right detected name follows the precision ray.
2. Aim at the driver-side door of a stopped vehicle and press `F` or
   `Square / X`; confirm that exact authored door is used and no corner rays are
   drawn.
3. Drive, bail out, finish recovery, return to the vehicle, and activate the
   door again; confirm re-entry succeeds and the vehicle remains stationary
   until fresh throttle input.
4. Confirm `Escape` or `Circle / B`, rather than Activate, performs vehicle exit.

In `/Game/OWS/Levels/OWS_CourseSection`, verify the six menu/style pads named
Game Animation Widget, View Controls, Read-Me, Level Style Dark, Level Style
Light, and Level Style Colorful. Aim at each pad and confirm its name appears in
the bottom-right readout. Press `F` or `Square / X` and confirm its existing
action runs. Walking over these six pads must not activate them. Then step on at
least one teleport pad elsewhere in the course and confirm its existing overlap
activation still works; teleport pads must not require selector Activate.

This selector suite is local/editor coverage. The authority boundary is defined
in [INTERACTION_TARGETING.md](INTERACTION_TARGETING.md); multiplayer execution
remains tracked by issue #27.
