# OWS Mobility Acceptance Matrix

> **Status:** Accepted target conformance specification for the [OWS Mobility Architecture](OWS_MOBILITY_ARCHITECTURE.md), explicitly accepted by Aurora on 2026-08-25 with the architecture and reconciled backlog. Publishing this matrix does not mean its planned capabilities are implemented or passing. Until linked release-candidate evidence exists, every future-capability row is `NR` and the applicable support claim is unavailable.

## Relationship to the current regression baseline

The [OWS Character and Vehicle Acceptance Matrix](CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md) remains the protected current-behavior regression suite for exact-door targeting, stopped entry/exit, current driving handoff, moving bailout, controlled roll, recovery, and re-entry. No new Mobility path may replace that behavior until its applicable rows below pass and the protected regression rows remain green.

This matrix expands that baseline. It does not rewrite evidence from an older commit and does not convert incidental moving-base or moving-entry behavior into supported functionality.

## Result language

| Result | Meaning |
| --- | --- |
| `NR` | Not run. No support conclusion is permitted. |
| `PASS` | Every expectation passed for the exact recorded configuration and evidence is linked. |
| `FAIL` | At least one expectation failed; a tightly scoped defect issue is linked. |
| `BLOCKED` | A named dependency or environment prevented valid execution; the blocker is linked. |
| `N/A` | The scenario cannot apply to the named, authored combination; the graph/capability reason is recorded and validated. |

There is no partial pass and results are never averaged. One failure in repeated runs is `FAIL` with the observed frequency.

## Required test record

Every record names:

| Field | Required value |
| --- | --- |
| Tester and UTC time | GitHub handle plus timestamp |
| Source | Full commit SHA and dirty/clean state |
| Engine/build | Exact Unreal version, configuration, Editor/standalone/packaged identifier |
| Platform | Client OS and server OS |
| Map/course | Exact asset path and authored fixture/version |
| Character | Variant, entity ID, dimensions, mass, control-source preset |
| Vehicle | Vehicle ID, graph ID/version, relevant body/point IDs |
| Mobility configuration | Feel profile, capability preset, accessibility settings, presentation providers |
| Network | Standalone/listen/dedicated, host/client/observer role, legacy/Iris |
| Impairment | RTT, jitter, loss, reorder, duplication profile |
| Performance | Frame cap, measured frame rate, physics rate, reference hardware |
| Scenario/result | Scenario IDs, repetitions, `PASS`/`FAIL`/`BLOCKED`/`N/A` |
| Evidence | Durable logs, test artifacts, traces, captures, and observations |
| Gap | Required issue for every `FAIL` or `BLOCKED` |

Machine-readable evidence uses one correlated request/relationship schema across server, owning participant, observers, physics, animation, and presentation. A screenshot alone cannot prove authority, timing, or force correctness.

## Coverage inventory

### Characters and control sources

The release grid contains every shipped OWS Character variant, currently:

| ID | Character |
| --- | --- |
| C01 | OWS Manny |
| C02 | OWS Manny Clone |
| C03 | OWS Quinn |

Every Character behavior is executed through both player and NPC control sources. NPC execution uses the same public actions, graph, capabilities, physics, animation, authority, and result path. A developer-disabled NPC option may be `N/A` only when the tested preset and reason are recorded; the common implementation still must conform.

### Vehicle fixtures

Every current road-Vehicle asset is inventoried and validated. The current roster IDs remain:

| ID | Vehicle | Required special coverage |
| --- | --- | --- |
| V01 | DefaultVehicle | Baseline implementation |
| V02 | DemoChassis | Full-roster regression |
| V03 | HyperCar | High-performance geometry |
| V04 | SportsCar | Ordinary road-car baseline |
| V05 | MuscleCar | Full-roster regression |
| V06 | Sedan | Four-seat/door authoring where present |
| V07 | SUV | Tall body and roof |
| V08 | SedanEV | Full-roster regression |
| V09 | GT3 | Low body/high-performance geometry |
| V10 | DriftCar | Current smoke fixture and high-slip behavior |
| V11 | TCR | Full-roster regression |
| V12 | Rally | Uneven-surface behavior |
| V13 | Bus | Articulated bodies, curb portals, passengers, and free interior |
| V14 | RC_Car | Extreme-size compatibility and explicit inapplicable-path validation |

V01, V04, V07, V10, V13, and V14 form the representative set. An authored Vehicle may legitimately expose no human-compatible seat or portal. That makes only the incompatible path `N/A`; it does not waive graph, identity, collision, kinematic, support, lifecycle, or diagnostic conformance.

### Mandatory configuration dimensions

| Dimension | Required release coverage |
| --- | --- |
| Feel | Balanced, Cinematic, Simulation |
| Accessibility | Grip hold/toggle/automatic; edge-catch default/automatic; timing assistance; remapped input; configurable camera motion |
| Frame behavior | 30, 60, 120 FPS |
| Network roles | Standalone; listen host and remote client; dedicated server and remote clients |
| Replication | Legacy replication and Iris |
| Supported impairment | 150 ms RTT, 30 ms jitter, 2% loss |
| Recovery stress | 250 ms RTT, 60 ms jitter, 5% loss plus reorder and duplication |
| Platforms | Windows client; Windows and Linux dedicated servers |
| Scale | 32 players, 32 active/relevant Vehicles, 16 concurrent Mobility relationships in one hotspot or large Vehicle |

The matrix uses constrained release suites rather than an unreviewable Cartesian explosion:

- **T0 independent conformance:** Character-only and Vehicle-only build/load/cook/package/test.
- **T1 change smoke:** C01 player + V10, Balanced, 60 FPS, standalone, plus the directly changed configuration.
- **T2 representative:** every Character × representative Vehicle; player and NPC sources; every applicable scenario family across the larger configuration grid.
- **T3 full roster:** every current Vehicle runs every applicable authored capability family with at least one Character, plus graph, identity, collision, boarding/exit applicability, lifecycle, and protected regressions across the declared full-roster combinations.
- **T4 network:** every named role/backend at supported impairment; representative Character/Vehicle pairs plus any risk-specific fixtures.
- **T5 release stress:** recovery impairment, frame-rate parity, scale, long cycles, Windows/Linux server, Shipping, and performance evidence.

Any failure expands that scenario to the full affected dimension before scope is declared.

Each row's **Setup/action** is its minimum procedure and precondition statement. Its durable test record supplies the exact actors, roles, configuration dimensions, repetitions, and evidence. Universal invariants always apply. A row without a narrower issue number inherits the owner family below; #147 defines the specification but owns no gameplay implementation.

| Scenario family | Implementation/evidence owner |
| --- | --- |
| Composition and public contracts | #107 Character, #108 Vehicle, #120 Integration, #121 migration |
| Graph, authoring, and exact point motion | #155 runtime records, #156 editor tooling, #154 point implementation, #41 benchmark; #48 retains driving tuning |
| Physical response and coupling | #21 physical contract; #107 Character and #120 Integration children |
| Actions, authority, and anticipation | #117 Input, #127 Shared Actions, #27 Mobility consumer, #120 Integration |
| Reservations, occupancy, and contention | #108 Vehicle records, #28 leases/contention, #120 orchestration |
| Animation, embodiment, camera, and accessibility | #107 Character, #117 Input, #119 UI, #120 Integration |
| Lifecycle and persistence | #120 Integration, #132 persistence, #138 recovery, #139 World Partition |
| Diagnostics, Shipping, scale, and release evidence | #140 observability, #141 conformance, #41/#49 performance |
| Protected current behavior and migration | Existing current matrix, #120 integration migration, #121 content migration |

## Universal invariants

Every scenario must satisfy all of these:

1. No crash, hang, ensure, invalid numeric state, world loss, or unrecoverable play state.
2. Exactly one authoritative record exists per stable Character entity, scoped Vehicle resource, Mobility relationship/revision, control grant, and request/terminal-result identity.
3. Character and Vehicle transforms, forces, velocities, and corrections remain finite.
4. No transition crosses an untraversed gap, moves the Character to a camera/stale transform, or silently changes targets.
5. Support velocity plus Character-relative velocity is inherited once; each new impulse and coupling force is applied once.
6. Rejection and cancellation before commitment leave no partial occupancy, reservation, control, collision, visibility, input, camera, or presentation mutation.
7. Failure after commitment resolves physically; it is not rolled back or teleported to success.
8. Stable IDs and revisions agree across server, owner, observer, persistence, and diagnostics.
9. Player and NPC control sources produce the same authoritative result for equivalent semantic actions.
10. Accessibility generates intent only inside the same physical, authored, permission, and authority envelope.
11. Legacy replication and Iris converge on the same canonical state and failure result.
12. Existing stopped entry/exit and bailout/recovery/re-entry behavior remains protected until its accepted replacement passes.
13. Malformed, unauthorized, stale, oversized, non-finite, or over-budget requests never mutate canonical state or allocate unbounded work.

## A. Composition and independent-domain conformance

| ID | Setup/action | Expected result | Required evidence / owner |
| --- | --- | --- | --- |
| MOB-BLD-001 | Build, load, cook, and package Character without Vehicle or Integration. | Character public contract and all supported independent behavior remain available; no Vehicle type/reference is required. | Clean-host build/load/cook/package logs; #107. |
| MOB-BLD-002 | Build, load, cook, and package Vehicle without Character or Integration. | Vehicle simulation, graph, authoring, occupancy/control, and kinematic contract remain available; no Character type/reference is required. | Clean-host build/load/cook/package logs; #108. |
| MOB-BLD-003 | Build Integration with both supported public contracts. | Dependency graph is acyclic and contains no UI or device-Input dependency, fork-private/demo-shell API use, or hard Vehicle-to-Character content reference. | Dependency/API/asset/cook lint evidence; #120. |
| MOB-BLD-004 | Disable Integration in a Character-only and Vehicle-only host. | Each domain starts cleanly and reports only unavailable cross-domain capability, not missing-module failures. | Startup tests; #107/#108. |
| MOB-BLD-005 | Cook/package OWS default installation from documented workflow. | One OWS installation contains its maintained foundations; no upstream plugin download or manual dependency assembly is requested. | Clean-machine installation/package test. |
| MOB-BLD-006 | Inspect public Blueprint/C++/data/tag surfaces and provider registrations. | Supported, Experimental, Internal, deprecated, optional, authoritative, and aggregate surfaces follow Stage 1 multiplicity/version rules. | API/conformance report. |
| MOB-BLD-007 | Attempt to register two authoritative providers for one contract. | Editor/startup validation rejects ambiguous ownership with actionable guidance. | Automated negative test. |
| MOB-BLD-008 | Upgrade graph/schema and compatibility data from the previous supported version. | Stable IDs and overrides migrate or fail explicitly; no silent data loss occurs. | Migration fixture and diff; #156/#120. |
| MOB-BLD-009 | In separate startup runs, replace descendant tag/typed-context, Character capability, Vehicle graph/point/kinematic, Integration routing/assist-selection, and Character-owned animation/alignment/camera/embodiment surfaces with Blueprint and C++ conformance fixtures. | Each documented replacement path uses only Supported contracts and preserves canonical identity, authority, physics, and results; any documented C++-only engine limitation is explicit. Runtime hot-swap is not implied. | Clean sample extensions and default/replacement result diff; #107/#108/#120. |
| MOB-BLD-010 | Run with each optional provider absent, then register two aggregate diagnostic listeners. | Absence produces the declared no-op/default without breaking unrelated Mobility; both listeners receive the same immutable event and neither can consume, reorder, mutate, or own it. | Optional-provider and multiplicity tests; #107/#108/#120/#140. |
| MOB-BLD-011 | Introduce prohibited reverse/UI/device-Input dependencies, private peer APIs, Vehicle-authored Character asset references, Integration-owned Character movement, and Integration-owned Vehicle force application in negative fixtures. | Dependency, registration, asset, cook, or conformance validation rejects each violation with the owning-domain correction; no forbidden path reaches Shipping. | Static/runtime negative suite; #107/#108/#120/#141. |
| MOB-BLD-012 | Register incompatible providers or a provider that tries to bypass revision, authority, exact point motion, physical validity, atomic reservation, or coupling ownership. | Editor/cook/startup validation rejects the configuration before gameplay and identifies the violated invariant. | Provider abuse/compatibility fixtures; #107/#108/#120. |

