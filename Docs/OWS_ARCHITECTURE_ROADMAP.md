# OWS Product Architecture Roadmap

> **Status:** Accepted product direction and execution order. Stage 1 and Stage 2 are accepted; later subsystem architecture remains subject to repository research and Aurora's interview-driven approval.

## Purpose

This document prevents OWS features from becoming separate systems that merely happen to ship in the same repository. It records the accepted product shape, the order in which the major architecture programs proceed, the gates that protect downstream implementation, and the decisions that remain open.

Coherence is a first-class product requirement, not a cleanup phase. OWS is judged by whether its systems share understandable contracts, controls, state, authority, persistence, tooling, and player-facing behavior—not by how many unrelated features can be bundled together. A feature that works only through a private parallel framework does not satisfy the OWS architecture.

The roadmap is a target and planning contract. It does not claim that a planned plugin, module, API, gameplay framework, or living-world capability currently exists in `main`.

## Accepted product shape

OWS is both:

1. A complete, immediately playable Unreal Engine starter project that users can download and begin building from.
2. A suite of reusable first-party OWS plugins beneath that starter project.

The project game module and demo content form a thin composition, integration, and demonstration shell. They configure, connect, and demonstrate reusable OWS systems; reusable subsystem ownership does not remain in that shell as the accepted long-term architecture.

The controlling [OWS Platform Architecture and Composition Contract](OWS_PLATFORM_ARCHITECTURE.md) defines how these layers compose, how the included maintained forks evolve as OWS, and how later migration is gated. It does not authorize moving existing files outside a separately claimed implementation issue.

## Accepted architecture sequence

### Stage 1 — OWS platform architecture

