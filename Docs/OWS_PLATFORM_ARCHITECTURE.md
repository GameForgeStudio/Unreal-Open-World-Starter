# OWS Platform Architecture and Composition Contract

> **Status:** Accepted Stage 1 product architecture. This document defines target ownership and migration rules and does not claim that the target modules, plugins, assets, tooling, or conformance automation already exist in `main`.

## Purpose

OWS is one coherent Unreal Engine starter package, not a collection of unrelated systems that happen to share a repository. This contract defines how the complete playable starter project is built over reusable first-party OWS plugin families, how the maintained implementation forks evolve as OWS, which dependencies are permitted, what the project shell owns, and what evidence is required before a subsystem is presented as reusable OWS product code.

Coherence is a first-class product requirement. Installation, controls, settings, state, extension points, tests, cooking, migration, and player-facing behavior must read as one product even when the implementation is internally modular.

This contract controls Stage 1 of the [OWS Product Architecture Roadmap](OWS_ARCHITECTURE_ROADMAP.md). The accepted [City Foundation architecture](CITY_FOUNDATION_ARCHITECTURE.md) remains controlling within its own scope. Neither contract silently overrides the other; a conflict returns to Aurora for an explicit decision.

## Product and installation contract

OWS is both:

1. A complete, immediately playable starter project.
2. A reusable family of first-party OWS plugins beneath that project.

Internal modularity is not a user installation burden.

- A normal user obtains one complete, version-matched OWS bundle.
- The included starting plugins are never separate downloads or separately maintained prerequisites.
- The OWS starter project arrives configured and opens into a working canonical experience.
- An existing-project workflow uses an OWS Editor setup assistant rather than a manual sequence of plugin, tag, input, collision, cook, or configuration changes.
- The assistant installs or applies the complete compatible OWS bundle, detects the engine and project state, previews every change, creates a recoverable backup, applies only changes the user approves as one atomic operation, reports conflicts instead of overwriting them silently, validates the result, offers specific repair actions, and can roll back a failed or rejected application.
- The exact release archive, installer presentation, and distribution channel remain owned by the distribution program; they may not weaken the one-bundle and reviewed-setup requirements.

## Maintained-fork model

GASPALS, KinetiForge, Sigil Inventory, Save Extension, and other included starting implementations are stepping stones incorporated into OWS. OWS maintains and evolves the repository's included copies as part of the complete product.

- OWS starts from each included implementation as a whole rather than selecting only the portions currently considered useful.
- OWS owns continued modification, integration, testing, upgrade, and coherent presentation of its maintained forks.
- Users do not install or update an upstream original independently of OWS.
- An existing top-level plugin folder or descriptor identity may remain as the intentional provenance boundary.
- Below that boundary, newly supported modules, APIs, assets, settings, tags, events, tests, and documentation use OWS-facing identity and contracts.
- Inherited names and types may remain Internal or deprecated during compatibility-first migration; they do not define new supported OWS surfaces.
- GASPALS evolves in place into the complete OWS Character domain family.
- KinetiForge evolves in place into the complete OWS Vehicle domain family.
- The standard GASPALS and KinetiForge implementations do not receive permanent wrapper plugins. A compatibility bridge is justified only for a future alternative implementation. The approved Stage 2 decisions make the complete maintained Sigil fork OWS's generalized inventory/equipment core and the complete maintained Save Extension fork OWS's low-level persistence engine behind OWS-owned public contracts.

This fork model makes OWS self-contained. OWS must build, package, run, migrate, and evolve from the contents of the OWS release itself.

## Target product layers

```text
OWS starter project and /Game/OWS showcase
        |
        | selects one OWS composition profile
        v
Narrow cross-domain integration plugin families
        |
        v
OWS domain plugin families
        |
        v
Minimal OWS Platform contracts and registries
        |
        v
Unreal Engine
```

The arrows show allowed downward dependencies. A lower layer never depends on a higher layer. Events, registered contracts, and dependency inversion may carry information upward without reversing the build or content-reference graph.

## Target plugin-family ownership

The table names accepted logical families. Exact descriptor and module inventories are created by their scoped implementation or domain-architecture issues and must follow the naming and dependency rules in this contract.