## B. Graph authoring, identity, and kinematics

| ID | Setup/action | Expected result | Required evidence / owner |
| --- | --- | --- | --- |
| MOB-GRAPH-001 | Generate a draft graph for every roster Vehicle. | Bodies, supported doors, seats, collision/surfaces, and candidate connections appear as an unaccepted draft; no draft becomes a support claim automatically. | Editor automation and per-Vehicle report; #156. |
| MOB-GRAPH-002 | Accept, manually edit, and regenerate a graph. | Regeneration presents a diff, preserves stable IDs/manual overrides, and never overwrites accepted work silently; Vehicle data retains neutral recipe IDs rather than Character animation assets. | Before/after asset/reference diff; #156. |
| MOB-GRAPH-003 | Author a graph fully manually. | The same schema, visualization, validation, and runtime contract work without draft generation. | New example Vehicle workflow; #156. |
| MOB-GRAPH-004 | Create duplicate/missing IDs, invalid providers, unreachable required points, bad clearance, and forbidden edges. | Save/CI/cook validation reports exact actionable failures; required invalid data blocks release/cook and optional invalid edges disable deterministically. | Automated negative fixtures; #156. |
| MOB-GRAPH-005 | Visualize zones, anchors, edges, portals, seats, clearance, provider bodies/bones, and validation results. | Editor visualization matches stored data, remains non-Shipping, and allows exact point identification. | Editor capture plus data comparison. |
| MOB-GRAPH-006 | Sample rigid-body points through translation and rotation. | World transform, linear/angular/point velocity, and acceleration use documented units/reference time and match independent calculation within published tolerance. | Numeric automated test and trace; #154/#41. |
| MOB-GRAPH-007 | Sample a skeletal bone, articulated bus body/portal, and locally moving point. | Local, angular, centripetal, tangential, and Coriolis contributions are represented at the same authoritative time. | V13 trace and independent calculation; #154/#41. |
| MOB-GRAPH-008 | Use a project-defined kinematic provider, then invalidate it. | Valid provider conforms through the public interface; invalidation rejects before commit or releases from the last valid same-time sample. | Extension fixture and failure trace. |
| MOB-GRAPH-009 | Author ordinary physical surfaces plus explicit include/exclude zones. | Valid ordinary support works; authored overrides win; precise actions still require authored points/edges. | Surface matrix; #156/#21. |
| MOB-GRAPH-010 | Test standing/crouching/crawling clearance in car, hatchback/cargo space, bus, and incompatible space. | Interior eligibility follows geometry and pose, never category labels. V13 provides the required free-interior path. | Clearance traces and video; #156/#150/#162. |

## C. Dynamic support, physical response, and coupling

