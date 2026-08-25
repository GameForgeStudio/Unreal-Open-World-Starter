# OWS interaction targeting and activation contract

## Purpose

OWS provides one reusable world-target selector and one Activate path. Vehicles,
characters, pickups, hacking objects, and future gameplay systems must consume
this foundation rather than polling their own selection buttons or building a
second player-side detector.

This contract proves current local targeting and Activate dispatch. It does not implement the shared server-authoritative action spine defined by the accepted [Stage 2 contract](OWS_GAMEPLAY_SPINE_ARCHITECTURE.md). The accepted [Stage 1 platform contract](OWS_PLATFORM_ARCHITECTURE.md) assigns selection and activation to the OWS Interaction domain and semantic input routing to OWS Input. Their current project-module placement describes `main`, not the accepted target or permission to move them outside a scoped migration issue.

## Passive awareness detection

`UOWSSelectorComponent` owns an ordered, configurable range-selector stack. The
supplied `Activate` stack contains a 125 cm reach orb, a 15 m / 90-degree short
cone, and a 40 m / 50-degree long cone. Each entry independently exposes its
enabled state, dimensions, facing requirement, and detectable collision object
types. At least one entry must remain enabled.

The range volumes are visible while authoring and hidden during play. Their only
runtime responsibility is passive awareness: they collect enabled interaction
targets that a presentation system can outline or otherwise identify. They do
not choose the bottom-right target and do not gate Activate. Outline rendering
and category-to-color styling are separate presentation consumers of this list.

## Precision targeting

A separate, configurable precision ray follows the character's head/view
direction, or the camera aim direction while L2 is held. It defaults to 40 m
and independently exposes its detectable collision object types. No range orb
or cone must overlap an object for the ray to resolve it. No additional
bounding-box-corner rays run during selection or activation.

Ordinary collidable actors may be resolved and shown by the bottom-right debug
readout. An object becomes activatable only by owning an enabled
`UOWSInteractionTargetComponent` resolved on the precision-ray hit actor.
When one actor owns multiple interaction points, the point nearest the precision
ray's impact is the resolved target.

## Interaction points

`UOWSInteractionTargetComponent` is an editor-configurable scene component with:

- A stable local-key `InteractionId`. In the accepted Stage 2 target, the canonical interaction-point identity is scoped as `{owner entity + interaction-point category + InteractionId}`.
- An optional readable display name.
- An enabled state.
- A configurable selection radius.
- An `OnActivated` Blueprint event.

An actor may own any number of independently positioned interaction points. An
actor or one of its components may implement `IOWSInteractionTargetHandler` to
provide availability checks and target-specific activation behavior. Targets
without a handler remain usable through their Blueprint `OnActivated` event.

## Activate result and failure behavior

`F` and `Square / X` invoke `UOWSSelectorComponent::ActivateCurrentTarget`.
Activate never searches for another object or interaction point. It consumes
the currently resolved point exactly once.

The point first evaluates its enabled state and the handler's
`CanActivateOWSTarget` result. If accepted, it calls `ActivateOWSTarget`, emits
its `OnActivated` event, and the selector emits `OnActivateTarget`. If no point
is resolved, availability fails, or behavior rejects the request, the selector
does not fall through to another target and emits `OnActivateTargetFailed` with
a readable reason.

## Vehicle doors

Every authored vehicle door becomes its own interaction point. The point uses
the existing door definition's relative transform and Door ID; this migration
does not change any entry or exit transform. Selecting the driver-side point
passes that exact Door ID through Activate to vehicle entry. The gameplay path
does not run the legacy vehicle-wide overlap/facing search after activation.

This selector contract chooses the requested interaction point; it does not decide whether a moving boarding, passenger, grip, infiltration, or transfer action is physically or authoritatively valid. The selected [OWS Mobility program](OWS_MOBILITY_PROGRAM_CHARTER.md) and issue #147 own relative point motion, approach, clearance, capability, occupancy/control role, reservation, interruption, and transition rules. Current `main` still accepts only the control-seat handoff described by the existing implementation and must not be presented as supported moving entry.

The nearest-vehicle search remains available only to the explicitly named
`DebugEnterNearestVehicle` automation/debug helper. It is not a player input
path.

## Authority boundary

Selection and focus are local presentation and produce an activation request,
not authority over world state. A networked mutation uses the accepted OWS
action request and scoped target identity. The server derives the profile from
the authenticated connection and revalidates target identity, availability,
distance, permissions, occupancy, and gameplay state before authorizing the
result. A client Actor pointer, supplied ID, or success event is not authority.

The exact shared request, validation, prediction, failure, and persistence semantics are controlled by the accepted [Stage 2 contract](OWS_GAMEPLAY_SPINE_ARCHITECTURE.md). Mobility consumers additionally follow [issue #147](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/147) and its eventual accepted matrix. Consumer systems may not treat this local selector contract as a substitute for either architecture.

The City Foundation follows the same boundary. A cached cosmetic instance is
never authoritative. If a generated city feature becomes interactive, OWS
addresses it through the city's stable feature ID and promotes its mutable state
to an authoritative replicated Actor or server record. Cache eviction or
regeneration must not erase or replace that gameplay state. See
[CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md).

OWS does not claim multiplayer verification for this selector task. Vehicle
RPCs, contention, and network execution are tracked by
[issue #27](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/27).

## Hacking consumers

- [Issue #51](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/51)
  defines hackable behavior behind an OWS interaction point/handler instead of
  defining another selector.
- [Issue #52](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/52)
  consumes the selector's resolved actor and interaction point for focus and
  highlighting.
- [Issue #53](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/53)
  supplies hacking availability, range, permission, and state validation through
  the interaction handler and server-authoritative gameplay layer.