| Logical family | Physical direction | Accepted ownership |
| --- | --- | --- |
| Platform | Retain `Plugins/OWSFramework`; replace the current misleading `OWSCore` boundary compatibility-first | Package contracts, registration, profiles, settings registry, compatibility, setup, validation, and conformance infrastructure; no domain gameplay |
| Input | First-party OWS domain family | Semantic input registration, context stack, routing, priority groups, rebinding, and device-independent control behavior |
| Interaction | First-party OWS domain family | Selector stacks, precision targeting, interactable contracts, activation, visualization, validation, and domain tests |
| UI | First-party OWS domain family | Hotbars, prompts, menus, notifications, settings presentation, view models, and presentation tooling; it observes public state and requests actions but cannot possess pawns, replace gameplay contexts, or own authoritative rosters |
| Character | Evolve the complete GASPALS fork in place | Locomotion, traversal, animation architecture, movement modes, character camera behavior, character input assets, authoring, validation, and tests |
| Vehicle | Evolve the complete KinetiForge fork in place | Vehicle simulation, driving, vehicle-specific input and settings, authoring, validation, and tests; exact internal architecture remains Stage 3 work |
| Character–Vehicle integration | Narrow first-party integration plugin family; exact descriptor name deferred | Reusable possession, entry, exit, bailout, recovery, and handoff behavior using only the participating domains' public contracts |
| City Foundation | Planned `Plugins/OWSCityFoundation` | The independently accepted City runtime/editor boundary defined by `CITY_FOUNDATION_ARCHITECTURE.md` |
| Starter shell | `Source/OWS` and `/Game/OWS` | Composition, configuration, canonical playable characters and variants, maps, game modes, controllers, showcase content, and end-to-end acceptance |
| Future domains | Added only by an accepted architecture program | Hacking, Combat, Living World, and other domains; Stage 1 does not invent their internal contracts |

### Module-family rule

A cohesive domain uses one plugin family and separates concerns with modules when needed:

- **Runtime** modules contain shippable behavior and data types.
- **Editor** modules contain authoring tools, importers, setup UI, visualization, migration helpers, and editor validation.
- **Test/Validation** boundaries contain developer-only automation and fixtures. Their descriptor types, target/configuration filters, compile guards, and cook/staging rules must prevent them from entering Shipping builds; Unreal does not provide a magic `Tests` module category, and the exact module names remain scoped implementation decisions.

A domain is split into another plugin only when independent installation, activation, cooking, or dependency direction genuinely requires it. Character–Vehicle integration qualifies because placing it inside either domain would make the whole host plugin depend on the other domain.

Retained top-level fork identities are the only naming exception. Newly supported modules and public surfaces use an `OWS` identity. Exact module names, suffixes, and loading phases require a scoped implementation issue.

Debug overlays and diagnostic presentation are developer tooling. They are excluded from Shipping builds unless a separately approved product requirement explicitly enables a particular diagnostic surface.

## Platform responsibilities

The Platform family is deliberately small. It may own:

- Public package-wide service and registration contracts.
- Composition-profile schema and lifecycle.
- Extensible settings registration and common presentation metadata.
- Compatibility, version, deprecation, redirect, and migration infrastructure.
- Setup-assistant and project-conformance contracts.
- Dependency, reference, cook, configuration, and API validation infrastructure.
- Package-wide diagnostic conventions that do not own domain state.

Platform may not own character movement, vehicle behavior, interaction targeting, UI presentation, hacking, combat, City generation, inventory behavior, saving behavior, or another domain implementation. The accepted [Stage 2 shared gameplay contract](OWS_GAMEPLAY_SPINE_ARCHITECTURE.md) owns the shared gameplay semantics; Platform may not duplicate or pre-empt them.

## Capability ownership and active implementations

OWS may contain alternative implementations in source, but every capability declares how many implementations may be active:

- An authoritative capability requires exactly one active owner. Character movement, Vehicle physics, Input routing, and persistence authority are examples.
- An optional capability allows zero or one active owner.
- Multiple active implementations are permitted only for a contract explicitly designed to aggregate them, such as diagnostic sinks or extension listeners.
- A composition profile selects among compatible implementations already installed, enabled, compiled, and packaged in that build.
- Setup, Editor, cook, and startup validation reject missing, duplicate, or incompatible authoritative owners before gameplay begins.
- The complete default OWS profile resolves every required capability without additional installation or user assembly.

This rule permits intentional replacement without allowing competing systems to control the same character, vehicle, input stream, or persistent state.

## Dependency contract

### Allowed

- The starter project may depend on OWS domain and integration plugin families.
- A domain may depend on Platform and Unreal Engine facilities.
- Domains may depend on an explicitly approved peer's Supported public contract when the ownership matrix requires it and the edge remains acyclic. Input registration, Interaction consuming the semantic Activate action, and UI observing read-only domain contracts are accepted examples.
- A domain may use its own maintained-fork internals behind OWS-facing public surfaces.
- A narrow integration plugin may depend on the public contracts of every domain it integrates.
- Editor and Test/Validation modules may depend on their owning Runtime module.
- A composition profile may select among domains and integrations that are already installed, enabled, compiled, and packaged for that build; it cannot enable absent code through data alone.

### Forbidden

- Platform depending on a domain, an integration plugin, the project `OWS` module, or `/Game` content.
- A reusable plugin referencing `/Game` content.
- A domain accessing a peer domain's Private headers, assets, mutable state, or implementation types.
- A base domain depending on a cross-domain integration plugin.
- Fork-origin implementation types leaking through supported OWS public APIs.
- Circular plugin, module, content, configuration, or initialization dependencies.
- A miscellaneous catch-all glue module.
- Runtime code depending on Editor or Test/Validation modules.
- Current file placement, plugin enablement, a hard-coded asset path, or technical convenience being treated as authorization for a dependency.

Code dependencies and content-reference dependencies must both be enforced. A valid C++ graph does not excuse a reverse `/Game` reference or a hard asset load that makes a plugin project-specific.

An Unreal plugin reference marked optional only permits the referenced plugin to be absent; it does not make public headers, `Build.cs` dependencies, or asset references optional. Alternative implementations therefore require an isolated OWS-facing contract and compatibility boundary. Registration and initialization may not rely on incidental module order inside one loading phase; required ordering uses controlled module loading such as `LoadModule`/`LoadModuleChecked`, or an explicit readiness lifecycle.

## Cross-domain integration

Cross-domain cohesion is reusable product behavior, not starter-map scripting.

- Each integration boundary has one explicit purpose and names the participating domains.
- It consumes only supported public contracts.
- It owns no duplicate domain state and cannot become the authority for either domain.
- The default OWS profile enables required integrations automatically.
- Character–Vehicle entry, possession, exit, bailout, recovery, and re-entry migrate from the project module only after both Character and Vehicle contracts exist.
- Other integrations receive their own scoped boundary only when an actual dependency requires one.

## Composition profiles and feature activation

OWS owns a stable profile asset that declares the domains, integrations, runtime content, defaults, and configuration composing an experience.

- The starter project ships with one complete default profile that works immediately.
- Developers may create profiles that omit or replace domains without deleting OWS source or content. Build and packaging configuration must include the corresponding plugin set before that profile can use it.
- Profiles use explicit asset ownership, and omitted content must not be loaded or cooked accidentally. Soft references may control deferred loading but do not guarantee cooking; Asset Manager rules, Primary Asset Labels, or equivalent validated build automation own inclusion.
- Unreal Game Features and Modular Gameplay may receive an optional OWS bridge.
- Game Features are not the OWS source of truth, and base OWS startup never depends on dynamic feature activation succeeding. This optional boundary also avoids making OWS's baseline depend on systems Epic labels Beta in Unreal Engine 5.8.
- A profile cannot reverse the accepted dependency graph.

The exact profile classes, fields, identifiers, and optional chunk layout are implementation details owned by scoped Platform and packaging issues.

## Content ownership and cooking tiers