| ID | Setup/action | Expected result | Required evidence / owner |
| --- | --- | --- | --- |
| MOB-PHY-001 | Stand/walk on each representative stopped eligible surface. | Character is supported without penetration, jitter, artificial attachment, or Vehicle/Integration dependency inside Character. | Contact/relationship trace and video. |
| MOB-PHY-002 | Repeat during steady translation at multiple headings. | Support-relative locomotion matches exact point motion with no double application or world-space drift. | Kinematic comparison; #21/#120. |
| MOB-PHY-003 | Repeat while accelerating, braking, reversing, and changing acceleration rapidly. | Measured load moves through stable/braced/sliding/failure states according to profile and capability; absolute speed alone does not decide. | Force/state trace at 30/60/120 FPS. |
| MOB-PHY-004 | Repeat during turning, body rotation, suspension/articulation, rollover, and recovery. | Angular point motion and support geometry drive finite, coherent response. | V13 plus rigid-Vehicle traces. |
| MOB-PHY-005 | Walk, sprint, crouch, jump, land, and change direction on a moving surface. | Character input and pose alter measured capability; Character and Vehicle exchange force once at actual contact points. | Coupling/impulse ledger. |
| MOB-PHY-006 | Compare upright and crouched poses under identical motion/wind. | Results use pose-specific support geometry/area/capability; crouch may help but never creates adhesion. | Numeric profile comparison. |
| MOB-PHY-007 | Exercise no-input automatic balance inside the recoverable envelope, then apply limited Move counterbalance while load rises through and beyond capacity. | Character balances automatically; valid counterbalance measurably helps only within available capacity, then saturates into stable → braced → sliding without a timer or unrelated animation deciding the outcome. | Correlated input/load/physical/state trace. |
| MOB-PHY-008 | Hold Grip at valid and invalid authored anchors. | Valid Grip consumes measured capability/load and couples forces; invalid reach/clearance/load rejects without attachment. | Anchor/load trace. |
| MOB-PHY-009 | Enable/disable optional stamina while maintaining identical Grip load. | Grip works with stamina absent; enabled drain follows authoritative load/policy rather than a fixed timer. | Capability-provider test. |
| MOB-PHY-010 | Approach an authored reachable edge with default and automatic catch. | Default catch requires Grip; accessibility automatic catch issues intent only when reach, clearance, and survivable load are valid. | Input/physics trace. |
| MOB-PHY-011 | Lose support with and without valid Grip. | Valid catch/hang branches physically; otherwise release preserves exact point plus relative velocity and enters airborne response. | Velocity/impulse ledger. |
| MOB-PHY-012 | Land from varying contact energies and geometry. | Contact evidence selects controlled roll or tumble and emits bounded consequence context to the accepted vitality/damage owner; an animation timer never selects physics. | Contact-energy/state trace; #21/#107. |
| MOB-PHY-013 | Apply rider weight, walking, jump/landing, impact, and Grip to light/heavy Vehicles. | Equal-and-opposite response is present once, scales with actual mass/contact, and remains finite. | Vehicle momentum/force comparison. |
| MOB-PHY-014 | Exercise relationship-aware collision beside world and unrelated bodies. | Only duplicate/fighting Character–carrier contacts are filtered; world and unrelated collision remain active. | Collision-filter matrix; #21. |
| MOB-PHY-015 | Crush/pin, destroy one Vehicle part, enter water/fire, and cross world bounds. | Typed interruptions invalidate only affected points; optional providers own consequences; safe-anchor relocation occurs only as last resort. | Event/state/recovery trace; #21/#120. |
| MOB-PHY-016 | Run Balanced, Cinematic, and Simulation on the same physical course. | Profiles alter declared assistance, not identities, force accounting, reach/clearance validity, or state meaning. | Comparable traces and subjective review. |
| MOB-PHY-017 | Enable optional damage/environment consequences and trigger one contact through a relationship-aware collision path. | Mobility emits one correlated load/hazard context and consumes one authoritative result; exactly one ASC/vitality owner mutates and no collision/coupling path duplicates damage, stamina, or health authority. | Attribute/effect/event/force correlation; #21/#107/#128. |

## D. Actions, boarding, seats, and interiors

