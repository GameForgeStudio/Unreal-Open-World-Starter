# OWS Automated Testing

## Clean-start smoke test

The clean-start smoke test detects Git LFS corruption, missing project modules, required-asset failures, canonical-map load failures, Map Check errors, and incorrect setup of the three OWS character instances.

### Prerequisites

- Windows with Unreal Engine 5.8.1.
- Git and Git LFS available on `PATH`.
- `OWSEditor Win64 Development` built for the checkout being tested.
- The repository cloned with the Windows setup documented in the root README.

### Run

From the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunOWSCleanStartSmoke.ps1 -EngineRoot C:\UE_5.8
```

You may omit `-EngineRoot` when Unreal is installed at `C:\UE_5.8`, under the standard Epic Games installation directory, or when `OWS_UNREAL_ENGINE_ROOT` points to the engine installation.

The successful result ends with:

```text
[OWS smoke] PASS: Git LFS, module startup, required assets, canonical maps, Map Check, and three-character setup passed.
```

Any failed assertion or Unreal process failure returns exit code `1`. The runner prints the failure reason and relevant Unreal log lines, then removes its temporary logs. This command is suitable for local contributor checks and as the canonical-map smoke step in future Windows CI.

### What it verifies

- Every checked-out Git LFS object passes `git lfs fsck`.
- Unreal starts the project and loads its required runtime and editor modules.
- Required OWS, GASPALS, and KinetiForge foundation assets load.
- `/Game/OWS/Levels/OWS_CombinedDemo` loads and passes Map Check without errors.
- `/Game/OWS/Levels/OWS_CourseSection` loads with exactly three OWS character instances.
- Every OWS character instance uses the shared experimental setup marker.
- No retired `OWSPrototype` or `OWSTestLab` dependency appears during startup or map loading.
