# Unreal Open World Starter

An open-source Unreal Engine package for building open-world games from a capable, playable foundation.

> **Plays like GTA, drives like Forza Horizon, hacks like Watch Dogs.**

## Requirements

- Your own licensed installation of Unreal Engine. Unreal Engine itself is not included in this repository.
- Unreal Engine **5.8.1** is the supported starting version. Earlier versions may work, but are untested and have no official support.
- [Git LFS](https://git-lfs.com/) is required before cloning so Unreal assets download correctly.
- Windows contributors who build the C++ project need the Visual Studio C++ toolchain supported by Unreal Engine 5.8.

The project’s third-party plugins are included under `Plugins/`. Unreal Engine’s built-in plugins remain part of the required Unreal Engine installation; they are not copied into this repository.

## Start

```bash
git lfs install
git clone -c core.longpaths=true https://github.com/GameFusi/Unreal-Open-World-Starter.git
```

`core.longpaths=true` is required on Windows because some included Unreal assets have paths longer than Git for Windows accepts by default. The clone option stores that setting in this repository without changing your global Git configuration.

On Windows, run the clone from a short parent directory such as `C:\Projects`. UnrealBuildTool also generates deeply nested files and rejects build paths longer than 260 characters even when Git long paths are enabled.

Open `OWS.uproject` in Unreal Engine 5.8.1. On the first C++ setup, generate the project files and build when Unreal prompts you. This public starter intentionally excludes copied Epic template assets; obtain optional template/example content from your licensed Unreal installation.

## City Foundation architecture

OWS is developing a self-contained real-world city workflow that imports OSM/GeoJSON and terrain sources in Unreal Editor, generates an editable cooked city foundation, and can add deterministic World Partition-aligned detail through shipped prewarm data, a player-configurable persistent cache, and live cosmetic PCG. This is the accepted target architecture, not a claim of current `main` functionality. The baked city remains fully playable when every optional runtime layer and cache is disabled.

The accepted implementation boundary is a first-party `OWSCityFoundation` plugin with separate runtime and editor modules so raw import tooling cannot enter packaged builds. The plugin is planned work and is not yet present in `main`.

Read the accepted [OWS City Foundation architecture](Docs/CITY_FOUNDATION_ARCHITECTURE.md) for the input contract, terrain conformance, editable roads, double-hybrid cell system, cache controls, multiplayer/save boundaries, and acceptance invariants.

Delivery is tracked under [epic #82](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/82), including the [cooked city core](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/96), [enhancement streaming](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/97), [prewarm and persistent cache](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/98), [player storage controls](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/99), and [end-to-end validation](https://github.com/GameFusi/Unreal-Open-World-Starter/issues/100).

## Included plugins

OWS Framework is a first-party OWS plugin created and owned by the project.

OWS City Foundation is also a first-party OWS plugin in the accepted target architecture; its implementation is tracked under epic #82 and is not yet present in `main`.

GASPALS, KinetiForge, Sigil Inventory, and Save Extension are included attributed foundations. Their original plugin identities, notices, and licenses remain with them; OWS-owned systems built from or around those foundations are documented as OWS.

## Community and contributions

GitHub is the source of truth for code, issues, proposals, feature requests, technical discussions, contributions, and project decisions. Discord is for community conversation, discovery, announcements, and general public discussion.

**New to GitHub or ready to help?** Read [How to Contribute](CONTRIBUTING.md) for a beginner-friendly walkthrough using either GitHub Desktop or the GitHub website.

Join the [OWS community chat on Discord](https://discord.com/channels/1536853959463936100/1539311739340849234).

Contributions are welcome when they support or extend the project north star. Contributions that contradict that direction are out of scope.

## License

This project is licensed under the [Apache License 2.0](LICENSE). Third-party components retain their own notices and licenses.