| ID | Setup/action | Expected result | Required evidence / owner |
| --- | --- | --- | --- |
| MOB-ACT-001 | Remap Activate, Cancel, Exit, Bailout, Release, Grip, movement, aim, and driving actions across controller and keyboard, then submit equivalent NPC Actions. | Player device intent passes through OWS Input; NPC intent enters the same Shared semantic Action contract without a device; prompts update and no raw input path bypasses the contract. | End-to-end player/NPC action automation. |
| MOB-ACT-002 | Stopped Activate at every valid portal/seat path. | Exact selected target reserves, approaches, aligns, and enters once; no other Vehicle/portal substitutes silently. | Target/request/transition trace and video. |
| MOB-ACT-003 | Press the player-facing Cancel binding before physical commitment, after physical commitment, and later while in an established relationship. | Pre-commit submits `Cancel` and cleans up; a committed transition cannot be cancelled and resolves physically; in an established context the same binding routes a new governed `Exit`, `Bailout`, or `Release` Action rather than rewriting the prior lifecycle. | Action-tag/lifecycle/state-mutation audit. |
| MOB-ACT-004 | Request moving boarding from unreachable gap, incompatible approach, or unsurvivable load. | Server rejects before launch with governed reason and no partial state. | Rejection trace. |
| MOB-ACT-005 | Run alongside a moving Vehicle and create a valid intercept. | Relative point motion, not absolute Vehicle speed, permits the opportunity; player times Activate, establishes catch with discrete Grip, and maintains it through continuous Grip/body-shift control. | Same-time kinematic/action/control trace. |
| MOB-ACT-006 | Change speed/gap/acceleration after physical commitment. | Attempt resolves as catch, slide, miss, fall, or recovery without teleport or rollback. | Continuous multi-endpoint capture. |
| MOB-ACT-007 | Hold/repeat/duplicate delayed Activate requests. | One request identity commits once; duplicates receive the cached terminal result. | Idempotency test. |
| MOB-ACT-008 | Enter through multiple doors while another Vehicle is also selectable. | The exact scoped portal and Vehicle remain authoritative throughout. | Selector/action identity trace. |
| MOB-ACT-009 | Use Activate, discrete Grip establish/release, and continuous maintained Grip/body shift in valid and invalid contexts. | Activate never maintains Grip; establish/release each produces one governed result; continuous control cannot create a relationship or bypass a rejected discrete action. | Action/control/relationship trace. |
| MOB-ACT-010 | Run Requested → Validated → Started → Committed → terminal for every supported runtime family: dynamic support/Grip, stopped boarding, moving boarding/infiltration, interior traversal, seat/handoff, and exit/bailout. | Each family uses its declared preconditions, lease, physical commit, later domain commit where applicable, and exactly one terminal result; physical phase remains separate from Action lifecycle. Cross-Vehicle runtime remains disabled and is covered only by authoring validation. | Phase/revision/mutation ledger for every supported edge family. |
| MOB-ACT-011 | Cancel or interrupt every edge family at each pre-commit phase and immediately after physical commitment; inject stale, duplicate, and out-of-order phase updates. | Pre-commit cleanup leaves no partial mutation; post-commit resolves physically; revisions never regress; stale/duplicate updates cannot reopen, recommit, or create a second terminal result. | Phase-boundary fault-injection suite. |
| MOB-OCC-001 | Enter each compatible passenger and control seat. | Occupancy is finite and independent from possession/control permission; passengers do not possess the Vehicle. | Occupancy/control snapshot. |
| MOB-OCC-002 | Omit a Seat ID and request automatic selection. | Deterministic best compatible seat through the chosen portal is selected; unavailable alternatives are reported. | Deterministic-repeat test. |
| MOB-OCC-003 | Explicitly request an unavailable Seat ID. | Request fails without silently choosing another seat. | Conflict result trace. |
| MOB-OCC-004 | Switch seats through valid/invalid authored routes. | Destination reserves before source surrender; invalid route rejects; failure preserves the source occupant. | Reservation/occupancy revisions. |
| MOB-OCC-005 | Perform voluntary driver handoff. | New control grant commits atomically; old control persists until commit and no frame has two drivers. | Server/owner/observer control trace. |
| MOB-OCC-006 | Cancel, disconnect, die, destroy target, or expire during reservation. | Lease releases exactly once; committed occupancy is not treated as a renewable reservation. | Lifecycle/lease test. |
| MOB-OCC-007 | Observe seated Character from inside/outside and apply valid targeting/damage queries. | Same Character remains visible, targetable, damageable, and authoritative; locomotion capsule does not fight the Vehicle. | Embodiment/collision/query evidence. |
| MOB-OCC-008 | Exit seated embodiment through valid/blocked exits. | Full Character collision returns only at valid placement; rejection preserves occupancy and control. | Placement/embodiment trace. |
| MOB-INT-001 | Walk, crouch, crawl, stop, jump where supported, and enter seats inside V13 while it moves/articulates. | Interior uses the same support/physics contract; graph/pose clearance controls availability. | V13 synchronized capture. |
| MOB-INT-002 | Traverse exterior surface → Grip/hang → portal → interior → seat and reverse. | Every authored same-Vehicle edge preserves identity, physical continuity, reservation, and recovery. | Full path trace/video. |
| MOB-INT-003 | Initiate an infiltration path from a seat/interior to an exterior point. | Path physically crosses the source portal/contact; no interior-to-exterior teleport occurs. | Transition geometry/phase capture. |
| MOB-INT-004 | Author a cross-Vehicle route and validate it without enabling runtime certification. | Data, identities, kinematics, and validation are representable; support status remains deferred/disabled. | Editor/validation evidence. |

## E. Exit, bailout, failure, and recovery

| ID | Setup/action | Expected result | Required evidence / owner |
| --- | --- | --- | --- |
| MOB-EXIT-001 | Execute stopped exit through preferred clear portal. | Character returns through the authored path at valid support with identity/state intact. | Protected baseline plus new-contract trace. |
| MOB-EXIT-002 | Make an automatic/untargeted exit request with the preferred portal blocked and a valid alternative clear. | Deterministic valid alternative is selected only after clearance failure; selected reason is inspectable. | Clearance/selection evidence. |
| MOB-EXIT-003 | Block all exits. | Exit rejects and preserves occupancy/control/embodiment without partial mutation. | Negative test. |
| MOB-EXIT-004 | Bail out across representative relative point motions with no steering and limited steering. | Release uses exact point plus relative motion once and enters the accepted physical graph; camera follows Character. | Kinematic/impulse/video evidence. |
| MOB-EXIT-005 | Allow controlled roll to finish with no movement input. | Recovery returns ordinary Character control without unrelated stumble/run animation or camera/Character snap. | Animation/state/video evidence. |
| MOB-EXIT-006 | Hold movement during roll/recovery. | Allowed steering remains limited by physics and held input flows smoothly into ordinary locomotion. | Input/state trace. |
| MOB-EXIT-007 | Interrupt exit/bailout through impact, lost floor, destruction, water, or blocked target. | Typed interruption reaches one valid terminal state; no stale reservation/control/presentation remains. | Failure-injection suite. |
| MOB-EXIT-008 | Re-enter before, during, and after recovery. | Eligibility follows current relationship/capability and authored validation; rejected attempts leave no stale lock and valid later attempts succeed. | Repeated-cycle test. |
| MOB-EXIT-009 | Repeat stopped and moving cycles without restarting. | No identity, occupancy, control, input, collision, camera, animation, force, reservation, or performance error accumulates. | Long-cycle automated artifact. |
| MOB-EXIT-010 | Force a correction during support, Grip, airborne, roll, and recovery. | Small errors smooth; material errors enter typed physics; hard relocation occurs only for declared last-resort cases. | Authoritative/presentation traces. |
| MOB-EXIT-011 | Explicitly target a blocked exit while another exit is clear. | Request rejects with the scoped reason and never substitutes the clear portal; occupancy/control/embodiment remain intact. | Target/clearance/terminal-result trace. |

## F. Animation, camera, accessibility, and control-source parity