| Tier | Owner | Default shipping behavior |
| --- | --- | --- |
| Reusable OWS product content | Owning domain plugin mount | Included by the build/cook manifest for profiles packaged in that build |
| Starter composition and showcase | `/Game/OWS` | Included by the starter project's canonical build/cook profile |
| Inherited demonstrations and engineering labs | Maintained fork or explicit development area | Available in source; excluded from normal OWS cooks and the standard user workflow |
| Editor tools and authoring previews | Owning Editor module/plugin content | Explicitly excluded from runtime cooks and staging |
| Test fixtures | Owning Test/Validation boundary | Explicitly excluded from Shipping code, cooks, and staging |

`OWS_CombinedDemo` remains the canonical default playable experience. It evolves deliberately into an OWS-authored showcase for the complete default profile. The six current level utility pads remain starter-shell composition unless a later scoped issue identifies and extracts genuinely reusable behavior. This contract does not authorize deleting, stripping, relocating, or rebuilding any current level or asset. Each content migration requires an exact asset list, owner, redirect plan, cook comparison, and unchanged-behavior evidence.

The complete OWS source installation contains every maintained fork, tool, and development resource. A developer's packaged game contains only the Runtime plugins and assets selected by its pre-runtime build/cook manifest for the profiles it ships. Packaging validation must report missing required content and forbidden development-content leaks with repair guidance. Module type alone does not exclude plugin content; explicit validated cook and staging rules enforce the asset boundary.

## Input contract

The Input domain is the single routing authority.

- Domains own their semantic actions and default mappings and register them with Input.
- Activate, Cancel, Aim, and other shared actions have one canonical OWS meaning.
- The starter profile selects the complete default control scheme.
- Domains request and release contexts through the router; they may not remove or override another domain's context directly.
- Context priority groups are centrally defined and validated.
- Supported controls are rebindable and device-independent.
- Runtime product code may not poll hard-coded keyboard keys, mouse buttons, or controller buttons.
- Debug/test input may use explicit development-only paths that cannot ship accidentally.

Stage 2 owns any shared authoritative gameplay-action semantics beyond local input routing.

## Settings and configuration contract

Each domain owns the meaning and runtime consumption of its developer, project, and player settings. Platform owns an extensible registry that gathers them into one coherent OWS experience.

- OWS supplies working defaults.
- Projects override supported defaults without editing OWS source.
- New domains can register settings without replacing a monolithic global class.
- A registered setting must have a tested runtime consumer.
- The current non-extensible global `final` settings-class arrangement migrates compatibility-first through its own issue.
- Package-wide tags, collision channels, input priorities, profiles, and comparable shared registries have one OWS authority.
- Fork-origin configuration migrates deliberately into OWS-owned schemas rather than being copied across project and plugin files.
- Redirects remain with the system that owns the renamed data wherever Unreal packaging permits. Unreal Engine 5.8 does not automatically package game-plugin configuration with a project, so required redirects, Asset Manager registrations, tags, collision settings, and comparable configuration are merged into project configuration through the reviewed setup workflow and verified in a packaged build.
- Runtime domains never silently rewrite global project configuration.
- Secrets, credentials, and machine-specific tokens are invalid repository configuration.

## Public extension and compatibility contract

Every exposed surface is classified:

| Classification | Promise |
| --- | --- |
| Supported | Stable game-developer contract with migration guarantees |
| Experimental | Available for evaluation but explicitly subject to documented change |
| Internal | Implementation detail that downstream projects must not be required to use |

Supported surfaces include applicable C++ headers and types, Blueprint nodes and pins, interfaces, events, tags, asset identifiers, data assets, settings keys, profile fields, configuration keys, and serialized schemas.

- Normal extension uses components, interfaces, events, registered services, profiles, and data assets rather than edits to maintained-fork internals.
- Blueprint and C++ receive equivalent extension points where Unreal permits.
- Supported APIs never expose inherited fork-specific types.
- Public dependencies are minimized; implementation-only dependencies remain Private.
- Replacement APIs are introduced before supported APIs are removed.

### Versioning

OWS release tooling enforces semantic versioning; Unreal's descriptor `Version` and display-oriented `VersionName` fields do not enforce this policy by themselves:

- Patch releases fix defects without breaking Supported contracts.
- Minor releases add capabilities and may change Experimental surfaces.
- A Supported API is deprecated in a minor release, retained through at least the following minor release, and removed only in a major release.
- Renamed code, assets, Blueprint nodes, tags, settings, profile fields, and serialized data receive redirects or migration tooling.
- Every breaking migration is documented and testable.

Each OWS release targets an explicit Unreal minor version. The current Unreal minor receives active development; the immediately previous minor receives compatibility and critical-fix maintenance through a separately maintained, built, tested, and distributed engine-specific release line. That previous-minor support promise begins only when its separate release line passes the full applicable conformance matrix. One compiled OWS plugin build or newer-version asset bundle is never presented as safely compatible across incompatible Unreal minor versions. Older versions may remain available as tagged releases but are not advertised as supported. Every release publishes an OWS-to-Unreal compatibility table and migration guidance.

## Runtime, editor, and test boundaries

- Runtime modules may ship and therefore contain no editor-only dependencies, utilities, or test fixtures.
- Editor modules own setup UI, importers, authoring tools, visualization, asset generation, migration helpers, and editor validation.
- Test/Validation modules own automation and development fixtures. Descriptor types, target/configuration filters, automation compile guards, cook rules, and staging validation keep them out of Shipping builds.
- Integration tests that require the full starter project remain in the shell's test boundary; they do not prove plugin reuse independently.
- Editor visualization embedded in a runtime component must be guarded and owned so it cannot create a runtime dependency on `UnrealEd`.

## Reusable-product conformance

A subsystem may be presented as a reusable OWS domain only when evidence demonstrates all applicable items:

1. Its Runtime module builds without the starter-project module.
2. It loads in a minimal clean host project.
3. It contains no forbidden `/Game`, peer-private, or reverse-layer references.
4. Its Supported C++ and Blueprint surfaces compile and validate.
5. Its domain-owned automated tests pass.
6. The complete OWS integration tests pass.
7. A selected profile cooks, packages, launches, and passes its smoke test.
8. Installation, upgrade, redirects, and migration checks pass.
9. Its declared multiplayer matrix passes when it advertises network support.
10. Generated files and test state are cleaned up and the host project is left as found.

The canonical playable map remains essential integration evidence, but it is not sufficient evidence of independent reuse.

## Platform and multiplayer support claims

Architecture requirements and certified support are different claims.

- OWS architecture preserves authority boundaries and must not preclude multiplayer or dedicated-server support from the beginning; this is a design requirement, not certification that current behavior is multiplayer-safe.
- A platform or network mode is advertised only after its complete build, cook, launch, and gameplay matrix passes.
- The current verified baseline is Windows Editor/client and single-player behavior.
- The accepted Stage 2 contract defines the first listen-server, dedicated-server, multi-client, late-join, reconnect, authority, and persistence matrix; support remains unadvertised until that matrix passes.
- Consoles and additional desktop platforms remain eligible but are not advertised without actual testing.

## Current implementation audit

This table records the principal Stage 1 findings at the time the contract was accepted. It is evidence of migration need, not permission to refactor outside a claimed issue.

| Current state | Contract gap |
| --- | --- |
| The project `OWS` Runtime module owns public selector, interaction, vehicle handoff, bailout, debug, and editor-test code | Reusable behavior remains in the thin shell; editor/test dependencies are mixed into Runtime |
| `Plugins/OWSFramework/OWSCore` owns hotbar, raw input/context changes, a global final settings class, and vehicle-specific settings | The module is neither a minimal Platform boundary nor a cohesive domain owner |
| OWSFramework loads both GASPALS and `/Game/OWS` input assets by literal path | A reusable plugin depends on a maintained-fork detail and reverse-references project content |
| Vehicle interaction exposes and calls concrete KinetiForge/Chaos types and hard-loads KinetiForge/GASPALS assets | Fork-specific implementation leaks through OWS integration and public surfaces |
| Input uses Enhanced Input contexts, legacy bindings, raw polling, and hard-coded buttons across several owners | No single routing, priority, semantic-action, or rebinding authority exists |
| Tests live inside the project Runtime module and hard-code the canonical map and foundation assets | Integration coverage exists, but plugin-local and clean-host conformance do not |
| Cook policy relies mainly on transitive references and always cooks an engine example input directory | Product, example, lab, editor, and optional-profile cook ownership is not explicit |
| The canonical map composes fork-derived demo content and editor utility actors | Current playability is established, but showcase, lab, and packaged-runtime tiers are not physically separated |
| Project configuration duplicates or owns fork redirects, tags, and example settings | Configuration ownership and migration are inconsistent |
| Project configuration contains a committed static Android File Server token | Security remediation is required in a separate issue; the token is not reproduced here |

