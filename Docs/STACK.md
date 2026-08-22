# Open World Starter Runtime

OWS is the product. Its included systems and plugins are implementation foundations that OWS integrates into one starter package.

## Canonical playable experience

| Responsibility | OWS authority |
| --- | --- |
| Default and editor startup map | `/Game/OWS/Levels/OWS_CombinedDemo` |
| Default game mode | `/Game/OWS/GameModes/GM_OWSCharacterDemo` |
| Character controller | `/Game/OWS/Controllers/PC_OWSCharacterDemo` |
| Character and traversal | OWS character assets built from the included GASPALS foundation |
| Vehicle driving | OWS vehicle assets built from the included KinetiForge foundation |
| Character/vehicle handoff | `UOWSStockVehicleInteractionComponent` and `UOWSVehicleInteractionComponent` in the OWS runtime module |
| OWS runtime UI/settings | `OWSCore` hotbar, widget, and user-settings classes |

`OWS_CombinedDemo` is the supported place to evaluate character locomotion, traversal, vehicle entry, driving, and vehicle exit together. Retired labs and subsystem evaluation maps are not part of the package.

## Runtime ownership

Character movement owns the on-foot pawn. The active vehicle pawn owns driving physics while occupied. OWS owns the transition between those states, including interaction discovery, possession, camera handoff, safe exit placement, and recovery behavior.

The OWS runtime module contains the native character/vehicle integration. The OWS Framework plugin contains reusable OWS runtime UI and settings support. Content under `/Game/OWS` supplies the canonical maps, game modes, controllers, characters, and vehicle configuration.

## Acceptance

Use [CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md](CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md) for the repeatable release scenarios. At minimum, changes affecting the playable experience must be checked in `OWS_CombinedDemo` for analog slow movement, walk/run/sprint transitions, traversal, camera behavior, vehicle entry, driving, and exit.