| ID | Setup/action | Expected result | Required evidence / owner |
| --- | --- | --- | --- |
| MOB-PRES-001 | Execute every enabled transition recipe on every required Character skeleton/variant. | Context-correct recipe plays, reaches validated moving targets, and never substitutes an unrelated animation. | Recipe coverage report and video; #107/#120. |
| MOB-PRES-002 | Remove a required recipe, then disable the associated optional edge. | Missing enabled recipe blocks certification/cook with guidance; disabling the edge removes only that capability deterministically. | Negative validation test. |
| MOB-PRES-003 | Replace Motion Warping with a conforming test adapter. | Canonical relationship and physics remain unchanged; only presentation provider changes. | Provider conformance test. |
| MOB-PRES-004 | Exercise roof riding, Grip/hang, boarding, failure, roll, recovery, and correction cameras. | View blends continuously, filters unwanted vibration, frames relevant motion, and never snaps Character to camera/stale state. | Continuous captures. |
| MOB-PRES-005 | Change shake, roll, horizon lock, FOV, and camera-motion accessibility settings. | Settings apply without changing physical state, feel profile, authority, or target validity. | Config/state comparison. |
| MOB-PRES-006 | Aim during stable, braced, slide, Grip/hang, leap, roll, and recovery states. | Aim is available only when pose/equipment/stability/free-hand capability allows; camera look remains available. | Capability/input matrix. |
| MOB-ACC-001 | Complete Grip/catch paths with hold, toggle, and automatic Grip. | All modes issue equivalent valid intent; none bypasses reach/load/authority. | Accessibility-mode comparison. |
| MOB-ACC-002 | Use timing assistance and alternatives to prolonged/repeated/simultaneous/analog-only input. | Assisted intent occurs only at a valid physical opportunity and prompts reflect bindings. | Accessibility test record. |
| MOB-ACC-003 | Configure developer-certified ranges/presets, exercise player-selectable values and device-local persistence, then remove an accepted alternative. | Players cannot exceed certified ranges; accessibility/control/camera preferences persist as device settings while feel remains authoritative gameplay configuration; removing an alternative requires an equivalent path or forfeits that conformance claim. | Settings/lifetime/capability-manifest audit. |
| MOB-AI-001 | Replay each T2 semantic action sequence through player and NPC control sources. | Canonical action, recipe/phase, state, animation execution, physics, authority, failure, and evidence are equivalent; disposable local owner anticipation may differ without changing the result. | Paired machine-readable result. |
| MOB-AI-002 | Disable selected NPC options through OWS recommended and custom presets. | Only configured capability is unavailable; shared implementation and other capabilities remain intact. | Preset/configuration test. |
| MOB-AI-003 | Switch player, NPC, and neutral control sources during stable support, occupancy, reservation, and committed transition phases. | The same Character and relationship remain; only control source/policy changes, reservations and physical state follow their normal rules, and no duplicate Character or simplified path appears. | Entity/relationship/control-source trace. |

## G. Authority, multiplayer, and impairment

| ID | Setup/action | Expected result | Required evidence / owner |
| --- | --- | --- | --- |
| MOB-NET-001 | Run each core transition as standalone, listen host, remote listen client, and remote dedicated client. | Server owns one canonical result; owner and observers converge without role-specific behavior drift. | Synchronized multi-endpoint artifacts. |
| MOB-NET-002 | Repeat core network suite under legacy replication and Iris. | Backend-neutral schema and terminal outcomes are equivalent. | Paired backend diff. |
| MOB-NET-003 | Predict continuous support, balance, slide, Grip body shift, airborne, roll, and recovery. | Owner predicts only inside authorized relationship; proxies interpolate; server correction cannot double-apply Vehicle motion. | Prediction/correction trace. |
| MOB-NET-004 | Begin disposable transition anticipation, then accept and reject in separate runs. | Anticipation never mutates collision/occupancy/control; acceptance aligns to authoritative start and rejection cancels smoothly with reason. | Owner/server state diff. |
| MOB-NET-005 | Validate client-originated and NPC/server-originated moving requests using historical Character and articulated target samples. | Source-typed time maps client intent or uses server time correctly; both participants evaluate at the same instant while current occupancy/permission/revision still gates commit. | Rewind/commit trace. |
| MOB-NET-006 | Submit request outside declared history/skew envelope. | Server rejects without clamping or partial mutation. | Boundary test. |
| MOB-NET-007 | Delay, duplicate, reorder, and replay identical requests/results. | Request commits at most once and cached terminal result remains stable. | Packet/test harness evidence. |
| MOB-NET-008 | Acquire a reservation after all recipe/data dependencies are ready, then stop authoritative phase progress before and after physical commitment. | Lease never spans asset loading/client wait, never renews from client heartbeat, and ends deterministically on timeout or the catch/miss/arrival terminal result. | Dependency-readiness and lease timeline. |
| MOB-NET-009 | Have multiple players/NPCs contend for one seat, portal, anchor, and edge with controlled normalized intent times, receive order, and request IDs. | Committed/valid reserved targets are not preempted; ordering is normalized intent time → server receive sequence → stable request ID; every loser receives reason, current revision, retryability, and valid alternatives. | Repeated contention/result-payload trace. |
| MOB-NET-010 | Enable a deterministic role-priority provider. | Provider affects only uncommitted contenders and remains identical on repeat; committed relationships cannot be stolen. | Policy test. |
| MOB-NET-011 | Cause errors on both sides of every Character-scale-normalized correction boundary at 30/60/120 FPS and accepted impairment profiles. | Each certified configurable tier uses the declared smoothing/physical/recovery behavior with no camera-driven or stale-transform snap; out-of-range configuration fails validation. | Boundary/cross-rate/network correction matrix. |
| MOB-NET-012 | Run the T4 suite at the supported impairment profile. | All behavior passes; presentation may not corrupt canonical state. | Network emulation artifacts. |
| MOB-NET-013 | Run recovery cases at the stress impairment profile with reorder/duplication. | Presentation may degrade, but state never corrupts, duplicates, or becomes unrecoverable. | Stress artifacts. |
| MOB-NET-014 | Join late during each stable relationship and committed transition phase. | Snapshot reconstructs current state/phase without replaying stale transient events. | Late-join endpoint trace. |
| MOB-NET-015 | Drop relevance/dormancy pressure on one participant in an active relationship. | Related actors remain co-relevant server-side; client presentation suspends/restores from a fresh snapshot without detachment. | Relevancy trace. |
| MOB-NET-016 | Lose replication provider/backend during idle, reservation, and committed motion. | New work fails closed; valid canonical state persists where possible; otherwise typed recovery converges across endpoints. | Failure-injection trace. |
| MOB-NET-017 | Spoof source/avatar identity, use foreign/stale target IDs or time, send invalid revisions/types, NaN/Inf/out-of-range values, arbitrary asset/class paths, oversized context/collections, and request/reservation floods while valid work continues. | Instigator derives from the authenticated connection; bounded allowlisted admission rejects or rate-limits before expensive history/physics work; queues, history, leases, and terminal-result caches remain bounded; no state mutates or source starves committed recovery/well-behaved traffic. | Auth/fuzz/rate/resource artifacts; #27/#141. |

