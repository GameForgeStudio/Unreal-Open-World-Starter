# Open World Starter Documentation

This directory documents the current OWS implementation, accepted target architecture, and acceptance standards.

## Documents

- [STACK.md](STACK.md) — current package structure and runtime ownership.
- [OWS_PLATFORM_ARCHITECTURE.md](OWS_PLATFORM_ARCHITECTURE.md) — accepted Stage 1 product layers, maintained-fork model, plugin families, dependency and ownership matrix, extension contract, conformance rules, and migration backlog.
- [OWS_GAMEPLAY_SPINE_ARCHITECTURE.md](OWS_GAMEPLAY_SPINE_ARCHITECTURE.md) — accepted Stage 2 identity, action, GAS, inventory/equipment, authority, networking, persistence, recovery, notification, and conformance contract.
- [OWS_ARCHITECTURE_ROADMAP.md](OWS_ARCHITECTURE_ROADMAP.md) — accepted product shape, five-stage architecture sequence, checkout gates, and later-stage decisions.
- [OWS_MOBILITY_PROGRAM_CHARTER.md](OWS_MOBILITY_PROGRAM_CHARTER.md) — accepted Stage 3 Mobility selection, current baseline, Character/Vehicle/Integration ownership split, physical principles, capability envelope, and architecture-program gates.
- [OWS_MOBILITY_ARCHITECTURE.md](OWS_MOBILITY_ARCHITECTURE.md) — accepted Stage 3 Character, Vehicle, and narrow Integration ownership, physical, authoring, transition, authority, presentation, persistence, extension, and support-claim contract.
- [OWS_CHARACTER_ARCHITECTURE.md](OWS_CHARACTER_ARCHITECTURE.md) — accepted complete OWS Character-domain target architecture and independent conformance contract for issue #107.
- [OWS_VEHICLE_ARCHITECTURE.md](OWS_VEHICLE_ARCHITECTURE.md) — accepted complete OWS Vehicle/driving-domain target architecture and independent conformance contract for issue #108.
- [OWS_CHARACTER_VEHICLE_DOMAIN_MATRIX.md](OWS_CHARACTER_VEHICLE_DOMAIN_MATRIX.md) — accepted cross-domain ownership, dependency-direction, and reconciliation matrix joining the independent Character and Vehicle contracts without duplicating Mobility.
- [OWS_MOBILITY_ACCEPTANCE_MATRIX.md](OWS_MOBILITY_ACCEPTANCE_MATRIX.md) — accepted shared result-record/T0–T5 envelope and complete composed-Mobility matrix; detailed independent Character/Vehicle support lives in the `CHAR-*`/`VEH-*` contracts above.
- [OWS_MOBILITY_IMPLEMENTATION_BACKLOG.md](OWS_MOBILITY_IMPLEMENTATION_BACKLOG.md) — reconciled, architecture-gated issue map and dependency order for Character, Vehicle, Integration, shared-domain adapters, conformance, and deferred cross-Vehicle work.
- [OWS_MOBILITY_EVIDENCE_BASELINE.md](OWS_MOBILITY_EVIDENCE_BASELINE.md) — issue #147 current-runtime audit, Unreal 5.8 and physics research, comparable-product evidence, and closed Aurora decision record; it is non-normative evidence for the accepted Mobility architecture.
- [CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md) — accepted real-world city import, terrain conformance, World Partition, procedural generation, and adaptive cell-cache contract.
- [INTERACTION_TARGETING.md](INTERACTION_TARGETING.md) — current selector, precision targeting, Activate, interaction-point, and authority-boundary contract.
- [CONTROLS.md](CONTROLS.md) — verified keyboard, mouse, controller, hotbar, and vehicle controls.
- [CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md](CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md) — protected current-runtime Character/Vehicle regression scenarios and evidence rules; future Mobility support uses the target Mobility matrix.
- [TESTING.md](TESTING.md) — automated OWS test commands, prerequisites, and expected results.
- [../README.md](../README.md) — setup, requirements, and contribution links.

## Source of truth

When documents disagree, use this order:

1. Aurora's latest explicit requirement.
2. The accepted contract applicable to the subject and within that contract's explicit scope, including `OWS_PLATFORM_ARCHITECTURE.md` for package-wide Stage 1 rules, `OWS_GAMEPLAY_SPINE_ARCHITECTURE.md` for Stage 2 shared gameplay semantics, `OWS_MOBILITY_PROGRAM_CHARTER.md` for the selected Stage 3 program and design constraints, `OWS_MOBILITY_ARCHITECTURE.md` and `OWS_MOBILITY_ACCEPTANCE_MATRIX.md` for the controlling Mobility target and proof, `OWS_CHARACTER_ARCHITECTURE.md` and `OWS_VEHICLE_ARCHITECTURE.md` for the complete independent domains, `OWS_CHARACTER_VEHICLE_DOMAIN_MATRIX.md` for their shared seam, and `CITY_FOUNDATION_ARCHITECTURE.md` for City Foundation target behavior.
3. Current source, assets, and project configuration for what is implemented today.
4. `STACK.md` for the documented current runtime architecture.

No accepted platform, shared, or subsystem contract silently overrides another accepted contract. An overlap or conflict returns to Aurora for an explicit decision. Target architecture governs future design within its scope; current implementation remains the evidence for what exists today. Neither may be presented as the other.
