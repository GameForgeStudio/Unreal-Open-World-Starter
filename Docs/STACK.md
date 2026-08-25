# Open World Starter Runtime

OWS is the product. Its included starting systems are maintained OWS forks that evolve together inside one self-contained starter package.

## Accepted target product shape

OWS is a complete, immediately playable starter project built over reusable first-party OWS plugins. The project game module and `/Game/OWS` demo content are the thin composition, integration, and demonstration shell in the accepted target architecture; reusable subsystem ownership does not remain there long-term.

The accepted [OWS Platform Architecture and Composition Contract](OWS_PLATFORM_ARCHITECTURE.md) defines the target plugin families, ownership, dependency directions, public extension rules, conformance evidence, and compatibility-first migration backlog. Current placement describes `main`; it is not authorization to move or duplicate a reusable system outside a separately claimed issue.

## Canonical playable experience

| Responsibility | Current authority in `main` |
| --- | --- |
| Default and editor startup map | `/Game/OWS/Levels/OWS_CombinedDemo` |
| Default game mode | `/Game/OWS/GameModes/GM_OWSCharacterDemo` |
| Character controller | `/Game/OWS/Controllers/PC_OWSCharacterDemo` |
| Character and traversal | OWS character assets configured from the maintained GASPALS fork |
| Vehicle driving | OWS vehicle assets configured from the maintained KinetiForge fork |
| Character/vehicle handoff | `UOWSStockVehicleInteractionComponent` and `UOWSVehicleInteractionComponent` in the OWS runtime module |
| OWS runtime UI/settings | `OWSCore` hotbar, widget, and user-settings classes |

`OWS_CombinedDemo` is the supported place to evaluate character locomotion, traversal, vehicle entry, driving, and vehicle exit together. Retained labs and subsystem evaluation content may remain in the complete source installation, but they are outside the canonical user workflow. Excluding them from normal runtime cooks is the accepted target and must be proven by the baseline and domain cook-manifest work; this current-state summary does not claim that exclusion is implemented yet.

## Current physical runtime ownership

Character movement owns the on-foot pawn. The active vehicle pawn owns driving physics while occupied. OWS owns the transition between those states, including interaction discovery, possession, camera handoff, safe exit placement, and recovery behavior.

The OWS runtime module contains the native character/vehicle integration. The OWS Framework plugin contains reusable OWS runtime UI and settings support. Content under `/Game/OWS` supplies the canonical maps, game modes, controllers, characters, and vehicle configuration.

This section records current physical implementation. It does not override the accepted target product shape or authorize a migration by itself.

## Accepted target ownership

| Responsibility | Target owner |
| --- | --- |
| Package contracts, profiles, settings registry, compatibility, setup, and validation | Minimal Platform family retained under `Plugins/OWSFramework` |
| Input contexts, semantic actions, priorities, and rebinding | OWS Input domain family |
| Selection, targeting, interactables, and activation | OWS Interaction domain family |
| Host-neutral hotbar backend source/module, prompts, menus, notifications, settings presentation, and view models | OWS UI domain family under the accepted Stage 1 physical boundary; individual hotbar instances have developer-selected hosts |
| Shared player/entity identity, actions, GAS ownership, vitality, affiliation, events, and authority semantics | OWS Shared Gameplay family |
| Generalized inventory and equipment implementation | Maintained Sigil Inventory fork evolving as OWS |
| Persistence schemas, checkpoints, providers, migration, and recovery | OWS Persistence contract over the maintained Save Extension fork |
| Complete character and traversal implementation | Maintained GASPALS fork evolving in place as the OWS Character family |
| Complete vehicle implementation | Maintained KinetiForge fork evolving in place as the OWS Vehicle family |
| Character/vehicle dynamic support and riding, boarding, occupancy/control handoff, exit, bailout, recovery/re-entry, exterior traversal, grip, moving entry, infiltration, transfer, reservations, and multiplayer orchestration | Narrow Character–Vehicle Mobility integration plugin family governed by the selected Stage 3 program |
| Playable composition and showcase | `Source/OWS` and `/Game/OWS` thin shell |

The complete ownership and dependency matrix is maintained in [OWS_PLATFORM_ARCHITECTURE.md](OWS_PLATFORM_ARCHITECTURE.md). Target ownership is migrated one responsibility at a time with baselines, compatibility shims or redirects, and explicit verification.

The accepted [OWS Shared Gameplay Spine, Authority, and Persistence Contract](OWS_GAMEPLAY_SPINE_ARCHITECTURE.md) defines the shared Stage 2 target. It does not claim that the current runtime has been migrated or that multiplayer and persistence conformance already pass.

The accepted [OWS Mobility Program Charter](OWS_MOBILITY_PROGRAM_CHARTER.md) records the active Stage 3 design program. Current `main` still provides only the verified entry/exit/bailout baseline described above; it does not yet implement passengers, supported exterior riding, moving entry, infiltration, or vehicle-to-vehicle transfer.

## Accepted City Foundation target

The accepted OWS City Foundation target imports real-world map and elevation sources in Unreal Editor, produces normalized OWS city data and a cooked playable foundation, and supports deterministic World Partition-aligned visual enhancement through shipped prewarm data, a disposable player-configurable persistent cache, and live cosmetic microdetail. These systems are planned under the City Foundation epic and are not claimed as current `main` functionality.

The accepted implementation boundary is the first-party `Plugins/OWSCityFoundation` plugin with an `OWSCityRuntime` module and an editor-only `OWSCityEditor` module. The project `OWS` module integrates and demonstrates the subsystem. Raw geographic input remains outside Unreal Content, per-city assets remain under `/Game/OWS/CityFoundation/Cities/<CityId>`, and only runtime products cross the cook boundary. Exact ownership and paths are defined in [CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md).

The cooked foundation supplies authoritative terrain, roads, buildings, collision, navigation, gameplay anchors, and stable feature identities, plus baked presentation fallbacks and HLOD. OWS server and save systems own authoritative multiplayer and persistent-state changes against those identities. Runtime generation never accepts arbitrary player-supplied geographic files and never becomes the only source of gameplay truth. See [CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md) for the controlling target design and conformance invariants.

## Acceptance

Use [CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md](CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md) for the repeatable release scenarios. At minimum, changes affecting the playable experience must be checked in `OWS_CombinedDemo` for analog slow movement, walk/run/sprint transitions, traversal, camera behavior, vehicle entry, driving, and exit.

City Foundation changes must conform to [CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md). Current and required future City Foundation coverage is documented in [TESTING.md](TESTING.md); no importer, generator, cache, or runtime-cell behavior is considered present until its linked implementation and verification issues provide evidence.
