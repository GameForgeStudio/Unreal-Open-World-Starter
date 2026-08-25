# OWS Shared Gameplay Spine, Authority, and Persistence Contract

> **Status:** Accepted Stage 2 target architecture. Nothing in this contract claims that the target is implemented in `main`; implementation is tracked through separately scoped issues and the conformance matrix below.

## Purpose

OWS is one cohesive Unreal starter package. Character, Vehicle, Interaction,
Inventory, Combat, Hacking, City Foundation, UI, multiplayer, persistence, and
future living-world systems therefore share one language for identity, actions,
state, authority, events, inventory, saving, and recovery.

This contract prevents an included maintained fork or a future domain from
creating a private parallel gameplay framework. It defines the stable public
semantics those domains consume while preserving each domain's ownership of its
specialized rules and implementation.

The contract is subordinate to Aurora's latest explicit requirement and works
with the accepted [OWS Platform Architecture and Composition
Contract](OWS_PLATFORM_ARCHITECTURE.md). The accepted [City Foundation
Architecture](CITY_FOUNDATION_ARCHITECTURE.md) remains controlling inside its
domain. A real conflict returns to Aurora; no document silently overrides
another accepted contract.

## Product and implementation status

Target architecture and implemented capability are different claims.

- Gameplay Abilities, Sigil Inventory, and Save Extension are enabled included
  foundations, but current OWS gameplay code does not integrate them into a
  shared spine.
- Current interaction, vehicle handoff, hotbar, and vehicle-networking paths use
  separate action, identity, authority, event, and failure paths. The included
  inventory and save foundations remain unwired to a shared OWS gameplay spine.
- Current OWS assets do not install an OWS PlayerState, shared Ability System
  Component, generalized inventory owner, vitality contract, team/faction
  contract, or OWS persistence identity.
- Existing local character, vehicle, selector, and bailout tests remain valuable
  behavior baselines. They do not prove this multiplayer and persistence target.
- The current verified product baseline remains Windows Editor/client and
  single-player behavior until an Aurora-accepted Stage 2 conformance matrix
  passes.

No consumer may cite this document as evidence that a target type, module,
network mode, save format, or recovery path already exists.

## Ownership and dependency boundary

The shared gameplay spine is a first-party OWS domain family. Exact descriptor,
module, and file names are selected by separately scoped migration issues under
the Stage 1 compatibility gates.

| Responsibility | Accepted owner |
| --- | --- |
| Public identity, action, state, event, failure, vitality, affiliation, and authority contracts | OWS Shared Gameplay family |
| Player identity projection and player-scoped ASC lifecycle | OWS Shared Gameplay family |
| GameplayTag root governance and compatibility mappings | One package-wide OWS registry authority |
| Generalized inventory and equipment implementation | Complete maintained Sigil Inventory fork evolving as OWS |
| Persistence semantics, schemas, checkpoints, providers, migration, and recovery | OWS Persistence public contract |
| Serialization, Actor/component capture, compression, and storage mechanics | Complete maintained Save Extension fork evolving as OWS |
| Character movement and traversal rules | OWS Character family evolving from the maintained GASPALS fork |
| Vehicle simulation and vehicle-specific consequence rules | OWS Vehicle family evolving from the maintained KinetiForge fork |
| Combat formulas, weapons, reactions, and presentation | OWS Combat domain |
| Hacking validation and device-specific consequences | OWS Hacking domain |
| City source identity, recipe epochs, cells, promotion, and cache behavior | OWS City Foundation |
| Hotbar backend source/module and UI presentation | OWS UI family under the accepted Stage 1 physical boundary; each runtime hotbar instance remains host-neutral and developer-selected, with no inherent UI, player, inventory, Pawn, or vehicle owner |
| Playable composition and demonstration | Thin `Source/OWS` and `/Game/OWS` shell |

Shared Gameplay may depend on the minimal public Platform contracts and Unreal
runtime systems such as GameplayTags, GameplayAbilities, and the Gameplay
Message runtime. It may not depend on Character, Vehicle, Combat, Hacking, City,
UI presentation, the project shell, or another consumer.

Consumers depend only on public Shared Gameplay contracts. Domain-specific
adapters depend on the participating public domains; they do not add reverse
dependencies or expose peer-private headers. Save Extension remains a lower
level engine and does not depend on every gameplay domain. Each domain supplies
its explicit persistence record adapter through public registration seams.

## Identity contract

### Player identity and lifetime

OWS distinguishes the person, the live session representation, the controller,
and the current physical avatar.

| Layer | Contract |
| --- | --- |
| Player Profile ID | Permanent opaque OWS-owned identity. Works offline, survives saves and server restarts, and can map to changing account providers. Its exact storage representation remains an implementation decision. |
| PlayerState | Server-authoritative live-session projection of that profile. Player-scoped runtime state follows it across possession, respawn, supported travel, and reconnect. |
| PlayerController | Private/local intent and input routing for the owning connection. It is not durable player identity. |
| Pawn or vehicle | Replaceable physical avatar with its own entity identity and body-local state. It is not the player's identity. |

