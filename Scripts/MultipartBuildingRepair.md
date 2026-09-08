# Multipart building recovery (#180)

`BuildMultipartBuildingManifest.py source.osm output.json` reads an existing OSM
export without changing it. It selects building relations whose outer members
contain open ways, joins those ways by original node IDs, and preserves inner
courtyard rings separately. Reversed member order is supported; missing or
ambiguous continuations fail instead of inventing closing edges. Output creation
is exclusive and never overwrites an existing manifest.

The coordinates use the StreetMap importer's node-average sinusoidal projection
in centimetres. The input must be a deduplicated export with the same nodes as
the imported master. Consumer code must validate every fragment against that
master before applying a correction. The manifest contains original fragments
for matching, complete outer rings, inner rings, and relation metadata.

This is original, optional recovery tooling for the external StreetMap workflow,
not a new OWS runtime dependency or a replacement for City Foundation. It does
not modify maps or launch Unreal. No city data or vendor implementation is
included in this repository.

The authorized Entourage consumer retains its original cell keys, removes matched
fragment indices from the in-memory building index, and triangulates complete
polygons with Unreal GeometryCore constrained Delaunay. Roof area must match outer
area minus courtyard area. Existing affected building components are explicitly
backed up and rebuilt; their road components and all non-city content remain
untouched. At 2026-09-08 17:25:50 UTC, native source matching and roof-area checks
passed for 33 relations, matching 76 legacy fragments across 30 cells. Five of
those cells were already saved; their building components were backed up, rebuilt,
and saved. The previously failing cell 5762 saved at 17:25:51 UTC, advancing the
unchanged index scheme to checkpoint 5763/29290. The headless commandlet exited 0
with zero errors and 18 existing project warnings. No Play or graphical editor
was launched. Full-city baking, unresolved unrelated geometry assertions, HLOD,
streaming configuration, and mandatory cleanup remain separate pending work.