## Migration method

Migration is additive and compatibility-first:

1. Establish the accepted target owner and behavioral baseline before moving responsibility.
2. Migrate one tightly scoped responsibility per issue.
3. Keep temporary redirects or compatibility shims so existing C++, Blueprints, assets, and configuration continue working.
4. Verify the canonical experience and the relevant clean-host/domain evidence after each move.
5. Retire legacy surfaces only after every consumer has migrated and the accepted deprecation window permits removal.
6. Never perform a repository-wide rename, asset move, or architectural cutover as one task.

## Migration backlog order

| Wave | Tracking work | Outcome | Entry gate | Required evidence | Primary risk controlled |
| --- | --- | --- | --- | --- | --- |
| 0. Publish | [#103](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/103), [#63](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/63) | Merge this contract, matrix, and backlog; synchronize roadmap, contributor, distribution, canonical-map, Character, and Vehicle issue language | Aurora accepts #103 | Canonical documentation and the live issue graph contain no external-provider, cherry-pick, ownership, or gate contradictions | Contributors acting on rejected architecture |
| 1. Baseline and guardrails | [#115](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/115); security remediation remains separate in [#114](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/114) | Capture current build, canonical-map, cook/package, asset-reference, dependency, and test evidence; add non-mutating reporting guardrails | Stage 1 contract published and #115 claimed | Reproducible current-state reports and unchanged canonical behavior | Enforcing a target before known violations and behavior are measured |
| 2. Minimal Platform | [#116](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/116) | Add neutral Runtime, Editor/Validation, and Test boundaries; introduce only accepted profiles, registries, compatibility, setup, and validation infrastructure; retain legacy `OWSCore` surfaces as shims | #116 design approved, rewritten as ready, and claimed; no Stage 2 gameplay semantics | Platform builds and loads in a clean host with no domain or `/Game` dependency; legacy surfaces and the canonical demo still function | Creating a gameplay god module or breaking serialized `OWSCore` consumers |
| 3. Shared gameplay spine | [#105](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/105) | Publish the accepted [Stage 2 contract](OWS_GAMEPLAY_SPINE_ARCHITECTURE.md), reconcile City overlaps, link consumers, and create the gated implementation backlog | Stage 1 published; Aurora explicitly opens Stage 2 and #105 becomes active | Stage 2 document merged, #105 accepted and closed, consumers linked, and implementation children remain gated to the contract | Domains creating incompatible substitute spines or treating architecture as implementation |
| 4. Shared domain migrations | [#117](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/117), [#118](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/118), [#119](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/119) | Migrate Input first, then Interaction, then UI, one responsibility per issue | Platform interfaces available; applicable Stage 2 contracts accepted; each migration issue separately ready and claimed | Unchanged controls, targeting, activation, and UI behavior; independent clean-host/domain tests; no raw runtime key polling or `/Game` reverse references | Regressing verified controls or moving mixed hotbar responsibilities to the wrong owner |
| 5. Maintained domain forks | [#107](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/107), [#108](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/108) | Evolve the complete Character and Vehicle forks in place under their accepted domain architectures | Aurora selects the applicable Stage 3 domain; #107 or #108 is accepted; a separate implementation issue is rewritten, ready, and claimed | Complete inherited-capability regression, clean-host build/load, OWS-facing Supported surfaces, and domain cook/package evidence | Losing capabilities through selective migration or reintroducing the rejected external-provider model |
| 6. Cross-domain integration | [#120](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/120) | Move Character–Vehicle entry, exit, bailout, recovery, possession, and tests into its narrow integration plugin | Both participating public contracts implemented; #120 separately ready and claimed | Character and Vehicle build independently; repeated entry/exit/bailout/recovery/re-entry passes through public contracts | Dependency cycles or damage to the accepted bailout and handoff behavior |
| 7. Content/config/cook ownership | [#121](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/121) | Migrate exact asset and configuration sets one domain at a time; enforce build/cook manifests and development-content exclusions | Owning domain stable; exact asset/config list and redirect plan approved in a claimed child issue | Redirect and reference validation, before/after cook manifests, no required-content loss, and no editor/lab/test leakage | Broken binary references, World Partition damage, or accidental cook expansion |
| 8. Thin shell | [#122](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/122) | Leave only composition and showcase responsibility in `Source/OWS` and `/Game/OWS` | Every displaced responsibility has a verified owner; #122 rewritten as ready and claimed with an exact inventory | Project-shell dependency audit passes and `OWS_CombinedDemo` remains visually and behaviorally unchanged | Treating “thin” as permission to strip working showcase content |
| 9. Installation and release hardening | [#123](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/123), [#65](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/65), [#66](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/66) | Complete the one-bundle starter install, reviewed existing-project setup and recovery, profile packaging, compatibility table, CI, and release evidence | Stable Platform/profile/config contracts and migrated domains; distribution issues separately ready and claimed | Starter opens and plays without setup; existing-project installation is reviewable and recoverable; supported-engine lines, CI, packaged launch, and release artifacts pass | Exposing internal complexity, overwriting projects, or advertising untested support |

City implementation continues independently under its accepted contract; only later alignment with Platform or Stage 2 waits for those interfaces. This migration order does not select Character, Hacking, or Driving within Stage 3 and does not alter or open Roadmap Stages 4 or 5. Their existing gates remain controlling.

## Not authorized by this contract

- Moving or deleting source, assets, maps, external actors, plugin directories, configuration, or tests.
- Creating the target plugins or modules without a separately claimed implementation issue.
- Replacing working character, vehicle, interaction, input, UI, or bailout behavior.
- Inventing Stage 2 gameplay identity, action, authority, inventory, save, or failure semantics.
- Choosing the Stage 3 domain order.
- Choosing Character Movement Component, Mover, Gameplay Abilities, Sigil Inventory, Save Extension, or another foundation's detailed long-term role.
- Advertising untested platforms or network modes.
- Treating a target architecture statement as evidence that its implementation exists.

## Deferred implementation details

The following are intentionally delegated rather than left ambiguous inside Stage 1:

- Exact descriptor names, module names, module counts, loading phases, and file destinations within the accepted family and naming rules.
- Exact public interfaces, tags, events, profile fields, settings keys, and serialized schemas; Stage 2 or the owning domain architecture controls gameplay semantics.
- Exact Input priority values and mapping assets.
- Responsibility-by-responsibility extraction of the current hotbar component.
- Exact asset moves, redirect inventories, cook chunks, and optional profile contents.
- Exact installation archive and setup-assistant presentation.
- The detailed roles of Gameplay Abilities, Sigil Inventory, Save Extension, Mover, and other enabled foundations.
- The Stage 3 domain order and the first Living World slice.

These details require scoped issues under the accepted dependency and migration rules. They may not reverse a settled Stage 1 decision.

## Research basis

- [Plugins in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/plugins-in-unreal-engine)
- [Unreal Engine Modules](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-modules)
- [Gameplay Modules](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-modules-in-unreal-engine)
- [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)
- [Game Features and Modular Gameplay](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-features-and-modular-gameplay-in-unreal-engine)
- [Asset Management](https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine)
- [Cooking and Chunking](https://dev.epicgames.com/documentation/en-us/unreal-engine/cooking-content-and-creating-chunks-in-unreal-engine)
- [Automation Test Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine)
- [Data Validation](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-validation-in-unreal-engine)
- [Core Redirects](https://dev.epicgames.com/documentation/en-us/unreal-engine/core-redirects-in-unreal-engine)
