# OWS Mobility Evidence Baseline

> **Status:** Evidence, research, and accepted interview-decision record for [issue #147](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/147). Audited on 2026-08-24 and reconciled through the closed interview on 2026-08-25 against baseline commit `705ef58013e2fe074687b1f006c91c81989c108a`, Unreal Engine 5.8.1 changelist 56057345. This record is non-normative; the accepted target is the [OWS Mobility Architecture](OWS_MOBILITY_ARCHITECTURE.md), and its proof is defined by the accepted [OWS Mobility Acceptance Matrix](OWS_MOBILITY_ACCEPTANCE_MATRIX.md). Neither document authorizes gameplay implementation.

## Purpose

This baseline preserves the five evidence classifications used during the completed Mobility audit, interview, and architecture work:

| Classification | Meaning |
| --- | --- |
| **Implemented and covered** | Source or asset behavior exists and current OWS automation or accepted manual evidence covers the explicitly named slice. Test code existing is not evidence that it passed in this audit. |
| **Implemented but unverified** | Code or data exists, but the relevant runtime, network, or coverage claim has not passed declared evidence. |
| **Incidental** | Unreal or an inherited foundation may produce the behavior, but OWS does not own, author, verify, or support it. |
| **Absent** | No supported OWS path or public contract was found. |
| **Legacy** | Code or data remains but is not on the active runtime path and must not be described as current behavior. |

Target architecture is not evidence of current implementation. Current implementation does not automatically conform to a proposed or accepted target architecture.

## Executive finding

Current OWS provides a valuable **local-player, single-driver integration slice**:

- shared precision selection and Activate routing;
- authored vehicle doors, seats, exit transforms, and replicated seat occupancy;
- stopped entry, vehicle possession, authored-door exit with ground/non-vehicle-blocker fallback, and re-entry;
- moving bailout with inherited motion, a physics-inspired energy-loss model, controlled roll, recovery, and re-entry; and
- automated editor test definitions covering representative selector, entry, exit, and bailout state handoffs.

Current OWS does **not** yet provide the general Mobility relationship system required by the accepted program charter. Passenger gameplay, supported moving surfaces, exterior riding, balance, grip, hanging, moving entry, infiltration, transfers, explicit pending seat/anchor/portal/transition reservations, cross-system prediction, authoritative remote-client transitions, and recovery from their failure modes are absent. Any current ability to stand on or enter a moving vehicle is incidental or incomplete and cannot be advertised as supported OWS behavior.

The largest immediate architecture risk is not a missing animation or threshold. It is the absence of an explicit, replicated relationship and transition model separating player identity, Character embodiment, occupancy, control permission, presentation, and physical contact.

## Current runtime inventory

### Composition and inherited foundations

- OWS character assets currently inherit the maintained GASPALS Blueprint architecture. `CBP_OWSCharacter_Base` is a child of `CBP_SandboxCharacter` and adds `OWSSelectorComponent`; the OWS controller inherits `PC_Sandbox` and owns the controller hotbar and vehicle interaction components.
- GASPALS is currently content-only and enables Pose Search, Chooser, Motion Warping, Animation Warping, the Animation Locomotion Library, Mover, and Network Prediction. See [`GASPALS.uplugin`](../Plugins/GASPALS/GASPALS.uplugin).
- KinetiForge is a C++ Runtime plugin and the current OWS game module directly depends on it, Chaos, Chaos Vehicles Core, Chaos Modular Vehicle Engine, Enhanced Input, NetCore, PhysicsCore, and OWSCore. The project also enables Chaos Modular Vehicles and its examples. See [`OWS.Build.cs`](../Source/OWS/OWS.Build.cs) and [`OWS.uproject`](../OWS.uproject).
- Async physics, physics prediction, and physics-history capture are enabled in current configuration. Those low-level settings are not evidence that Character–Vehicle transitions are predicted, authoritative, or multiplayer-ready. See [`DefaultEngine.ini`](../Config/DefaultEngine.ini).
- The current project module and content still contain direct `/GASPALS/...` and `/KinetiForge/...` runtime references. GASPALS and KinetiForge already form the retained provenance boundaries for the complete maintained Character and Vehicle forks. What does not yet exist is their accepted OWS-facing public contracts, independent conformance evidence, and the narrow Character–Vehicle Integration family that consumes both contracts without exposing fork-specific internals.

This is a current-placement fact, not permission to preserve that coupling as the public architecture.

### Character and animation

| Capability | Current evidence | Classification |
| --- | --- | --- |
| OWS character variants | Asset inspection and accepted manual evidence identify Manny, Manny Clone, and Quinn as OWS/GASPALS-derived variants retaining their visual differences. Current smoke code only counts three OWS Character actors and checks shared setup markers; it does not prove each named identity, parent chain, or appearance. | Implemented, manually verified, and only partially covered by automation. |
| Ordinary locomotion and traversal | Inherited GASPALS content and Blueprint behavior is present. | Implemented but only manually verified; no native OWS automation covers the full locomotion/traversal surface. |
| Experimental GASPALS state-machine content | Assets and per-character `Force SM Setup` markers exist; the global experimental CVar defaults off. | Implemented but runtime behavior is unverified. |
| Generic Mobility relationship/state API | No Character-owned native Mobility state, relationship, transition request/result, or animation-handoff API exists. | Absent. |
| Controlled bailout presentation | Vehicle integration directly replaces the AnimBP with a root-locked single-node GASPALS roll, drives its pose by traveled distance, then restores the cached AnimBP. | Implemented and covered for the current bailout slice; tightly coupled rather than a general Character contract. |
| Old ragdoll ejection/recovery | Functions and tuning data remain, but no active caller was found. | Legacy. |

Relevant source: [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp), [`OWSVehicleInteractionComponent.h`](../Source/OWS/Public/OWSVehicleInteractionComponent.h), [`DefaultEngine.ini`](../Config/DefaultEngine.ini), and [`OWSCleanStartSmoke.py`](../Scripts/SmokeTest/OWSCleanStartSmoke.py).

### Selection and Activate

- `UOWSSelectorComponent` provides configurable sphere and cone range detectors, an independent precision ray, local target readout, and activation delegates.
- Activate currently polls raw `F` and Gamepad Face Button Left/Square locally rather than consuming a semantic Enhanced Input action.
- The precision ray hits an actor, then resolves an enabled interaction target nearest the impact point. It does not literally ray-hit a door marker. Once chosen, the exact Vehicle and Door ID are preserved.
- `SelectionRadius` and `bRequiresFacing` are exposed but are not consumed by the current selection calculations.
- `UOWSInteractionTargetComponent` routes activation through component or actor handlers.

Relevant source: [`OWSSelectorComponent.cpp`](../Source/OWS/Private/OWSSelectorComponent.cpp), [`OWSSelectorComponent.h`](../Source/OWS/Public/OWSSelectorComponent.h), [`OWSInteractionTargetComponent.cpp`](../Source/OWS/Private/OWSInteractionTargetComponent.cpp), and [`OWSInteractionTargetComponent.h`](../Source/OWS/Public/OWSInteractionTargetComponent.h).

The selector is a useful public interaction foundation. Local polling and local dispatch are not an authoritative Mobility request path.

### Vehicle structure and authoring

`UOWSStockVehicleInteractionComponent` currently provides:

- editable seat definitions with authored `FName` IDs, seat type, control permission, relative transform, and clearance;
- editable door definitions with authored `FName` IDs, door and exit transforms, plus ordered accessible seats;
- default driver and passenger data with left and right doors;
- per-door interaction targets created at runtime;
- per-instance overrides, including the bus's front and rear curb doors; and
- a replicated array of seat occupant Actor references.

ID uniqueness and version stability are not currently validated; duplicate lookup returns the first matching definition. Another authored door—not a per-door fallback transform—is the runtime stopped-exit fallback. This is a strong starting data model for stopped doors and seats. It does not yet describe standable zones, grip anchors, edges, portals beyond doors, transition edges, moving sub-bodies, point-kinematic sources, alignment recipes, or authored failure envelopes.

Relevant source: [`OWSStockVehicleInteractionComponent.h`](../Source/OWS/Public/OWSStockVehicleInteractionComponent.h) and [`OWSStockVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSStockVehicleInteractionComponent.cpp).

### Current entry relationship

The active entry path currently performs these operations sequentially:

1. resolve the selected vehicle and door;
2. select an available **control seat**;
3. record authoritative seat occupancy;
4. stop Character movement and disable Character collision;
5. hide and attach the Character at the seat;
6. possess the vehicle; and
7. add a higher-priority vehicle input context and retarget the camera while the existing OWS context remains present.

The relationship is represented through transient controller-component pointers and booleans such as `HomeCharacter`, `OccupiedVehicle`, `OccupiedSeatId`, `OccupiedDoorId`, and bailout/recovery flags. There is no explicit relationship object, transition phase, revision, reservation token, durable result, or atomic multi-owner transaction.

Passenger seat data can be selected by lower-level vehicle code, but both player entry paths reject any seat that does not control the vehicle. A player-controlled vehicle is rejected wholesale, so another player cannot currently enter as a passenger.

Entry validates range, target, controller, and seat availability. It does not validate vehicle speed, exact target-point motion, relative velocity, gap, approach, acceleration, time-to-contact, or a moving transition recipe. A moving unoccupied vehicle can therefore be entered through the same instantaneous hidden-character seat placement. That is incidental behavior, not supported moving entry.

### Current stopped exit

At or below 5 mph, Cancel exits immediately. The stopped path:

- prefers the door used for entry;
- traces for walkable support;
- validates walkable ground and clearance from non-vehicle blockers;
- uses another authored door only when the preferred result is unsafe; and
- restores Character possession, movement, collision, vehicle-input context, and camera; the ordinary AnimBP was never replaced on this path.

The clearance query intentionally ignores the vehicle, so clearance from the vehicle itself relies on the authored exit location plus representative tests. Exit also calls the current driver-removal path, which zeroes the vehicle's linear and angular velocity and sleeps its simulated bodies. This is existing accepted behavior, but it is also an architectural limitation to inventory rather than silently preserve as a universal exit rule. The verified stopped-exit cycle remains protected until any replacement passes the expanded Mobility matrix.

### Current moving bailout

Above 5 mph, Cancel requires a two-second hold.

- From greater than 5 through 15 mph, OWS returns directly to Character locomotion with the vehicle's captured linear velocity.
- Above 15 mph, OWS starts the controlled roll.
- The roll applies sliding work, aerodynamic work, and synthesized per-cycle impact-energy loss, limits steering, starts in walking mode, and returns directly to ordinary locomotion at the configured stand speed. Existing representative test definitions cover recovery on the default course; behavior when floor contact is lost is unverified.

The current inherited velocity is the simulating root primitive's linear velocity, falling back to the Vehicle Pawn velocity when the root is not simulating. It omits angular point velocity, articulated door or anchor motion, and actual Character-relative movement at release. This is sufficient for the accepted current bailout presentation but not the general point-kinematic contract needed by moving supports and transfers.

Like stopped exit, bailout currently removes the driver through a path that zeroes and sleeps the vehicle after velocity is captured. A re-entry block exists only while the greater-than-15-mph controlled roll is active and clears when recovery completes; the 5–15-mph path sets no such block. The current test definition expects re-entry immediately after recovery rather than enforcing a post-recovery retreat interval.

The bailout code emits warning-level telemetry at 100 ms intervals by default from vehicle entry through post-exit stop. That is current debug/tuning residue, not a release telemetry policy.

### Moving supports and exterior traversal

No OWS source implementation was found for:

- authored standable zones;
- `SetBase` or an OWS moving-support abstraction;
- stable support or contact-anchor identity;
- support-relative balance, bracing, crouch benefit, sliding, grip, hanging, or stamina;
- exterior-rider occupancy or capacity;
- moving-entry eligibility and speed matching;
- same-vehicle exterior/interior infiltration; or
- vehicle-to-vehicle transfer.

Unreal's Character Movement Component can derive a movement base from a walkable floor and follow its transform. A vehicle surface that happens to qualify may therefore carry a Character today. OWS has not inventoried the qualifying surfaces, collision configuration, rotating-body behavior, correction, or failure cases. That behavior is **incidental and unsupported** until evidence passes.

## Public-contract and architecture gaps

1. **No general relationship model.** Current state is implicit in one controller component rather than an explicit, replicated relationship with orthogonal bodily, occupancy, transition, and control concepts.
2. **No authoritative remote-client transition path.** Selection and Activate run locally; seat mutation requires authority; the vehicle interaction component has no Server RPC or replicated Mobility request/result path and calls `Possess` directly.
3. **No atomic transition orchestration.** Occupancy, attachment, visibility, collision, possession, input, and camera mutate sequentially without a transaction coordinating contention, destruction, disconnect, or correction.
4. **Passenger data without passenger gameplay.** Occupancy and control are still coupled at the player flow.
5. **No exterior-mobility substrate.** Supported surfaces, anchors, grip, moving entry, infiltration, and transfer are absent.
6. **Incomplete point kinematics.** Chassis-center linear velocity is used where the exact target point and local articulated motion are required.
7. **Hardwired animation handoff.** Integration directly controls a GASPALS mesh and named roll assets rather than requesting Character-owned bodily execution and presentation.
8. **No stable cross-domain public seam.** Most transition APIs are private C++; the current vehicle authoring/query API is not a complete Blueprint-usable package contract.
9. **Current composition is coupled.** The retained complete Character and Vehicle forks exist, but their accepted OWS-facing public contracts and independent conformance do not; the narrow first-party Integration family has not yet been created.
10. **Inherited vehicle networking is unaccepted.** KinetiForge declares replicated/RPC behavior, but the inspected lifetime replication and RPC declarations are inconsistent enough that OWS cannot claim a multiplayer vehicle foundation without focused tests.

## Multiplayer support truth

Replicated seat occupancy is useful state, but it does not make the current entry/exit flow multiplayer-ready.

- A client-to-server Mobility request must travel through an Actor owned by that client, such as its PlayerController, Character, or an owned interaction bridge. An RPC invoked through another player's or an unowned vehicle is not a valid general request path.
- The server must validate actor identity, current relationship revision, scoped target IDs, occupancy/reservations, temporal target-point motion, capability, and clearance before committing a transition.
- Seats, exterior anchors, grip ownership, portal reservations, and current relationships are durable state and must replicate as state; multicast events alone cannot reconstruct them for late joiners or newly relevant actors.
- Discrete reservation and relationship mutations can be atomic. A physical leap or release cannot be rolled back after launch; a miss or interruption must converge on a valid airborne or failure state.
- Character prediction and vehicle prediction can operate on different timelines. Cross-system validation needs an accepted timestamp/history policy.

No network-mode support claim is authorized until listen host, remote client, contention, correction, late join, disconnect, destruction, relevance, and recovery scenarios pass with lag, jitter, packet loss, and correction—not only ideal local PIE.

## Unreal Engine 5.8 capability research

### Maturity and safe baseline

| Unreal system | UE 5.8.1 status | Evidence-supported use in #147 |
| --- | --- | --- |
| **Character Movement Component (CMC)** | Supported and production-proven; Epic describes it as battle-hardened and committed to continued support. | Supported reference baseline. It already provides saved moves, server correction/replay, smoothing, moving bases, swept movement, and root-motion handling. |
| **Mover / ChaosMover** | Experimental; incomplete features and mutable APIs with limited shipped-project and scaling evidence. | Research/benchmark candidate only. It must not become the mandatory OWS baseline without an accepted benchmark and migration decision. |
| **Classic Chaos Vehicles** | Experimental plugin. | Current/inherited vehicle primitive requiring validation; its documentation does not establish Modular Vehicles' rewind/resimulation claim. |
| **Chaos Modular Vehicles** | Experimental plugin with documented Network Physics rewind/resimulation. | Isolated benchmark candidate, not an automatic replacement decision. |
| **Motion Warping** | Beta. | Candidate presentation-alignment primitive after a transition is accepted; not a physics, authority, clearance, or reservation solver. |
| **Networked Physics** | Available low-level facility with Default, Predictive Interpolation, and Resimulation modes. | Possible Vehicle/integration primitive. Resimulation requires explicit C++ integration and additional CPU/memory for history. |

Primary Unreal references:

- [CMC networked movement](https://dev.epicgames.com/documentation/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine)
- [Mover and CMC comparison](https://dev.epicgames.com/documentation/unreal-engine/comparing-mover-and-character-movement-component-in-unreal-engine)
- [Mover status](https://dev.epicgames.com/documentation/unreal-engine/API/PluginIndex/Mover) and [Mover overview](https://dev.epicgames.com/documentation/unreal-engine/mover-in-unreal-engine)
- [ChaosMover status](https://dev.epicgames.com/documentation/unreal-engine/API/PluginIndex/ChaosMover)
- [Chaos Vehicles status](https://dev.epicgames.com/documentation/unreal-engine/API/PluginIndex/ChaosVehiclesPlugin) and [Chaos Vehicles overview](https://dev.epicgames.com/documentation/unreal-engine/chaos-vehicles)
- [Chaos Modular Vehicles status](https://dev.epicgames.com/documentation/unreal-engine/API/PluginIndex/ChaosModularVehicle)
- [Chaos Modular Vehicles](https://dev.epicgames.com/documentation/unreal-engine/chaos-modular-vehicles-overview)
- [Motion Warping status](https://dev.epicgames.com/documentation/unreal-engine/API/PluginIndex/MotionWarping) and [Motion Warping overview](https://dev.epicgames.com/documentation/unreal-engine/motion-warping-in-unreal-engine)
- [Networked Physics](https://dev.epicgames.com/documentation/unreal-engine/networked-physics-overview)

Network Physics is not a free synchronization layer. Epic documents that Default replication can overwrite local state, Predictive Interpolation becomes less stable as latency and velocity increase, and Resimulation stores history and can still converge differently enough to require correction. Any candidate backend therefore needs measured CPU, memory, bandwidth, correction, and failure evidence rather than a feature-presence claim.

### CMC moving-base foundation

CMC moving bases already do more than ordinary actor attachment:

- follow the base's transform delta using relative position and optional rotation;
- normally sweep the Character while applying base motion and report when an obstacle prevents the move;
- save relative base location and rotation;
- carry base component, optional skeletal bone, relative transform, and unresolved-base state;
- include starting and ending base data in saved moves for prediction/correction; and
- impart computed base velocity when the Character leaves the base.

Unreal 5.8 moves extension points toward `FMovementBaseInterfaceData`; new OWS work must not build against deprecated direct-component assumptions.

`bFastAttachedMove` directly sets the destination without the ordinary obstruction sweep. It is inappropriate for exterior traversal through uncertain geometry unless clearance is guaranteed.

CMC provides a credible supported foundation for standing on a moving base. It does not automatically provide grip, hanging, climbing a moving body, moving entry, transfer, reservation, balance failure, or an OWS authoring contract.

Relevant local engine source:

- `C:/UE_5.8/Engine/Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp`, especially based movement, base velocity, and release paths.
- `C:/UE_5.8/Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h`, especially `FBasedMovementInfo` and saved-move base data.
- `C:/UE_5.8/Engine/Source/Runtime/Engine/Private/Character.cpp`, especially tangential base velocity.

### Point kinematics

For a point rigidly attached at offset `r` from a vehicle reference body:

`point velocity = linear velocity + angular velocity × r`

`point acceleration = linear acceleration + angular acceleration × r + angular velocity × (angular velocity × r)`

For a target point that also moves relative to a rotating parent, the complete relative-motion form adds local velocity and acceleration, written below after transforming the relative terms into world coordinates:

`point velocity = origin velocity + angular velocity × r + relative local velocity`

`point acceleration = origin acceleration + angular acceleration × r + angular velocity × (angular velocity × r) + 2 × angular velocity × relative local velocity + relative local acceleration`

The Coriolis term matters when a door, articulated section, skeletal bone, or other target point moves inside its rotating parent frame. Prefer the actual body/component/bone's point data when available. Unreal's point-velocity APIs provide velocity for a selected physics body; they do not supply acceleration history, and kinematic or teleported targets require time-consistent transform sampling or movement-system evidence.

Unreal exposes the same concept through `FBodyInstance::GetUnrealWorldVelocityAtPoint` and **Get Physics Linear Velocity at Point**. CMC also calculates tangential base velocity from angular velocity crossed with radial distance.

Moving-entry, grip, and transfer decisions begin with:

`relative velocity = character world velocity - target-point world velocity`

Absolute vehicle speed alone cannot determine safety. A Character and door moving together at high world speed may have little relative motion; a nearby slow door moving across the Character can be unsafe.

References:

- [Rigid-body kinematics, University of Illinois](https://mechref.engr.illinois.edu/dyn/rkg.html)
- [MIT rotating-frame dynamics](https://ocw.mit.edu/courses/16-07-dynamics-fall-2009/resources/mit16_07f09_lec08/)
- [Unreal point-velocity Blueprint API](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Physics/GetPhysicsLinearVelocityatPoint)
- [Unreal `FBodyInstance` point-velocity API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FBodyInstance/GetUnrealWorldVelocityAtPoint)

### Contact, balance, sliding, grip, and release

For a simple passive contact, no slip is possible only while the required tangential load remains within the available static-friction capacity:

`|tangential force| <= static-friction coefficient × normal force`

After slip begins, a basic Coulomb model uses kinetic friction opposing relative tangential motion. Normal force is not always Character weight: slope, vehicle acceleration, contact loss, and impact change it, and multiple simultaneous contacts require a declared contact-wrench/friction-cone model or an explicitly documented gameplay simplification. Acceleration, braking, turning, angular acceleration, bumps, loss of normal force, slope, and impacts therefore matter; constant linear speed by itself does not throw a rider off.

Friction capacity alone does not establish balance or prevent tipping. Support geometry, center of mass relative to the support area, net force, net torque, angular capability, and active grip all matter. Turning load scales with speed squared divided by turn radius. Aerodynamic drag scales with air-relative speed squared. Crouching can alter support geometry and projected area or drag coefficient, but those effects are Character- and pose-specific; physics supplies no universal crouch multiplier.

CMC characters are kinematic rather than passive rigid bodies. A Chaos Physical Material exposes one friction value and combine policy for simulated contacts; it does not provide separate human static/kinetic grip behavior, balance, or active bracing. If OWS retains CMC, Character-owned balance and grip capability must be compared against physically derived required loads and authored assistance. The architecture must not pretend a controlled person is a loose block.

When a Character leaves a CMC movement base, `GetImpartedMovementBaseVelocity` can conditionally include base linear and tangential angular velocity through CMC's impart flags. The accepted contract declares one governed release-motion path: OWS must either consume that CMC result or replace it, never pre-add the same motion twice. Any inherited motion and intentional separation impulse must each be applied exactly once.

References:

- [OpenStax friction](https://openstax.org/books/university-physics-volume-1/pages/6-2-friction)
- [OpenStax static equilibrium](https://openstax.org/books/university-physics-volume-1/pages/12-1-conditions-for-static-equilibrium)
- [OpenStax centripetal force](https://openstax.org/books/university-physics-volume-1/pages/6-3-centripetal-force)
- [NASA drag equation](https://www1.grc.nasa.gov/beginners-guide-to-aeronautics/drag-equation/)
- [NASA factors that affect drag](https://www1.grc.nasa.gov/beginners-guide-to-aeronautics/factors-that-affect-drag/)
- [OpenStax impulse and collisions](https://openstax.org/books/university-physics-volume-1/pages/9-2-impulse-and-collisions)
- [Unreal Physical Materials](https://dev.epicgames.com/documentation/unreal-engine/physical-materials-reference-for-unreal-engine)
- [CMC imparted movement-base velocity](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent/GetImpartedMovementBaseVelocity)

### Animation and alignment

Motion Warping can align root motion to a moving component or skeletal bone. It is a candidate for boarding, climbs, and transfers after the server selects a valid transition. It does not decide feasibility or provide collision clearance, authority, grip, friction, or reservations. Following a moving component also requires correct tick prerequisites to avoid a one-frame target lag, and the warp target is not documented as automatically replicated. Server and owning client need the same authoritative target identity/transform and montage trigger.

Arbitrary client transform changes are not captured by CMC saved moves and invite server correction. Network-safe custom movement must participate in the Character movement prediction path or use supported root-motion sources/montages. Root-motion montage movement can replicate, but its trigger and transition state still require authoritative replication.

References:

- [Motion Warping](https://dev.epicgames.com/documentation/unreal-engine/motion-warping-in-unreal-engine)
- [`FMotionWarpingTarget`](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/MotionWarping/FMotionWarpingTarget)
- [Animation Montages](https://dev.epicgames.com/documentation/unreal-engine/animation-montage-in-unreal-engine)

### Authoring, diagnostics, and performance evidence

Unreal's Data Validation system supports project-specific C++, Blueprint, and Python validators; validation from an asset, dependency set, folder, entire project, asset save, and command line; and use cases such as conventions, performance budgets, and dependency cycles. That makes editor-visible authoring checks and CI validation credible package requirements for Mobility assets. It does not decide which surfaces, anchors, portals, or transition recipes OWS will require.

Unreal also provides complementary evidence tools rather than one universal Mobility log:

- Visual Logger can correlate actor snapshots, categorized messages, and world-space debug shapes for relationship, target, reservation, and failure decisions.
- Rewind Debugger can review animation, montage, notify, and movement history.
- Chaos Visual Debugger can record and inspect physics particles, contacts, constraints, and network/physics timing.
- Unreal Insights and Networking Insights can measure CPU, memory, packet, replicated-property, and RPC cost.
- Network Emulation can impose lag, jitter, loss, reordering, and duplication for correction and recovery tests.

Temporal claims need correlated server, owner, observer, physics, and animation evidence; a screenshot of one endpoint cannot establish authority or synchronization. Decisions 42–43 and the accepted contract define the trace fields, correlation, privacy, and Shipping boundaries. Stage 2 already requires diagnostics to remain local and privacy-preserving unless a developer explicitly integrates and obtains consent for external transmission; Shipping diagnostics are disabled by default.

References:

- [Unreal Data Validation](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-validation-in-unreal-engine)
- [Visual Logger](https://dev.epicgames.com/documentation/unreal-engine/visual-logger-in-unreal-engine)
- [Rewind Debugger](https://dev.epicgames.com/documentation/unreal-engine/animation-rewind-debugger-in-unreal-engine)
- [Chaos Visual Debugger](https://dev.epicgames.com/documentation/unreal-engine/getting-started-with-chaos-visual-debugger)
- [Unreal Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-in-unreal-engine)
- [Networking Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-insights-in-unreal-engine)
- [Network Emulation](https://dev.epicgames.com/documentation/unreal-engine/using-network-emulation-in-unreal-engine)

### Input and accessibility evidence

Microsoft's Xbox Accessibility Guideline 107 recommends remapping semantic actions rather than only physical controls, alternatives to prolonged holds, repeated presses, or simultaneous inputs, digital alternatives where analog input is required, and prompts that reflect remapping. Guideline 116 recommends alternatives or adequate time for timing-dependent interactions when timing is not essential to the experience.

These are design guardrails, not a selected OWS control scheme. They establish that boarding, exit, grip, detach, seat change, and transfer cannot be architected as permanently hardwired buttons or an unchangeable hold/timing gesture. OWS's accepted Stage 1 semantic-input contract already points in the same direction; the current local raw-key polling is implementation evidence, not the target contract.

References:

- [Xbox Accessibility Guideline 107: Input](https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/107)
- [Xbox Accessibility Guideline 116: Time limits](https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/116)

## Comparable professional gameplay evidence

Comparable games do not provide one complete blueprint for OWS, but they establish player expectations that the architecture must be able to express:

- **Battlefield** exposes finite free seats, explicit seat selection and seat-change actions, multiplayer deployment into another player's available seat, directional exits, and exterior riding on supported vehicles. This supports separating occupancy, seat identity, control role, exterior relationship, and dismount selection. [Battlefield vehicle guide](https://help.ea.com/en/articles/battlefield/battlefield-6/vehicles-guide/) and [Battlefield 2042 controls](https://www.ea.com/able/resources/battlefield-2042/pc/mouse-keyboard-edit-controls)
- **GTA Online** has explicitly allowed passengers in a personal vehicle while reserving the driver seat to its owner. This supports treating occupancy, permission, and vehicle control as separate concepts. [GTA Online title update 1.10](https://support.rockstargames.com/articles/5zj42eWi3wpl0Txb7Mabyb/gtav-title-update-1-10-notes)
- **Just Cause 2** explicitly markets stunt jumping on cars, while the series broadly markets vehicle-enabled traversal freedom. Exterior vehicle movement is a recognizable sandbox capability rather than an engine edge case. [Just Cause 2](https://avalanchestudios.com/games/just-cause-2) and [Just Cause 3](https://avalanchestudios.com/games/just-cause-3)
- **Uncharted 2** required dedicated dynamic traversal technology for continuously moving train cars. This is evidence that reliable moving-carrier traversal is a first-class system rather than ordinary attachment. [Naughty Dog postmortem](https://media.gdcvault.com/GD_Mag_Archives/GDM_March_2010.pdf)
- **Fortnite's vehicle/seat APIs and chair device** expose seat identity, driver-seat distinction, passenger enumeration, exit enablement, interaction timing, directional dismount, and seat/eject events. This supports data-authored seats and transitions rather than one hardwired driver possession path. [Fortnite ecosystem API update](https://dev.epicgames.com/documentation/fortnite/41-00-fortnite-ecosystem-updates-and-release-notes) and [chair device](https://dev.epicgames.com/documentation/fortnite/using-chair-devices-in-fortnite-creative)
- **Sea of Thieves** presented automated multiplayer validation across the initiating client, server, and observing client. Mobility acceptance similarly needs synchronized authority and observer evidence. [Automated testing of multiplayer games](https://media.gdcvault.com/gdc2019/presentations/Masella_Robert_AutomatedTestingOf.pdf)

These examples inform capability and test design. They do not dictate OWS's tuning, controls, assists, animations, or release slice.

## Commercial starter-package benchmark

Current Fab listings for the two commercial benchmarks Aurora named advertise more than raw feature count:

- **Action-Adventure Movement System (AAMS V2)** presents one connected gameplay foundation with movement, traversal, environmental interaction, multiplayer, replaceable animation data, Motion Matching and State Machine examples, modular Actor-component integration, own-character onboarding, a playable demo, documentation, and tutorials.
- **R-Tune Vehicle Physics 2.0 Pro** presents a custom C++ vehicle foundation with rapid static-mesh setup, broad vehicle classes, exposed core parameters, procedural animation, multiplayer, frame-rate consistency, performance/network demonstrations, documentation, and backward-compatible upgrades with new features disabled by default.

These are vendor claims on Epic's marketplace, not independent verification. The useful product inference is that OWS must compete on **coherence, onboarding, customization, evidence, documentation, and upgrade safety** as well as capabilities. A long feature list cannot compensate for systems that require users to repair their integration, edit fork internals for normal extension, or discover multiplayer and packaging limits after adoption.

References:

- [Action-Adventure Movement System](https://www.fab.com/listings/dd774925-1d69-4695-8599-35a31bc3558e?lang=en)
- [R-Tune Vehicle Physics 2.0 Pro](https://www.fab.com/listings/90911f6b-d960-4138-aa4d-283058895980?lang=en)

## Research-supported constraints consumed by the closed interview

The following are evidence constraints or already accepted charter requirements, not newly selected gameplay:

1. CMC/current vehicles remain the reference baseline unless Aurora later accepts a measured migration. Experimental Mover, ChaosMover, or Modular Vehicles may be benchmarked without becoming mandatory.
2. Supported vehicle surfaces and anchors require stable scoped identities and exact body/component/bone kinematics.
3. Occupancy, permission, control, embodiment, presentation, physical support, grip, and transition phase cannot be represented as one overloaded possession flag.
4. Moving eligibility must use motion relative to the exact target point plus approach, gap, clearance, capability, and temporal evidence.
5. Balance and grip must combine physics-derived loads with Character capability and accepted assists; neither pure rigid-body friction nor magical attachment is sufficient.
6. Release must preserve actual pre-release world motion and avoid double application of base velocity or impulses.
7. Presentation alignment cannot authorize a transition or substitute for collision and authority validation.
8. Durable relationships replicate as state. Requests and transient cues do not replace late-joinable state.
9. Existing stopped entry/exit and controlled bailout remain protected until replacement scenarios pass.
10. No carrier, transition, network mode, or performance envelope is supported merely because an engine primitive or asset exists.

## Already accepted inputs—not interview questions

The Stage 1 platform contract, Stage 2 gameplay-spine contract, and Mobility charter already settle the following boundaries. The interview may refine their implementation, but it must not silently reopen them:

1. OWS ships as one self-contained, coherent starter package. GASPALS and KinetiForge are complete maintained forks evolved in place into the OWS Character and Vehicle domains; users do not install upstream originals, and the standard implementations do not receive permanent wrapper plugins.
2. Character owns bodily locomotion, stance, balance/grip capability, traversal, animation, fall, roll, and recovery. Vehicle owns simulation, exact point kinematics, seats/control roles, doors/portals, exterior zones/anchors, stable scoped identities, and Vehicle authoring. The narrow Integration family owns relationships, transitions, reservations, and cross-domain orchestration.
3. Character and Vehicle each build without Integration. Integration is acyclic and separately buildable when both supported public contracts are present; it is not independent of those dependencies.
4. Player identity, Character embodiment, occupancy, presentation/camera focus, and permission to control a vehicle are separate concepts. Passenger support cannot require every occupant to possess the Vehicle.
5. Client intent, server validation/commit, stable scoped IDs, revisions/idempotency, atomic discrete cross-owner mutation, durable late-joinable state, and physically coherent non-rollback failure after launch are Stage 2 requirements.
6. Every advertised networked Mobility feature must pass equivalent evidence under both legacy replication and opt-in Iris. Accepted decision 4 includes networking in the first slice, so neither backend can be claimed without passing.
7. Diagnostics are local and privacy-preserving by default, external transmission requires an explicit developer integration and consent, and diagnostic presentation is disabled in Shipping by default.
8. Existing verified entry, stopped exit, bailout, roll, recovery, and re-entry behavior remains protected until replacement behavior passes its declared matrix.
9. No gameplay implementation is authorized by #147. Accepted architecture must first produce separately scoped, ready, claimed implementation work.

References: [platform architecture](OWS_PLATFORM_ARCHITECTURE.md), [gameplay-spine architecture](OWS_GAMEPLAY_SPINE_ARCHITECTURE.md), and [Mobility program charter](OWS_MOBILITY_PROGRAM_CHARTER.md).

## Interview method (completed)

The interview settled the release envelope first, then authoring, physical behavior, actions/transitions, presentation/accessibility, authority/recovery, and finally AI/extension/diagnostics. The final pass converts every accepted answer into the canonical relationship model, public contracts, support profiles, acceptance matrix, and tightly scoped backlog. This order prevented a low-level threshold or animation answer from silently deciding the product slice that should have constrained it.

## Accepted interview decisions

1. **First supported carrier slice — accepted 2026-08-25.** Support every current OWS road vehicle, including rigid cars/SUVs and the articulated bus. The architecture remains capable of representing boats, trains, aircraft, and custom carriers, but OWS does not advertise those carrier classes until their separate evidence matrices pass.
2. **First supported capability slice — accepted 2026-08-25.** Deliver the complete same-vehicle loop across the supported road-vehicle slice: exterior riding and standing, crouching, balance, grip, sliding or falling, stopped and moving boarding, passengers, seat switching and driver handoff, exterior-to-interior infiltration, exit, bailout, and recovery. Vehicle-to-vehicle transfer remains representable in the architecture but follows as a later implementation slice.
3. **Mobility feel profiles — accepted 2026-08-25.** Ship data-driven Balanced, Cinematic, and Simulation profiles. Balanced is the physics-grounded default; Cinematic applies stronger assists; Simulation minimizes assists. Accessibility settings are orthogonal to the feel profile so a player can change hold/toggle, timing, or other input assistance without changing the selected physical-behavior profile.
4. **First-slice multiplayer commitment — accepted 2026-08-25.** The first supported Mobility slice includes standalone, listen-server host, remote listen client, dedicated server, and remote dedicated clients. Every advertised networked Mobility feature must pass equivalent evidence under both legacy replication and opt-in Iris before OWS advertises that support.
5. **First-slice movement and vehicle foundations — accepted 2026-08-25.** Retain OWS Character's current CMC foundation and OWS Vehicle's maintained current foundation for the first supported slice. Experimental Mover, ChaosMover, and alternative Chaos Vehicle backends may be benchmarked separately without blocking the slice. Migration requires measured benefit, compatibility evidence, and Aurora's explicit approval; feature presence alone cannot trigger replacement.
6. **First-slice platform envelope — accepted 2026-08-25.** Certify Windows PC clients plus Windows and Linux dedicated servers first. Keep the architecture portable to licensed current-generation consoles, but advertise console support only after platform-hardware conformance passes. macOS, Linux desktop clients, mobile, and VR remain representable but deferred from the first support claim.
7. **First-slice scale and frame-rate envelope — accepted 2026-08-25.** Certify sessions with 32 players, 32 simultaneously active and relevant vehicles, and 16 concurrent Mobility relationships concentrated in one relevancy hotspot or large vehicle. Target 60 FPS on a published midrange reference PC and require behavioral parity at 30, 60, and 120 FPS; the exact reference hardware and per-system budgets must be published with benchmark evidence.
8. **First-slice network-impairment envelope — accepted 2026-08-25.** Supported play must pass at 150 ms round-trip latency, 30 ms jitter, and 2% packet loss. Recovery stress must pass at 250 ms round-trip latency, 60 ms jitter, and 5% packet loss, plus packet-reordering and duplication cases. Presentation may degrade under the recovery-stress profile, but authoritative Mobility state may not corrupt, duplicate, or become unrecoverable.
9. **Vehicle-surface eligibility — accepted 2026-08-25.** Use an authored-policy hybrid. Valid physical Vehicle surfaces may provide ordinary standing and walking support. Explicit include and exclude zones override automatic eligibility. Precise doors, grips, portals, boarding, infiltration, and transfer actions always require authored identities and transition data.
10. **Mobility-graph authoring — accepted 2026-08-25.** The Editor generates a draft graph from Vehicle collision, bodies, doors, and seats. A developer reviews and accepts it before OWS treats the graph as supported authoring. Regeneration is non-destructive, preserves manual overrides and stable identities, and presents a clear diff. Fully manual authoring remains available.
11. **Mobility-point attachment contract — accepted 2026-08-25.** One common scoped Mobility-point contract supports rigid bodies, skeletal bones, articulated components, and project-defined kinematic providers through a supported extension interface. Actor-root-only targeting is insufficient. Every accepted point exposes stable identity and exact world-space point kinematics through the Vehicle public contract.
12. **Mobility authoring safeguards — accepted 2026-08-25.** Provide live Editor visualization for zones, anchors, edges, clearance, identities, and validation failures. Validate on save and in CI. Invalid required Mobility data blocks release validation and cooking with actionable repair guidance; an invalid optional edge disables deterministically rather than triggering a guessed transition. Schema migrations are versioned and non-destructive and preserve stable identities.
13. **Passive balance and crouch — accepted 2026-08-25.** Automatically balance within the Character capability and selected profile's recoverable envelope. Limited movement input can counterbalance. Crouching changes support geometry, projected area, and capability through Character/pose-specific data rather than a universal multiplier. Exceeding balance capacity progresses into bracing or sliding instead of instant detachment. Cinematic, Balanced, and Simulation profiles scale assistance without changing the underlying measured loads.
14. **Active-grip input — accepted 2026-08-25.** Provide a dedicated semantic Grip action. Hold is the default interaction, with toggle and automatic-grip accessibility alternatives. Activate may initiate a contextual boarding or infiltration transition, but it never maintains continuous grip. Projects may rebind both semantic actions through the shared Input contract.
15. **Stamina integration — accepted 2026-08-25.** Provide stamina as an optional Character capability provider that is included but disabled by default. Balance and grip remain fully functional without it. When enabled, stamina drain scales with the authoritative grip load and selected policy or feel profile rather than using a fixed timer.
16. **Edge-catching policy — accepted 2026-08-25.** Edge catch is capability-driven and input-assisted. It requires an authored reachable edge, valid hand reach and clearance, and a relative load within the Character's survivable capability. Grip is required by default; automatic catch is an accessibility option. Feel profiles may adjust tolerances but never bypass physical or authored validity.
17. **Mobility failure progression — accepted 2026-08-25.** Use a branching, measurement-driven state graph rather than a mandatory linear animation chain. Supported motion progresses from stable to braced to sliding as balance capacity is exceeded. A valid Grip opportunity can branch into caught or hanging; failed support or grip causes release and airborne motion. Ground contact resolves into a controlled roll or tumble according to contact energy, followed by optional damage and recovery. Regaining valid support may return through braced to stable. Physical evidence, never an animation timer, authorizes each transition.
18. **Character–Vehicle physical coupling — accepted 2026-08-25.** Mobility relationships use real, server-authoritative two-way coupling. Rider weight, movement, jumps, impacts, and Grip apply equal-and-opposite forces at their actual contact points. One Mobility coupling solver owns those forces so collision and gameplay code cannot double-apply them. Relationship-aware filtering suppresses only Character/carrier contacts that would duplicate or fight the acknowledged relationship; collisions with the world and unrelated bodies remain active.
19. **Mobility interruption policy — accepted 2026-08-25.** Treat collision, crushing, destruction, environmental hazards, and world bounds as typed, server-authoritative interruptions. Collision loads enter the normal measured failure graph. Crushing or destruction invalidates only affected surfaces and anchors while surviving authored points remain usable; any release preserves exact point velocity. Optional Damage and Environment capability providers determine injury, fire, and water consequences rather than hardcoded detachment. World-bounds recovery is a last resort that returns the Character to the latest validated safe anchor.
20. **Stopped and moving boarding actions — accepted 2026-08-25.** For a stopped Vehicle, Activate reserves a valid seat and portal and automates the authored approach, alignment, and entry; Cancel may abort before commitment. For a moving Vehicle, the player must physically approach, speed-match, and time Activate. Grip maintains an established catch, after which OWS performs only validated authored climbing and entry transitions. Boarding never teleports a Character from outside valid reach or grants artificial reach extension.
21. **Boarding rejection and physical commitment — accepted 2026-08-25.** Before launch, the server rejects an attempt when no valid route or reservation exists, no reachable intercept is predicted, or the predicted contact load already exceeds Character capability. Physical commitment begins when the Character leaves secure support or establishes contact. Subsequent changes in relative speed, gap, acceleration, approach, or time to contact resolve as a catch, slide, miss, or fall without rollback or teleportation. All validation uses exact relative point kinematics rather than Vehicle speed bands.
22. **Seat selection, switching, and handoff — accepted 2026-08-25.** All seat operations are server-authoritative transactions over stable Seat IDs. An explicit valid seat request takes precedence; otherwise a deterministic policy selects the best compatible available seat reachable through the chosen portal. A destination is reserved before the current seat is surrendered, and switching follows authored seat-to-seat paths. Driver authority transfers only when the handoff commits. Failure preserves the original occupant and control assignment. Cancel, timeout, disconnect, death, or destruction releases affected reservations.
23. **Seated Character embodiment — accepted 2026-08-25.** The default is a hybrid embodiment: the same Character actor remains authoritative, visible, targetable, and damageable, while independent locomotion and capsule physics that would fight the Vehicle are suspended. Authored seated hit and query volumes follow the seat and Vehicle body. Full Character collision returns only at a validated exit. Projects may replace this policy, including with a presentation-only embodiment, but presentation-only is not the OWS default.
24. **Vehicle-interior traversal — accepted 2026-08-25.** Every Vehicle uses the seat and portal graph. Authored interior regions additionally allow free movement through the same moving-support system when their geometry provides valid clearance for the Character's current supported pose, including standing, crouching, or crawling. Eligibility is geometry- and pose-based rather than inferred from Vehicle class or size. Motorcycles and ATVs use seats and exterior anchors unless a project authors valid interior space; cars and hatchbacks may expose any valid cabin or cargo space. The articulated bus is the first required free-interior validation Vehicle.
25. **First required Mobility paths — accepted 2026-08-25.** The first supported slice ships same-Vehicle paths from ground through a portal to an interior or seat; between an exterior surface, Grip or hang point, portal, and interior; between an interior and seat; between seats; and from any occupied relationship to a valid exit, bailout, or recovery. Cross-Vehicle edges must be representable, authorable, and validatable in the architecture now, but their certification remains a later implementation slice.
26. **Interior-initiated cross-Vehicle transfer — accepted 2026-08-25.** The architecture allows a Character to initiate a transfer from a seat or freely traversable interior. Its validated route must physically pass through an authored source portal or exterior contact and into a valid target-Vehicle point; direct interior-to-interior teleportation is forbidden. A continuous valid physical transition does not require the Character to pause at an intermediate exterior anchor.
27. **Mobility animation recipes — accepted 2026-08-25.** Every enabled authored transition requires a context-correct presentation recipe. OWS ships bespoke recipes for boarding and exit, seat changes and handoff, catches, hangs, climbs, moving entry, bailout and roll, and recovery; ordinary supported locomotion continues to use its locomotion system. If a recipe is missing, physical state remains authoritative and OWS never substitutes an unrelated animation. Validation reports the missing recipe and blocks certified shipping until the recipe is supplied or the affected transition is disabled.
28. **Motion-Warping provider boundary — accepted 2026-08-25.** Motion Warping is OWS's default transition-alignment provider behind a replaceable Integration adapter. Mobility supplies the validated path, targets, and timing; authoritative relationship state never depends on Motion Warping itself. A project may substitute Control Rig, custom root motion, or another provider without rewriting Mobility domain logic.
29. **Mobility camera policy — accepted 2026-08-25.** Every Character exposes one replaceable Character-owned camera capability and context policy. A locally controlled player activates its view; NPC presets may leave local-view presentation inactive without creating a second Character path. The policy preserves the current view and blends between contexts rather than forcing unexplained cuts. Exterior riding filters unwanted Vehicle vibration. Grip, boarding, and transfer frame the Character and relevant contact path. Failed catches, falls, rolls, and recovery continuously follow the authoritative Character without snap-back. Network corrections smooth presentation; hard snaps are reserved for last-resort recovery. Camera shake, roll, horizon lock, and field-of-view effects are configurable.
30. **Mobility control preservation — derived 2026-08-25 from accepted physical-state and camera contracts.** Player control is capability- and physics-limited rather than arbitrarily locked. Slide, leap, airborne, and roll states retain the steering permitted by available traction and momentum. Grip and hang accept body-shift and directional-reach input. Recovery returns control continuously as support is regained, and held movement flows into locomotion. Aim remains available only when pose, stability, equipment, and free-hand requirements permit. Camera look remains available throughout.
31. **Player/NPC Character parity — accepted 2026-08-25.** OWS treats player-controlled and NPC-controlled Characters as the same Mobility participant. A control-source flag and policy select who supplies semantic actions; controls, animations, capabilities, physics, graph, requests, authority, validation, and supported outcomes remain identical. Features unnecessary for a particular NPC are individually developer-configurable rather than removed from the NPC path. OWS supplies recommended NPC presets, while projects may enable or disable those options without creating a second Mobility implementation.
32. **Semantic Mobility actions and accessibility — derived 2026-08-25.** Every intent that starts, sustains, changes, or ends a Mobility relationship is device-independent and uses the same Shared semantic Action contract; runtime systems never poll physical controls. Player device intent enters through rebindable OWS Input, while NPC and server control sources submit the same semantic Actions without pretending to own a device. Activate requests the context-selected authored path. Cancel terminates a pre-commit Action; established relationships end through governed Exit, Bailout, or Release Actions. The default player-facing Cancel binding may route one of those contextual Actions without redefining it as cancellation. Grip establishment and release are discrete Actions; maintained Grip and body shift are continuous control, with hold as default and mandatory toggle and automatic alternatives. Explicit seat or target requests carry stable scoped IDs. Character Move, Look, Crouch, Jump or Leap, and Aim remain Character-owned; driving controls remain Vehicle-owned. Accessibility may buffer or generate intent at a physically valid opportunity and provide alternatives to prolonged, repeated, simultaneous, or analog-only input, but it never invents reach, bypasses clearance, load, capability, reservation, or authority, teleports, or changes the selected feel profile. Player prompts always reflect the active binding.
33. **Character, Vehicle, and Integration execution ownership — derived 2026-08-25.** Character owns bodily locomotion, pose and stance geometry, Character capabilities, falling, landing, roll or tumble, recovery, Character-side prediction, body-animation execution, and the Character camera provider. Vehicle owns simulation and collision bodies, exact point kinematics, authored Mobility geometry and stable IDs, capacity and compatibility, canonical occupancy and control-grant records, driving controls, and Vehicle camera data; it never moves or animates a Character. Integration owns only the relationship: target and path validation, reservations, dynamic support, exterior riding, Grip, boarding, infiltration, seat and control handoff orchestration, exit, bailout, transfer, cross-domain recovery, relationship-specific collision filtering, and the single coupling solver. Integration calculates relationship loads and deduplicates coupling while Character executes bodily motion and Vehicle applies accepted simulation forces through their public contracts. Integration selects a validated presentation recipe and supplies its path, target, and timing through replaceable adapters; Character executes body animation, Vehicle retains structural animation, OWS Input owns player-device routing and contexts, and Shared Gameplay owns semantic Action lifecycle and failure semantics for every control source.
34. **Continuous prediction and proxy presentation — derived 2026-08-25.** The server owns every canonical Mobility relationship and transition. The owning participant predicts continuous Character motion only inside an already authorized relationship, including support-relative locomotion, balance, bracing, sliding, Grip body shift, leap, fall, roll, and recovery; every prediction carries the relationship ID and revision. Reservations, occupancy, control grants, damage, destruction, and relationship changes never commit early. Simulated proxies interpolate authoritative support-relative pose, velocity, and phase. Legacy replication and Iris consume the same backend-neutral state schema and must produce equivalent evidence.
35. **Discrete-transition anticipation — derived 2026-08-25.** Before server acceptance, an owning client may begin disposable camera, user-interface, audio, and animation anticipation that does not move the collision body, surrender support, reserve or occupy a point, or alter control. Every request is reliable and idempotent and carries a request ID, expected revisions, stable source and target IDs, and source-typed time: a mapped client timestamp for authenticated client intent or authoritative server time for NPC/server intent. Acceptance returns the committed transition identity and revision, authoritative start time, recipe, and exact source and target state. Rejection cancels anticipation smoothly and returns a governed semantic reason.
36. **Time-consistent validation — derived 2026-08-25.** The server maintains synchronized authoritative history for participating Characters and every relevant Vehicle body, bone, articulated component, and kinematic provider long enough to validate the accepted network-impairment envelope. Character and target are rewound to the same mapped instant and interpolated between authoritative samples; exact point kinematics are calculated at that instant. Current occupancy, reservation, permission, and revision are rechecked at commitment, so rewind may prove physical reach but never resurrect stale availability. Out-of-window requests are rejected rather than silently clamped, and terminal request results remain cached long enough to make delayed or duplicated delivery idempotent. History, skew, and retention limits are data-driven, published, and established by evidence rather than guessed constants.
37. **Reservation leases — derived 2026-08-25.** Portal, anchor, edge, and seat reservations are server-owned monotonic leases. A lease covers the authored maximum transition duration plus a validated network-safety margin and renews only while authoritative phase progress occurs, never from a client heartbeat alone. Cancel, rejection, timeout, disconnect, death, target invalidation, revision change, destruction, or pre-commit reachability loss releases it immediately. After physical commitment, it lasts only until the catch, miss, or arrival terminal result. Committed occupancy is durable relationship state, not a renewable reservation.
38. **Deterministic contention — derived 2026-08-25.** Existing committed occupancy and active valid reservations cannot be preempted. New valid contenders are ordered atomically by server-normalized intent time within the accepted rewind window, then server receive sequence, then stable request ID. An explicit target failure never silently substitutes another target; an automatic-selection request may choose the next deterministic valid alternative. Losers receive the conflict reason, current revision, retryability, and valid alternatives. Projects may install a deterministic authoritative role-priority policy, but it cannot override a committed relationship.
39. **Mobility correction policy — derived 2026-08-25.** Disagreement is measured in the authoritative support or target frame. Small errors use normal movement prediction and smoothing. Larger but clearance-valid errors keep collision authoritative while mesh and camera presentation converge. A clearance-invalid or physically material disagreement enters the typed physical failure or recovery graph, preserving exact point velocity plus Character-relative velocity and applying any new impulse once. Thresholds are normalized to Character scale, configurable within certified ranges, and established by cross-frame-rate and impairment evidence. OWS never moves a Character to the camera, a stale Vehicle transform, or a past predicted location. Hard relocation is limited to invalid numeric state, world bounds, or unrecoverable penetration and uses the latest server-validated safe anchor.
40. **Supported Mobility extension surface — derived 2026-08-25.** OWS governs canonical Mobility actions, lifecycle, failure, state, relationship, and capability semantics while projects may register documented descendant tags and typed context without reinterpreting core tags. Supported extensions are Character capability providers; Vehicle point, graph, and exact-kinematics providers; Integration routing and assist-selection policies; Character-owned animation, Motion-Warping, camera, and embodiment adapters selected from relationship context; validated data assets and profiles; and explicitly aggregate diagnostic listeners. Authoritative contracts permit exactly one active provider, optional contracts permit zero or one, and only aggregate contracts permit multiple consumers. Editor, cook, and startup validation reject incompatible ownership. No extension may bypass server authority, revisions, idempotency, atomic reservations, exact point kinematics, physical validity, or the single coupling solver.
41. **Assist configuration ownership — derived 2026-08-25.** Developers define enabled capabilities, valid physical and authored envelopes, supported ranges, project defaults, and available presets. Players select exposed accessibility behavior within those ranges. OWS ships conformant defaults for hold, toggle, and automatic Grip, optional automatic edge catch, timing assistance, binding-aware prompts, and configurable camera motion. A project that removes an accepted alternative must provide an equivalent accessible path or relinquish that conformance claim. Accessibility remains orthogonal to the selected Cinematic, Balanced, or Simulation feel profile.
42. **Correlated Mobility diagnostics — derived 2026-08-25.** One structured schema correlates request and relationship IDs, revision, authority role, shared network and physics time, stable scoped participant and graph identities, state before and after each decision, route and clearance evidence, reservation and contention results, exact point and relative kinematics, release velocity and impulses, contacts and support geometry, balance and Grip budgets, coupling ownership, prediction history, correction reason, and multi-endpoint agreement. Editor graph visualization, local runtime overlays, Visual Logger, Unreal and Networking Insights, Rewind Debugger, Chaos Visual Debugger, and machine-readable multiplayer test artifacts are adapters over that schema rather than independent truths. Filters and governed reason codes make every transition and rejection inspectable without requiring per-frame text spam.
43. **Diagnostics privacy and Shipping policy — derived 2026-08-25.** Editor overlays, test fixtures, and diagnostic presentation remain outside ordinary Shipping builds. Shipping retains only a dormant structured-sink interface and produces no overlay, file, trace, network request, or external transmission by default. Explicit Shipping enablement requires an approved product and build configuration, bounded sampling and retention, rate and performance budgets, stable redacted identifiers, and exclusion of secrets and external account IDs. External transmission additionally requires a developer-supplied integration and applicable player consent; OWS supplies no default endpoint.
44. **Mobility failure-state convergence — derived 2026-08-25.** Relationship, support, transition, control source, embodiment, and presentation are orthogonal state fields rather than one combined enum. Late join reconstructs the current state and phase from an authoritative snapshot without replaying old transient events. Disconnect releases pending reservations and control immediately while the same server-simulated Character and valid physical relationship remain; project policy selects neutral input, NPC control, reconnect grace, or a governed termination. Active related participants remain co-relevant and cannot independently dorm; client relevance loss suspends only presentation. Destruction follows the accepted affected-point invalidation rule. Owner or possession loss revokes control without silently changing occupancy or embodiment. Interrupted presentation cancels before physical commitment and otherwise leaves authoritative physics to complete or enter a valid failure outcome. A replication-backend failure fails closed to new transitions, preserves canonical state when possible, and invokes typed server recovery otherwise; legacy replication and Iris converge on the same result.
45. **Mobility persistence lifecycle — accepted 2026-08-25.** Death and respawn plus non-seamless map travel terminate Mobility relationships. Seamless travel and World Partition preserve stable relationships only when every participant and stable graph identity migrates and revalidates; related active participants co-stream. Save, load, and checkpoints serialize stable relationships only. In-progress transitions, reservations, and prediction never serialize and instead resolve to an authored safe checkpoint state. Failed restoration uses the latest server-validated safe anchor.

## Interview closure

Aurora closed every consequential choice in this audit on 2026-08-25. Decisions 1–45 above form the traceable decision record consumed by the canonical Mobility architecture and matrix. Any later change to an accepted product rule requires an explicit, separately recorded decision; implementation tuning supported by this architecture requires measured evidence in its owning issue rather than another product interview.


## Current-source evidence map

The audit's central current-runtime claims can be rechecked at these source locations. Line anchors describe the audited baseline commit and will naturally move as later implementation changes.

| Evidence | Source |
| --- | --- |
| Selector precision ray, actor hit, target choice, Activate dispatch, and raw-key polling | [`OWSSelectorComponent.cpp`](../Source/OWS/Private/OWSSelectorComponent.cpp#L387), [`OWSSelectorComponent.cpp`](../Source/OWS/Private/OWSSelectorComponent.cpp#L421), [`OWSSelectorComponent.cpp`](../Source/OWS/Private/OWSSelectorComponent.cpp#L479), [`OWSSelectorComponent.cpp`](../Source/OWS/Private/OWSSelectorComponent.cpp#L517) |
| Interaction-target routing | [`OWSInteractionTargetComponent.cpp`](../Source/OWS/Private/OWSInteractionTargetComponent.cpp#L29) |
| Seat and door authoring definitions plus currently unvalidated IDs | [`OWSStockVehicleInteractionComponent.h`](../Source/OWS/Public/OWSStockVehicleInteractionComponent.h#L16), [`OWSStockVehicleInteractionComponent.h`](../Source/OWS/Public/OWSStockVehicleInteractionComponent.h#L37), [`OWSStockVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSStockVehicleInteractionComponent.cpp#L721) |
| Runtime door-target creation | [`OWSStockVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSStockVehicleInteractionComponent.cpp#L74), [`OWSStockVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSStockVehicleInteractionComponent.cpp#L132) |
| Authority-gated replicated occupancy | [`OWSStockVehicleInteractionComponent.h`](../Source/OWS/Public/OWSStockVehicleInteractionComponent.h#L145), [`OWSStockVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSStockVehicleInteractionComponent.cpp#L298) |
| Exact-door/control-seat entry and possession handoff | [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L316), [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L369), [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L398) |
| Stopped-exit support, non-vehicle blocker clearance, and another-door fallback | [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L439), [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L1239), [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L1318) |
| Bailout energy, steering, roll presentation, and recovery | [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L578), [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L706), [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L807), [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L926) |
| Driver removal zeroing/sleeping Vehicle bodies | [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L534), [`OWSStockVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSStockVehicleInteractionComponent.cpp#L507) |
| Root-primitive linear-velocity query with Pawn fallback | [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L1563) |
| High-speed-only re-entry block and recovery-time clearing | [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L550), [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp#L989) |
| Implicit controller-owned state and absence of a public relationship model | [`OWSVehicleInteractionComponent.h`](../Source/OWS/Public/OWSVehicleInteractionComponent.h#L126) |
| Current selector and Character/Vehicle test definitions, including debug entry/exit APIs | [`OWSSelectorTests.cpp`](../Source/OWS/Private/Tests/OWSSelectorTests.cpp#L265), [`OWSVehicleInteractionComponent.h`](../Source/OWS/Public/OWSVehicleInteractionComponent.h#L33), [`OWSCharacterVehicleFunctionalTests.cpp`](../Source/OWS/Private/Tests/OWSCharacterVehicleFunctionalTests.cpp#L835), [`OWSCharacterVehicleFunctionalTests.cpp`](../Source/OWS/Private/Tests/OWSCharacterVehicleFunctionalTests.cpp#L1081) |
| Inherited KinetiForge RPC/replication declarations and lifetime registration | [`VehicleDriveAssemblyComponent.h`](../Plugins/KinetiForge/Source/KinetiForge/Public/VehicleDriveAssemblyComponent.h#L104), [`VehicleDriveAssemblyComponent.cpp`](../Plugins/KinetiForge/Source/KinetiForge/Private/VehicleDriveAssemblyComponent.cpp#L124) |

## Verification gaps

No new Unreal session or multiplayer run was performed for this evidence audit. The automation described above was inspected as source but was not executed in this audit. Binary Blueprint graph internals, vehicle movement replication flags, exact per-instance surface collision, and animation-state behavior still require runtime/asset inspection before they can become support evidence.

The current Character/Vehicle functional tests call debug entry/exit methods rather than exercising the real Activate/Cancel inputs or two-second hold. Bailout definitions use the default DriftCar and injected representative speeds. They do not assert exact 5/15-mph boundaries, continuous ground contact, quantitative energy/contact behavior, or telemetry accuracy.

Current automation does not cover:

- the full ordinary locomotion and traversal surface;
- experimental locomotion behavior;
- end-to-end semantic input, remapping, prompts, and hold/toggle alternatives;
- moving supports or exterior riding;
- passengers or seat switching;
- moving entry, infiltration, or vehicle-to-vehicle transfer;
- authoritative remote-client entry/exit/bailout;
- contention, simulated proxies, correction, late join, disconnect, destruction, relevance, or dedicated server behavior; or
- lag, jitter, packet loss, and cross-system prediction correction.

Those are acceptance and future implementation gaps, not permission to broaden a child issue while working on another scope.

## Reconciled implementation backlog

The live issue graph is reconciled in [OWS Mobility Implementation Backlog](OWS_MOBILITY_IMPLEMENTATION_BACKLOG.md):

- Character public contract and independent conformance remain in [#107](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/107), with Mobility implementation children #150–#153;
- Vehicle public contract and independent conformance remain in [#108](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/108), with Mobility implementation children #154–#156;
- collision, stability, slide, grip load, impact, and detachment: [#21](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/21);
- driving and point-kinematic evidence: [#41](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/41);
- Vehicle driving tuning in [#48](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/48), Vehicle Mobility graph runtime/editor work in #155/#156, and Integration route/recipe resolution in #160;
- [#120](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/120) coordinates same-Vehicle Integration children #157–#167, while later cross-Vehicle runtime work is isolated in #168;
- shared authority consumer [#27](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/27) and verification [#28](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/28) no longer duplicate #120 relationship ownership;
- #111 remains the Stage 3 coordination gate and does not open Stage 4 without Aurora's separate decision;
- #167 owns the dedicated Mobility conformance implementation suite consuming the Stage 2 harness; and
- separately scoped defects for every failed scenario.

Aurora explicitly accepted the assembled artifacts and reconciled backlog on 2026-08-25. None of the implementation work above becomes ready through that acceptance alone.

Organization-URL cleanup remains owned by separate issue [#149](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/149) and is intentionally not mixed into #147. No implementation child becomes ready merely because this evidence baseline exists.