Steam, Epic, console, or another online identity maps to the OWS Player Profile
ID; it never replaces it. A multiplayer server derives the acting profile from
the authenticated connection rather than trusting an ID supplied in a gameplay
request.

Each local player has a distinct profile. A profile may associate with zero or
more persistent character entities, while one PlayerController possesses only
one primary avatar at a time. When an offline profile later maps to an account
that already has another established profile, OWS never silently merges or
deletes either one. The product presents an explicit choose, link, or audited
merge flow and retains the redirects and recovery information required by the
chosen result.

### Definition, entity, and scoped identity

OWS uses strongly typed identity categories that cannot be substituted for one
another accidentally.

- A **definition identity** describes what something is. Globally addressable
  definitions use Unreal Primary Asset identity.
- An **entity identity** is an OWS-owned GUID identifying one particular
  persistent character, vehicle, item, stack, collection, container, world
  object, or promoted City feature.
- A **scoped identity** is `{owner entity + typed category + stable local key}`.
  Seats, doors, interaction points, equipment slots, and comparable sub-parts
  use this form.
- A correlation or action request ID is transient operational identity and is
  not reused as persistent entity identity.

Pointers, Actor names, object paths, array positions, replication order, load
order, and display labels are not canonical identities. Identity is assigned by
the authoritative creator, importer, or editor workflow and is replicated and
persisted by its owning contract.

City source features preserve deterministic identities across regeneration and
map into the shared entity contract. A rename, generator change, or replacement
that cannot preserve identity requires an explicit redirect, migration,
tombstone, or compatibility epoch. Existing Sigil GUIDs can map into this model.
Save Extension name/class matching is a compatibility locator only.

A persistent entity ID is never reused. Destruction, consumption, or replacement
retires the affected identity. Respawn preserves the existing persistent
character entity ID unless that entity is explicitly destroyed and replaced;
any replacement receives a new ID. Split, merge, and crafting follow the rules
below. Tombstones, redirects, and lineage remain until checkpoint, reference,
migration, and recovery safety permits compaction; compaction never permits ID
reuse.

## Shared OWS Action Contract

Every participating gameplay domain uses one OWS action request, lifecycle,
result, and failure language.

An action can carry:

- a governed `OWS.Action.*` semantic tag;
- a unique request/correlation ID;
- authenticated instigator and current avatar identity;
- stable target identities;
- strongly typed, bounded context;
- optional expected authoritative revisions or other action-specific preconditions;
- optional declared prediction policy; and
- a stable machine-readable failure reason.

The common lifecycle can express `Requested`, `Validated`, `Started`,
`Committed`, `Completed`, `Rejected`, `Cancelled`, `Interrupted`, and `Failed`.
An atomic action need not manufacture unused intermediate states. Exact state
transition rules are declared by the action definition. Post-commit compensation
is not a package-wide default; an action-specific contract may define it only in
separately accepted scope and must preserve the action invariants.

Each action resolves to exactly one active executor.

- Gameplay Abilities is the default executor for work requiring duration,
  costs, cooldowns, effects, cancellation, interruption, tasks, or prediction.
- A simple atomic domain operation may use a lightweight executor, but it still
  enters and leaves through the OWS request/result contract.
- Continuous movement, camera, throttle, and steering use their specialized
  continuous-control paths rather than becoming gameplay abilities.
- Save operations, menus, and editor tooling remain outside GAS.

OWS owns the public contract. A consumer cannot make GAS internals, a Blueprint
delegate, a direct RPC, or a private foundation type the only public route for a
shared OWS action.

Failure results use governed `OWS.Failure.*` tags plus typed diagnostic context.
Localized text is selected by presentation policy; text is not the failure's
machine identity.

## Ability System Component ownership

OWS uses a two-host ASC model.

### Player ASC

The OWS PlayerState owns the player ASC and player-scoped abilities, effects,
tags, cooldowns, and attributes that must survive avatar changes. Possession
changes the ASC's active avatar. The newly active avatar grants its explicitly
declared ability sets, and those grants are tracked and removed when the avatar
relationship ends.

### Entity ASC

A persistent entity may own an entity ASC when it requires independent state
while uncontrolled, controlled by different players, or targeted by gameplay.
Vehicles, NPCs, hackable devices, and destructible entities are examples. An
entity that does not need abilities, effects, attributes, or tag state receives
no ASC.

Two ASCs may participate in one interaction, but each authoritative capability
or attribute has exactly one owner. Vehicle durability remains on the vehicle;
driver skills remain on the player. Health, fuel, cooldowns, or status cannot be
duplicated merely to make access convenient.

Possession must preserve the accepted grant/revoke and one-owner rules.
Out-of-order readiness and the exact initialization and teardown mechanism
remain scoped implementation decisions.

## GameplayTag governance

`OWS.*` is the governed public package vocabulary. Structural roots include
`OWS.Action`, `OWS.State`, `OWS.Status`, `OWS.Event`, `OWS.Failure`,
`OWS.Damage`, `OWS.Interaction`, `OWS.Inventory`, `OWS.Equipment`,
`OWS.Vehicle`, `OWS.Character`, `OWS.City`, and future assigned domains.

