# How to Contribute

Thank you for helping build Unreal Open World Starter. You do not need to be an expert in GitHub to contribute. This guide explains the project workflow in plain language and gives you two ways to work: GitHub Desktop for work on your computer, and the GitHub website for small browser-based changes.

## The short version

1. Find or discuss a piece of work.
2. Make your changes somewhere other than `main`.
3. Open a pull request (PR) asking to add those changes to `main`.
4. A maintainer reviews the PR and decides whether to merge it.

You can download, read, and experiment with this public project whenever you want. A PR is only needed when you want your changes added to the shared project.

## What the GitHub words mean

| Term | Plain-language meaning |
| --- | --- |
| **Repository (repo)** | The project’s shared home on GitHub: files, history, issues, and discussions. |
| **Clone** | Download a working copy of the repo to your computer. |
| **Fork** | Your own GitHub copy of the repo. Anyone can create one and experiment safely. |
| **Branch** | A separate line of work. It lets you change things without changing `main`. |
| **Commit** | A saved checkpoint with a short explanation of what changed. |
| **Push** | Upload commits from your computer to GitHub. |
| **Pull request (PR)** | A request to merge your branch or fork into the shared `main` branch. It does **not** mean someone is downloading code. |
| **Review** | Looking at a PR’s changed files, commenting, approving, or requesting changes. |
| **Merge** | Add an accepted PR to `main`, the shared project version. |

## Where things belong

