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

## Accepted City Foundation target

The accepted OWS City Foundation target imports real-world map and elevation sources in Unreal Editor, produces normalized OWS city data and a cooked playable foundation, and supports deterministic World Partition-aligned visual enhancement through shipped prewarm data, a disposable player-configurable persistent cache, and live cosmetic microdetail. These systems are planned under the City Foundation epic and are not claimed as current `main` functionality.

The accepted implementation boundary is the first-party `Plugins/OWSCityFoundation` plugin with an `OWSCityRuntime` module and an editor-only `OWSCityEditor` module. The project `OWS` module integrates and demonstrates the subsystem. Raw geographic input remains outside Unreal Content, per-city assets remain under `/Game/OWS/CityFoundation/Cities/<CityId>`, and only runtime products cross the cook boundary. Exact ownership and paths are defined in [CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md).

The cooked foundation supplies authoritative terrain, roads, buildings, collision, navigation, gameplay anchors, and stable feature identities, plus baked presentation fallbacks and HLOD. OWS server and save systems own authoritative multiplayer and persistent-state changes against those identities. Runtime generation never accepts arbitrary player-supplied geographic files and never becomes the only source of gameplay truth. See [CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md) for the controlling target design and conformance invariants.

## Acceptance

Use [CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md](CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md) for the repeatable release scenarios. At minimum, changes affecting the playable experience must be checked in `OWS_CombinedDemo` for analog slow movement, walk/run/sprint transitions, traversal, camera behavior, vehicle entry, driving, and exit.

City Foundation changes must conform to [CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md). Current and required future City Foundation coverage is documented in [TESTING.md](TESTING.md); no importer, generator, cache, or runtime-cell behavior is considered present until its linked implementation and verification issues provide evidence.