## H. Lifecycle and persistence

| ID | Setup/action | Expected result | Required evidence / owner |
| --- | --- | --- | --- |
| MOB-LIFE-001 | Disconnect during reservation. | Reservation and control release once; no partial occupancy/transition remains. | Server snapshot and lease trace. |
| MOB-LIFE-002 | Disconnect while seated, supported, gripping, or in physical flight; attempt duplicate connection and authenticated stale reclaim. | Same Character remains server-simulated; configured neutral/NPC/grace/termination policy applies, duplicate live control rejects, and stale reclaim succeeds only after relationship/control revalidation. | Policy-by-state and connection-identity tests. |
| MOB-LIFE-003 | Change owner/possession/control source while occupied. | Control grant changes independently; occupancy, embodiment, and relationship remain unless explicitly transitioned. | Orthogonal-state trace. |
| MOB-LIFE-004 | Interrupt presentation before and after physical commitment. | Before commit cancels; after commit physics completes/fails without depending on animation state. | Animation failure injection. |
| MOB-LIFE-005 | Destroy one point/body and then the whole Vehicle. | Only affected points invalidate first; release preserves last valid same-time motion, and any physically produced destruction impulse, if present, is applied once. | Destruction trace. |
| MOB-LIFE-006 | Die/respawn during each relationship family. | Death resolves/terminates physical state and clears reservations/control; respawn begins without a stale relationship. | Server/observer state records. |
| MOB-LIFE-007 | Seamless-travel with a stable relationship and all IDs migrating. | Relationship rebinds only after every participant/graph ID revalidates. | Travel snapshot/rebind trace. |
| MOB-LIFE-008 | Non-seamless map travel. | Relationship terminates safely; destination does not inherit transient state. | Travel test. |
| MOB-LIFE-009 | Cross a World Partition boundary while actively related. | Participants co-stream; forced unload performs safe authoritative termination before separation. | Cell/state trace. |
| MOB-LIFE-010 | Save/checkpoint/load one world/server-lifetime stable relationship referenced by Character, Vehicle occupancy/control, and graph records. | One persistence coordinator owns one committed relationship record; cross-record IDs/revisions revalidate atomically before any live field publishes, otherwise recovery uses the latest validated safe anchor. | Record-owner/persistence artifact and restore trace. |
| MOB-LIFE-011 | Save/checkpoint during possession/control, reservation, prediction/anticipation/presentation, or an in-progress transition. | None of those transient/session/device fields serializes or resurrects; work resolves to authored safe checkpoint state and stable occupancy/support restores only under its declared lifetime. | Field-by-field snapshot inspection. |
| MOB-LIFE-012 | Load missing/corrupt/outdated graph identity. | Version migration or governed safe fallback occurs; no guessed target or partial relationship is created. | Negative persistence fixtures. |
| MOB-LIFE-013 | Restart the authoritative server from the last committed checkpoint. | Stable world-owned relationships restore only after revalidation; possession, control, reservations, transitions, prediction, and presentation begin fresh. | Pre/post-restart field-lifetime diff. |
| MOB-LIFE-014 | Audit every serialized Mobility field across save/load, travel, World Partition, reconnect, and restart. | Every field has exactly one declared owner and controlling lifetime; no Character, Vehicle, Integration, profile, or cache record duplicates canonical truth. | Schema/record ownership report; #132/#138/#139. |

## I. Diagnostics, Shipping, performance, and scale