`Game.*` remains available for an adopting game's vocabulary. Domains own
approved descendants beneath their assigned roots but cannot create competing
package-wide roots.

Tags express semantics, categories, conditions, and relationships. They are not
canonical object identity, display text, or arbitrary payload storage. Public
roots and native contract tags are centrally registered and validated. Renaming
a public tag requires a redirect, compatibility interval, deprecation notice,
and removal policy.

Existing GASPALS, Sigil, and other foundation tags remain available through
documented compatibility mappings during migration. They do not become parallel
OWS authorities. Profiles using fast tag replication require identical client
and server dictionaries; the exact negotiation or rejection UX is a profile and
implementation decision.

## Gameplay events and messages

Public cross-domain events use `OWS.Event.*` tags and strongly typed, versioned
payloads. The envelope can carry event and correlation identity, stable source
and target identities, a related action ID, authoritative time/sequence context,
and intended audience.

Unreal's Gameplay Message runtime is the target default in-process provider
behind the OWS-owned contract. The current project does not yet declare that
provider. #127 must add and package the required Unreal Engine plugin/module as
part of the one-bundle OWS dependency set; if it is unavailable on a supported
Engine line, implementation returns for architecture review rather than silently
substituting another public contract. Component-local delegates may remain
private implementation details; cross-domain consumers use the OWS event
contract.

Events report occurrences. They are not authoritative state.

- Replicated current state remains the source of truth.
- Late join and recovery use snapshots, not old RPC or message history.
- Cross-network delivery is explicit per event; the local bus is not assumed to
  replicate.
- Gameplay Cues are cosmetic and cannot control authoritative gameplay.
- Events are transient by default. Only an explicitly designated history,
  mission, audit, or persistence record is retained.

UI, audio, analytics, quests, tutorials, and diagnostics may observe events
without becoming gameplay authorities.

## Vitality, damage, healing, teams, and factions

OWS owns reusable package-wide primitives for vitality, damage, healing,
reactions, incapacitation, terminal state, teams, factions, and attitude
queries. Participation is optional; not every entity has health or affiliation.

For a standard playable body, health, incapacitation, and terminal state belong
to the persistent character entity ASC. PlayerState contains only genuinely
player-scoped attributes that must survive changing bodies or vehicles.

A damage or healing transaction can carry stable source, instigator, target,
and action identities; magnitude; governed type tags; hit/context data; and
authority/correlation information. GAS Gameplay Effects and executions are the
default path for changing vitality attributes. Authoritative instantaneous
damage is not client-predicted, although reversible presentation may be.

The shared affiliation query provides one consistent relationship answer to
Combat, AI, Hacking, Vehicle, missions, police/wanted behavior, UI, and future
living-world systems. An entity may have one optional operational team and zero
or more faction memberships. Data-defined faction relationships are directed.
Explicit personal or context overrides take precedence through one deterministic
resolver; domains may extend data without creating a competing resolver.

Domains retain their rules:

- Combat owns weapons, ballistics, combat calculations, resistances, combat hit
  reactions, death presentation, and combat-specific rules.
- Vehicle owns conversion of impacts and mechanical failures into vehicle
  consequences.
- Hacking owns device-specific validation and consequences.
- Other domains own their corresponding policies.

Those policies consume shared primitives and cannot create competing
package-wide health, damage, death, team, faction, or attitude authorities.

## Inventory, equipment, and hotbar

### Generalized inventory core

The complete included Sigil Inventory fork becomes OWS's maintained default
inventory and equipment foundation. It is not a separate installation, demo
add-on, or source for cherry-picked features. Attribution remains at the
included plugin/package boundary while the fork evolves under OWS contracts.

The OWS-facing container model supports player, NPC, character, vehicle, trunk,
chest, shop, world-loot, crafting, currency, and future storage owners.
Player-owned inventory follows PlayerState/profile lifetime; entity-owned
storage remains with its persistent owner.

Definitions use definition identity. Items, stacks, collections, containers,
and other persistent instances retain exact entity identities. Inventory
mutation is an atomic, server-authoritative OWS action with validation,
revision/precondition checks, contention handling, and machine-readable results.

Replication and persistence round-trip exact item, stack, collection, container,
slot, quantity, currency, and equipment state. Deserialization cannot silently
generate replacement IDs, select substitute slots, or discard unknown durable
state.

Full container contents and currency replicate only to the server and authorized
viewers. Equipment manifestations and deliberately public summaries may
replicate to observers. Every container declares its access policy; proximity or
replication relevance alone never grants inspection authority.

On a split, the source stack keeps its identity and the new child receives a new
identity. On a merge, the destination keeps its identity and the consumed stack
is tombstoned. Crafting creates new output identities and records input lineage
unless the recipe explicitly declares an in-place transformation.

### Equipment

