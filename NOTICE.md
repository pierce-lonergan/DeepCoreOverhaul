# Attribution and provenance

## This project is derived from OpenLRR

DeepCoreOverhaul is a fan project built on top of
[**OpenLRR** by trigger-segfault](https://github.com/trigger-segfault/OpenLRR).

Essentially all code in this repository at the point of forking was written by the OpenLRR
authors, not by this project. The full upstream commit history has been preserved in this
repository so that authorship remains attributable via `git log` and `git blame`.

Upstream remains the canonical project. If you are looking to contribute to the
re-implementation of the base game itself, contribute there.

```
upstream  https://github.com/trigger-segfault/OpenLRR.git
```

## Licensing status — read this

**OpenLRR does not carry a license file**, and this repository therefore cannot grant one.
No rights over the inherited code are claimed or offered by this project.

This is not an oversight upstream; it is a documented consequence of the code's origins.
From `src/openlrr/engine/README.md`:

> The engine for OpenLRR is implemented based on the abandonware Gods98 source code. For that
> reason, this probably can't be placed under the same project-wide license that would be
> applicable to the game code.

Practical consequences:

- Any original work contributed *by this project* is offered for use by the fan community, but
  it cannot override the inherited status of the surrounding code.
- If the upstream authors object to this fork's existence, naming, or content, the intent is to
  comply rather than argue. Open an issue.

## Naming rule — inherited from upstream, and honored here

Upstream's README states:

> When using the name "OpenLRR" within this project, the **L** must never be expanded (i.e. do
> not write "Open _LEGO_ Rock Raiders").

**This project honors the same rule.** In this repository, "LRR" is never expanded, and the
project name is "DeepCoreOverhaul" — chosen from in-fiction terminology rather than from any
trademark.

## No affiliation

DeepCoreOverhaul is not associated with, endorsed by, or connected to The LEGO Group, Data
Design Interactive, or the OpenLRR project. LEGO® is a trademark of The LEGO Group.

## No game assets are distributed here

This repository contains **no game content** — no models, textures, sounds, maps, or
configuration from the original game, and no copy of the original executable.

Running this software requires your own legally obtained installation of the original game.
It functions by loading alongside the original executable; it is not a standalone game and
cannot be used to play without those files.

## Third-party components

| Component | Location | Origin |
| --- | --- | --- |
| Direct3D Retained Mode headers/libs | `lib/d3drm/` | Microsoft Corporation, 1995–1997. Retained-mode DirectX SDK, no longer shipped in modern SDKs. |