Tracking: [epic #102](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/102), [architecture issue #103](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/103), and [publication issue #63](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/63).

The accepted [Stage 1 platform contract](OWS_PLATFORM_ARCHITECTURE.md) defines the product-wide rules every OWS subsystem must follow:

- Which responsibilities, if any, must be shared package-wide and where they belong.
- First-party plugin and module boundaries.
- Allowed dependency directions and forbidden coupling.
- Reusable product code and content versus the composition/demo shell, examples, and engineering labs.
- How each included maintained implementation fork evolves as an OWS-owned domain without becoming a separately installed dependency.
- Runtime, editor, developer-tool, validation, and test ownership.
- Public C++, Blueprint, data, event, tag, settings, and configuration extension rules.
- Input and context-routing ownership.
- Cooking, packaging, optional profiles, compatibility, versioning, deprecation, and migration.
- Conformance evidence required before a system is presented as reusable OWS product code.

Stage 1 produces the accepted platform contract, dependency and ownership matrix, and compatibility-first migration backlog. It does not perform module moves, asset relocation, or subsystem refactors; those require separately claimed implementation issues under the accepted gates.

### Stage 2 — Shared gameplay spine and authority/persistence

Tracking: [epic #104](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/104) and [architecture issue #105](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/105).

Stage 2 defines the shared semantics that characters, vehicles, interaction, combat, hacking, City Foundation, UI, multiplayer, saving, and future living-world systems consume:

- Stable player, world-object, item, equipment, vehicle, and streamed-feature identity.
- Shared state, tags, events, actions, cancellation, interruption, costs, cooldowns, and failure reasons.
- Whether health, attributes, damage, healing, reactions, death, teams, and factions are package-wide shared responsibilities or explicitly domain-owned contracts.
- Inventory and equipment integration.
- Client intent, server validation, authority, prediction, reconciliation, contention, late join, reconnect, and recovery.
- Player-profile, session, world-save, and server-save ownership.
- Schema versioning, migration, missing content, corruption, rollback, and recovery.
- Shared UI notification and observability boundaries.

Stage 2 has approved how OWS uses enabled foundations such as Gameplay Abilities, Sigil Inventory, and Save Extension. Their presence in the project did not pre-decide their OWS-facing responsibilities and does not permit consumer systems to expose their internal APIs as the OWS contract.

The accepted [OWS Shared Gameplay Spine, Authority, and Persistence Contract](OWS_GAMEPLAY_SPINE_ARCHITECTURE.md) establishes PlayerState-centered player identity, stable definition/entity/scoped IDs, the shared OWS action/event/tag language, the two-host GAS model, shared vitality and affiliation primitives, the maintained Sigil fork as the generalized inventory/equipment core, the maintained Save Extension fork as the low-level persistence engine, and the snapshot/journal/checkpoint recovery model. Implementation and support claims still require separately scoped migration issues and the accepted complete conformance matrix.

### Stage 3 — Domain architecture selection

Stage 3 is an intentionally unordered choice group coordinated by [gate issue #111](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/111):

- **Character and traversal** — [epic #106](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/106) and [architecture issue #107](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/107).
- **Systemic hacking** — [epic #10](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/10) and [architecture issue #50](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/50).
- **Vehicle platform and driving** — [epic #9](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/9), [platform issue #108](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/108), [benchmark issue #41](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/41), and [authoring issue #48](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/48).

Priority labels do not select an order among these three programs. Aurora explicitly chooses the next Stage 3 domain after Stages 1 and 2 are accepted, based on competitive parity, product differentiation, or the greatest immediately measurable improvement. Choosing one does not cancel or silently narrow the others.

Each selected program uses the City Foundation design process: audit the existing implementation, research current technical and product standards, interview Aurora on every consequential open choice, record the accepted architecture and acceptance matrix, and then rewrite or create implementation issues against that contract.

### Stage 4 — Combat implementation

Tracking: [epic #8](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/8), [combat architecture issue #30](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/30), and implementation issues #31–#40.

Combat consumes the accepted platform and shared gameplay-spine contracts plus the relevant accepted Stage 3 decisions. Stage 4 opens only after #111 records completion of the Aurora-selected Stage 3 architecture. It begins by resolving the combat-specific contract in #30 within those already accepted shared boundaries; it does not create another roadmap stage or reopen shared architecture. Issues #31–#40 then implement the firearm vertical slice.

Combat issues may not create private replacements for the accepted OWS-wide inventory, equipment, action, authority, vitality, affiliation, or persistence contracts. Shared primitives are owned by the Stage 2 spine; Combat owns combat-specific weapons, formulas, reactions, and presentation.

### Stage 5 — Living-world simulation

Tracking: [epic #109](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/109) and [architecture issue #110](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/110).

Stage 5 follows completion of the Stage 4 Combat epic. It establishes the first accepted living-world product slice and its architecture before implementation children are created. Research and interview topics include pedestrians, traffic, NPC state, schedules, representation and simulation level of detail, Actor promotion, interactions, missions, factions, systemic response, police/wanted behavior, City integration, World Partition, authority, persistence, networking, performance, authoring, and debugging.

Those topics are not yet accepted feature scope. Aurora selects the first living-world slice during Stage 5.

## Architecture-program method

Every architecture program follows the same decision process:

1. Audit current source, assets, configuration, documentation, issues, and observed behavior.
2. Research current Unreal Engine capabilities, relevant technical approaches, and professional product expectations.
3. Separate existing requirements from consequential choices that remain open.
4. Interview Aurora and record each explicit decision.
5. Publish a canonical accepted architecture and acceptance matrix.
6. Build a dependency and ownership map.
7. Create, rewrite, label, and gate implementation issues against the accepted contract.
8. Implement only through tightly scoped claimed issues, with verification and follow-up gaps recorded separately.

An architecture document is not evidence that its target is implemented. Passing tests for current behavior does not establish a future architecture stage.

## Checkout and issue-status rules

The GitHub issue labels carry the following meaning:

- `status: ready` — sufficiently defined and free of open architecture gates; the issue may be claimed.
- `status: in progress` — actively claimed and being worked on.
- `status: needs design` — research, interview, and decision work may be claimed; it does not authorize implementation.
- `status: architecture gated` — requirements and evidence may be refined, but implementation may not be claimed until the linked contracts are accepted and Aurora opens the applicable stage.
- `status: needs reproduction` — reproduce and measure the problem before proposing a fix.

The project field named `Architecture gate` identifies the roadmap gate governing an issue. It is not permission to start the issue. Checkout permission comes from the issue status, cleared native blockers, and Aurora's explicit stage or domain selection. Priority labels rank work only inside a program Aurora has opened; they never override an architecture gate or select the Stage 3 domain.

An issue marked architecture-gated is not a contributor implementation task even if its requirements appear straightforward. A consumer issue may not define a substitute shared contract simply to remove its blocker.

Existing tightly scoped maintenance, reproduction, testing, and documentation may continue when the issue is genuinely ready and the work does not alter an unresolved architecture boundary. Out-of-scope findings become separate issues rather than expanding the claimed task.

## City Foundation coordination

The accepted [City Foundation architecture](CITY_FOUNDATION_ARCHITECTURE.md) remains independently controlling for City work. Its editor/runtime split and first-party plugin boundary are compatible with the accepted OWS product shape.

City import, normalization, roads, baking, streaming, cache, and validation work may continue under its accepted contract. The accepted Stage 1 product-wide rules apply wherever they do not conflict with City's controlling domain contract. The approved Stage 2 decisions supply shared player/entity identity, action, server, and persistence semantics while City remains authoritative for source provenance, recipe epochs, logical cells, feature identity generation, Actor promotion, and cache behavior. A conflict returns to Aurora for an explicit decision; no document silently overrides another accepted contract.

## Deferred implementation and later-stage decisions

Stage 1 deliberately delegates details that require their own accepted architecture or tightly scoped implementation issue:

- Exact descriptor names, module inventories, loading phases, and file destinations within the accepted Platform and domain-family rules.
- Exact public API, tag/event payload, profile/settings/configuration schemas, C++/Blueprint symbols, file destinations, and internal data layouts that implement the approved Stage 2 semantics.
- The precise roles of Character Movement Component, Mover, and later domain-specific foundations not decided by Stages 1 or 2.
- Exact asset moves, redirect inventories, Input priority values, cook chunks, optional profile contents, installer presentation, and release artifacts.
- Detailed Character, Vehicle, Hacking, Combat, and Living World implementation technology.
- The internal order of Character, Hacking, and Driving within Stage 3.
- The first accepted Living World feature slice.

These are not permission to reopen settled Stage 1 product, fork, ownership, dependency, installation, input, content-tier, extension, compatibility, or migration rules. Silence, current file placement, an enabled plugin, a technical convenience, or a consumer issue does not decide a delegated detail.

## Source-of-truth relationship

When repository documents disagree, use this order:

1. Aurora's latest explicit requirement.
2. The accepted contract applicable to the subject and within that contract's explicit scope.
3. Merged source, assets, and configuration for what is implemented today.
4. Current-stack summaries and working documents.

No accepted platform, shared, or subsystem contract silently overrides another accepted contract. An overlap or conflict returns to Aurora for an explicit decision. Target architecture governs future design within its scope; current implementation remains the evidence for what exists today. Neither may be presented as the other.
