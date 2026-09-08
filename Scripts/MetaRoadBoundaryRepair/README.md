# Optional MetaRoad boundary repairs — issues #174 and #176

This is opt-in maintenance for a **separately licensed MetaRoad 3.2.0 installation**.
MetaRoad remains outside OWS and is not an OWS dependency. These files do not
implement or change the accepted City Foundation architecture. No vendor source,
assets, binaries, or license are redistributed here.

## Defect and narrow change

The boundary walk checked whether a neighboring vertex had *any* allowed incident
edge. That did not establish that the edge leading to that neighbor was allowed.
An excluded connector between two valid surface contours could therefore be
traversed, causing the walk to lose a contour.

The repair enumerates incident edges and applies the existing group predicate and
undirected skip list to the actual edge before considering its opposite endpoint.
It leaves angular ordering, starting vertex selection, contour removal, and the
caller-provided group predicate unchanged. In particular it does not turn the
caller's exact centerline exclusion into a bit-mask exclusion: combined
centerline/surface groups remain eligible under that predicate.

Issue #176 concerns termination at shared vertices. Revisiting a junction does
not necessarily close a contour: another lobe may remain. The second repair
tracks directed edges, accepts closure only when the first directed edge would
repeat, and rejects any other repeated directed edge. Angular selection is
unchanged and the walk remains bounded by the graph's directed edges.

## Apply locally

Review the source and the script first. Run from the repository root:

```powershell
# Read-only preview (the default)
./Scripts/MetaRoadBoundaryRepair/RepairBoundaryWalk.ps1 -PluginDirectory '<licensed plugin directory>'

# Explicit mutation, with an existing controlled backup directory
./Scripts/MetaRoadBoundaryRepair/RepairBoundaryWalk.ps1 -PluginDirectory '<licensed plugin directory>' -Apply -BackupDirectory '<controlled scratch directory>'
```

The script requires version 3.2.0 and exactly one matching traversal block. It
preserves line endings and UTF-8 BOM, refuses to overwrite a backup, supports
`-WhatIf`, and makes no change when the reviewed repair is already present.
It does **not** compile, deploy, reload, change engine settings, or launch Unreal.
Keep the backup until the repair is accepted, then clean the controlled scratch
directory under the task's ownership rules. Never restore it over later edits.

## Native verification

`BoundaryWalkTests.cpp` is original, optional Unreal automation coverage. Compile
it in a consumer's editor module that **already** depends on licensed
`MetaRoadEditor`, with a direct `GeometryCore` module dependency for the graph
container's exported functions; do not add MetaRoad to OWS to run it. Use it as a `.cpp` translation
unit, or rename it `.inl` and include it once in an existing editor translation unit
when adding a new file through Live Coding is not picked up. Never do both.

Run `OWS.City.VerifyMetaRoadBoundary` in the editor console for the synchronous,
graph-only check, or run `Automation RunTests OWS.External.MetaRoad.BoundaryEdgeFilter`.
The synchronous command refuses to interrupt another active automation test.
Neither check starts Play, opens a map, or requires rendering. The normal automation
runner may independently wait for its configured interactive frame-rate threshold.

The fixtures contain two squares connected by a disallowed edge whose endpoints
each touch allowed edges. Four scenarios cover:

- excluded group on the connector;
- explicit skip edge in each orientation;
- combined centerline/surface groups remaining allowed.

Every scenario requires a four-edge first contour, no excluded connector in that
contour, and recovery of both disconnected surfaces through `FindBoundaries`.

Additional fixtures cover two squares sharing a non-start vertex, two triangles
sharing the start vertex, and termination of an open single-edge graph. Both
shared-vertex fixtures require the entire perimeter and both lobes to survive
`FindBoundaries`, not just a successful return value.

Also rebake the consumer's previously failing cell through its real native pipeline.
A source edit, successful project Live Coding build, or passing synthetic test
alone does not prove that the installed plugin DLL changed or that the city cell
was repaired. Keep #174 open until the replacement is loaded and both native and
cell-level verification are recorded. Respect the user's editor restart authority.

## Current evidence

On 2026-09-08 the graph-only test compiled and ran in the existing EntourageUE
editor against its loaded MetaRoad DLL and reported **FAIL** in all four scenarios:
closure failed, six edges were returned instead of four, the excluded connector
was traversed, and zero surfaces were recovered instead of two.

The source repair was applied and the separately built Win64 Development editor
plugin compiled and linked successfully (154 build actions). A temporary headless
commandlet host then loaded that replacement MetaRoadEditor DLL, ran the same
original regression, reported **PASS**, and exited with code 0 at
2026-09-08 15:10:30 UTC. Its log identified the replacement DLL inside the isolated
host, not the old engine DLL. There were zero errors and two existing Python-name
collision warnings concerning MetaRoad enum/struct exposure. The commandlet exited
without opening a map, entering Play, or restarting the user's editor.

After the user saved and closed the editor, the #174 replacement was installed
and EntourageUEEditor rebuilt. Real city cell 280 still failed without advancing
its checkpoint. Its graph contains 1,920 vertices and 2,237 edges in one connected
component. The native walk rejected a shared-vertex revisit after 154 edges.
This led to separate issue #176; no consumer graph data is redistributed here.

At 2026-09-08 15:29:24 UTC, the expanded native regression against the #174-only
DLL reproduced #176: shared non-start closure failed and retained six of eight
edges, while shared-start closure prematurely retained three of six edges.
The original four edge-filter cases continued to pass. The #176 repair and
real-cell retry remain under verification; neither issue is being closed yet.
