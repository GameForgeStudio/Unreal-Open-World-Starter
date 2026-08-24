# OWS City Foundation Architecture

This document is the controlling product and architecture contract for the OWS City Foundation. It records the accepted design for [issue #83](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/83) under the [OWS City Foundation epic](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/82).

> **Status:** Accepted target architecture; implementation is tracked by the City Foundation epic and its child issues. This document does not claim that the importer, generators, cell orchestrator, or cache currently exist in `main`.

The City Foundation turns bounded real-world map and elevation data into an editable, playable Unreal city. OWS imports and validates source data in the editor, generates an authoritative cooked city foundation, and can enrich that foundation through deterministic, World Partition-aligned runtime layers. Runtime generation and caching are accelerators and presentation systems; they are never the only source of gameplay truth.

## Product boundary

- OWS accepts local OSM XML and GeoJSON map data.
- Raw OSM, GeoJSON, GeoTIFF, heightmap, and other geographic source files are imported only in Unreal Editor.
- Map data can be imported for inspection without elevation, but OWS blocks terrain-conformed playable generation until a valid terrain source is supplied.
- A shipped game does not accept arbitrary geographic files from a player and does not build an arbitrary uploaded city.
- The editor produces normalized OWS city data, cooked Unreal assets, deterministic cell recipes, and optional prewarmed cell results.
- The shipped game may assemble only the predetermined city content authorized and cooked by its developer.
- The supported workflow is local-file import using Unreal Engine 5.8 built-ins, OWS, and user-supplied source data. It does not require a built-in downloader, City Sample, MetaRoad, Fab content, an external importer plugin, an API key, QGIS, Blender, or Python.

## Supported terrain inputs

All six initial input paths are required:

| Input | Contract |
| --- | --- |
| Existing Unreal Landscape | OWS uses a Landscape already present in the project without replacing it. A Landscape with valid georeferencing aligns automatically; one without it uses editor registration. |
| GeoTIFF / Cloud Optimized GeoTIFF | OWS imports elevation while retaining or reading available geographic bounds, CRS/projection, horizontal scale, and georeferencing metadata. |
| 16-bit grayscale PNG | OWS supports Unreal's standard 16-bit heightmap workflow without reducing height precision. |
| R16 | OWS supports Unreal's native 16-bit raw heightmap interchange format. |
| RAW plus JSON metadata | OWS supports Unreal-compatible RAW heightmaps whose dimensions and bit depth are supplied by the JSON sidecar. |
| OWS georeferencing sidecar | PNG, R16, and RAW inputs can carry the explicit spatial metadata required to align terrain with OSM or GeoJSON instead of relying on undocumented manual placement. |

OWS supports cropping and resampling elevation to the authorized map boundary. Tiled elevation input retains one geospatial frame and is compatible with the World Partition authoring workflow; tile boundaries cannot become independent manual alignment points.

## Authoring pipeline

1. Import bounded OSM XML or GeoJSON and record its origin, scale, bounds, provenance, attribution, supported feature tags, and immutable source fingerprint.
2. Select or import a terrain source.
3. Crop and resample elevation to the authorized boundary when required, preserving tiled inputs in one shared spatial frame.
4. Resolve CRS/projection, axis order, horizontal scale, elevation scale, vertical-datum metadata, pixel registration, and resolution.
5. Automatically align a georeferenced Landscape, or use the four-corner editor registration workflow with a placement preview for a Landscape without geographic metadata.
6. Block road and building conformance until the four shared X/Y corners and required spatial metadata pass the alignment gate.
7. Normalize source features into independent OWS records with stable source-derived identities.
8. Generate editable roads, buildings, areas, gameplay anchors, and derived assets without altering the source layer.
9. Bake the authoritative playable foundation, World Partition data, collision, navigation data, HLOD, and optional prewarmed enhancement results.

Invalid, unsupported, mismatched, or insufficiently georeferenced input must stop the affected operation with actionable diagnostics. OWS must not silently generate misplaced content.

## Source, generated, and override ownership

City content has three ownership layers:

| Layer | Ownership contract |
| --- | --- |
| Source | Original map, elevation, provenance, and registration inputs are immutable. OWS never rewrites them. |
| Generated | Roads, buildings, registration products, recipes, and other derived assets are regenerable. |
| User override | Deliberate user changes are preserved and never silently overwritten by regeneration. |

Reimport updates generated features without overrides. Moved buildings, replacement meshes, road-style changes, intentional deletions, and other explicit overrides survive. An override whose source feature disappears becomes an orphan that OWS reports for review. Users can explicitly discard an override and return a feature to generated behavior.

Exact asset paths must make these ownership layers visibly distinguishable. Stable feature identities must survive regeneration so overrides, saves, multiplayer state, and cache records never depend on transient Actor names or load order.

## Module, content, and cook ownership

City Foundation is a first-party OWS subsystem packaged as `Plugins/OWSCityFoundation`. It is not part of an attributed foundation plugin. The existing project `OWS` module integrates and demonstrates City Foundation but does not own its reusable implementation.

The plugin contains exactly two production modules:

| Module | Type | Ownership |
| --- | --- | --- |
| `OWSCityRuntime` | Runtime | `CityCellID`, stable feature identities, cooked recipes and catalogs, layer policies, runtime cell orchestration, optional presentation, shipped-prewarm reading, writable-cache schema and services, telemetry, and runtime feature-promotion interfaces. |
| `OWSCityEditor` | Editor | OSM XML, GeoJSON, and elevation import/reimport; provenance and fingerprints; CRS resolution and registration previews; cropping and resampling; normalization and terrain conformance; road/building generation; override preservation; World Partition, HLOD, navigation, recipe, and prewarm baking; cook validation; and editor UI. |

`OWSCityEditor` depends on `OWSCityRuntime`. `OWSCityRuntime` never depends on `OWSCityEditor`, `UnrealEd`, the project `OWS` module, GASPALS, or KinetiForge.

The first-party plugin layout is:

```text
Plugins/OWSCityFoundation/
  OWSCityFoundation.uplugin
  Source/OWSCityRuntime/
  Source/OWSCityEditor/
  Content/Runtime/
  Content/Editor/
```

Reusable plugin assets use these mount paths:

```text
/OWSCityFoundation/Runtime/Defaults/
/OWSCityFoundation/Runtime/Materials/
/OWSCityFoundation/Runtime/Catalogs/
/OWSCityFoundation/Runtime/Blueprints/
/OWSCityFoundation/Editor/Preview/
/OWSCityFoundation/Editor/Widgets/
/OWSCityFoundation/Editor/Icons/
```

Developer-supplied raw geographic files remain outside Unreal Content and outside the cook:

```text
<Project>/SourceData/OWS/CityFoundation/<CityId>/Map/
<Project>/SourceData/OWS/CityFoundation/<CityId>/Elevation/
<Project>/SourceData/OWS/CityFoundation/<CityId>/Metadata/
<Project>/SourceData/OWS/CityFoundation/<CityId>/Attribution/
```

Per-city Unreal assets use this ownership tree:

```text
/Game/OWS/CityFoundation/Cities/<CityId>/
  World/
  Source/
  Generated/
    Authoring/
    Runtime/
      Core/
      Recipes/
      Catalogs/
      Attribution/
      Prewarm/
  Overrides/
```

`Source` contains editor-only manifests, registration data, provenance, and fingerprints. `Generated/Authoring` contains regenerable editor intermediates. `Generated/Runtime` contains cookable city products. `Overrides` is user-owned, and regeneration never writes there.

The cook includes the stable `World`, referenced `Overrides`, `Generated/Runtime`, and `/OWSCityFoundation/Runtime`. It excludes raw `SourceData`, per-city `Source`, `Generated/Authoring`, and `/OWSCityFoundation/Editor`. Editor-only types, Asset Manager cook rules, and a cook validator enforce the boundary; folder names alone are not security or cook controls. World Partition external-actor paths remain Unreal-managed, with generated and overridden actors distinguished through Data Layers, stable feature IDs, and ownership metadata.

The writable cache is not a Content asset. Its logical platform-resolved location is:

```text
<PlatformWritableData>/OWS/CityFoundation/Cache/<ProjectId>/<WorldId>/<Schema>/
```

`OWSCityRuntime` owns developer-authorized runtime layer policies and player cache controls. `OWSCityEditor` owns non-authoritative editor preferences. Reproducibility-critical import, registration, and generation values live in versioned per-city assets rather than personal editor preferences. OWS Framework may present City options in a unified settings interface, but City Runtime owns their meaning and behavior.

## Terrain-conformed buildings

- Each building footprint controls an independently addressable logical building feature. Compatible presentation may be batch-rendered, but conformance, stable identity, and overrides remain per building.
- OWS samples every actual base-level footprint vertex.
- Long perimeter edges receive configurable, measurement-only intermediate samples.
- Measurement samples do not subdivide, deform, or add vertices to the building geometry.
- The first placement pass does not sample the footprint interior.
- The whole building remains upright and moves only along Z until no sampled perimeter point is floating.
- Terrain penetration elsewhere is accepted during the first pass and may be refined procedurally or manually later.

## Editable terrain-conformed roads

- OWS generates editable spline-based roads from normalized OSM/GeoJSON centerlines.
- Ordinary roads conform to terrain.
- Bridges and tunnels use separate elevation and conformance rules and are never blindly draped onto the Landscape.
- Generated road geometry supplies stable UVs, tangents, collision, physical-material support, and replaceable material slots.
- Parallax road presentation is optional and does not require city or road regeneration.
- OWS provides a global parallax quality setting and a per-road-style inherit/override control.
- Standard and parallax-capable material variants, parameters, performance tiers, and fallbacks are owned by [issue #88](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/88).

## Double-hybrid city execution

The accepted design combines two hybrids:

1. **Representation hybrid:** a baked, fully playable city foundation plus procedural visual enhancement.
2. **Computation hybrid:** shipped precomputed results plus locally cached deterministic generation plus live ephemeral detail.

The result is a four-rung cell ladder:

| Rung | Contents | Persistence | Authority |
| --- | --- | --- | --- |
| Cooked core | Terrain, roads, bridges, building fallback shells, collision, navigation, gameplay anchors, stable IDs, and HLOD | Shipped | Static gameplay baseline is authoritative; HLOD and fallback visuals are presentation |
| Shipped prewarm | Optional precomputed facades, signs, rooftop kits, street furniture, vegetation placements, and comparable enhancement data | Shipped, read-only | Predetermined presentation |
| Persistent local cache | Eligible deterministic cell-layer results generated on a cache miss | Player device, disposable | Presentation only |
| Live microdetail | Grass, litter, tiny debris, particles, and comparable dense ambient scatter | RAM/GPU only | Cosmetic only |

Every configurable generation layer supports these execution policies:

- **Baked Only:** generate in the editor and ship as cooked core; do not generate or cache this layer during play.
- **Shipped Prewarm:** generate before release and ship an optional read-only cell product; it is not part of the player's writable cache budget and is unaffected by Off, Custom, eviction, or Clear Cache.
- **Generate and Persist:** use a valid local result when present; otherwise generate deterministically and persist when writable caching is enabled. With caching Off, the permitted result remains session-only.
- **Live Only:** generate when needed and never write the result to persistent storage.
- **Disabled:** omit the optional layer and retain the complete baked fallback.

Any runtime-capable layer can be permanently generated before shipping. No runtime layer may be required for the city to remain playable.

In this document, **baked foundation** and **cooked core** refer to the same authoritative shipped rung. **Shipped prewarm** is optional read-only installation content. **Persistent local cache** is optional writable derived data on the player's device. They are separate products with separate budgets and failure behavior.

## World Partition and logical city cells

World Partition streams the authoritative cooked world. OWS uses the same streaming sources to predict and schedule enhancement work, but it owns a stable logical `CityCellID` rather than treating an internal World Partition cell identifier as durable product data.

World Partition, PCG partitioning, PCG hierarchical generation, HLOD, navigation chunks, and the OWS cache may use different grids. Their origins and dimensions must be aligned deliberately. OWS logical cells use deterministic integer coordinates and hierarchy levels, with grid sizes chosen as compatible multiples or subdivisions. The architecture should use one principal World Partition runtime grid unless measured project requirements justify otherwise.

Large or cross-cell features belong to a canonical owner cell or a coarser logical macrocell. Neighboring cells consume stable-ID or soft-reference indirection to that result rather than generating duplicates or creating hard cross-cell Actor references that force World Partition cells to load together. Each generation cell reads a non-owning border halo of adjacent inputs, and the halo participates in invalidation. This prevents road cracks, duplicate props, incomplete buildings, inflated streaming units, and results that depend on cell load order.

## Cell lifecycle

1. A distant baked HLOD or fallback representation remains visible.
2. A World Partition streaming source predicts an approaching cell and requests the cooked core in the `Loaded` state.
3. OWS checks active memory, shipped prewarm data, and the writable persistent cache.
4. A cache hit is read, decompressed, validated, and prepared asynchronously.
5. A cache miss for an eligible layer runs deterministic generation asynchronously. A `Generate and Persist` result is committed atomically only when writable caching is enabled; `Live Only` and cache-Off results remain session-only.
6. World Partition activates the cooked core after its required collision and gameplay state are ready; this is the only representation permitted to gate traversal.
7. Prepared enhancement is activated in bounded game-thread and render-thread work slices.
8. Live microdetail is generated only after its supporting representation is ready.
9. Leaving a cell releases live Actors, components, and GPU resources while retaining eligible persistent data.

Prediction considers streaming-source position, velocity, view direction, known routes, and declared teleport destinations. Core, collision, and required gameplay state may block traversal. Optional enhancement never does. If enhancement is late, missing, disabled, corrupt, or over budget, the baked representation stays visible.

## Persistent cache contract

The OWS shipping cache is not Unreal's development Derived Data Cache and does not rewrite cooked World Partition or IoStore packages. Unreal's native PCG runtime cache is also not this persistent store: it is a transient, memory-budgeted cache of runtime CPU-node results rather than a cross-session, patch-aware city-cell product. OWS stores compact, declarative, versioned cell products such as:

- Logical asset catalog IDs and soft references
- Quantized cell-local transforms
- Instance batches and custom instance data
- Material parameters and variant selections
- Stable feature IDs
- Importance ranks used for scalable density

It does not store live Actor graphs, transient UObject pointers, or the only copy of gameplay state. Cache hits should rehydrate into bounded instance/component batches rather than thousands of individual Actors.

### Player controls

Players receive these cache controls:

- **Off:** no persistent cache writes; the baked city remains available and permitted runtime layers may remain session-only.
- **Automatic:** OWS recommends and manages the writable cache budget.
- **Custom:** the player selects the maximum writable cache size.
- **Clear Cache:** all disposable local city-cache data can be removed safely.

Automatic sizing detects the filesystem volume that will actually contain the writable cache and recommends a budget from that volume's capacity and free space while preserving a safe emergency reserve. The default recommendation is associated with the installation volume when it also backs the cache; when platform rules place writable data elsewhere, the actual cache volume controls the recommendation. The interface reports current usage, recommended and configured limits, cache hit rate, generation time avoided, and expected benefit.

Eviction runs between a high-water mark and lower target to avoid constant churn. It prioritizes retained entries by generation cost avoided, predicted revisit probability, visual value, and compressed size. Cache size and visual-quality settings remain independent wherever a deterministic ranked superset can serve multiple quality levels.

### Identity, invalidation, and recovery

A cache request identifies at least the world/region, logical cell, hierarchy level, content layer, stable seed, local and halo source hashes, override hash, generator/schema version, relevant asset-placement signature, and output profile. A separate payload hash detects corruption and enables deduplication.

Invalidation is as narrow as correctness allows:

- A material or texture-only change does not invalidate unchanged placements.
- A generator change invalidates only its layer and dependent cells.
- A road/source edit invalidates intersecting cells, border halos, and dependent layers.
- A grid or serialization-schema change creates a new cache namespace.
- A build number alone does not invalidate valid content.

Writes are asynchronous, checksummed, and atomically committed. Writable cache records are treated as untrusted input: readers enforce the declared schema and version, decompressed-size, record-count, and numeric-range limits, and an allowlist of logical asset-catalog IDs. Cache payloads cannot supply arbitrary class or package paths and cannot invoke generic UObject deserialization. Malformed, corrupt, oversized, or incomplete records are discarded safely without causing a crash, unbounded allocation, or any effect on saves or baked content. Repeated storage failures disable persistent writes for the session and fall back to memory-only enhancement. Cache cleanup never blocks World Partition unloading.

## Performance and presentation rules

- One scheduler arbitrates World Partition I/O, cache I/O, decompression, PCG CPU work, game-thread instantiation, GPU upload, and cleanup against frame and arrival deadlines.
- Optional work is cancellable and cannot starve core streaming.
- Separate preload, activation, and cleanup radii plus dwell hysteresis prevent boundary thrashing and visible popping.
- Shaders and material variants used by generated content participate in the project's PSO-precache workflow.
- GPU PCG is limited to non-authoritative microdetail because GPU-only procedural instances do not provide the required persistence, collision, navigation, or HLOD contract.
- Generation policy and defaults must be validated on representative minimum-spec hardware, high-speed vehicles, teleportation, multiplayer joins, and constrained storage.

## Collision, navigation, HLOD, and vehicles

- Roads, sidewalks, entrances, bridges, tunnels, and other gameplay-critical surfaces use baked collision and authoritative physical materials.
- Base navigation is generated before shipping and streams with the world where supported. Unreal 5.8 documents World Partitioned Navigation Mesh as Experimental, so OWS must validate it on every shipping platform and document an approved fallback rather than assuming production readiness.
- Runtime enhancement is navigation-irrelevant by default; bounded dynamic obstacles or modifiers require explicit support and testing.
- Baked HLOD and fallback content cover unloaded or unenriched cells.
- OWS vehicles consume generated road collision, physical materials, and semantic road data. City Foundation does not own vehicle physics.
- A deleted or disabled writable cache cannot change vehicle handling, collision, navigation connectivity, or reachable gameplay space.

## Multiplayer, saving, and authority

The cooked foundation defines baseline collision, navigation, gameplay anchors, and stable feature identities. The server owns mutable and replicated gameplay truth. A writable client cache is never trusted for collision, navigation, quests, loot, economy, interactable availability, or authoritative placement.

- Purely cosmetic enhancement may be loaded or generated independently by each client.
- A cached base feature that gains dynamic gameplay state is addressed by its stable feature ID and promoted to an authoritative replicated Actor or state record.
- Dedicated servers may skip visual caches entirely.
- Save data stores player/world deltas, recipe epoch, and mutations keyed by stable feature IDs; it never relies on cache residency.
- Cache data is disposable and remains logically separate from every profile and save slot.
- Generator changes that cannot preserve stable IDs require an explicit migration, tombstone mapping, or retained compatibility epoch.

## Implementation conformance invariants

An implementation does not conform to this accepted architecture unless all of these remain true:

1. Deleting, disabling, filling, or corrupting the writable cache cannot make the city unplayable.
2. Disabling runtime PCG removes only optional visual richness.
3. Fast travel can complete safely using the cooked core even when enhancement misses its deadline.
4. Multiplayer authority and save restoration do not depend on client-generated data.
5. Cell generation is deterministic under changed worker counts and task completion order for every layer that claims persistent identity.
6. Cache writes, reads, invalidation, eviction, corruption, and version migration are observable and testable per logical cell and layer.
7. Representative high-speed traversal does not expose empty cells, collision gaps, road seams, or blocking generation hitches.
8. Delete or corrupt the entire writable cache, disable runtime PCG, join multiplayer through host, client, late-join, and dedicated-server paths, and fast-travel to any tested cell; gameplay, collision, navigation, vehicles, authority, and save restoration must still work, with only optional visual richness absent.

## Deferred input adapters

ASCII Grid, HGT/SRTM, DTED, BIL/HDR, GridFloat, ArcGrid, and IMG are legitimate later terrain adapters but are not initial requirements. LAS/LAZ requires a separate LiDAR pipeline for classification, ground extraction, removal of buildings and vegetation, rasterization, holes, and resampling; it is not treated as another heightmap extension.

## Implementation tracking

- [Epic #82](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/82) owns the complete City Foundation outcome.
- [Issue #83](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/83) owns this architecture and dependency contract.
- [Issue #84](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/84) owns OSM XML, GeoJSON, and terrain-source import.
- [Issue #85](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/85) owns normalized recipes, stable identities, logical cells, and editable road/lot conversion.
- [Issue #88](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/88) owns the baked road foundation and road presentation.
- [Issue #96](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/96) owns the authoritative cooked World Partition city core and HLODs.
- [Issue #97](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/97) owns optional runtime city-cell enhancement streaming.
- [Issue #98](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/98) owns the shipped-prewarm and writable persistent-cache format.
- [Issue #99](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/99) owns player cache controls, automatic recommendation, storage pressure, and eviction.
- [Issue #100](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/100) owns end-to-end fallback, seam, travel, multiplayer, save, and performance validation.

## Engine references

- [World Partition](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine)
- [PCG generation modes](https://dev.epicgames.com/documentation/unreal-engine/using-pcg-generation-modes-in-unreal-engine)
- [PCG runtime generation debugging and native cache behavior](https://dev.epicgames.com/documentation/unreal-engine/pcg-runtime-generation-debugging-in-unreal-engine)
- [World Partition PCG Builder commandlet](https://dev.epicgames.com/documentation/unreal-engine/world-partition-builder-commandlet-reference)
- [PCG with GPU processing](https://dev.epicgames.com/documentation/unreal-engine/using-pcg-with-gpu-processing-in-unreal-engine)
- [World Partition HLOD](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition---hierarchical-level-of-detail-in-unreal-engine)
- [World Partition navigation](https://dev.epicgames.com/documentation/unreal-engine/world-partitioned-navigation-mesh)
- [Asynchronous asset loading](https://dev.epicgames.com/documentation/unreal-engine/asynchronous-asset-loading-in-unreal-engine)
- [Derived Data Cache](https://dev.epicgames.com/documentation/unreal-engine/using-derived-data-cache-in-unreal-engine)
- [Saving and loading](https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine)
