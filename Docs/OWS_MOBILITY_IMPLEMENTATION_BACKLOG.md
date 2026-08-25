# OWS Mobility Implementation Backlog

> **Status:** Accepted reconciled architecture-gated backlog for [issue #147](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/147), explicitly accepted by Aurora on 2026-08-25 with the Mobility architecture and matrix. It does not authorize checkout or claim that any future capability exists. Every implementation issue remains unavailable until its controlling contracts exist and Aurora separately makes that exact issue ready.

## Purpose

This document is the navigation map from the accepted [Mobility Architecture](OWS_MOBILITY_ARCHITECTURE.md) and [Mobility Acceptance Matrix](OWS_MOBILITY_ACCEPTANCE_MATRIX.md) to tightly owned GitHub work. The GitHub issue body is the live scope for a claimed task; this map records dependency and non-overlap.

An out-of-scope discovery becomes a separate issue. It never broadens the issue currently being worked.

## Controlling work

| Issue | Controlling responsibility |
| --- | --- |
| [#147](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/147) | Architecture, evidence, complete target matrix, decision traceability, and this reconciled backlog only |
| [#107](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/107) | Complete Character/traversal architecture; #147 supplies its accepted Mobility-facing contribution |
| [#108](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/108) | Complete Vehicle/driving architecture; #147 supplies its accepted Mobility-facing contribution |
| [#21](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/21) | Collision/force-response contract and evidence-derived ranges, not implementation |
| [#41](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/41) | Driving/point-kinematic benchmark and evidence, not implementation |
| [#48](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/48) | Vehicle driving/tuning data and workflow; graph tooling moved to #156 |
| [#120](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/120) | Integration implementation-family coordination and compatibility migration gates, not one mega implementation |
| [#27](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/27) | Mobility consumer for Shared Actions, source-time/history validation, idempotent results, defensive admission, and disposable anticipation |
| [#28](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/28) | Multiplayer reservation/contention/recovery verification, not lease implementation |
| [#111](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/111) | Stage 3 coordination gate; it remains open until Aurora separately opens Stage 4 |

## Domain implementation children

| Order | Issue | Exact scope | Principal blockers | Exit proof |
| --- | --- | --- | --- | --- |
| 1 | [#150](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/150) | Character Mobility participant/capability contract | #107, #125, #127 | Independent build and synthetic player/NPC/provider conformance |
| 2 | [#151](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/151) | CMC support-relative execution and Character history | #150, #21 | Moving-support, prediction, same-time history, frame/network evidence |
| 3 | [#152](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/152) | Balance, counterbalance, Grip, edge catch, failure, optional stamina | #150/#151, #21 | MOB-PHY Character-owned rows without competing vitality/stamina authority |
| 4 | [#153](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/153) | Character animation, embodiment, and camera providers | #150 and accepted descriptors | Recipe/provider/camera/embodiment conformance independent of Integration implementation |
| 5 | [#154](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/154) | Exact Vehicle point kinematics and authoritative history | #108, #41, #125 | Analytic rigid/rotating/bone/articulated/custom-provider evidence |
| 6 | [#155](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/155) | Vehicle graph runtime, occupancy, and control records | #154, #108 | Independent graph/record/replication conformance |
| 7 | [#156](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/156) | Vehicle graph draft/manual authoring, visualization, validation, migration | #154/#155, #108 | MOB-GRAPH editor/CI/cook/migration evidence |

Character and Vehicle work may proceed in parallel after their own prerequisites. Neither waits for Integration, and neither may take a private dependency on the other.

## Integration implementation children

| Order | Issue | Exact scope | Principal blockers | Exit proof |
| --- | --- | --- | --- | --- |
| 8 | [#157](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/157) | Integration shell and authoritative relationship snapshot | Implemented public domain contracts, #125/#127 | Acyclic build, orthogonal state, late-join snapshot, no current-gameplay replacement |
| 9 | [#27](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/27) | Authority/Action/source-time adapter | #157, #125/#127/#138 | Idempotency, source-typed history validation, anticipation, security limits |
| 10 | [#158](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/158) | Reservation leases and deterministic contention | #157, #27, #155 | Lease/multiplicity/release invariants; #28 supplies multiplayer proof |
| 11 | [#159](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/159) | Exterior support and one two-way coupling path | #151/#152, #154/#155, #157, #21 | MOB-PHY force/contact/collision evidence |
| 12 | [#160](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/160) | Transition-route and presentation-recipe resolution | #153, #156, #157 | Neutral-ID/provider/missing-recipe conformance |
| 13 | [#161](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/161) | Stopped/moving boarding and same-Vehicle infiltration | #27, #151–#160 as applicable | MOB-ACT lifecycle, physical continuity, player/NPC/network evidence |
| 14 | [#162](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/162) | Passengers, seats/handoff, embodiment orchestration, interiors | #153/#155/#158/#160/#161 | MOB-OCC and MOB-INT same-Vehicle evidence, V13 required |
| 15 | [#163](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/163) | Stopped exit/bailout/roll/recovery/re-entry compatibility migration | Applicable prior slices, #21/#27 | MOB-EXIT plus every protected MOB-REG row before legacy retirement |
| 16 | [#164](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/164) | Prediction, correction, relevance, and network failure convergence | #157 and implemented feature slices, #27/#138 | MOB-NET across roles/backends/frame/impairment axes |
| 17 | [#165](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/165) | Persistence, travel, World Partition, save/checkpoint/restart adapter | #157/#164, #132/#138 | MOB-LIFE field-lifetime and cross-record revalidation evidence |
| 18 | [#166](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/166) | Mobility observability adapters | #140 plus each instrumented slice | Complete correlated, bounded, privacy-preserving evidence schema |
| 19 | [#167](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/167) | Generated Stage 3 Mobility conformance/release suite | #141 plus stable fixtures/slices | Complete T0–T5 matrix and support manifest |
| Later | [#168](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/168) | Cross-Vehicle runtime implementation and certification | Complete passing same-Vehicle profile plus separate Aurora approval | Separately expanded transfer matrix; never blocks slice one |

## Shared-domain consumers

These existing issues retain their accepted owners and do not become Mobility implementations:

- [#117](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/117) maps player devices and registers domain-owned semantic Actions/default mappings. NPC/server intent does not pretend to own a device.
- [#118](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/118) selects exact targets and affordances; Integration validates Mobility feasibility and authority.
- [#119](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/119) presents prompts/settings/results without owning input, camera transforms, or gameplay state.
- [#125](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/125) and [#127](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/127) supply stable identity and Shared Action semantics.
- [#132](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/132), [#138](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/138), and [#139](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/139) supply generic persistence/recovery/City boundaries consumed by #165.
- [#140](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/140) supplies shared observability infrastructure consumed by #166.
- [#141](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/141) supplies the Stage 2 harness consumed by #167; it is not Stage 3 evidence by itself.

## Dependency spine

`accepted shared contracts → #21/#41 and #107/#108 contracts → #150–#156 independent domain implementation → #157 relationship snapshot → #27 → #158 → #159/#160 → #161/#162 → #163/#164/#165 → #167 release certification → #168 later transfer slice`

Input, Interaction, UI, observability, accessibility, security, and tests advance with each applicable feature rather than being postponed to final hardening. #166 instrumentation arrives with each slice; #167 aggregates the complete release proof.

## Checkout and support gates

1. Architecture acceptance does not make an implementation issue ready.
2. A child becomes claimable only after its explicit blockers exist, Aurora or the controlling process changes that exact issue to `status: ready`, and one contributor claims it.
3. Implementation never expands because a test finds an unrelated failure; the failure receives its own issue.
4. No current behavior is retired until the replacement and every affected protected regression pass.
5. A capability is supported only when #167 records a complete passing matrix for its exact carrier, Character, profile, accessibility, platform, network, lifecycle, and performance envelope.