| Use this | For this |
| --- | --- |
| [Discussions](https://github.com/GameFusi/Unreal-Open-World-Starter/discussions) | Questions, ideas, introductions, design conversations, and help that is not yet a concrete task. |
| [Issues](https://github.com/GameFusi/Unreal-Open-World-Starter/issues) | A bug, a proposed feature, documentation work, or another trackable piece of work. |
| [Project board](https://github.com/orgs/GameFusi/projects/1/views/1) | The shared backlog and progress view. |
| Pull requests | Proposed code or documentation changes that are ready for maintainer review. |
| Discord | Community conversation, discovery, and announcements. GitHub remains the source of truth for project decisions and work. |

If you are unsure where something belongs, start a Discussion and ask.

## Before you begin

- Read the repository’s [README](README.md) and the issue or discussion you plan to work on.
- Before proposing cross-system, plugin, module, input, settings, content-ownership, cooking, compatibility, authority, persistence, or major feature work, read the accepted [OWS Platform Architecture and Composition Contract](Docs/OWS_PLATFORM_ARCHITECTURE.md), [OWS Shared Gameplay Spine, Authority, and Persistence Contract](Docs/OWS_GAMEPLAY_SPINE_ARCHITECTURE.md), and [OWS Product Architecture Roadmap](Docs/OWS_ARCHITECTURE_ROADMAP.md). Character, Vehicle, collision, passenger, moving-entry, exterior-traversal, or transfer work must also follow the selected [OWS Mobility Program Charter](Docs/OWS_MOBILITY_PROGRAM_CHARTER.md) and issue #147.
- Before proposing City Foundation work, read the accepted [City Foundation architecture](Docs/CITY_FOUNDATION_ARCHITECTURE.md); implementation issues may refine it but must not silently replace its contracts.
- For a new idea, start a Discussion before doing large or architectural work.
- For a bug or a defined task, use an existing Issue or create one with the appropriate template.
- Work in a branch or fork. Never push directly to `main`.
- Keep each PR focused on one purpose. Separate unrelated changes into separate PRs.
- Before opening a code or Unreal-content PR, run the applicable checks in [Docs/TESTING.md](Docs/TESTING.md) and report the result in the PR.

## Architecture and checkout gates

- Check the issue's status and linked architecture blockers before claiming it.
- An issue labeled `status: architecture gated` may collect research and requirements, but it is not available for implementation checkout until its linked contracts are accepted and Aurora opens the applicable roadmap stage.
- OWS Mobility architecture is active through #147/#107/#108; that opens audits, research, and Aurora interviews only. It does not make a related gameplay implementation issue claimable.
- Keep the claimed issue's scope exact. Record an out-of-scope discovery as a separate issue instead of following it into another contributor's area.
- Do not move reusable behavior into the project shell, add reverse `/Game` references from reusable plugins, expose maintained-fork implementation types as new public OWS contracts, or create peer-domain private dependencies.
- Architecture migration is compatibility-first: establish the approved owner and baseline, move one responsibility in one scoped issue, preserve redirects or shims, verify behavior, and retire legacy surfaces only after their deprecation window.

## Choose your route

### Use GitHub Desktop for Unreal and code work

GitHub Desktop is the recommended route for Unreal project files, source code, Blueprints, assets, and any change that needs testing on your computer.

1. Create a free GitHub account and install [GitHub Desktop](https://desktop.github.com/).
2. Sign in to GitHub Desktop.
3. Open this repository on GitHub and click the green **Code** button.
4. Choose **Open with GitHub Desktop**, then choose where to save the project on your computer.
5. In GitHub Desktop, make a new branch: **Current Branch** → **New Branch**. Use a clear name such as `fix/vehicle-exit` or `docs/improve-onboarding`.
6. Open the project from that folder and make your change.
7. Test what you changed when possible. For Unreal changes, explain in the PR what you tested and what you could not test.
8. Return to GitHub Desktop. Review the changed files, write a short commit summary, and click **Commit to _your-branch_**.
9. Click **Publish branch** or **Push origin**.
10. Click **Create Pull Request**, then continue at [Open your pull request](#open-your-pull-request).

If GitHub Desktop says you do not have permission to publish a branch to this repository, use the fork workflow below instead. That is normal for public projects.

### Use the GitHub website for small text changes

The website route is useful for documentation, typo fixes, issue templates, and other small text-based changes. Do not use it for normal Unreal asset or code work; clone the project and use GitHub Desktop instead.

1. Open the [repository](https://github.com/GameFusi/Unreal-Open-World-Starter) and click **Fork** near the top-right. This makes your own safe copy.
2. In your fork, open the file you want to change and click the pencil icon (**Edit this file**).
3. Make the change, then scroll down to **Commit changes**.
4. Select **Create a new branch for this commit and start a pull request**. Give the branch a clear name.
5. Click **Propose changes**.
6. GitHub opens the comparison page. Click **Create pull request**, then continue below.

## Open your pull request

A PR is how you ask the project to consider your completed work. It does not put anything into `main` by itself.

1. Confirm the PR points **into** `GameFusi/Unreal-Open-World-Starter:main`.
2. Give it a specific title, such as `Fix vehicle exit input` instead of `Changes`.
3. Describe what changed, why it changed, how you tested it, and anything reviewers should know.
4. Link the related Issue or Discussion when there is one. Writing `Fixes #123` in the description closes Issue 123 automatically when the PR is merged.
5. Review the **Files changed** tab yourself before requesting review.
6. Submit the PR. A maintainer will review it, leave comments if needed, and decide whether to merge it.

Ordinary PRs require one approving review before they can merge. The maintainer controls what is accepted into `main`; contributors cannot push directly to `main`.

## When a review asks for changes

Nothing has gone wrong. Make the requested changes on the **same branch**, commit them, and push again. The existing PR updates automatically; do not open a second PR for the same work.

If you disagree or need clarification, reply in the PR conversation. Keep the decision and technical context there so the project has a durable record.

## After your PR is merged

- GitHub automatically removes the merged remote branch.
- In GitHub Desktop, switch back to `main` and click **Fetch origin** / **Pull origin** to get the latest shared work.
- Delete the local branch when GitHub Desktop offers to do so, or keep it only while you still need it.
- Start your next piece of work from an up-to-date `main` branch.

## A safe first contribution

If you are new, look for Issues labeled [`good first issue`](https://github.com/GameFusi/Unreal-Open-World-Starter/labels/good%20first%20issue). Good starting contributions include improving documentation, clarifying setup steps, reproducing a bug, or helping triage an Issue.

## Need help?

Ask in [GitHub Discussions](https://github.com/GameFusi/Unreal-Open-World-Starter/discussions). Questions are welcome. It is better to ask early than to spend time building against an assumption.
