# OWS Mobility Program Charter

> **Status:** Accepted Stage 3 program selection and design brief. Aurora selected OWS Mobility on 2026-08-24 and closed the architecture interview and explicitly accepted its [architecture](OWS_MOBILITY_ARCHITECTURE.md), [acceptance matrix](OWS_MOBILITY_ACCEPTANCE_MATRIX.md), and reconciled backlog on 2026-08-25. Implementation, evidence-derived tuning, and support claims remain separately gated under their exact owning issues.

## Purpose

This charter records why Character and Vehicle architecture must be designed together and which decisions already constrain that work. It prevents either domain from freezing a public contract that cannot support the other while preserving the accepted OWS requirement that Character and Vehicle each build independently without Integration. The Integration family necessarily consumes both supported public contracts but remains an acyclic, separately buildable first-party plugin when both domains are present.

This is an architecture-program contract, not evidence that exterior riding, passengers, moving entry, infiltration, or vehicle-to-vehicle transfer exists in `main`.

The accepted [OWS Mobility Architecture](OWS_MOBILITY_ARCHITECTURE.md) defines the target design, and the accepted [OWS Mobility Acceptance Matrix](OWS_MOBILITY_ACCEPTANCE_MATRIX.md) defines its proof and support-claim rules. This charter remains the accepted program-selection and gate record.

## Stage 3 selection

Stage 3 now contains two architecture programs:

1. **OWS Mobility — selected and active.** Character, Vehicle, and their dynamic relationship are co-designed through #147, Character #106/#107, Vehicle #9/#108, collision/force response #21, benchmark #41, Vehicle tuning #48, graph runtime/editor #155/#156, and the decomposed integration implementation/migration family #120.
2. **Systemic Hacking — preserved and gated.** #10/#50 remains a complete later Stage 3 program. Selecting Mobility does not cancel, narrow, or silently defer its product goal beyond its explicit gate.