The durable inventory item instance remains authoritative for ammunition,
durability, modifications, quantities, and custom fragment data. A durable
equipment assignment references the item, an explicit equipment host, a
loadout/context, and a scoped slot.

Visible weapons, clothing, tools, or mounted Actors/components are derived
manifestations. They can be destroyed and rebuilt without changing item
identity or durable state. Equipment grants abilities/effects to an explicitly
designated player or entity ASC, records every granted handle, and removes those
grants cleanly.

Possession changes tear down old-context manifestations and rebuild only those
valid for the new avatar. Incompatible equipment remains safely owned and
returns a failure result. Drop or transfer of equipped equipment is one atomic
unequip-and-transfer transaction. Saves store durable item state and equipment
assignments, not temporary manifestations.

### Hotbar

The OWS hotbar is a host-neutral modular slot and routing backend. It does not
inherently belong to a player, inventory, Pawn, vehicle, UI, or another consumer.
A developer explicitly chooses:

- what owns or hosts each instance;
- what its slots reference;
- what input or system drives it;
- what presentation consumes it; and
- whether and how its configuration persists.

The backend may route inventory, equipment, actions, abilities, developer
functionality, or other systems without becoming their authority. Sigil's
inventory-specific QuickBar may integrate through the generic hotbar contract;
it cannot redefine the generic backend as player-owned inventory. The current
placeholder/demo hotbar implies no ownership or completed gameplay wiring.

## Authority, replication, prediction, and contention policy

Server-authoritative mutation, explicit prediction, replicated current state,
and the detailed policies below are the accepted Stage 2 implementation
contract.

### Client intent and server commit

Clients submit intent, never authoritative outcomes. The server derives player
identity from the authenticated connection and validates ownership, possession,
avatar readiness, action availability, tags, costs, cooldowns, target identity,
range, line of sight, permissions, rate limits, expected revisions, and domain
invariants before committing a mutation.

Request IDs make discrete actions idempotent inside their declared retention
window. Within that window, a duplicate returns the retained result rather than
executing again. After expiry, the original result is not guaranteed and the
caller reconciles from authoritative current state. Actions whose replay risk
outlives ordinary retention use a durable domain idempotency record. A result
carries lifecycle outcome, failure tag, authoritative revisions, and required
reconciliation data.

Discrete committed actions default to reliable delivery semantics unless their
declared action policy proves another mode. Payload size, request rate,
idempotency retention, and queues are bounded; delivery time is not guaranteed.
Continuous movement, steering, camera, and throttle use specialized
high-frequency paths. RPCs and transient events do not replace replicated
current state.

### Prediction tiers

| Tier | Permitted behavior |
| --- | --- |
| None | Inventory/equipment ownership, seat occupancy, instantaneous damage, economy, persistent hacking consequences, saves, and world mutations remain server-only. |
| Cosmetic | Animation, audio, camera, UI, and other disposable owner-facing feedback may begin immediately and be cancelled or corrected. |
| Reversible gameplay | Only an action explicitly designed, tested, and certified for GAS prediction and rollback may predict gameplay state. |

Rejected prediction rolls back cleanly. Domain-specific smoothing may present
authoritative correction without hiding disagreement. No predicted client state
can authorize another mutation.

### Contention

Mutable records carry authoritative revisions or equivalent preconditions.
Contention is resolved atomically on the server. The declared domain commit order and
any domain fairness/queue policy are observable and deterministic for the same
authoritative inputs; a client cannot declare itself the winner.

The losing request receives `OWS.Failure.Conflict` or a more specific descendant,
the current revision, and an explicit retryability result. Long-lived locks may
not span client waits, asset loads, or unrelated asynchronous work. Multi-owner
transactions use a declared coordinator and either commit every participating
mutation or none.

### Replication backend

OWS public APIs remain replication-backend neutral. Current-state structures
group values whose ordering is semantically related, and late join never relies
on `OnRep` ordering across unrelated properties. Collection order uses explicit
slot/order identity rather than Fast Array arrival order.

The OWS public contract remains backend-neutral. OWS initially certifies every
advertised networked feature under both legacy replication and opt-in Iris.
Neither backend becomes a gameplay authority, and neither is advertised for a
feature until its complete OWS matrix passes.

## Travel, late join, disconnect, reconnect, and restart

Snapshot-first late join, durable profile identity, checkpoint-backed recovery,
and the live-session defaults below are accepted policy.

- Seamless travel preserves the live player projection where supported and
  revalidates avatar, ability grants, inventory views, and domain readiness.
- Non-seamless travel reconstructs the new live projection from the
  authoritative session/profile record rather than retaining stale object
  pointers.
- A late joiner receives current authoritative snapshots for relevant players,
  occupancy, entities, inventories, actions, City mutations, and world state.
  Historical transient events are not replayed as state.
- A disconnected profile may retain a configurable server-owned grace record.
  Reconnect requires the same authenticated profile and reattaches only after
  ownership and state validation. Eligible same-process session state and prior
  possession may resume only when still valid and authorized.
- A second active connection for the same profile is rejected. An
  authenticated recovery may reclaim a connection already marked stale or
  disconnected; it never creates simultaneous control.