| ID | Setup/action | Expected result | Required evidence / owner |
| --- | --- | --- | --- |
| MOB-OPS-001 | Inspect one action in Editor overlay, runtime overlay, Visual Logger, Insights, Rewind Debugger, Chaos Visual Debugger, and test artifact. | Every view correlates request/transition/relationship/revision, authority/time, participant/graph/point/body identities, before/after state, route/clearance/intercept, lease/contention, target/relative kinematics, release/impulse/contact/support, balance/friction/aerodynamic/Grip/coupling budgets, prediction/correction/resimulation, endpoint agreement, and reason codes. | Schema-completeness and cross-tool correlation record. |
| MOB-OPS-002 | Filter diagnostics by relationship, participant, category, and failure reason. | Relevant evidence remains inspectable without unconditional per-frame log spam. | Tooling test. |
| MOB-OPS-003 | Build ordinary Shipping configuration and execute core loop. | No overlay, file, trace, network request, or external transmission is produced by default. | File/network/process audit. |
| MOB-OPS-004 | Enable an approved bounded Shipping sink. | Sampling, retention, stable-ID redaction, performance, and consent rules apply; no secrets or external account IDs leak. | Privacy/security/performance review. |
| MOB-OPS-005 | Run representative physical/action suite at 30, 60, and 120 FPS. | Canonical state, transition outcomes, force accounting, and recovery remain equivalent within published tolerances. | Cross-rate numeric diff. |
| MOB-OPS-006 | Run at 60 FPS target on published midrange reference PC. | Declared frame and per-system budgets pass without concealing simulation/network work. | Profile capture and hardware manifest. |
| MOB-OPS-007 | Run 32 players, 32 active/relevant Vehicles, and 16 concurrent relationships in one hotspot/large Vehicle. | Server/client budgets, relationships, contention, diagnostics, and recovery remain within published limits. | Scale automation and profiles. |
| MOB-OPS-008 | Run Windows and Linux dedicated servers with Windows clients. | Behavior, authority, persistence, and evidence format remain equivalent. | Platform CI artifacts. |
| MOB-OPS-009 | Run long-cycle mixed player/NPC boarding, riding, switching, exit, bailout, and recovery. | No accumulating memory, reservation, identity, input, collision, force, presentation, or performance leak occurs. | Soak report. |
| MOB-OPS-010 | Attempt unsupported carrier/platform/profile configuration. | OWS reports the capability as unverified/unsupported rather than silently advertising it. | Capability-report test. |
| MOB-OPS-011 | Sustain the maximum accepted diagnostic event rate, then exceed the configured sampling/retention/trace budgets. | Memory, history, files, and traces remain bounded; dropped/sampled counts are visible; diagnostics never change gameplay or authority. | Long-run budget and resource audit; #140/#141. |
| MOB-OPS-012 | Feed secrets, external account IDs, raw unredacted identities, and sensitive typed context through every Editor/test/runtime adapter and evidence exporter. | Every sink remains local/privacy-preserving by default, redacts stable identities, excludes prohibited data, and performs no external transmission without developer integration and applicable consent. | Adapter-by-adapter privacy/data-flow audit; #140/#141. |

## J. Protected migration regressions

The following must remain green until equivalent accepted paths replace them, and remain regression coverage afterward:

| ID | Protected behavior | Source matrix |
| --- | --- | --- |
| MOB-REG-001 | Precision ray resolves exact Vehicle and Door ID; activation does not substitute the nearest Vehicle. | CV-ENTRY-001/003 |
| MOB-REG-002 | Stopped entry, control handoff, input isolation, and camera remain usable. | CV-CORE-001; CV-DRIVE-001–005 |
| MOB-REG-003 | Preferred-door stopped exit, safe alternative, blocked rejection, and repeated cycles remain valid. | CV-EXIT-001–005 |
| MOB-REG-004 | Cancel never becomes a Vehicle brake/control action. | CV-DRIVE-007 |
| MOB-REG-005 | Existing low-speed and controlled-roll bailout behavior remains protected until the accepted physical replacement passes. | CV-BAIL-001–008 |
| MOB-REG-006 | Recovery and re-entry preserve Character identity, input, camera, collision, animation, and occupancy over repeated cycles. | CV-RECOVERY-001–005 |
| MOB-REG-007 | Current collision and impact universal safety invariants remain finite and recoverable. | CV-IMPACT-001–008 |

## Support-claim gates

### Independent domains

Character and Vehicle contracts are accepted only when their T0 suites pass independently, their public/extension surfaces are documented, and no private peer or Integration dependency exists.

### Same-Vehicle Mobility first slice

The first supported Mobility slice requires:

1. every applicable scenario above has a current result and none is `NR`, `FAIL`, or `BLOCKED` for the declared coverage;
2. every current road Vehicle has graph/identity/kinematic/collision coverage and every compatible path has the required full-roster result;
3. player/NPC parity, all three feel profiles, required accessibility alternatives, and 30/60/120 FPS evidence pass;
4. standalone, listen, dedicated, legacy, Iris, supported impairment, recovery stress, Windows/Linux server, and scale evidence pass;
5. all protected migration regressions remain green;
6. diagnostics and ordinary Shipping/privacy audits pass; and
7. every result is tied to the exact release-candidate commit, engine, assets, profiles, and durable evidence.

### Cross-Vehicle transfer

Graph representation and validation pass in the first architecture, but runtime transfer support remains unavailable until a separately accepted implementation issue expands this matrix with paired moving-vehicle approach, departure, catch, contention, miss, collision, correction, lifecycle, and recovery scenarios and every row passes.

## Turning failures into work

Every independent failure becomes one tightly scoped issue containing:

- scenario and configuration IDs;
- exact reproduction and frequency;
- expected and observed behavior;
- durable evidence;
- demonstrated affected scope;
- controlling architecture and owning domain;
- explicit blockers and non-goals; and
- done-when coverage including relevant protected regressions.

An implementation issue may group failures only when evidence demonstrates one root cause and its acceptance criteria name every affected scenario. An out-of-scope discovery never broadens the issue currently being worked.
