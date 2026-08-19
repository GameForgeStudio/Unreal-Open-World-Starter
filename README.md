# Unreal Open World Starter

An open-source Unreal Engine starter framework for the OWS Tactics project.

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
git clone https://github.com/GameFusi/Unreal-Open-World-Starter.git
```

Open `OWS.uproject` in Unreal Engine 5.8.1. On the first C++ setup, generate the project files and build when Unreal prompts you. This public starter intentionally excludes copied Epic template assets; obtain optional template/example content from your licensed Unreal installation.

## Included third-party plugins

The following project plugins are included in this repository: GASPALS, OWS Framework, Sigil Inventory, Save Extension, KinetiForge, and VibeUE. Their respective notices and licenses remain with those plugins.

## Community and contributions

GitHub is the source of truth for code, issues, proposals, feature requests, technical discussions, contributions, and project decisions. Discord is for community conversation, discovery, announcements, and general public discussion.

**New to GitHub or ready to help?** Read [How to Contribute](CONTRIBUTING.md) for a beginner-friendly walkthrough using either GitHub Desktop or the GitHub website.

Join the [OWS Tactics project chat on Discord](https://discord.com/channels/1536853959463936100/1539311739340849234).

Contributions are welcome when they support or extend the project north star. Contributions that contradict that direction are out of scope.

## License

This project is licensed under the [Apache License 2.0](LICENSE). Third-party components retain their own notices and licenses.