- Transient in-flight actions cancel by default. A domain may resume an action
  only from an explicit versioned resumable-action record.
- A server restart restores the last committed checkpoint into a fresh session.
  Durable profile and world state survive; uncommitted actions, prior possession,
  temporary session state, and cosmetic prediction do not resurrect.
- Reconnect never relies only on Unreal's inactive PlayerState behavior. OWS
  persistence bridges live PlayerState lifetime to durable profile and world
  records.

Grace duration and the presentation of rejection, recovery, and possession
failure remain configurable. They cannot weaken the one-live-controller,
validated-reclaim, or committed-state requirements.

## Persistence lifetimes

Every state category explicitly declares exactly one controlling lifetime.
Attachment to a player, Pawn, component, hotbar, or UI does not infer ownership
or persistence.

| Lifetime | Ownership contract |
| --- | --- |
| Device settings | Installation-local graphics, audio, accessibility, controls, cache preferences, and explicitly device-owned data. |
| Player profile | Permanent profile identity and only data explicitly declared player-owned and durable. |
| Session | Live server-authoritative possession, active action, temporary effect, match, and reconnect-grace data. Normally temporary. |
| World/server | Persistent entity and world mutations, including City-feature deltas, vehicles, containers, missions, economy, and game-defined world records. |
| Disposable cache | Regenerable performance and visual-enhancement data. Never profile or save truth. |

PlayerState is a live projection rather than permanent storage. Offline data may
use a local provider. Networked player/world authority uses the server or a
trusted backend and never trusts client-submitted inventory, progression, or
world mutation.

A user-facing save slot can present multiple records together but does not erase
their ownership or schema boundaries. A checkpoint references mutually
compatible revisions. Cloud stores and external databases are optional provider
implementations behind the same public contract.

## Save Extension and record architecture

The complete included Save Extension fork becomes OWS's maintained default
low-level persistence engine. OWS evolves and hardens it instead of building a
competing engine beside it.

The public OWS Persistence contract owns identities, lifetimes, schemas,
providers, checkpoints, migration, and recovery. Save Extension supplies and
evolves lower-level serialization, Actor/component capture, asynchronous work,
compression, and storage mechanics.

Authoritative OWS domains expose explicit versioned records. Generic `SaveGame`
reflection remains available for developer-defined compatible data but cannot
replace domain schemas. Canonical restoration uses accepted identities;
Actor-name/class matching is a compatibility path only.

World Partition, logical City cells, Actor promotion, dedicated-server
authority, profile/session restoration, and reconnect each require explicit
integration. Local storage writes use checksum verification, same-volume
temporary output, validation, atomic replacement, and recoverable previous
generations. A multiplayer client cannot write authoritative world or
server-owned profile state.

## Snapshots, journals, and checkpoints

- Small records such as device settings and ordinary profiles use atomic
  versioned snapshots.
- A large world uses a global record plus independently loadable domain/cell
  shards keyed by stable OWS and City identities.
- Cross-cell vehicles, NPCs, and other persistent entities retain identity and
  durable ownership independently of their current spatial cell. Their scoped
  implementation issue selects the storage and spatial-index representation;
  cell membership never becomes entity identity.
- Each shard maintains a verified snapshot and a bounded append-only change
  journal.
- Journals enable incremental autosave and crash recovery. Compaction creates a
  new verified snapshot and retires incorporated entries.
- The journal is not permanent event sourcing; validated current state remains
  authoritative.
- A checkpoint manifest coordinates profile, world, domain, and shard revisions.
  A partial checkpoint never becomes visible or loadable.
- Unloaded regions can be persisted from records without spawning every Actor.
  Internal World Partition runtime-cell IDs do not become save identity.
- State capture occurs at a safe authoritative synchronization point.
  Serialization, compression, checksum generation, and provider I/O operate
  asynchronously on immutable captures.
- Autosave frequency, capture/I/O budgets, journal bounds, compaction thresholds,
  backup retention, and storage limits are configurable.

Single-player, listen-server, and dedicated-server profiles consume the same
model at appropriate scale.

## Schema, migration, missing content, and recovery

Every record declares its OWS format version, domain schema version, checkpoint
identity, composition/content manifest, relevant generator or recipe epoch, and
integrity checksum.

Migration is explicit, deterministic, testable, version-to-version, and
copy-on-write. The last verified generation remains untouched until the
candidate replacement passes every required validation and commits atomically.
An unsupported newer required schema refuses safely instead of being guessed.

Missing definitions never cause silent deletion or substitution. Their stable
records remain unresolved and recoverable until content returns or an explicit
redirect, tombstone, or migration resolves them. Data from an unavailable
optional domain may be retained as checked, inert opaque data when safe; it
cannot affect gameplay while its owner is unavailable.

Loading validates envelope, checksum, version, migration, references, and domain
invariants before state becomes visible. An independently corrupt optional
shard may be quarantined only when its domain declares and validates a safe
fallback. Corruption affecting identity, ownership, inventory transactions,
checkpoint consistency, or another required invariant rolls back the complete
coordinated checkpoint.

