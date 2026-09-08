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
untouched. Native source matching and real-cell replacement evidence are pending.