[Issue #111](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/111) records the remaining Mobility program gates and prevents Stage 4 Combat from opening until Aurora explicitly opens Stage 4.

## Why Mobility is one coordinated program

Character locomotion can traverse a vehicle while Vehicle physics moves it. Boarding can begin on the ground, on an exterior surface, at a grip anchor, inside another vehicle, or during relative motion. Passengers, drivers, exterior riders, and transferring characters share occupancy, control, animation, collision, camera, authority, and recovery concerns.

Designing Character and Vehicle independently and adding this behavior afterward would either create private dependencies and cycles or force the integration layer to duplicate domain state. OWS therefore accepts one coordinated architecture pass with three ownership boundaries rather than one monolithic runtime domain.

## Current baseline

Current OWS already provides:

- the complete maintained GASPALS and KinetiForge starting foundations;
- exact authored vehicle-door targeting and stopped entry/exit;
- seat and door definition data with replicated seat occupancy;
- character/vehicle possession, input, camera, visibility, and collision handoff;
- moving bailout, inherited velocity, controlled roll, recovery, and re-entry; and
- possible incidental Unreal moving-base behavior when a vehicle surface happens to qualify as walkable; this behavior is not yet inventoried, verified, or supported by OWS.

Current OWS does **not** provide a supported passenger path, exterior-rider relationship, authored standable-zone or handhold model, active grip, moving-entry transition, speed matching, infiltration, or vehicle-to-vehicle transfer. Current entry accepts only a control seat, rejects another player-controlled vehicle, performs no relative-motion validation, and completes through an instantaneous hidden-character seat placement. Incidental ability to target a moving unoccupied vehicle is not accepted moving-entry gameplay.

Existing verified behavior remains protected until an accepted replacement path passes the complete Mobility matrix.

## Accepted ownership boundaries

| Owner | Accepted responsibility |
| --- | --- |
| **OWS Character** | Bodily locomotion, stance, balance and grip capability, animation, traversal, falling, rolling/tumbling, recovery, and Character-side prediction/presentation. |
| **OWS Vehicle** | Vehicle simulation, collision bodies, linear/angular and point kinematics, seats, control roles, doors, portals, standable zones, edges, handholds, capacities, stable scoped identities, and Vehicle-side authoring/validation. |
| **OWS Character–Vehicle Mobility** | The relationship and transitions between domains: dynamic support, exterior riding, gripping, boarding, moving entry, passenger/driver handoff, infiltration, transfer, detachment, reservations, Mobility-specific server coordination through the shared Stage 2 authority framework, and cross-domain recovery orchestration. Integration coordinates relationship, load, and relationship-specific collision state; Character executes bodily motion and Vehicle applies accepted simulation forces through public contracts. |

No boundary owns duplicate state from another domain. Character and Vehicle must each build and load without the Integration family. Integration consumes both supported public contracts, plus shared OWS contracts, and cannot become a gameplay catch-all or replace the Stage 2 authority framework.

Player identity, physical character embodiment, occupancy, camera/presentation, and permission to control a vehicle are separate public concepts. Passenger support cannot assume every occupant possesses the vehicle, and driving cannot imply that the persistent character entity ceased to exist.

## Accepted mobility capability envelope

The architecture must be capable of representing:

- standing, walking, crouching, crawling where geometry permits, jumping, sliding, and falling on or from accepted moving vehicle surfaces;
- passive balance and an explicitly authored active-hold/grip path;
- progressive instability, bracing, sliding, reachable grip or hanging, grip failure, edge departure, airborne motion, and recovery;
- entry into stopped and moving vehicles through the selected door, portal, handhold, or transition path;
- a running character matching enough of a vehicle's motion to create a valid boarding opportunity;
- passengers, multiple occupants, control roles, seat switching, and driver handoff;
- movement from an exterior surface into the same vehicle's interior;
- movement from one vehicle's interior or exterior to another vehicle's exterior or interior;
- deliberate infiltration attempts, safe rejection, interruption, contention, and physically coherent failure; and
- player and NPC control sources using the same Character, actions, capabilities, animation, physics, graph, requests, authority, and outcomes, with developer-configurable NPC presets rather than a second implementation.

This capability envelope requires an extensible system. The first supported slice includes the three accepted Balanced, Cinematic, and Simulation profiles and the complete same-Vehicle loop; other carrier classes, cross-Vehicle runtime certification, and unadvertised presentation variants remain separately gated.

## Accepted physical principles

Absolute vehicle speed is not the sole stability, ejection, boarding, or transfer rule. Steady speed, acceleration, braking, turning, angular motion, impacts, surface properties, relative wind, posture, balance, and grip create different outcomes.

For a point rigidly attached at offset `r` from the vehicle reference body, the required starting quantity is:

`point velocity = vehicle linear velocity + vehicle angular velocity × r`

An articulated door, moving anchor, suspension-linked part, or other locally moving target also contributes its local motion transformed into world space:

`target point velocity = rigid-body point velocity + articulated/local point velocity`

The character's motion relative to the selected point is:

`relative velocity = character velocity - target point velocity`

The architecture must also account for the accepted combination of point acceleration, angular acceleration, jerk/contact impulse, approach direction, gap, time-to-contact, surface normal and slope, friction/material response, clearance, wind exposure, Character capability, stance, grip capacity, and optional stamina policy.

Consequently:

- running alongside a vehicle can make a previously unsafe boarding attempt catchable;
- two fast vehicles travelling side by side with compatible point motion can permit a transfer;
- nearby vehicles moving oppositely, separating, rotating incompatibly, or accelerating beyond the accepted envelope can reject or interrupt it;
- crouching may improve the accepted balance and wind envelope but cannot become magical adhesion; and
- forced separation preserves the character's actual world velocity immediately before release—including its motion relative to the support—and applies any new contact or release impulse exactly once rather than resetting to point velocity, double-applying an impulse, or adding an unexplained teleport or launch.

The accepted decisions require the branching physical response, two-way coupling, dedicated Grip, optional stamina provider, capability-driven edge catch, profile and accessibility separation, and context-correct animation requirements in the accepted architecture. Exact response curves and certified engineering ranges remain evidence-derived work in their owning implementation and benchmark issues.

## Authored mobility graph

Each compatible vehicle can contribute a data-authored mobility graph containing the accepted subset of:

- standable zones such as roofs, hoods, beds, steps, or interior floors;
- grip anchors such as rails, handles, ledges, and edges;
- portals such as doors, windows, and hatches;
- seats, control roles, and finite-capacity positions;
- valid directed transitions between locations;
- approach and clearance volumes;
- neutral recipe IDs and alignment metadata, with Motion Warping as the default provider behind a replaceable adapter and no unrelated-animation fallback; Vehicle never owns Character animation assets;
- supported force, stance, capability, and transition envelopes; and
- stable scoped identities, versioning, validation, visualization, and diagnostics.

Ordinary movement may use explicitly accepted physical surfaces. Precise boarding, grip, portal, seat, and transfer actions use authored locations and transition data. Vehicle owns its structural contribution; Integration owns the cross-domain action and reservation; Character owns bodily execution and presentation.

## Authority and networking constraints

- Authenticated clients submit idempotent Mobility intent with mapped client time; NPC/server sources submit the same semantic Actions with authoritative server time. The server validates identity, relationship revision, target scoped IDs, same-time relative motion, permissions, capability, clearance, occupancy, reservations, and transition state against synchronized authoritative history and current commit-time availability.
- Finite seats, grip anchors, portals, and conflicting transitions use authoritative reservation, revision, idempotency, contention, and release rules.
- Each discrete multi-owner reservation, relationship, occupancy, and control-grant mutation is atomic under the Stage 2 transaction rules. A physical in-flight transfer is continuous and cannot be rolled back after launch; interruption or missed capture commits the appropriate airborne/failure state instead.
- The owning participant predicts continuous Character motion only inside an authorized relationship; proxies interpolate authoritative support-relative state, and corrections are measured in the authoritative support frame so Vehicle and Character motion cannot be applied twice.
- Late join, disconnect, destruction, relevance loss, possession/control loss, and interrupted animation must converge on one valid relationship and release stale reservations.
- Unsupported network modes remain explicitly unsupported until their complete matrix passes.

## Architecture work and issue ownership

| Work | Owner |
| --- | --- |
| Joint audit, research, accepted decision record, architecture, and complete matrix | [#147](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/147) |
| Character public contract and conformance contribution | [#107](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/107) |
| Vehicle public contract and conformance contribution | [#108](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/108) |
| Collision, stability, slide, grip-load, impact, and detachment response | [#21](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/21) |
| Driving and point-kinematic benchmark evidence | [#41](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/41) |
| Vehicle-owned driving tuning | [#48](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/48) |
| Vehicle Mobility graph runtime and editor authoring | #155/#156 under #108 |
| Integration cross-domain route/recipe resolver | #160 under #120 |
| Cross-domain implementation and compatibility-first migration | [#120](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/120) after all controlling contracts exist |
| Server-authoritative transition and contention consumer slices | [#27](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/27) and [#28](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/28) after their exact issue gates and prerequisites are satisfied |

## Program exit gate

Acceptance of the Mobility architecture does not finish the broader Stage 3 execution gate. #111 records each remaining gate independently:

1. [x] the existing implementation and capability inventory is classified, including explicit unverified/unsupported gaps;
2. [x] current Unreal, physics, product, animation, authoring, accessibility, AI, and multiplayer research is recorded;
3. [x] Aurora answered every consequential open choice;
4. [x] Aurora explicitly accepts the assembled canonical Mobility architecture, complete matrix, and backlog;
5. [ ] #107 and #108 publish their broader compatible public contracts and independent conformance requirements against the accepted Mobility contribution;
6. [ ] #21, #41, #48, #120, and implementation/evidence children #150–#168 are reconciled and merged as their roadmap gates require; and
7. [ ] Aurora separately opens Stage 4 Combat.

## Not authorized by this charter

- Gameplay, physics, animation, possession, passenger, collision, entry, transfer, or multiplayer implementation.
- Replacing, retuning, or weakening verified stopped entry/exit or moving bailout behavior.
- Merging Character and Vehicle into one plugin or introducing a private dependency cycle.
- Treating current incidental behavior, current file placement, or an experimental Unreal backend as the accepted public contract.
- Claiming support for a carrier type, transition, network mode, or performance envelope before its declared evidence passes.