Recovery searches verified generations newest to oldest and reports what it
restored. If none is valid, OWS reports a recoverable failure with repair/export
paths. It never silently starts a new game over damaged data.

## City Foundation coordination

The accepted City contract remains controlling for import, source provenance,
recipe epochs, logical `CityCellID`, generated-feature identity, Actor
promotion, and cache behavior. Stage 2 supplies the shared player/entity/action,
server, and persistence semantics used at the boundary.

- City stable feature identity maps into OWS entity identity without becoming
  an Actor name or internal World Partition cell ID.
- World mutations are server-owned deltas keyed by stable feature identity and
  recipe epoch.
- A promoted Actor and its unloaded record are two representations of one
  entity, never duplicate authorities.
- City save shards can align with logical City cells while global or cross-cell
  transactions remain coordinated by the checkpoint manifest.
- The writable City enhancement cache is untrusted, disposable, and separate
  from profiles, saves, collision, navigation, economy, quests, and authoritative
  placement.
- Deleting all writable City cache data cannot change authoritative restore,
  travel, vehicle, inventory, or multiplayer results.

## UI notification and observability boundary

Gameplay domains emit machine-readable action results, state, and events. They
do not choose permanent widgets or call ad hoc on-screen debug output as their
public contract.

The OWS notification policy maps those semantics into replaceable presentation:

- audience and owning local player;
- severity and channel;
- localization key and typed arguments;
- priority, lifetime, deduplication, and correlation;
- accessibility alternatives; and
- optional developer detail excluded from shipping presentation.

Prompts, HUD feedback, toasts, modal errors, logs, audio, and accessibility
output can consume the same semantic source without becoming gameplay authority.
Dedicated servers route diagnostics without constructing player UI.

OWS ships both the reusable notification router/contract and a working,
accessible default presentation. Notifications are transient by default. A
durable inbox, acknowledgment, or history exists only for records explicitly
designated by their owning domain.

The OWS observability surface uses structured categories, governed event/failure tags,
stable redacted identities, correlation IDs, authority role, timestamps, and
relevant revisions. Debug overlays, Unreal Insights traces, logs, and test
evidence consume that structure. High-frequency telemetry is sampled and
budgeted; secrets and external account identifiers are never emitted casually.

Diagnostics are local and privacy-preserving by default. OWS performs no
external transmission without explicit developer integration and an applicable
player-consent policy. Shipping diagnostics are disabled by default; a developer
may explicitly enable bounded retention and sampling for a particular product.

## Security and defensive limits

- Client requests, save files, cache data, and provider responses are untrusted
  until validated by their controlling boundary.
- Payload type, size, count, numeric range, tag, definition, and identity inputs
  are bounded and allowlisted where applicable.
- Clients cannot supply arbitrary class or package paths for authoritative spawn
  or deserialization.
- Checksums detect accidental corruption. Providers requiring tamper resistance
  add authentication or signatures; a checksum is not treated as proof of trust.
- Rate limits and idempotency bounds prevent replay or request-table exhaustion.
- Failed validation cannot partially mutate gameplay, inventory, possession,
  persistence, or checkpoint visibility.

## Accepted product policies

The shared spine uses these accepted OWS defaults. Adopting projects may expose
configuration only where doing so preserves the controlling invariants.

| Topic | Accepted OWS default |
| --- | --- |
| Profile/account reconciliation | Never silently merge two established OWS profiles. Present an explicit choose, link, or audited merge workflow; retain redirects and recovery. Each local player has a distinct profile. |
| Playable-character vitality | Body health, incapacitation, and terminal state live on the persistent character entity ASC. PlayerState holds only genuinely player-scoped attributes that must survive changing bodies or vehicles. |
| Profile and character relationship | One profile may associate with zero or more persistent characters. Each PlayerController has one primary possessed avatar at a time. |
| Affiliation resolution | One optional operational team, zero or more faction memberships, directed data-defined faction relationships, then explicit personal/context overrides through one deterministic resolver. |
| Replication baseline | Keep OWS public contracts backend-neutral and initially certify supported features under both legacy replication and opt-in Iris before advertising either profile. |
| Reconnect and duplicate profiles | A same-process reconnect may resume explicitly eligible state and valid possession. Server restart restores durable state into a fresh session. A second active same-profile connection is rejected; authenticated recovery may reclaim only a stale or disconnected connection. |
| Inventory disclosure | Container contents and currency replicate only to the server and authorized viewers. Equipment manifestations and deliberately public summaries may replicate to observers. Each container declares its access policy. |
| Split, merge, and crafting identity | Split retains the source stack and creates one new identity; merge retains the destination and tombstones the consumed stack; crafting creates new output identities and records input lineage unless a recipe explicitly declares an in-place transformation. |
| Persistent entity retirement | Entity IDs are never reused. Destruction and replacement are explicit records; tombstones, redirects, and lineage remain until safe compaction. |
| Notification product surface | Ship both the reusable notification router/contract and a working accessible default UI. Notifications are transient by default; durable inbox or acknowledgment behavior exists only for explicitly designated records. |
| Observability and privacy | Ship bounded structured local diagnostics with privacy-preserving identifiers. No external transmission occurs without explicit developer integration and an applicable player-consent policy. Shipping diagnostics are disabled by default and may be explicitly enabled with bounded retention and sampling. |

