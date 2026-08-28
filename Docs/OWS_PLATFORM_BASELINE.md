# OWS Platform Migration Baseline and Guardrail Contract

> **Status:** Reproduction contract and current known-gap ledger for [issue #115](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/115). The static findings below describe the compatibility baseline that later migrations must preserve or deliberately replace. They do not authorize a fix, file move, configuration change, API promotion, cook exclusion, or runtime-behavior change. Unreal-dependent results remain `NR` until the full baseline runner records them against the exact tested commit.

## Purpose

This baseline makes the pre-migration OWS state reproducible before ownership changes begin. It records:

- repository, engine, platform, target, configuration, profile, and command identity;
- module, plugin, source, public-API, content-reference, and configuration edges;
- the current build, canonical-map smoke, automation, cook, package, and staged-file result;
- known architecture violations without silently correcting them; and
- a clean pre-run/post-run working-tree comparison and cleanup result.

The controlling target is the [OWS Platform Architecture and Composition Contract](OWS_PLATFORM_ARCHITECTURE.md). That target defines what later work must achieve; it is not a reason to make the current baseline look cleaner than it is. A known violation remains visible until its separately accepted owning issue changes it and supplies before/after evidence.

## Result vocabulary

| Result | Meaning |
| --- | --- |
| `NR` | Not run against the recorded commit, or no durable result has been attached yet. |
| `PASS` | The exact declared command and evidence completed without violating its oracle. |
| `FAIL` | The command ran and an expected current behavior or invariant failed. |
| `BLOCKED` | A required engine, tool, build product, credential-safe workflow, or environment was unavailable. |
| `N/A` | The check is provably outside the declared run; the record must state why. |

Results are never averaged. A static report does not imply a build pass, a successful editor smoke does not imply a package pass, and a full-project pass does not prove independent reusable-plugin conformance.

## Non-mutating runner entrypoints

Run commands from the repository root. Put generated JSON outside the repository so verification does not alter the checkout or become an accidental deliverable.

PowerShell 7 is required because report acceptance uses Draft 2020-12 JSON Schema validation through `Test-Json`. The supported entrypoints below use `pwsh`. A caller may start the scripts with Windows PowerShell 5.1 only when `pwsh` is also available on `PATH`; in that case the validation module delegates the schema check to an isolated PowerShell 7 child process and treats an unavailable validator as a failure.

### Runner self-test

```powershell
pwsh -NoProfile -File .\Scripts\PlatformBaseline\TestOWSPlatformBaseline.ps1
```

The self-test exercises the reporting and guardrail logic without claiming an Unreal build, smoke, cook, or package result. It must cover deterministic output, known-violation classification, output-path safety, and secret redaction.

### Static capture

```powershell
pwsh -NoProfile -File .\Scripts\RunOWSPlatformBaseline.ps1 `
  -StaticOnly `
  -OutputPath <outside-repo-json>
```

Static capture inventories descriptors, modules, dependencies, source boundaries, literal and binary-string reference leads, configuration ownership, public fork-type exposure, and Runtime/Editor/Test mixing. It does not launch Unreal and cannot classify a serialized string as a hard, soft, manage, Blueprint-parent, World Partition, cook, or staged-file dependency without later Asset Registry and package evidence.

### Full capture

```powershell
pwsh -NoProfile -File .\Scripts\RunOWSPlatformBaseline.ps1 `
  -EngineRoot C:\UE_5.8 `
  -OutputPath <outside-repo-json>
```

The full run first executes and records the standalone guardrail self-test, then adds the declared Unreal 5.8 build, canonical-map smoke, automation, cook/package, Asset Registry/reference, and staged-manifest evidence. The report preserves the exact command and exit status for every phase. BuildCookRun exit/cook status and the package-output oracle are separate results, so missing package evidence cannot rewrite the recorded UAT exit. Packaged launch is currently `BLOCKED` with explicit ownership by issue #17; that one documented block does not invalidate an otherwise valid current-state capture, while every other `FAIL` or `BLOCKED` result does.

## Required execution record

The first accepted full run replaces the `NR` values below with the exact durable record. Until then, this table makes no execution claim.

| Field | Current recorded value |
| --- | --- |
| Report schema/version | `NR` |
| UTC start and finish | `NR` |
| Git commit and branch | `NR` |
| Pre-run working-tree state | `NR` |
| Post-run working-tree equivalence | `NR` |
| Unreal Engine version and root | `NR` |
| Host OS and architecture | `NR` |
| Target and configuration | `NR` |
| Declared composition/profile | `NR` |
| Static guardrail self-test | `NR` |
| Static dependency/reference capture | `NR` |
| `OWSEditor Win64 Development` build | `NR` |
| Clean-start canonical-map smoke | `NR` |
| Selector and Character/Vehicle automation | `NR` |
| Cook/package command and result | `NR` |
| Cooked/staged manifest and size | `NR` |
| Cleanup audit | `NR` |
| External report location and checksum | `NR` |

The report also records each phase's executable/argument vector, exit code, duration, and embedded evidence reference. `-OutputPath` receives the self-contained JSON, and the runner writes a sibling `<report>.sha256` sidecar containing the report's SHA-256 checksum. Environment-specific absolute engine/report paths may be recorded when needed for reproduction, but secrets, personal access tokens, private keys, signed URLs, and secret-derived values may not appear.

## Confirmed static inventory

This source/configuration inventory is independently inspectable without launching Unreal:

- The project declares one Runtime module, `OWS`.
- Five local plugins declare seven additional modules: Runtime `OWSCore`, `KinetiForge`, `AsyncTickPhysics`, `SigilInventory`, and `SaveExtension`; Editor `SaveExtensionEditor`; and DeveloperTool `SaveExtensionTest`. GASPALS is content-only.
- `Source/OWS` contains public Interaction, selector, Vehicle handoff/occupancy, bailout/recovery, and debug surfaces plus two editor-automation source files inside the Runtime module.
- `Plugins/OWSFramework/Source/OWSCore` currently combines input-context manipulation, raw physical-key routing, hotbar presentation, Pawn-roster possession, and a Vehicle-specific user preference.
- The maintained GASPALS and KinetiForge content roots remain current compatibility inputs. Their presence and current use do not promote raw fork paths or types to Supported OWS contracts.

## Current known gaps and owning work

The table reports current facts; it does not authorize the owning issue or broaden its scope.

| Category | Current evidence | Owning work / treatment |
| --- | --- | --- |
| Runtime, Editor, and Test mixing | [`OWS.Build.cs`](../Source/OWS/OWS.Build.cs) conditionally adds `UnrealEd` to the Runtime module, while [`OWSSelectorTests.cpp`](../Source/OWS/Private/Tests/OWSSelectorTests.cpp) and [`OWSCharacterVehicleFunctionalTests.cpp`](../Source/OWS/Private/Tests/OWSCharacterVehicleFunctionalTests.cpp) compile inside that module. Save Extension Runtime also conditionally declares `UnrealEd`. | [#116](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/116) creates approved Runtime/Editor/Test boundaries. [#122](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/122) thins the shell only after every displaced responsibility has a verified owner. Save Extension changes remain separately gated by [#134](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/134) and an exact migration owner. |
| Reverse project-content dependency | [`OWSControllerHotbarComponent.cpp`](../Plugins/OWSFramework/Source/OWSCore/Private/OWSControllerHotbarComponent.cpp) literal-loads both a GASPALS input context and `/Game/OWS/Input/IMC_OWSCharacter` from reusable OWSFramework Runtime code. | #116 preserves the legacy surface through a Platform shim; [#117](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/117) owns Input routing; [#119](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/119) owns UI presentation; [#131](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/131) owns the host-neutral hotbar backend. Exact physical/config migration belongs to [#121](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/121). |
| Misowned Platform responsibilities | OWSCore directly binds physical controls, replaces an input context, creates a UMG hotbar, sorts and possesses a Pawn roster, and stores a Vehicle-specific setting in [`OWSUserSettings.h`](../Plugins/OWSFramework/Source/OWSCore/Public/OWSUserSettings.h). | #116 may add only neutral Platform facilities and compatibility shims. #117, #119, and #131 own the separated Input, presentation, and host-neutral hotbar responsibilities. No issue may move the mixed component wholesale. |
| Concrete fork and provider leakage | [`OWSStockVehicleInteractionComponent.h`](../Source/OWS/Public/OWSStockVehicleInteractionComponent.h) exposes concrete Chaos Modular Vehicle and KinetiForge pointer types; its implementation directly calls KinetiForge. [`OWSVehicleInteractionComponent.cpp`](../Source/OWS/Private/OWSVehicleInteractionComponent.cpp) literal-loads two GASPALS roll animations and the KinetiForge input context. | The protected current cross-domain path is coordinated by [#120](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/120) and migrated only through [#163](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/163) after its prerequisites and parity gates. Character/Vehicle public replacement surfaces remain separately gated by #107/#108 children; no current issue authorizes broad fork-API cleanup. |
| Peer-private dependency | Save Extension Editor includes Unreal's private `ClassViewerNode` header from `Editor/ClassViewer/Private`. | Record under #115. The maintained Save Extension fork is hardened by #134; if that accepted scope does not own this Editor dependency, create a tight follow-up rather than silently expanding #134 or #121. |
| Module dependency leakage | `OWS` publicly declares OWSCore although current use is in Private source, privately declares KinetiForge while exposing a KinetiForge type, and KinetiForge lists AsyncTickPhysics as both Public and Private. Sigil public serialization headers consume StructUtils types without a direct StructUtils declaration. | #116/#122 own Platform/shell dependency cleanup only after compatibility owners exist. Vehicle-facing changes require a tight #108 child. Sigil dependency correction belongs to the separately gated generalized inventory work in [#129](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/129) or a newly accepted tight maintenance issue. |
| Configuration duplication and conflict | Project configuration mirrors GASPALS renderer/CVar/collision/near-clip entries and the 25-line inherited tag list, mirrors KinetiForge redirects and async-physics configuration, and contains project/plugin physics values whose effective precedence must be measured. GASPALS also supplies host-wide startup settings that the project overrides. | #121 coordinates exact owner-by-owner inventories after the owning contracts stabilize. #107/#108 own Character/Vehicle meaning; #116 owns profile/settings registration; [#123](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/123) later owns reviewed existing-project merge/recovery. |
| Unscoped plugin and cook composition | [`OWS.uproject`](../OWS.uproject) enables maintained domains and engine example plugins globally. [`DefaultGame.ini`](../Config/DefaultGame.ini) always cooks an engine example input directory, while most inclusion otherwise relies on transitive references. | #116 owns explicit composition-profile infrastructure. #121 owns exact cook/config inventories and exclusions. No baseline check may delete an example or narrow a cook to make the report pass. |
| Editor-content reachability lead | Static binary strings show the current course-section map reaches Blutility/Editor Utility identities, and inherited GASPALS maps/widgets contain the same family. | This is a reference lead, not yet a hard-reference or staged-leak claim. #121 requires Asset Registry, map, external-package, cook, stage, and packaged-load proof before an exact migration or exclusion. |
| Static `/Game` reference leads | Binary-string scanning finds `/Game` strings in hundreds of inherited GASPALS assets and a small number of KinetiForge assets. Many name absent source-project paths, so raw strings alone cannot distinguish serialized dependencies from import/editor metadata. | #115 records candidates without auto-fixing. #121 later consumes typed Asset Registry and cook evidence for an approved exact inventory. |
| Protected shell-to-plugin composition | Current `/Game/OWS` Character assets consume GASPALS; the canonical map and external actors consume KinetiForge; the current controller consumes OWSCore. | Shell-to-plugin composition is allowed and must remain behaviorally protected. #120/#163, #121, and #122 control later migration and retirement; a guardrail reports these edges but does not fail merely because they exist. |
| Committed Android File Server credential | Project configuration contains a non-empty committed static credential. Its value is intentionally omitted. | [#114](https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/114) exclusively owns safe removal, rotation/revocation, future local/generated handling, and Aurora's separate history-remediation decision. #115 may detect and redact; it may not remediate. |

## Dynamic evidence still required

Every row below remains `NR` until the full runner records the exact result.

| Evidence | Status | Required oracle |
| --- | --- | --- |
| Git/LFS checkout integrity | `NR` | Checked-out LFS objects are valid; command and commit are recorded. |
| Editor target build | `NR` | `OWSEditor Win64 Development` completes for the declared Unreal 5.8 installation. |
| Canonical-map startup and Map Check | `NR` | Required modules/assets load; the canonical maps and protected three-character setup satisfy the existing smoke oracle. |
| Current integration automation | `NR` | Selector and Character/Vehicle suites retain their exact existing results without being treated as reusable-domain conformance. |
| Asset Registry dependency graph | `NR` | Hard-package, soft-package, hard-management, soft-management, and searchable-name edges are recorded separately. Blueprint parent tags and map, HLOD, Data Layer, World Partition, external-actor, and external-object roles are classified from unloaded Asset Registry class, path, and tag data where the engine exposes them; World Partition maps retain and validate the authoritative `LevelIsPartitioned` tag. |
| Cook and package | `NR` | The declared target/profile cooks and packages; exact commands and failures are preserved. |
| Cooked/staged contents and size | `NR` | Durable manifests record files, packages, modules/plugins, sizes, and unexpected Editor/Test/example content without silently removing it. |
| Packaged launch | `NR` | The current runner records `BLOCKED`, cause, and owner issue #17 until that issue supplies a deterministic packaged-launch oracle. |
| Working-tree preservation | `NR` | Pre-run tracked/untracked state equals post-run state after task-owned outputs are cleaned. |

## Secret-redaction contract

Delivered or retained baseline output must never include a secret value, token fragment, private key, authorization header, credential-manager payload, signed URL, or a hash derived solely from a secret. Detection records only a stable rule identifier, safe file location, line/key category when non-sensitive, and a redacted presence result. Console output, JSON, retained logs, test fixtures, failure messages, screenshots, checksum sidecars, and pull-request text follow the same rule.

Unreal, UBT, and UAT can transiently create their own logs, response files, JSON, CSV, or text evidence before the wrapper regains control. The runner therefore confines those files to its verified task-owned scratch tree, streams its own command log through redaction, uses `-NoLog`/`-nolog` where supported, and sanitizes changed text-like generated files immediately after each external phase. The isolated scratch tree is mandatory-delete task state, not delivered evidence; interruption or a sanitation/cleanup failure prevents a clean pass and must report the residue. The final JSON and checksum sidecar receive a separate secret scan before they are retained.

In particular, the Android File Server finding is reported as present or absent without reproducing its configured value. Self-tests use synthetic fixtures that cannot authorize any real service.

## Artifact and cleanup contract

- The self-contained JSON report and its sibling SHA-256 sidecar go to the explicitly supplied path outside the repository. Detailed AssetSizeQuery, UnrealPak, plugin-size, stage, archive, and Asset Registry facts are normalized into report inventories with embedded evidence references and checksums before scratch deletion.
- Temporary raw/sanitized logs, source CSVs, source manifests, staging directories, test results, and helper files remain isolated task-owned scratch and are removed after their required facts are safely embedded.
- The runner does not rewrite source, assets, descriptors, configuration, maps, redirects, or runtime behavior.
- A detected known violation is data, not permission to fix it.
- If the operating system prevents cleanup, the run is not a clean pass; the exact remaining path or process is reported without deleting or stopping anything of unknown ownership.
- Later before/after migration evidence must cite this baseline's exact commit and schema rather than an unversioned local result.
