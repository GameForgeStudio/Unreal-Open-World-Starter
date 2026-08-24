# Open World Starter Documentation

This directory documents the current OWS implementation, accepted target architecture, and acceptance standards.

## Documents

- [STACK.md](STACK.md) — current package structure and runtime ownership.
- [CITY_FOUNDATION_ARCHITECTURE.md](CITY_FOUNDATION_ARCHITECTURE.md) — accepted real-world city import, terrain conformance, World Partition, procedural generation, and adaptive cell-cache contract.
- [CONTROLS.md](CONTROLS.md) — verified keyboard, mouse, controller, hotbar, and vehicle controls.
- [CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md](CHARACTER_VEHICLE_ACCEPTANCE_MATRIX.md) — repeatable character/vehicle scenarios, coverage tiers, evidence rules, and the release grid.
- [TESTING.md](TESTING.md) — automated OWS test commands, prerequisites, and expected results.
- [../README.md](../README.md) — setup, requirements, and contribution links.

## Source of truth

When documents disagree, use this order:

1. Aurora's latest explicit requirement.
2. The subsystem's accepted contract, including `CITY_FOUNDATION_ARCHITECTURE.md` for City Foundation target behavior.
3. Current source, assets, and project configuration for what is implemented today.
4. `STACK.md` for the documented current runtime architecture.