Default save-slot UX, local-save tamper policy, exact reconnect grace durations,
the shipped set of cloud/database providers, compression/checksum algorithms,
shard sizes, journal thresholds, backup counts, and performance budgets remain
later scoped product or tuning decisions. Implementations may not guess those
defaults merely because this architecture is published.

## Conformance matrix

The implementation acceptance matrix requires automated coverage where deterministic and
manual evidence where real input, presentation, or platform behavior must be
observed.

| Area | Required evidence |
| --- | --- |
| Identity | Player identity survives character/vehicle possession, respawn, travel, live reconnect, and checkpoint restore; zero, one, and many character associations preserve one primary avatar per Controller; entity and scoped IDs remain stable across regeneration and streaming; retired IDs are never reused and tombstone/lineage compaction preserves reference safety. |
| Actions | Success, rejection, cancellation, interruption, duplicate request, and timeout preserve one mutation and one terminal result. Any action that declares compensation must prove its own invariant-preserving path. |
| ASC lifecycle | Out-of-order initialization, avatar swaps, grant/revoke, entity targeting, and teardown leave one owner for every attribute and no leaked ability grants. |
| Tags/events | Registry and redirect validation passes; client/server dictionaries match; events never substitute for late-join state. |
| Vitality/affiliation | Server-owned damage/healing and team/faction queries agree for players, AI, vehicles, and participating world entities. |
| Inventory | Concurrent pickup, transfer, split, merge, shop, crafting, currency, equip, drop, and rollback preserve the accepted surviving/new/tombstoned identities, quantity, slots, revisions, and atomicity; unauthorized viewers cannot receive private contents or currency. |
| Equipment/hotbar | Manifestations rebuild without duplication; grants revoke; hotbar instances remain host-neutral and only persist when explicitly configured. |
| Prediction | Each certified predicted action passes accept, reject, correction, packet loss, latency, and rollback tests; non-predictable mutations never execute early. |
| Replication backends | Every advertised networked feature passes equivalent authority, current-state, late-join, and correction evidence under both legacy replication and opt-in Iris before either profile is advertised. |
| Network roles | Standalone, listen host, remote listen client, dedicated server, two or more clients, late join, disconnect/reconnect, second-active-profile rejection, stale/disconnected reclaim, and restart pass the same authority rules. |
| Persistence | Profile and world snapshots, journals, compaction, cross-record checkpoints, unloaded-cell restore, cross-cell persistent-entity movement, server restart, and provider failure preserve identity and ownership invariants. |
| Migration/recovery | Supported upgrade chains, unsupported future data, redirects, missing content, corrupt optional shards, corrupt required records, rollback, and repair/export paths are exercised. |
| City boundary | Feature promotion/demotion, cell unload/load, recipe migration, full cache deletion/corruption, fast travel, and dedicated-server restore remain authoritative without cache data. |
| UI/observability | The accessible default UI and replaceable router map semantic results; transient and explicitly durable notification behavior remain distinct; dedicated servers remain UI-free; diagnostic correlation is complete; no external transmission occurs without explicit integration and applicable consent; Shipping diagnostics produce no output by default and remain bounded after explicit developer enablement. |
| Performance | Replication, save capture, serialization, journal growth, compaction, late join, and high-frequency diagnostics stay inside declared budgets. |

No multiplayer mode, persistence provider, prediction tier, or recovery guarantee
is advertised until its complete matrix passes on a recorded commit and target
platform.

## Migration and contributor gates

- This contract authorizes dependency-ordered implementation planning, not an
  unscheduled repository-wide rewrite. Gameplay implementation requires a
  separately ready, claimed issue.
- Every migration begins with current behavior and dependency baselines.
- Working behavior remains available through compatibility adapters or redirects
  until its OWS replacement passes equivalent and new conformance coverage.
- A consumer issue may not create a substitute ID, action lifecycle, ASC owner,
  inventory authority, save format, tag root, event bus, or notification model.
- Out-of-scope defects become separate issues. They are not repaired while
  implementing another Stage 2 child.
- Current public behavior is retired only through an explicitly claimed issue
  with evidence, redirects or migration, and Aurora's accepted scope.

## Accepted implementation program

Implementation proceeds through separate children of epic #104. Readiness
remains gated by the Stage 1 migration sequence and any required neutral
interfaces from #116.

