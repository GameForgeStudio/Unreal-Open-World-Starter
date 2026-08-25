# OWS Controls

This is the control reference for the configured OWS experience in `/Game/OWS/Levels/OWS_CombinedDemo`. It was verified against the Unreal Engine 5.8.1 input assets and the OWS runtime bindings on August 22, 2026.

This document records current configured behavior. The accepted target ownership is the dedicated OWS Input domain defined by the [OWS Platform Architecture and Composition Contract](OWS_PLATFORM_ARCHITECTURE.md); that target is not presented here as implemented.

Controller names use both PlayStation and Xbox labels: `Cross / A`, `Circle / B`, `Square / X`, `Triangle / Y`, `L1 / LB`, `R1 / RB`, `L2 / LT`, and `R2 / RT`.

## On foot

| Action | Keyboard and mouse | Controller |
| --- | --- | --- |
| Move | `W`, `A`, `S`, `D` | Left stick |
| World-space move | Arrow keys | — |
| Look | Mouse | Right stick |
| Toggle walk mode | Left `Ctrl` | `L3` / left-stick press |
| Sprint | Left `Shift` | `Circle / B` |
| Jump or traverse | `Space` | `Cross / A` |
| Crouch | `C` | `Triangle / Y` |
| Activate the selected world target | `F` | `Square / X` |
| Toggle strafe | Middle mouse button | — |
| Aim | Right mouse button | `L2 / LT` |
| Ragdoll | `X` | — |
| Select next placed OWS character | `N` | D-pad right with no hotbar modifier held |
| Select previous placed OWS character | — | D-pad left with no hotbar modifier held |
| Swap camera shoulder | Left `Alt` | `R3` / right-stick press while the character is in the supported strafe state |

Movement on the left stick is analog. Light input produces the slowest movement range; pressing `L3` toggles the restricted walk range. Keyboard movement is digital, so `Left Ctrl` supplies the walk toggle.

### Experimental presentation shortcuts

These inherited presentation controls remain available in the OWS character:

| Action | Keyboard and mouse | Controller |
| --- | --- | --- |
| Open the overlay-base selector, then change selection | `E`, then mouse wheel | — |
| Open the overlay-stance selector, then change selection | `Q`, then mouse wheel | — |
| Toggle the experimental first-person camera | `1` | — |
| Cycle between demonstration camera styles | `2` / `3` | — |

`E` therefore reaches both the normal interact action and the inherited overlay-base selector. The inherited gamepad camera-style shortcuts are superseded by the OWS controller's D-pad hotbar and roster routing; use the keyboard presentation shortcuts above in the configured OWS experience.

## Cross-hotbar routing

The OWS controller exposes four ordered hotbar layers. Hold `L1 / LB` or `R1 / RB` to select a layer. Holding both selects a separate layer based on which shoulder was pressed first.

While a layer is active, any D-pad direction or face button routes one of that layer's eight generic slots. OWS exposes 32 addresses in total; the gameplay system using a slot decides what that slot does. The developer hotbar display appears only when its presentation setting allows it.

Without a shoulder modifier, D-pad left and right cycle the placed OWS character roster. D-pad up and down have no standalone on-foot action. `R2 / RT` routes the generic primary-action press and release seam. `Escape` and `Circle / B` route the same Cancel press/release seam only while an OWS menu or other cancelable context explicitly enables it.

## Entering and leaving vehicles

| State | Keyboard | Controller | Behavior |
| --- | --- | --- | --- |
| On foot with an available vehicle-door interaction point targeted | Tap `F` | Tap `Square / X` | Enter through that exact authored door when it is within range and its control seat is available. |
| Driving at or below 5 mph | Tap `Escape` | Tap `Circle / B` | Exit at a safe stopped-exit location. |
| Driving above 5 mph | Hold `Escape` for 2 seconds | Hold `Circle / B` for 2 seconds | Bail out once the hold completes. Releasing early cancels the request. |

On-foot `F` and `Square / X` both enter through the OWS selector's `Activate`
function. Vehicle entry consumes the exact authored door interaction point
already resolved by the selector. It does not run a second vehicle detector or
choose another door after activation.

This table documents the current control-seat handoff only. OWS does not yet claim supported passenger entry, exterior riding, moving entry, relative-motion matching, infiltration, or cross-Vehicle transfer. Their accepted semantic Activate, pre-commit Cancel, governed Exit/Bailout/Release, discrete and maintained Grip, accessibility, and transition rules are defined by the target [OWS Mobility Architecture](OWS_MOBILITY_ARCHITECTURE.md), but remain unimplemented until their scoped work and [matrix](OWS_MOBILITY_ACCEPTANCE_MATRIX.md) evidence pass. The default physical Cancel binding may route an Exit/Bailout/Release request in an established relationship without changing that request into Action cancellation.

Above 5 mph through 15 mph, a moving bailout hands directly to ordinary locomotion without forcing a full-body recovery animation. Above 15 mph, the character performs a non-ragdoll controlled roll that inherits the vehicle's velocity, permits limited left-stick steering, and continues until the character reaches runnable speed before ordinary locomotion resumes. Vehicle re-entry is unavailable only while controlled recovery owns the character and becomes available when player control returns; there is no distance-based re-entry release requirement.

## Driving

The vehicle mapping context is active while the player possesses an OWS vehicle.

| Action | Keyboard and mouse | Controller |
| --- | --- | --- |
| Throttle | `W` or Up arrow | `R2 / RT` |
| Brake or request reverse | `S` or Down arrow | `L2 / LT` |
| Steer | `A` / `D` or Left / Right arrows | Left stick horizontally |
| Look around | Mouse | Right stick |
| Handbrake | `Space` | No standard gamepad binding; validation is tracked in [issue #43](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/43) |
| Toggle engine | `V` | D-pad down in the vehicle mapping context |
| Clutch | `C` | `Square / X` in the vehicle mapping context |
| Shift up | `O` | `R1 / RB` in the vehicle mapping context |
| Shift down | `P` | `L1 / LB` in the vehicle mapping context |

The persistent OWS hotbar router also reserves the controller shoulders, D-pad, face buttons, and `R2 / RT`. Those controls remain available to its generic routing seams during vehicle possession. `Circle / B` is not mapped to the vehicle brake or handbrake; it is reserved for stopped exit and moving bailout.

## Current implementation sources

- Character mapping context: `/Game/OWS/Input/IMC_OWSCharacter`
- OWS controller: `/Game/OWS/Controllers/PC_OWSCharacterDemo`
- Vehicle mapping context: `/KinetiForge/Template/Input/IMC_VehicleDefault`
- Vehicle entry and exit runtime: `Source/OWS/Private/OWSVehicleInteractionComponent.cpp`
- Selector and Activate runtime: `Source/OWS/Private/OWSSelectorComponent.cpp`
- Interaction-point contract: [INTERACTION_TARGETING.md](INTERACTION_TARGETING.md)
- Cross-hotbar runtime: `Plugins/OWSFramework/Source/OWSCore/Private/OWSControllerHotbarComponent.cpp`

If any documented control differs in the configured OWS experience, report the exact map, input device, action, and observed result as an issue.
