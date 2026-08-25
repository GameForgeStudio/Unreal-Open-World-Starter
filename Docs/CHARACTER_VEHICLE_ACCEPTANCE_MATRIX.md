# OWS Character and Vehicle Acceptance Matrix

This document defines the repeatable acceptance standard for the boundary between OWS on-foot play and OWS vehicles. It is the deliverable for [issue #20](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/20) under the [character and vehicle epic](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/7).

The matrix is intentionally a test specification, not a claim that every scenario already passes. Run results expose the exact gaps that must become independently actionable GitHub issues.

Passing this matrix accepts the tested current character/vehicle behavior only. It does not prove that current code conforms to the accepted [OWS Platform Architecture and Composition Contract](OWS_PLATFORM_ARCHITECTURE.md), implements the shared authority/persistence spine, or conforms to the target [OWS Mobility Architecture](OWS_MOBILITY_ARCHITECTURE.md) and [Mobility Acceptance Matrix](OWS_MOBILITY_ACCEPTANCE_MATRIX.md) tracked by [issue #147](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/147).

## Stage 3 Mobility expansion status

This document preserves the verified pre-architecture/current-runtime baseline. The normative target matrix now expands it with passengers and control roles; authored standable zones, crouch/balance, sliding, active Grip, hanging, and forced detachment; moving entry and relative-motion matching; same-Vehicle exterior/interior infiltration; deferred cross-Vehicle route representation; collision and failed-transition outcomes; authoritative reservation/contention; correction without double-applied motion; late join, disconnect, destruction, and recovery. Those capabilities remain planned rather than implemented until their separately scoped work and declared evidence pass.

## Current execution target

- Unreal Engine: 5.8.1
- Map: `/Game/OWS/Levels/OWS_CombinedDemo`
- Local and packaged Windows builds must both be identified in the test record.
- Standalone, listen-server host, and remote-client results must be recorded separately.
- Record the tested commit; do not report a result against an uncommitted description such as "latest."

`OWS_CombinedDemo` is the accepted canonical OWS demonstration experience and the execution target for this matrix. A later approved replacement would not invalidate the scenario IDs or expected behavior below.

## Result and evidence rules

Use exactly one result for every executed scenario:

| Result | Meaning |
| --- | --- |
| `NR` | Not run. No conclusion can be drawn. |
| `PASS` | Observed behavior matches every stated expectation. |
| `FAIL` | At least one stated expectation was violated. A linked defect is required. |
| `BLOCKED` | The test could not be completed because a named dependency, environment failure, or unresolved product contract prevents a valid result. |
| `N/A` | The scenario cannot apply to this named character/vehicle combination. State the reason. |

Every result must provide an evidence reference. Acceptable evidence is a durable GitHub attachment or link to a video, screenshot sequence, log, automated-test result, or written observation detailed enough for another contributor to verify the outcome. A `FAIL` also requires reproduction steps, actual behavior, frequency, and a linked GitHub issue. A `BLOCKED` result must link the blocking issue or name the exact external condition.

Do not average results. If four repetitions pass and one fails, the scenario is `FAIL` with frequency `1/5`.

## Test record

Copy this table into a GitHub issue comment or test report for each run. One record may cover multiple scenario IDs only when every field is identical.

| Field | Required value |
| --- | --- |
| Tester | GitHub handle |
| Date | UTC date and time |
| Commit | Full Git commit SHA |
| Engine | Unreal Engine version |
| Build | Editor PIE, standalone, or packaged build identifier |
| Net mode | Standalone, listen host, or remote client |
| Map | Map asset path |
| Input | Keyboard/mouse or named controller |
| Character | Character ID from this document |
| Vehicle | Vehicle ID from this document, or `N/A` |
| Scenario | One or more scenario IDs |
| Result | `PASS`, `FAIL`, `BLOCKED`, or `N/A` |
| Evidence | Durable evidence link and concise observation |
| Defect/blocker | Required issue link for `FAIL` or `BLOCKED` |

## Coverage inventory

### OWS characters

| ID | Display name | Current class/asset identifier |
| --- | --- | --- |
| C01 | OWS Manny | `CBP_OWSCharacter_Manny` |
| C02 | OWS Manny Clone | `CBP_OWSCharacter_MannyClone` |
| C03 | OWS Quinn | `CBP_OWSCharacter_Quinn` |

All three character variants currently present in `OWS_CombinedDemo` are required coverage. Differences in mesh, animation binding, or presentation do not reduce their behavioral requirements. Additions to the playable roster must be added here and to the full grid before release acceptance.

### OWS vehicles

| ID | Vehicle | Representative | Coverage purpose |
| --- | --- | :---: | --- |
| V01 | DefaultVehicle | Yes | Baseline vehicle implementation |
| V02 | DemoChassis | No | Full-roster regression |
| V03 | HyperCar | No | Full-roster regression |
| V04 | SportsCar | Yes | Normal road-car baseline |
| V05 | MuscleCar | No | Full-roster regression |
| V06 | Sedan | No | Full-roster regression |
| V07 | SUV | Yes | Taller passenger vehicle |
| V08 | SedanEV | No | Full-roster regression |
| V09 | GT3 | No | Full-roster regression |
| V10 | DriftCar | Yes | Muscle-car body, drift setup, and current smoke vehicle |
| V11 | TCR | No | Full-roster regression |
| V12 | Rally | No | Full-roster regression |
| V13 | Bus | Yes | Large and long vehicle footprint |
| V14 | RC_Car | Yes | Extreme small-vehicle footprint |

The representative set is V01, V04, V07, V10, V13, and V14. It spans baseline, ordinary, tall, high-slip, oversized, and undersized cases. It does not replace the full fourteen-vehicle grid.

## Execution tiers

| Tier | Required combinations | Purpose |
| --- | --- | --- |
| T1: change smoke | C01 + V10 (`OWS Vehicle 10`, `BP_DriftCar`); run CV-ENV-001, all CV-FOOT, CV-ENTRY-001/004, CV-DRIVE-001/002/004/005, CV-EXIT-001/004, and CV-BAIL-001/002/006/007 | Fast signal while developing; uses the vehicle identified in the first recorded smoke run |
| T2: representative | Every character × every representative vehicle; run all applicable local scenarios | Detect character binding and vehicle-shape failures |
| T3: full release | Every character × all fourteen vehicles; run CV-CORE-001, then complete all local, impact, recovery, and network scenarios using the coverage rules below | OWS release acceptance |

If a representative combination fails, expand the failing scenario to every vehicle before declaring its affected scope.

## Universal acceptance invariants

These apply to every scenario, even when the row has more specific expectations:

1. OWS does not crash, hang, emit an ensure, or enter an unrecoverable play state.
2. Character and vehicle transforms remain finite; no explosive launch, persistent penetration, or fall through the world occurs.
3. Exactly one intended pawn is player-controlled. Input is not applied simultaneously to the on-foot character and vehicle.
4. Enter, exit, and recovery preserve a usable OWS character instance, its selected variant, and its pre-entry gameplay state unless the scenario explicitly changes that state.
5. A rejected action does not partially mutate possession, visibility, collision, input context, or seat occupancy.

## Environment and on-foot baseline

Run CV-ENV-001 and all CV-FOOT scenarios once per character before using that character in vehicle tests. Repeat CV-FOOT-008 for every representative vehicle because it crosses the character/vehicle collision boundary.

| ID | Setup and action | Expected behavior | Required evidence |
| --- | --- | --- | --- |
| CV-ENV-001 | Open the configured map and begin play. | The selected OWS character spawns controllable; all fourteen listed vehicle types are present and discoverable; no startup error invalidates testing. | Spawn observation plus roster confirmation. |
| CV-FOOT-001 | Walk, run, sprint, stop, and change direction on level ground. | Locomotion responds correctly, transitions cleanly, and returns to idle without sliding or input lock. | Video or detailed observation. |
| CV-FOOT-002 | Crouch, move while crouched, stand, then repeat below a low obstruction. | Crouch locomotion remains controllable; standing succeeds in clearance and is rejected safely under obstruction. | Video or screenshot sequence. |
| CV-FOOT-003 | Jump from rest and while moving; land on level and sloped ground. | Jump, airborne, and landing states complete without persistent animation, collision, or control errors. | Video or detailed observation. |
| CV-FOOT-004 | Trigger traversal on representative world obstacles from valid and invalid approaches. | Valid traversal completes and restores control; invalid traversal is rejected without state corruption. | Video naming the tested obstacles. |
| CV-FOOT-005 | Toggle strafe and aim while stationary and moving; release each input. | Facing and locomotion follow the selected mode; releasing input restores the prior mode without lock. | Video or detailed observation. |
| CV-FOOT-006 | Trigger ragdoll while stationary and moving, then recover. | Ragdoll engages once, reacts to momentum and ground, and recovers to a controllable, correctly oriented character. | Video including recovery. |
| CV-FOOT-007 | Cycle through the complete character roster and return to the starting character. | Each listed variant becomes playable exactly once per cycle and retains the OWS movement feature set. | One roster-cycle video or screenshot sequence. |
| CV-FOOT-008 | Walk, sprint, jump, crouch, aim, and attempt traversal against a parked representative vehicle from multiple sides. | Ordinary current movement collides cleanly. No action causes penetration, explosive impulse, loss of control, or unintended possession. Target dynamic-support/traversal behavior is evaluated separately under MOB-PHY and issue #21 and is not implied by this baseline row. | Video naming character, vehicle, and approach sides; target-matrix result when applicable. |
| CV-FOOT-009 | Enter the supported strafe state, then press R3 while stationary and moving. | R3 toggles the OWS camera shoulder without changing camera style, locomotion mode, or character control. | Video showing both shoulder positions while stationary and moving. |

## Core character/vehicle loop

CV-CORE-001 is the required cross-product scenario. Run it for every character/vehicle combination in the full coverage grid.

| ID | Setup and action | Expected behavior | Required evidence |
| --- | --- | --- | --- |
| CV-CORE-001 | With the vehicle stopped and unoccupied: approach within entry range, enter, accelerate, steer, brake to a stop, exit, move beyond the vehicle, and re-enter. | The correct vehicle is entered; control transfers once; driving inputs affect only it; stopped exit places the same character safely nearby; on-foot control and presentation are restored; re-entry succeeds. All universal invariants hold. | Result per character/vehicle combination. Video is required for a failure; a batch observation may support passing combinations from the same run. |

## Entry matrix

Run all entry scenarios for every character × representative vehicle. Run CV-ENTRY-001 and CV-ENTRY-004 through CV-ENTRY-007 for C01 × all vehicles.

| ID | Setup and action | Expected behavior | Required evidence |
| --- | --- | --- | --- |
| CV-ENTRY-001 | From within 500 cm of an unoccupied, stopped vehicle, aim the precision ray at each authored door interaction point and Activate. Stop and exit after each successful entry so the used door can be observed. | OWS consumes the exact ray-resolved vehicle and Door ID, transfers control once, and preserves that door for the stopped-exit preference. It never substitutes another door or vehicle through a nearest-object search. | Video or door-by-door observation naming the resolved and used Door IDs. |
| CV-ENTRY-002 | Resolve one authored vehicle-door interaction point while just outside 500 cm and Activate, then step inside 500 cm without changing the intended door and retry. | The outside-range request is rejected without partial state. The inside-range request enters once through the same resolved door. No stale prompt, alternate-door search, or partial transition remains. | Evidence showing both attempts and the resolved Door ID. |
| CV-ENTRY-003 | Place two unoccupied vehicles within 500 cm and keep a door on each visible. Aim at a specific door on the farther vehicle while the nearer vehicle also remains inside the 500 cm entry radius, then Activate. Repeat with the other vehicle targeted. | Each activation enters the exact ray-resolved vehicle through the exact resolved door. Distance to another available vehicle does not replace the selected target. | Video or paired observation naming both intended vehicles and Door IDs. |
| CV-ENTRY-004 | Tap entry once, then repeat while holding and rapidly pressing the input. | One valid press causes one transition. Held or repeated input does not toggle, duplicate the character, or corrupt occupancy. | Video including repeated input. |
| CV-ENTRY-005 | Occupy the driver seat, or make its entry clearance invalid, then attempt entry. | Entry is rejected cleanly; existing occupant/control and entering character state remain unchanged. | Evidence identifying how the seat was made unavailable. |
| CV-ENTRY-006 | Approach a placed vehicle that begins with an AI controller and enter it. | The player takes valid control of the vehicle without duplicate control or destruction of the OWS character. Subsequent stopped exit remains functional. | Video including entry and exit. |
| CV-ENTRY-007 | Enter while walking, sprinting, crouched, aiming, and strafing in separate repetitions. Exit after each. | Each entry completes once. On stopped exit, the same character returns in a usable state with the pre-entry movement/presentation state restored where that state remains valid. | State-by-state observation; video for any mismatch. |

## Driving and possession matrix

Run every scenario for every character × representative vehicle. Run CV-DRIVE-001, CV-DRIVE-002, CV-DRIVE-004, and CV-DRIVE-005 for C01 × all vehicles.

| ID | Setup and action | Expected behavior | Required evidence |
| --- | --- | --- | --- |
| CV-DRIVE-001 | Enter, then exercise steering, throttle, brake, reverse, and handbrake separately. | Vehicle input becomes active immediately and each control produces its intended response without on-foot input leakage. | Input-by-input observation or video. |
| CV-DRIVE-002 | Drive through low, medium, and high speed; turn, brake, reverse, and come to rest. | The vehicle remains controllable and physically stable across the run. No character artifact obstructs the camera or vehicle. | Continuous-run video or detailed observation. |
| CV-DRIVE-003 | Rotate and reposition the camera while stopped, moving, reversing, and turning. | Camera remains usable, follows the possessed vehicle, and does not snap to a hidden or stale character transform. | Video. |
| CV-DRIVE-004 | Observe the character and collision while driving near obstacles and dynamic props. | The stored character is not visibly duplicated, does not collide with the vehicle/world while stored, and cannot receive independent movement. | Video or detailed observation. |
| CV-DRIVE-005 | Press on-foot jump, crouch, traverse, aim, ragdoll, and character-cycle inputs while driving. | On-foot actions do not execute against the stored character or disturb possession. Vehicle controls continue to work. | Video or input-by-input observation. |
| CV-DRIVE-006 | Roll or invert the vehicle, continue applying input, and attempt recovery using only currently exposed OWS controls. | Possession remains valid and physics remain finite. Whether OWS supplies an explicit vehicle-reset action is an unresolved product choice; record that portion as `BLOCKED` and link a dedicated decision issue if none exists. | Video plus blocker/decision link when applicable. |
| CV-DRIVE-007 | While driving above 5 mph, tap and hold Circle in separate attempts. | Circle requests vehicle exit/bailout only. It does not apply service brake, handbrake, throttle, steering, shifting, camera, or any other vehicle action. | Video showing vehicle speed and response to both attempts. |

## Stopped exit matrix

Run every scenario for every character × representative vehicle. Run CV-EXIT-001 and CV-EXIT-004 for C01 × all vehicles.

| ID | Setup and action | Expected behavior | Required evidence |
| --- | --- | --- | --- |
| CV-EXIT-001 | At or below 5 mph, press exit once. Repeat from driver-side, passenger-side, front, and rear obstruction layouts. | Exit occurs immediately at the first safe candidate; the character is upright, visible, collidable, and controlled. Placement is clear of vehicle and world geometry. | Video or layout-by-layout observation. |
| CV-EXIT-002 | Block every candidate exit location and press exit. | Exit is rejected. Player remains in full vehicle control, and stored character/occupancy state is unchanged. | Video showing blockers and rejected exit. |
| CV-EXIT-003 | Block the preferred driver-side exit but leave another candidate clear. | Exit uses a safe fallback position and restores normal character control. | Video showing the blocked and selected positions. |
| CV-EXIT-004 | Enter and stopped-exit five consecutive times. | Every cycle completes once with the same character variant. No drift, duplicate pawn, missing input, hidden character, lost collision, or stale occupancy accumulates. | One continuous video or cycle log. |
| CV-EXIT-005 | Exit while the vehicle rests on a slope and beside a curb. | The character is placed on valid support without penetration, falling through the world, or immediate collision launch. | Video for both surfaces. |

## Moving exit and bailout matrix

Run every scenario for every character × representative vehicle. Run CV-BAIL-001, CV-BAIL-002, CV-BAIL-006, CV-BAIL-007, and CV-BAIL-008 for C01 × all vehicles.

| ID | Setup and action | Expected behavior | Required evidence |
| --- | --- | --- | --- |
| CV-BAIL-001 | Travel above 5 mph and hold exit for less than 2 seconds, then release. | No exit occurs; control remains with the vehicle; progress resets when input is released. | Video with visible timing. |
| CV-BAIL-002 | Travel above 5 mph and hold exit continuously for at least 2 seconds. | One bailout occurs after the hold threshold. Control transfers once to the same character; no stopped-exit placement is used. | Video with visible timing. |
| CV-BAIL-003 | Bail out while moving forward, reversing, and turning in separate runs. | Character placement is clear of the vehicle and receives physically coherent motion for the vehicle's direction; no explosive impulse or immediate recollision trap occurs. | Direction-by-direction video. |
| CV-BAIL-004 | Bail out toward a nearby wall or obstacle. | OWS chooses a non-penetrating result or rejects the bailout without partial state mutation. Exact obstacle-priority behavior is governed by the collision contract in issue #21. | Video and issue #21 link if precise response remains blocked. |
| CV-BAIL-005 | Observe the camera continuously from a moving bailout through ordinary locomotion or controlled roll and back to normal control. | Camera follows the active character state, remains aligned through the transition, and never stays on the uncontrolled vehicle, snaps the character back to a stale camera position, clips indefinitely, or prevents player orientation after recovery. | Continuous video covering both speed bands. |
| CV-BAIL-006 | Travel above 5 mph through 15 mph, complete the exit hold, and release movement input. Repeat while holding forward input. | Bailout inherits coherent vehicle motion and hands directly to ordinary locomotion. It does not force a landing, stumble, controlled-roll, or ragdoll animation. With no movement input, ordinary locomotion owns the stop; with forward input, it continues naturally. Vehicle re-entry is available after control returns without requiring a release distance. | Video with visible speed, both input cases, and a successful re-entry. |
| CV-BAIL-007 | Bail out above 15 mph at moderate and high speed. Repeat with no steering and with sustained left/right stick input during recovery, and attempt vehicle re-entry before and after recovery. | A root-locked, non-ragdoll full-body roll inherits vehicle velocity without artificial sideways or upward launch, remains grounded, and loops until horizontal speed reaches the configured runnable threshold. Steering is limited to at most 15 degrees/second but remains useful. OWS then restores ordinary locomotion directly. Re-entry is rejected only while controlled recovery owns the character and succeeds after control returns; no distance gate remains. | Continuous video with visible speeds, repeated roll cycles, steering response, transition to locomotion, and both re-entry attempts. |
| CV-BAIL-008 | Complete five consecutive drive/bail/recover/re-enter cycles. | No possession, input, visibility, collision, camera, seat, or character-identity error accumulates across cycles. | One continuous video or cycle log. |

## Impact and shared-physics matrix

Run every scenario with C01 against every representative vehicle, then repeat with C02-C03 against V04, V13, and V14. Evaluate the current universal safety invariants here. Record planned Mobility relationship behavior in the target matrix and evidence-derived collision response under [#21](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/21); an unavailable implementation is not evidence against the accepted target architecture.

| ID | Setup and action | Expected behavior | Required evidence |
| --- | --- | --- | --- |
| CV-IMPACT-001 | Walk, sprint, and jump into each side of a parked vehicle. | Contact resolves without penetration, launch, stuck state, possession change, or lost character control. | Multi-side video or observation. |
| CV-IMPACT-002 | Drive into a stationary character at low speed from front, rear, and side. | Contact remains physically finite and recoverable. Precise knockdown threshold and reaction are governed by issue #21. Neither pawn enters invalid possession or unrecoverable collision. | Three-direction video and contract status. |
| CV-IMPACT-003 | Repeat the character strike at materially higher speed in a controlled clear area. | Higher-energy contact remains finite; character and vehicle stay in-world and recoverable. Exact damage/knockdown response is governed by issue #21. | Video and contract status. |
| CV-IMPACT-004 | Strike a character who is crouched, aiming, and manually ragdolled in separate runs. | No starting state creates penetration, explosive impulse, invalid animation/physics blending, or permanent input loss. | State-by-state video. |
| CV-IMPACT-005 | Pin the character between a slowly moving vehicle and a rigid wall, then reverse the vehicle away. | No crash or explosive response occurs. After pressure is removed, the character can recover to a valid controllable state. Precise injury response is governed by issue #21. | Continuous video. |
| CV-IMPACT-006 | Place the character on the hood/roof, then begin moving, turning, and braking. | Current Character and Vehicle behavior remains physically finite; no penetration or invalid attachment occurs. Supported surface eligibility, crouch/balance, slide, Grip, edge departure, and forced detachment are evaluated under the target MOB-PHY rows and #21 rather than inferred from this baseline. | Continuous video plus target-matrix implementation status. |
| CV-IMPACT-007 | Land an airborne vehicle near and in contact with a character in a controlled test area. | Contact does not crash, tunnel through the world, corrupt possession, or leave either actor in an unrecoverable state. Precise character response is governed by issue #21. | Video. |
| CV-IMPACT-008 | Roll a vehicle into or immediately beside a character. | Repeated body contact remains finite; character cannot become permanently embedded in the vehicle; recovery remains possible after separation. | Video. |

## Recovery and stress matrix

Run with C01 × every representative vehicle. Repeat a failing scenario with every character and every vehicle needed to establish scope.

| ID | Setup and action | Expected behavior | Required evidence |
| --- | --- | --- | --- |
| CV-RECOVERY-001 | Complete ten stopped enter/drive/exit/re-enter loops without restarting play. | All ten loops meet CV-CORE-001 with no accumulating state or performance failure. | Continuous video or timestamped cycle log. |
| CV-RECOVERY-002 | Alternate five stopped exits and five moving bailouts without restarting play. | Each transition uses the correct exit mode and threshold. Character, vehicle, camera, and input recover after every cycle. | Continuous video or timestamped cycle log. |
| CV-RECOVERY-003 | Trigger character ragdoll, recover, enter a vehicle, drive, stopped-exit, and ragdoll again. | Both ragdoll cycles and the complete vehicle transition remain valid; animation and physics modes do not leak across possession. | Continuous video. |
| CV-RECOVERY-004 | Bail out, recover, switch to the next character, then enter and exit the same vehicle. | The selected character variant becomes the active usable OWS character and completes the core loop without stale state from the prior character. | Continuous video. |
| CV-RECOVERY-005 | Cause any safely reproducible rejected entry or exit, remove the obstruction, and retry. | Rejection leaves no partial state; the next valid attempt succeeds without restarting play. | Video showing rejection, correction, and success. |

## Network matrix

Run with two players in listen-server PIE or equivalent: one listen host and one remote client. Start with C01 + V10, then run all three characters against V10 and C01 against the representative vehicles. These expectations protect the current runtime baseline: the server owns authoritative possession and seat state, and all relevant peers observe the same result. Future Mobility support requires the target Mobility matrix.

| ID | Setup and action | Expected behavior | Required evidence |
| --- | --- | --- | --- |
| CV-NET-001 | Host completes the stopped core loop while client observes. | Host transition succeeds authoritatively; client observes correct possession, vehicle motion, character visibility, and exit placement. | Synchronized host/client video or paired capture. |
| CV-NET-002 | Remote client completes the stopped core loop while host observes. | Client request is processed by the server exactly once; both peers agree on possession, occupancy, motion, and exit. | Synchronized evidence from both peers. |
| CV-NET-003 | Host and client attempt to enter the same driver seat nearly simultaneously. | Exactly one player obtains the seat. The other remains a valid on-foot character; all peers agree on the winner and occupancy. | Synchronized evidence. |
| CV-NET-004 | Remote client performs an above-15-mph moving bailout while host observes, then attempts re-entry during and after controlled recovery. | Server authorizes one bailout; both peers observe the same active character, vehicle continuation, looping non-ragdoll controlled roll, limited steering, and direct transition to ordinary locomotion. Re-entry is rejected only during controlled recovery and becomes available after control returns. | Synchronized evidence. |
| CV-NET-005 | Host and client enter separate vehicles, drive through each other's relevance range, exit, and re-enter. | Each player retains independent authoritative control; no input, character, camera, or occupancy state crosses between players. | Synchronized evidence. |
| CV-NET-006 | A player disconnects while driving; the remaining peer approaches the vehicle. | Current state remains valid and consistently replicated with no stale control or partial mutation. Compare the observed occupancy/control result with the configured target policy in MOB-LIFE-001/002 rather than treating immediate enterability as an unresolved architecture choice. | Host evidence plus target-matrix result or defect link. |
| CV-NET-007 | A client joins after another player has already entered a vehicle. | Late joiner receives the current possession, occupancy, character visibility, and vehicle transform state without an intermediate false state becoming persistent. | Host and late-join client evidence. |

## Full release coverage grid

Record CV-CORE-001 for every combination. Use `NR` until the combination has actually been tested; replace it with a linked test record, not an unsupported checkmark.

| Character | V01 | V02 | V03 | V04 | V05 | V06 | V07 | V08 | V09 | V10 | V11 | V12 | V13 | V14 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| C01 OWS Manny | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR |
| C02 OWS Manny Clone | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR |
| C03 OWS Quinn | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR | NR |

## Converting gaps into GitHub issues

Every `FAIL` represents work that is not yet up to the OWS acceptance standard. Create one issue per independently fixable defect; do not combine unrelated failures merely because they were discovered in the same run.

Use this minimum issue structure:

```markdown
## Acceptance gap

Scenario ID(s):
Character ID(s):
Vehicle ID(s):
Commit / UE version / build / net mode:

## Reproduction

1.
2.
3.

## Expected OWS behavior


## Actual behavior


## Frequency and affected coverage


## Evidence


## Done when

- The named scenario passes for the originally failing combination.
- Representative coverage establishes the defect's full scope.
- Relevant regression coverage passes without weakening another OWS behavior.
```

Link the defect to epic #7, apply the appropriate area/type/priority labels, add it to the OWS project, and link the originating test record. When multiple failures share a demonstrated root cause, one issue may track them only if its acceptance criteria name every affected scenario.

## Matrix acceptance gate

This protected current-runtime Character/Vehicle regression baseline is ready to be called accepted only when:

1. CV-CORE-001 has a current `PASS` record for all forty-two character/vehicle combinations.
2. Every required T2 and T3 scenario has a current result and none is `NR`.
3. No required scenario is `FAIL` or `BLOCKED`.
4. Standalone, listen-host, and remote-client coverage meets the rules above.
5. Every discovered regression has a linked GitHub issue and is retested after its fix.
6. Evidence is tied to the exact release-candidate commit and supported Unreal Engine version.

Creating this matrix completes the specification work in issue #20. It does not by itself satisfy the acceptance gate; executing it will create the concrete implementation and defect backlog.