| Order | Child issue | Scope and exit evidence |
| --- | --- | --- |
| 1 | [#125 — Stable identity foundation](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/125) | Strong profile/definition/entity/scoped identity types, serialization/net serialization, registries, redirects, tombstones, and legacy locators. Prove rename, replication, save, and deterministic City mapping. |
| 2 | [#126 — Player profile–PlayerState bridge](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/126) | Authenticated profile mapping and live PlayerState projection across possession, respawn, travel, disconnect, and restore. No durable authority leaks into Controller or Pawn. |
| 3 | [#127 — Shared action, tag, event, and GAS core](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/127) | OWS request/result types, executor registration, governed tags, message transport, player/entity ASC lifecycle, ability-set grants, and compatibility mappings. |
| 4 | [#128 — Vitality and affiliation primitives](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/128) | Shared optional attributes, damage/healing transaction, terminal-state and relationship interfaces, plus adapters for the selected affiliation policy. |
| 5 | [#129 — Generalized Sigil inventory transactions](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/129) | Evolve the complete fork for OWS identities, player/entity containers, atomic actions, revisions, contention, authorized replication, and all inherited capability families. |
| 6 | [#130 — Durable equipment assignments and manifestations](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/130) | Equipment assignments, manifestations, ASC grant cleanup, and atomic equipped transfer. |
| 7 | [#131 — Host-neutral hotbar backend](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/131) | Generic hotbar hosting, slots, routing, persistence policy, and the Sigil QuickBar adapter. Input adaptation remains with #117 and presentation remains with #119. |
| 8 | [#132 — Persistence envelope and provider contract](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/132) | Lifetimes, record/schema registry, immutable capture, revisions, manifests, checksums, provider authority, and coordinated checkpoint surface. |
| 9 | [#134 — Save Extension hardening](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/134) | Stable-ID adapters, correct server/client gates, validated atomic replacement, prior generations, async local provider, and generic compatible `SaveGame` support. |
| 10 | [#135 — Lossless inventory/equipment persistence](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/135) | Exact containers, items, stacks, slots, currency, fragments, assignments, unresolved definitions, and version migrations with no regenerated IDs. |
| 11 | [#136 — World shards, journals, and compaction](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/136) | Global/domain/logical-cell snapshots, cross-cell entity records, bounded journals, deterministic replay, partial-checkpoint invisibility, unloaded-region operation, and budgeted compaction. |
| 12 | [#137 — Migration, quarantine, rollback, and repair/export](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/137) | Copy-on-write migration, unsupported-newer refusal, opaque optional data, missing definitions, corruption fault injection, verified-generation recovery, and no silent new game. |
| 13 | [#138 — Multiplayer late join, reconnect, and restart](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/138) | Shared authority request path, current-state snapshots, live reconnect, duplicate-profile policy, server restart, and no duplicated ownership across all network roles. |
| 14 | [#139 — World Partition and City persistence adapter](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/139) | Map accepted City identities, recipe epochs, promotion state, logical `CityCellID` shards, and feature/entity deltas into shared persistence while excluding writable cache from saves. City remains owner and implementer of promotion/demotion through #133. |
| 15 | [#140 — Notification routing and observability](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/140) | Machine result/event routing, structured correlation, privacy policy, dedicated-server diagnostics, and bounded opt-in Shipping behavior. The accessible default presentation remains owned by #119. |
| 16 | [#141 — Stage 2 conformance harness](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/141) | Automation, functional, network multi-process, fault-injection, migration fixtures, performance budgets, accessible-default-UI evidence from #119, City-promotion evidence from #133, and the complete matrix in this document. |

Existing consumers must link to this contract and their required children:

- #147, #27, and #28 consume shared identity, scoped seat/door/surface/anchor
  identity, action authority, multi-owner transactions, contention, and network
  conformance for Character–Vehicle Mobility relationships and transitions.
- #30–#40 consume shared actions, GAS, vitality, affiliation, inventory,
  equipment, persistence, notification, and multiplayer contracts.
- #50–#59 consume shared identity, action validation, costs/cooldowns,
  affiliation, events, persistence, and authority without creating a private
  hacking spine.
- #85, #96, and #100 publish and test the City identity, promotion, persistence,
  and cache boundaries consumed by the Stage 2 City adapter.
- #147 coordinates compatible Character #107 and Vehicle #108 contracts plus
  their dynamic Mobility boundary; #110 selects Living World rules. None may
  reopen or privately replace the shared spine.
- #117 owns semantic Input adapters and may reference the host-neutral hotbar
  contract. #119 owns UI presentation and may present hotbar state. Neither owns
  the backend's routed gameplay authority.

## Primary Unreal references

- [Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine)
- [APlayerState](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/APlayerState)
- [Gameplay Ability System](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-the-unreal-engine-gameplay-ability-system)
- [Abilities in Lyra](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)
- [Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine)
- [Lyra Inventory and Equipment](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-inventory-and-equipment-in-unreal-engine)
- [Remote Procedure Calls](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-procedure-calls-in-unreal-engine)
- [Replicated Object Execution Order](https://dev.epicgames.com/documentation/en-us/unreal-engine/replicated-object-execution-order-in-unreal-engine)
- [Saving and Loading](https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine)
- [Asset and Package Versioning](https://dev.epicgames.com/documentation/en-us/unreal-engine/versioning-of-assets-and-packages-in-unreal-engine)
- [World Partition](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine)
