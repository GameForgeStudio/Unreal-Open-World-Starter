# OWS Mobility Architecture and Conformance Contract

> **Status:** Accepted Stage 3 target architecture for [issue #147](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/147), explicitly accepted by Aurora on 2026-08-25 with its conformance matrix and reconciled backlog. This contract defines future OWS behavior and ownership. It does not claim that the target is implemented, does not authorize gameplay work, and does not make any architecture-gated child ready for checkout.

## Purpose and controlling sources

OWS Mobility makes Character locomotion, Vehicle simulation, and movement onto, across, within, and between Vehicles behave as one coherent product without merging Character and Vehicle into one runtime domain.

This contract consumes and does not reopen:

- the [OWS Platform Architecture and Composition Contract](OWS_PLATFORM_ARCHITECTURE.md);
- the [OWS Shared Gameplay Spine, Authority, and Persistence Contract](OWS_GAMEPLAY_SPINE_ARCHITECTURE.md);
- the accepted program selection in the [OWS Mobility Program Charter](OWS_MOBILITY_PROGRAM_CHARTER.md); and
- the audited implementation, research, and decision record in the [OWS Mobility Evidence Baseline](OWS_MOBILITY_EVIDENCE_BASELINE.md).

The [OWS Mobility Acceptance Matrix](OWS_MOBILITY_ACCEPTANCE_MATRIX.md) defines the proof required for every support claim. The older [Character and Vehicle Acceptance Matrix](CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md) remains the protected current-behavior regression baseline until the replacement paths pass the Mobility matrix.

The [OWS Mobility Implementation Backlog](OWS_MOBILITY_IMPLEMENTATION_BACKLOG.md) maps this contract to non-overlapping GitHub issues. It is an execution map, not permission to check out any child.

## Non-negotiable product rules

1. OWS is the product. Its complete maintained Character and Vehicle foundations ship inside one easy-to-install OWS package; users do not obtain or integrate upstream stepping-stone plugins separately.
2. OWS Character and OWS Vehicle are independent first-party domains. Each builds, loads, and conforms without the other or the Integration family.
3. OWS Character–Vehicle Mobility is a narrow, acyclic Integration family that consumes both public contracts. It does not duplicate domain state or become a general gameplay catch-all.
4. Player identity, Character embodiment, occupancy, presentation, possession, and permission to control a Vehicle are separate concepts.
5. Physical motion is continuous. No valid transition teleports a Character across an untraversed gap, launches it with an unexplained impulse, or applies inherited motion twice.
6. The server owns canonical shared state. Prediction improves responsiveness but never grants occupancy, control, damage, destruction, or authority.
7. Existing verified entry, exit, bailout, roll, recovery, and re-entry behavior remains protected until an accepted replacement passes the complete matrix.
8. Planned architecture is not an implementation claim. A capability is supported only after its named profile and evidence matrix pass.

## First supported product slice

### Carrier and capability envelope

The first carrier slice covers every current OWS road Vehicle, including rigid cars, SUVs, the articulated bus, and extreme-size roster members. Boats, trains, aircraft, motorcycles, ATVs, and project-defined carriers remain representable through the public contracts but are not advertised until their own carrier matrices pass.

The first capability slice delivers the complete same-Vehicle loop:

- exterior standing, walking, crouching, crawling where geometry permits, jumping, bracing, sliding, gripping, hanging, falling, rolling or tumbling, and recovery;
- stopped and moving boarding through exact authored portals and points;
- passengers, finite seats, control roles, seat switching, and driver handoff;
- freely traversable authored interiors where geometry and pose clearance permit;
- same-Vehicle exterior-to-interior infiltration;
- stopped exit, moving bailout, re-entry, interruptions, and failure recovery; and
- identical Character capability for player and NPC control sources.

Cross-Vehicle transfer is representable, authorable, and validatable in the first architecture. Its runtime certification follows as a later implementation slice and cannot block certification of the same-Vehicle slice.

### Release envelope

| Dimension | First supported claim |
| --- | --- |
| Client platform | Windows PC |
| Dedicated server | Windows and Linux |
| Network modes | Standalone, listen host, remote listen client, dedicated server, remote dedicated client |
| Replication backends | Legacy replication and opt-in Iris with equivalent results |
| Scale | 32 players, 32 simultaneously active and relevant Vehicles, and 16 concurrent Mobility relationships in one hotspot or large Vehicle |
| Frame rates | Behavioral parity at 30, 60, and 120 FPS; 60 FPS target on a published midrange reference PC |
| Supported impairment | 150 ms RTT, 30 ms jitter, and 2% packet loss |
| Recovery stress | 250 ms RTT, 60 ms jitter, and 5% packet loss plus reorder and duplication |

The exact reference hardware and per-system budgets are published with benchmark evidence. Console-compatible architecture is required, but console support is advertised only after licensed hardware conformance. Other desktop platforms, mobile, and VR remain deferred.

### Feel and accessibility profiles

OWS ships three data-driven physical-feel profiles:

- **Balanced** — physics-grounded default;
- **Cinematic** — stronger assists without bypassing physical or authored validity; and
- **Simulation** — minimal assists.

Accessibility is orthogonal to feel. Changing hold/toggle behavior, timing assistance, automatic Grip, edge-catching assistance, prompts, or camera motion never silently changes the physical profile.

## Composition and dependency contract

| Family | May depend on | Must not depend on |
| --- | --- | --- |
| Character runtime | Platform, approved Shared contracts, and the Supported Input registration/routing contract | Vehicle internals, Character–Vehicle Integration, or Input implementation internals |
| Vehicle runtime | Platform, approved Shared contracts, and the Supported Input registration/routing contract | Character internals, Character–Vehicle Integration, or Input implementation internals |
| Character–Vehicle Integration runtime | Supported Character, Vehicle, Interaction, Shared Gameplay, Platform, and persistence/authority contracts | UI, device Input, fork-private internals, demo-shell types, or circular domain dependencies |
| Character editor/tests | Character runtime plus allowed Editor/Test support | Vehicle except in explicitly separate Integration conformance targets |
| Vehicle editor/tests | Vehicle runtime plus allowed Editor/Test support | Character except in explicitly separate Integration conformance targets |
| Integration editor/tests | Both supported public domain contracts plus allowed Editor/Test support | Demo-shell ownership or private provider APIs |

The project game module and demo content configure and demonstrate these families. Reusable Mobility ownership cannot remain in the demo shell.

## Canonical identity and state model

### Stable identities

All shared references use the Stage 2 identity contract:

- stable Character and Vehicle entity identities;
- a stable Mobility relationship identity and monotonic revision;
- a stable request identity and terminal result;
- a stable transition identity;
- a versioned Vehicle graph identity; and
- stable Vehicle-scoped body, bone, zone, edge, anchor, portal, seat, and control-role identities.

Runtime pointers, Actor names, array indices, component order, and transient possession are not persistent public identities. Regeneration and schema migration preserve stable IDs or emit an explicit migration failure.

### Orthogonal state axes

OWS does not encode every combination in one giant state enum. The authoritative snapshot keeps these axes separate:

| Axis | Examples | Owner |
| --- | --- | --- |
| Relationship topology | None, supported by point/zone, gripping, hanging, traversing a graph edge, occupying a seat | Integration |
| Bodily support response | Stable, braced, sliding, airborne, controlled roll, tumble, recovery | Character, driven by accepted Integration evidence |
| Shared Action lifecycle | Requested, Validated, Started, Committed, and exactly one terminal state | Shared Gameplay contract and the selected executor |
| Physical transition phase | None, secure, approaching, launched, contact established, arriving, recovery | Integration relationship; distinct from the Shared Action lifecycle |
| Occupancy | No seat or one stable Vehicle-scoped Seat ID | Vehicle canonical record |
| Control grant | No control or one stable Vehicle control role | Vehicle canonical record and Integration transaction |
| Embodiment | Ordinary, seated hybrid, or an approved project replacement | Character policy selected through Integration |
| Control source | Player, NPC, neutral, or another approved semantic-action provider | Shared Action/controller policy; Input routes player devices only |
| Presentation | Recipe, phase, camera context, smoothing state | Character presentation with Integration context |

Changing one axis never silently rewrites another. Losing possession does not delete occupancy; changing camera does not move the Character; disconnecting changes control source without creating a second Character class.

### Governed transition lifecycle

Every discrete Mobility transition maps directly to the Stage 2 Shared Action lifecycle. A feature may add typed relationship context, but it cannot skip or reinterpret these phases:

1. **Requested** — receive one semantic Action with a server-derived/authenticated instigator, stable request/source/target identities, expected revisions, bounded typed context, and a source-typed event time.
2. **Validated** — verify identity, schema, permission, authored route, capability, pose, clearance, occupancy, exact same-time point motion, current revisions, and recipe/data readiness without mutating canonical state.
3. **Started** — atomically lease every scarce portal, edge, anchor, or seat required for the declared operation and begin governed execution. Optional owner anticipation remains disposable and non-authoritative.
4. **Committed** — record the transition family's named physical commitment: the Character leaves secure support, establishes target contact, or otherwise crosses the accepted no-cancel boundary. The physical transition phase remains a separate field. Later seat, occupancy, and control mutations commit atomically at their own named arrival boundary and never become aliases for Action `Committed`.
5. **Terminal** — publish exactly one `Completed`, `Rejected`, `Cancelled`, `Interrupted`, or `Failed` result, release every remaining lease once, converge every endpoint, and either establish the valid destination state or enter the typed physical/recovery graph.

Before commit, Cancel, invalidation, timeout, destruction, or failed validation leaves no partial occupancy, control, collision, force, embodiment, or presentation mutation. After commit, OWS never rewinds physical history into success or restores a surrendered relationship by teleport; interruption resolves from the authoritative current state.

| Transition family | Required preconditions and reservations | Action `Committed` boundary and later domain commit | Valid terminal or recovery outcomes |
| --- | --- | --- | --- |
| Dynamic support, Grip, or hang | Valid contact or authored point, capability, clearance, exact point motion, and required point/edge lease | The authoritative relationship acknowledges support/coupling or Grip contact at one revision | Stable, braced, sliding, gripping, hanging, released, airborne, or typed recovery |
| Stopped boarding | Exact Vehicle/portal/path/seat, permission, compatibility, clearance, and required portal/seat leases | Character leaves secure support or establishes authored contact; destination occupancy commits only at arrival | Interior/support/seat destination, pre-commit rejection/cancel, or post-commit miss/fall/recovery |
| Moving boarding or infiltration | Stopped-boarding requirements plus valid same-time intercept, relative load, and reachable physical route | Character leaves secure support or establishes target contact | Catch, Grip/hang, exterior/interior/seat arrival, slide, miss, fall, or recovery |
| Interior traversal | Valid interior region, pose clearance, reachable edge/portal, and any scarce-edge lease | Crossing the authoritative graph boundary into the destination region | Destination region/seat/support, safe stop, rejection, or typed physical recovery |
| Seat switch or driver handoff | Valid source occupancy, reserved compatible destination, reachable authored path, and current revisions | Action commits when bodily movement crosses its physical boundary; destination occupancy and any control grant then commit atomically at arrival, while the source record remains until that transaction | New seat/role, unchanged source state, or typed recovery without duplicate occupants/controllers |
| Exit or bailout | Current occupancy, exact exit/release point state, valid route or bailout policy, and clearance where required | Action, occupancy/control surrender, and Character release state commit at the named physical release boundary | Valid exterior support, airborne/roll/tumble/recovery, or blocked pre-commit rejection |
| Cross-Vehicle transfer | Two valid graphs, physical source-to-target route, exact point histories, and all required leases | Same physical-contact rule as moving infiltration; never direct interior-to-interior state reassignment | Representable and validatable now; runtime outcomes remain uncertified until the later matrix passes |

## Public domain contracts

### OWS Character contract

Character owns and publicly exposes:

- CMC-based bodily locomotion for the first supported slice;
- stance and pose geometry, support footprint, reach, automatic balance within the recoverable envelope, limited Move counterbalance, Grip, edge catch, free-hand, jump, fall, roll or tumble, and recovery capabilities;
- pose-specific capability observations rather than universal stance multipliers;
- optional stamina, damage, and environment capability-provider hooks, with the included stamina provider disabled by default;
- support-relative movement input and remaining steering capacity;
- Character-side prediction and authoritative correction consumption;
- context-correct animation-recipe execution;
- body visibility, collision, hit/query representation, and seated embodiment transitions; and
- the Character camera provider and camera-motion preferences.

Character accepts public, provider-neutral support and transition execution requests and returns governed results. It does not know Vehicle classes, seats, doors, anchors, occupancy, or reservations. Mobility publishes measured load, contact, and hazard context to the accepted Character/Shared Gameplay capability owner and consumes its authoritative result; it never creates a second stamina, damage, vitality, or environmental-consequence authority.

### OWS Vehicle contract

Vehicle owns and publicly exposes:

- the maintained current OWS Vehicle foundation for the first supported slice;
- simulation, collision bodies, mass properties, physical materials, and Vehicle-specific driving behavior;
- versioned Mobility graph contributions and stable scoped identities;
- standable zones, include/exclude overrides, edges, anchors, portals, seats, control roles, capacities, compatibility, clearance, and Vehicle-side transition connections;
- canonical occupancy and control-grant records with revisions;
- exact world transform, linear velocity, angular velocity, point velocity, and time-consistent point acceleration for rigid bodies, skeletal bones, articulated components, and project-defined kinematic providers;
- structural animation and Vehicle camera anchors/data; and
- measurable telemetry required by the driving and Mobility matrices.

Vehicle never moves, hides, animates, or changes bodily state on a Character. It does not own cross-domain reservations or relationship transitions. Replacing the maintained foundation requires measured benefit, compatibility and migration evidence, and Aurora's explicit approval; the presence of an Experimental or alternative backend is not authorization.

### OWS Character–Vehicle Integration contract

Integration owns only the relationship between public Character and Vehicle contracts:

- target, route, capability, clearance, relative-motion, permission, and revision validation;
- dynamic support, exterior riding, Grip, hanging, boarding, infiltration, seat and control handoff, exit, bailout, transfer, and cross-domain recovery orchestration;
- atomic reservations and discrete multi-owner commits;
- relationship-specific collision filtering;
- the single two-way Character–Vehicle coupling solver;
- server request validation, history mapping, prediction reconciliation, and governed results; and
- selection of approved presentation, camera, embodiment, and assist policies through replaceable adapters.

Integration references Vehicle occupancy revisions rather than maintaining a second roster. It requests Character execution rather than replacing Character locomotion or animation internals.

## Vehicle Mobility graph and authoring

### Graph elements

A compatible Vehicle may contribute:

- physical standable zones and explicit include/exclude zones;
- reachable edges, handholds, Grip and hang anchors;
- portals such as doors, windows, hatches, steps, and openings;
- seats and control roles;
- interior walkable regions;
- directed transition edges among world approach points, surfaces, anchors, portals, interiors, seats, and exits;
- approach, intercept, occupancy, and clearance volumes;
- attachment providers identifying a rigid body, skeletal bone, articulated component, or custom kinematic source;
- neutral transition-recipe IDs and alignment metadata that Integration resolves to Character-owned presentation assets without a Vehicle-to-Character content reference;
- pose, capability, load, and compatibility requirements;
- safe fallback and checkpoint anchors; and
- stable IDs, schema version, validation metadata, and diagnostics.

Ordinary standing and walking may use a physically valid Vehicle surface. Authored include/exclude zones override automatic eligibility. Precise Grip, portal, boarding, infiltration, and transfer actions always require authored identities and transition data.

### Draft generation and manual control

The Editor generates a draft graph from collision, bodies, doors, seats, and other supported evidence. A developer reviews and accepts the draft before it becomes supported authoring. Regeneration is non-destructive, preserves stable IDs and manual overrides, and presents a clear diff. Fully manual authoring remains available.

Editor tooling provides live visualization for points, zones, edges, clearance, identity, provider attachment, reachability, and validation failures. Validation runs on save and in CI. Invalid required data blocks release validation and cooking with actionable repair guidance. An invalid optional edge disables deterministically rather than guessing another transition. Migrations are versioned and non-destructive.

### Interiors and first paths

Every Vehicle uses the seat/portal graph. Authored interior regions additionally allow free movement when geometry provides clearance for the Character's supported standing, crouching, or crawling pose. Eligibility is based on geometry and pose, not Vehicle category. The articulated bus is the first mandatory free-interior validation Vehicle.

The first slice supports:

- ground to portal to interior or seat and back;
- exterior surface to Grip or hang to portal or interior;
- interior to seat and seat to interior;
- seat-to-seat switching and driver handoff; and
- every occupied state to a valid exit, bailout, or recovery.

The graph can express a transfer initiated from a seat or interior. A cross-Vehicle route must physically traverse an authored source portal or exterior contact into the target Vehicle. It may be one continuous move and need not pause at an intermediate anchor, but it never teleports interior-to-interior.

## Physical and kinematic contract

### Exact point motion

For an attachment point at offset `r` from a reference body's origin:

`point velocity = body linear velocity + body angular velocity × r + local point velocity`

For an articulated or locally moving point, the complete rotating-frame starting model is:

`point acceleration = body linear acceleration + angular acceleration × r + angular velocity × (angular velocity × r) + 2 × angular velocity × local point velocity + local point acceleration`

Character-relative point motion is calculated at one shared time:

`relative velocity = Character velocity - target point velocity`

Absolute Vehicle speed is never the universal boarding, support, ejection, or transfer rule. Validation also considers approach, gap, time to contact, relative acceleration, jerk and impulse, surface normal and slope, support geometry, clearance, relative wind, current pose, capability, Grip load, and optional stamina.

### Contact, balance, and coupling

Normal force is measured or solved from the actual contact state; OWS does not assume it always equals body weight. Friction, support geometry, center of mass, torque, angular capacity, edge proximity, and active Grip determine whether support can be maintained. Inside the recoverable envelope, balance is automatic and limited Move input may counterbalance until physical capacity saturates into bracing or sliding. Aerodynamic load uses air-relative velocity and Character/pose-specific area and drag data; crouching is never a universal adhesion multiplier.

Grip works with no stamina provider. The included optional stamina provider is disabled by default; when enabled, drain follows authoritative Grip load and the selected certified policy/profile rather than an animation or fixed timer. Edge catch requires an authored reachable edge, valid hand reach and clearance, survivable relative load, and the Character capability. Grip is required by default; automatic catch is an accessibility option that submits the same governed intent.

The server-authoritative coupling solver owns all Character–Vehicle exchange. Integration calculates relationship loads and deduplicates their application; Character executes bodily motion and Vehicle applies accepted simulation forces through their public contracts. Rider weight, walking, jumping, landing, impacts, and Grip apply equal-and-opposite forces at the actual contact points. Relationship-aware collision filtering suppresses only contacts that would duplicate or fight that acknowledged coupling. World, unrelated body, and non-relationship collision remains active.

### Progressive response graph

Physical evidence drives a branching response, not an animation timer:

`stable → braced → sliding`

From sliding or threatened support, a valid Grip opportunity may produce caught or hanging. Failed support or Grip produces release and airborne motion. Ground contact selects controlled roll or tumble from contact evidence, then optional damage and recovery. Regaining valid support may return through braced to stable.

Release preserves exact target-point velocity plus Character-relative velocity and applies each new impulse once. The Character receives limited physically available steering during slide, leap, airborne, and roll states; Grip and hang accept body shift and directional reach. Recovery returns control continuously, and held movement flows into ordinary locomotion.

### Interruptions

Collision, crushing, destruction, environment, and world bounds are typed authoritative interruptions. Collision loads enter the same response graph. Crushing or destruction invalidates only affected graph points; surviving points remain valid. Damage, fire, and water consequences come from optional capability providers rather than hardcoded detachment. World-bounds relocation is a final recovery measure using the latest validated safe anchor.

## Actions, boarding, seats, and embodiment

### Semantic actions

Runtime Mobility consumes the Shared device-independent semantic Action contract. Player device intent enters through OWS Input; NPC and server control sources submit the same Actions directly without owning a physical device. No runtime Mobility system polls keys or controller buttons.

- **Activate** requests the context-selected authored boarding, infiltration, or transfer path.
- **Cancel** terminates the current pre-commit Action and cannot undo a committed physical transition.
- **Exit**, **Bailout**, and **Release** are governed semantic Actions that end an established relationship. The default player-facing Cancel binding may contextually submit one of them when there is no cancellable in-flight Action; that routing does not redefine their lifecycle as cancellation.
- **Grip** establishment and release are discrete Actions; maintained Grip and body shift are continuous control. Hold is default, with toggle and automatic alternatives.
- Explicit target and seat requests carry their stable scoped IDs.

Character Move, Look, Crouch, Jump or Leap, and Aim remain Character actions. Vehicle driving controls remain Vehicle actions. Different player contexts may share a default physical binding, and prompts always reflect current bindings.

### Boarding and commitment

For a stopped Vehicle, Activate reserves a valid seat and portal and may automate the authored approach, alignment, and entry. The player-facing Cancel control aborts before commitment; Integration expresses that intent through the governed Mobility action appropriate to the current relationship.

For a moving Vehicle, the Character must physically approach, speed-match, and time Activate. Grip maintains an established catch. OWS performs only authored and validated climb or entry transitions; it never extends reach or places the Character across an untraversed gap.

The server rejects before launch when no valid route or reservation exists, no reachable intercept is predicted, or predicted contact load already exceeds capability. Physical commitment begins when the Character leaves secure support or establishes contact. Later changes in speed, gap, acceleration, approach, or time to contact resolve physically as catch, slide, miss, fall, or recovery and are not rolled back.

### Seats and control handoff

Seat operations are server-authoritative transactions over stable Seat IDs. An explicit valid request wins over automatic selection; otherwise a deterministic policy selects the best compatible seat reachable through the chosen portal. Destination reservation occurs before source-seat surrender. Switching follows authored paths. Driver control transfers only at commit, and failure preserves the original occupant and control grant. Cancel, timeout, disconnect, death, revision change, or destruction releases affected reservations.

### Seated embodiment

The OWS default is hybrid embodiment. The same Character actor remains authoritative, visible, targetable, and damageable. Independent locomotion and capsule physics that would fight the Vehicle are suspended, while authored seated hit and query volumes follow the seat and Vehicle body. Full Character collision returns at a validated exit. Projects may replace this policy, including with presentation-only embodiment, without changing occupancy or control semantics.

## Animation, camera, and player control

Every enabled authored transition requires a context-correct presentation recipe. Vehicle data supplies neutral recipe IDs and alignment requirements; Character owns the actual animation assets, and Integration resolves the compatible pairing. OWS ships recipes for boarding, exit, seat changes and handoff, catch, hang, climb, moving entry, bailout and roll, and recovery. Missing required presentation never substitutes an unrelated animation. Physical state remains authoritative, validation reports the gap, and certified shipping is blocked until the recipe is supplied or the transition is disabled.

Motion Warping is the default transition-alignment provider behind a replaceable adapter. Integration supplies the validated path, targets, and timing; no canonical relationship state depends on Motion Warping. Projects may substitute Control Rig, custom root motion, or another provider.

Every Character exposes one replaceable Character-owned camera capability and context policy. A locally controlled player activates its view; an NPC preset may leave local-view presentation inactive without removing the common capability. The policy:

- preserves the current view and blends rather than forcing unexplained cuts;
- filters unwanted Vehicle vibration during exterior riding;
- frames the Character and relevant contact path during Grip, boarding, and transfer;
- follows the authoritative Character continuously through miss, fall, roll, and recovery;
- smooths network correction presentation; and
- exposes shake, roll, horizon-lock, and field-of-view preferences.

Aim remains available only when pose, stability, equipment, and free-hand capability allow. Camera look remains available throughout Mobility states.

## Player, NPC, and accessibility parity

Player and NPC Characters use the same Mobility participant, semantic Actions, animation, capability, physics, graph, requests, authority, validation, outcomes, and camera capability. A control-source flag and policy select whether Actions arrive from OWS Input, NPC logic, neutral control, or another supported source. A local-view camera may be inactive for an NPC preset, but the common capability and context contract remains. Features unnecessary for a given NPC are developer-configurable, and OWS supplies recommended NPC presets; there is no second, simplified NPC Mobility implementation.

Developers define enabled capabilities, valid envelopes, certified ranges, project defaults, and available presets. Players select exposed accessibility behavior within those valid ranges, including alternatives to prolonged, repeated, simultaneous, and analog-only input. Removing an accepted alternative requires an equivalent accessible path or relinquishes that conformance claim. Timing assistance may buffer or submit intent at a valid opportunity but cannot invent reach, bypass clearance, load, capability, reservation, or authority, teleport, or change the feel profile. Accessibility, control bindings, and camera preferences are device settings. The authoritative feel profile is synchronized server-owned gameplay configuration with an explicitly declared session or world lifetime, never a device preference.

## Authority, prediction, and contention

### Canonical authority and anticipation

The server owns every relationship and transition. The owning participant predicts continuous bodily motion only inside an already authorized relationship and tags it with relationship identity and revision. Simulated proxies interpolate authoritative support-relative pose, velocity, and phase. Reservations, occupancy, control grants, damage, destruction, and relationship changes never commit early.

Before server acceptance, the owner may play disposable UI, camera, audio, or animation anticipation that does not move collision, surrender support, reserve or occupy a point, or alter control. Requests are reliable and idempotent and carry request identity, expected revisions, stable source/target IDs, and a source-typed event time: a mapped client timestamp for authenticated client intent or authoritative server time for NPC/server intent. Acceptance supplies the committed transition and revision, authoritative start time, recipe, and exact states. Rejection cancels anticipation smoothly with a governed reason.

### Time validation

The server retains synchronized authoritative history for every participating Character and relevant Vehicle body, bone, articulated component, and kinematic provider. It rewinds Character and target to the same mapped instant and interpolates exact point kinematics there. Current occupancy, reservation, permission, and revision are always rechecked at commitment; rewind proves reach but never resurrects stale availability.

Out-of-window requests reject rather than clamp. Terminal results remain cached long enough to make delayed and duplicated delivery idempotent. History, skew, retention, and correction values are published, data-driven engineering values established against the accepted impairment matrix—not guessed product constants.

Request admission validates authenticated source ownership or permission, declared types, finite numeric values, payload and collection bounds, stable-ID scope, revision plausibility, and per-source/per-contract rate budgets before expensive history or physics work. Client-supplied transforms, kinematics, outcomes, and authority are never trusted as canonical evidence. Rate limiting and malformed-request rejection cannot mutate Mobility state, starve already committed recovery, or prevent a well-behaved source from receiving its governed terminal result. Exact limits are versioned engineering budgets established by security, network, and scale evidence.

### Reservations and contention

Reservations are monotonic server leases acquired only after every required recipe and data dependency is ready; a lock never spans asset loading, a client wait, or unrelated asynchronous work. A lease covers the authored transition duration plus a validated network margin. Only authoritative phase progress renews it. Cancel, rejection, timeout, disconnect, death, target invalidation, revision change, destruction, or pre-commit reachability loss releases immediately. After physical commitment it ends at the catch, miss, or arrival terminal result. Committed occupancy is durable state, not a renewable lease.

Committed occupancy and valid reservations are not preempted. Contenders order atomically by normalized intent time inside the rewind window, then server receive sequence, then stable request ID. An explicit target failure never silently substitutes another target; automatic selection may choose the next deterministic compatible alternative. A losing result includes the governed conflict reason, current revision, retryability, and valid alternatives. Projects may install a deterministic role-priority policy that cannot override a committed relationship.

### Correction

Correction error is measured in the authoritative support or target frame. Thresholds are normalized to Character scale, configurable only within certified ranges, and established by evidence across accepted frame rates and impairment profiles. Small errors use normal prediction smoothing. Larger clearance-valid errors keep collision authoritative while mesh and camera converge. Material or clearance-invalid disagreement enters the physical failure/recovery graph with exact inherited and relative motion and no duplicated impulse.

OWS never moves a Character to the camera, stale Vehicle state, or a past predicted location. Hard relocation is limited to invalid numeric state, world bounds, or unrecoverable penetration and uses the latest server-validated safe anchor.

Legacy replication and Iris use the same backend-neutral schema and must pass identical behavior and recovery evidence.

## Lifecycle and persistence

Relationship topology, support, transition, control source, embodiment, and presentation converge independently:

- **Late join:** reconstructs the current snapshot and phase without replaying stale transient events.
- **Disconnect:** releases pending reservations and control; the same Character and valid physical relationship remain server-simulated while project policy selects neutral input, NPC control, reconnect grace, or governed termination. Only one live controller may exist; duplicate connections reject, and a stale connection may be reclaimed only after authenticated relationship/control revalidation.
- **Relevance/dormancy:** active related participants remain co-relevant; client relevance loss suspends presentation only.
- **Destruction/provider failure:** invalidates affected points and releases from the last same-time authoritative sample.
- **Owner or possession loss:** revokes control without silently changing occupancy or embodiment.
- **Interrupted presentation:** cancels before physical commitment; after commitment, physics completes or enters a valid failure outcome.
- **Backend failure:** fails closed to new transitions, preserves canonical state when possible, and invokes typed server recovery otherwise.
- **Server restart:** begins a fresh session. Revalidated stable world-owned relationships may restore from committed records, but prior possession, control grants, reservations, in-flight transitions, prediction, and presentation never resurrect.

Death/respawn and non-seamless map travel terminate Mobility relationships. Seamless travel and World Partition preserve stable relationships only when every participant and stable graph identity migrates and revalidates; active related participants co-stream. Save/load and checkpoints serialize stable relationships only. In-progress transitions, reservations, and prediction resolve to an authored safe checkpoint state rather than serializing. Failed restoration uses the latest validated safe anchor.

A stable Mobility relationship is one world/server-lifetime record owned by the persistence coordinator and keyed by its stable relationship identity. Character, Vehicle occupancy/control, and save/checkpoint records reference that identity and revision instead of duplicating relationship truth. Restoration reads one committed cross-record snapshot and revalidates every participant, graph, point, occupancy, and policy reference before publishing any live field.

| Field family | Live replication | Save/checkpoint or restart | Seamless travel / World Partition |
| --- | --- | --- | --- |
| Stable participant, graph, point, and relationship identity/revision | Yes | Stable relationship snapshot only; revalidate before restore | Rebind only when every identity migrates/revalidates; co-stream while active |
| Stable support, Grip/hang, or seat occupancy relationship | Yes | May restore only when explicitly world-owned, stable, and revalidated | Preserve only with all participants and graph data |
| Possession and Vehicle control grant | Yes | Never resurrect from save/restart; reacquire through current policy | Revalidate independently from occupancy |
| Reservation/lease | Yes while pending | Never | Resolve or cancel before migration |
| In-progress transition and physical commitment phase | Yes | Never; resolve to authored safe checkpoint state | Resolve or cancel before travel |
| Client prediction, anticipation, smoothing, and presentation offsets | Client/runtime only | Never | Reconstruct from the new authoritative snapshot |
| Latest validated safe anchor | Authoritative recovery data | Yes when its stable identity remains valid | Migrate/revalidate with its owning world state |

## Extension and diagnostics contract

### Supported extension points

Projects may extend OWS through documented descendant gameplay tags and typed context; Character capability providers; Vehicle graph/point/kinematic providers; Integration routing and assist-selection policies; Character-owned embodiment, camera, presentation, and animation/Motion-Warping providers selected from relationship context; Vehicle structural anchors/data; validated profiles/data assets; and explicitly aggregate diagnostic listeners.

Canonical meanings cannot be reinterpreted. An authoritative contract has exactly one active provider, an optional contract zero or one, and only an explicitly aggregate contract permits multiple providers. Editor, cook, and startup validation reject ambiguous ownership. Extensions cannot bypass authority, revision, idempotency, atomic reservation, exact point motion, physical validity, or coupling ownership.

Every supported replacement surface provides equivalent Blueprint and C++ registration/conformance paths unless its documented role is C++-only for an engine limitation. Removing an optional provider preserves a declared no-op or default behavior and cannot break unrelated Mobility. Aggregate listeners receive the same immutable correlated event independently; they cannot mutate, consume, reorder, or become authorities over the event.

### Correlated evidence

One structured diagnostic schema correlates:

- request, transition, relationship, and revision identities;
- authority role and shared network/physics time;
- stable participant, graph, point, body, component, and bone identities;
- state before and after each decision;
- route, clearance, intercept, reservation, and contention evidence;
- exact target and relative kinematics, release motion, impulses, contacts, and support geometry;
- balance, friction, aerodynamic, Grip, and coupling budgets; and
- prediction history, correction reason, resimulation, and endpoint agreement.

Editor visualization, local runtime overlays, Visual Logger, Unreal and Networking Insights, Rewind Debugger, Chaos Visual Debugger, and machine-readable multiplayer test artifacts are adapters over this schema. Every sink and evidence artifact is local and privacy-preserving by default, uses stable redacted identities, and excludes secrets and external account identifiers. All adapters obey published event-rate, memory, history, file-size, and trace-duration budgets and expose dropped/sampled-event counts rather than allocating without bound. Shipping builds emit no overlay, file, trace, request, or transmission by default. Explicit Shipping diagnostics require approved build configuration, bounded sampling and retention, performance budgets, and—before external transmission—a developer integration and applicable player consent. OWS ships no external telemetry endpoint.

## Conformance and support claims

A release may claim only the profile that passed:

| Profile | Required proof |
| --- | --- |
| Character-independent | Character builds without Vehicle/Integration and passes its public capability, animation, player/NPC semantic-control, and Character-only tests through synthetic providers; separate registration conformance proves its owned actions/default maps register with Input |
| Vehicle-independent | Vehicle builds without Character/Integration and passes simulation, graph, identity, occupancy/control, point-kinematic, semantic-control, authoring, and Vehicle-only tests through synthetic providers; separate registration conformance proves its owned actions/default maps register with Input |
| Same-Vehicle Mobility | Both contracts plus Integration pass the complete same-Vehicle local, multiplayer, impairment, failure, lifecycle, performance, and accessibility matrix |
| Cross-Vehicle transfer | Same-Vehicle profile plus the separately accepted transfer matrix |
| Carrier profile | Every advertised carrier class passes its named graph, physics, transition, presentation, and network coverage |
| Platform/profile combination | Every advertised client/server platform, replication backend, feel profile, and accessibility path passes its declared subset |

No partial demo, incidental moving-base behavior, local-only smoke test, or passing animation is evidence for a broader profile.

## Compatibility-first implementation order

The architecture establishes this dependency order without authorizing checkout:

1. Publish independently buildable Character and Vehicle public contracts and conformance harnesses.
2. Publish exact point-kinematic/telemetry and collision/force-response contracts.
3. Implement versioned Vehicle graph data, draft generation, visualization, validation, and migrations.
4. Implement the backend-neutral Integration relationship snapshot, revisions, and Mobility request/result consumer without changing current gameplay; consume Shared Action from its Stage 2 owner and add structured observability through #140 rather than duplicating either.
5. Migrate stopped entry/exit and current bailout/recovery behind the new contracts and prove baseline parity.
6. Add passengers, seat switching, driver handoff, and hybrid seated embodiment.
7. Add dynamic support, exterior movement, balance, bracing, slide, Grip, hang, and failure response.
8. Add moving boarding and same-Vehicle exterior/interior infiltration.
9. Aggregate final network, persistence, accessibility, NPC/player parity, scale, performance, and both-replication-backend certification; every earlier slice must already have passed its applicable axes incrementally.
10. Scope and certify cross-Vehicle transfer as the next slice.

Each step is a separate tightly scoped issue or reviewed issue family. A failure discovered outside a claimed scope becomes a new issue; it does not broaden the current implementation task.

## Issue ownership

| Responsibility | Controlling issue |
| --- | --- |
| Canonical architecture and complete matrix | #147 |
| Character public contract and independent conformance | #107 |
| Vehicle public contract and independent conformance | #108 |
| Collision, balance, slide, Grip load, impact, coupling, and detachment response | #21 |
| Driving and exact point-kinematic benchmark evidence | #41 |
| Vehicle graph runtime records | #155 under #108 |
| Vehicle graph draft generation, visualization, validation, and migration | #156; #48 retains driving-tuning assets |
| Cross-domain route/recipe descriptor resolution | #160 under #120 |
| Cross-domain relationship implementation and compatibility migration | #120, decomposed into accepted child scopes |
| Shared authoritative consumer slices | #27 and #28 without duplicating #120 ownership |
| Stage 3 completion gate | #111 after the accepted documents and reconciled backlog merge |

The complete child/dependency map is maintained in [OWS Mobility Implementation Backlog](OWS_MOBILITY_IMPLEMENTATION_BACKLOG.md). #150–#168 are architecture-gated implementation or evidence slices; none becomes ready merely because it appears there.

## Decision traceability

The numbered decisions in the [evidence baseline](OWS_MOBILITY_EVIDENCE_BASELINE.md#accepted-interview-decisions) map one-to-one into this normative contract and its proof. Issue ownership identifies the implementation/evidence family; it does not make that issue ready for checkout.

| Decision | Architecture destination | Acceptance coverage | Owning issue family |
| --- | --- | --- | --- |
| 1. First carrier slice | First supported product slice | Vehicle inventory; MOB-GRAPH, MOB-OPS-010 | #108, #41, #155/#156 |
| 2. First capability slice | Carrier and capability envelope | MOB-PHY, MOB-ACT/OCC/INT, MOB-EXIT | #107, #108, #120 |
| 3. Feel profiles | Feel and accessibility profiles | MOB-PHY-016; MOB-ACC; T2–T5 | #21, #107, #141 |
| 4. Multiplayer commitment | Release envelope; authority | MOB-NET-001/002/012/013 | #27, #28, #120, #141 |
| 5. Foundations retained | Public domain contracts; deferred rules | MOB-BLD-001–004; protected regressions | #107, #108, #120 |
| 6. Platform envelope | Release envelope | MOB-OPS-008/010; T5 | #41, #49, #141 |
| 7. Scale and frame rate | Release envelope | MOB-OPS-005–007 | #41, #49, #141 |
| 8. Impairment envelope | Release envelope; time validation | MOB-NET-012/013/017 | #27, #28, #141 |
| 9. Surface eligibility | Graph elements; contact | MOB-GRAPH-009; MOB-PHY-001–006 | #21, #156, #159 |
| 10. Graph authoring | Draft generation and manual control | MOB-GRAPH-001–003 | #156 |
| 11. Point attachment | Vehicle contract; exact point motion | MOB-GRAPH-006–008 | #154, #41 |
| 12. Authoring safeguards | Draft generation and manual control | MOB-GRAPH-002–005; MOB-BLD-008 | #156 |
| 13. Balance and crouch | Contact, balance, and response graph | MOB-PHY-003/006/007/016 | #21, #107, #120 |
| 14. Active Grip | Semantic actions; response graph | MOB-PHY-008/010/011; MOB-ACC-001 | #117, #107, #120 |
| 15. Optional stamina | Character contract; contact/coupling | MOB-PHY-009; MOB-BLD-010 | #107, #21 |
| 16. Edge catching | Progressive response graph | MOB-PHY-008/010/011 | #21, #107, #120 |
| 17. Failure progression | Progressive response graph | MOB-PHY-007/011/012; MOB-EXIT-007 | #21, #107, #120 |
| 18. Two-way coupling | Contact, balance, and coupling | MOB-PHY-005/013/014 | #21, #108, #120 |
| 19. Interruptions | Interruptions; lifecycle | MOB-PHY-015; MOB-LIFE-004/005 | #21, #120, #138 |
| 20. Boarding actions | Semantic actions; boarding | MOB-ACT-001–006 | #117, #27, #120 |
| 21. Rejection and commit | Governed lifecycle; boarding | MOB-ACT-003–006; universal invariants 6–7 | #27, #120 |
| 22. Seats and handoff | Seats and control handoff | MOB-OCC-001–006; MOB-NET-009 | #108, #28, #120 |
| 23. Seated embodiment | Seated embodiment | MOB-OCC-007/008; MOB-BLD-009 | #107, #120 |
| 24. Interior traversal | Interiors and first paths | MOB-GRAPH-010; MOB-INT-001 | #156, #162 |
| 25. Required paths | Interiors and first paths | MOB-INT-002–004; MOB-EXIT | #160–#163 |
| 26. Interior transfer | Interiors and first paths; deferred rules | MOB-INT-003/004; cross-Vehicle gate | #160 representation; #168 later runtime |
| 27. Animation recipes | Animation, camera, and control | MOB-PRES-001/002 | #107, #120 |
| 28. Motion-Warping boundary | Animation, camera, and control | MOB-PRES-003; MOB-BLD-009 | #107, #120 |
| 29. Camera policy | Animation, camera, and control | MOB-PRES-004/005 | #107, #119, #120 |
| 30. Control preservation | Response graph; camera/control | MOB-EXIT-004–006; MOB-PRES-006 | #107, #117, #120 |
| 31. Player/NPC parity | Player, NPC, and accessibility parity | MOB-AI-001/002; all T2 paths | #107, #120, #141 |
| 32. Semantic actions/accessibility | Semantic actions; parity | MOB-ACT-001; MOB-ACC-001/002 | #117, #127, #120 |
| 33. Execution ownership | Composition; public contracts | MOB-BLD-001–004; MOB-PHY-013 | #107, #108, #120 |
| 34. Continuous prediction | Authority and anticipation | MOB-NET-001–003 | #27, #120, #138 |
| 35. Discrete anticipation | Authority and anticipation | MOB-ACT-007; MOB-NET-004 | #27 |
| 36. Time validation | Exact point motion; time validation | MOB-GRAPH-006–008; MOB-NET-005–007 | #27, #41 |
| 37. Reservation leases | Reservations and contention | MOB-OCC-006; MOB-NET-008/009 | #28 |
| 38. Deterministic contention | Reservations and contention | MOB-NET-009/010 | #28 |
| 39. Correction | Correction | MOB-EXIT-010; MOB-NET-003/011 | #27, #120 |
| 40. Extension surface | Supported extension points | MOB-BLD-006/007/009/010; MOB-GRAPH-008 | #107, #108, #120, #140 |
| 41. Assist ownership | Feel/accessibility; parity | MOB-PRES-005; MOB-ACC-001/002 | #117, #119, #120 |
| 42. Correlated diagnostics | Correlated evidence | MOB-OPS-001/002/011 | #140, #141 |
| 43. Privacy and Shipping | Correlated evidence | MOB-OPS-003/004/011 | #140, #141 |
| 44. Failure convergence | Orthogonal state; lifecycle | MOB-NET-014–016; MOB-LIFE-001–005 | #120, #138 |
| 45. Persistence lifecycle | Lifecycle and persistence | MOB-LIFE-006–013 | #132, #138, #139, #120 |

## Explicitly deferred or forbidden

- Gameplay implementation inside #147.
- Replacing the current Character or Vehicle foundation merely because an Experimental Unreal backend exists.
- Making Motion Warping, Mover, Chaos Mover, or an alternative Vehicle backend part of canonical state identity.
- A permanent external-provider or separately installed upstream-plugin model for OWS's maintained foundations.
- A Character-to-Vehicle or Vehicle-to-Character private dependency.
- Seat occupancy inferred from possession or every passenger possessing the Vehicle.
- Absolute-speed-only boarding/ejection rules, animation-timer physics, unrelated fallback animations, teleporting transitions, double-applied motion, or camera-driven Character correction.
- Cross-Vehicle runtime support claims before its later matrix passes.
- Console or other deferred-platform claims without their hardware evidence.
