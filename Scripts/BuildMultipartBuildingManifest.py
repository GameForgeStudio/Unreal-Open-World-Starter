"""Recover multipart building rings from an OSM export, without changing it.

Produces a reviewed sidecar for repairing formerly independent outer fragments.
Only relations containing open outer ways are selected. Coordinates use the
StreetMap importer's original node-average sinusoidal projection, in centimetres.
"""
import argparse
import json
import math
import xml.etree.ElementTree as ET
from pathlib import Path


def elements(path):
    events = ET.iterparse(path, events=("start", "end"))
    _, root = next(events)
    for event, element in events:
        if event == "end" and element.tag in {"node", "way", "relation"}:
            yield element
            root.clear()


def join_rings(parts):
    remaining = {key: list(points) for key, points in parts.items()}
    rings = []
    while remaining:
        key = min(remaining, key=int)
        ring = remaining.pop(key)
        if len(ring) < 2:
            raise ValueError(f"Way {key}: fewer than two nodes")
        while ring[-1] != ring[0]:
            matches = [(k, p if p[0] == ring[-1] else list(reversed(p)))
                       for k, p in remaining.items()
                       if p[0] == ring[-1] or p[-1] == ring[-1]]
            if len(matches) != 1:
                raise ValueError(f"Ambiguous or missing continuation at node {ring[-1]}")
            k, p = matches[0]
            ring.extend(p[1:])
            del remaining[k]
        if len(ring) < 4:
            raise ValueError("Closed ring has fewer than three vertices")
        rings.append(ring[:-1])
    return rings


def build(source):
    relations, count, lat_sum, lon_sum = [], 0, 0.0, 0.0
    for e in elements(source):
        if e.tag == "node":
            count += 1
            lat_sum += float(e.get("lat")); lon_sum += float(e.get("lon"))
        elif e.tag == "relation":
            tags = {t.get("k"): t.get("v") for t in e.findall("tag")}
            if tags.get("type") == "multipolygon" and "building" in tags:
                members = [(m.get("ref"), m.get("role")) for m in e.findall("member")
                           if m.get("type") == "way"]
                if any(role not in {"outer", "inner", ""} for _, role in members):
                    raise ValueError(f"Unsupported role in building relation {e.get('id')}")
                relations.append((e.get("id"), members, tags))
    wanted = {k for _, members, _ in relations for k, _ in members}
    ways = {}
    for e in elements(source):
        if e.tag == "way" and e.get("id") in wanted:
            ways[e.get("id")] = [n.get("ref") for n in e.findall("nd")]
    selected = []
    for relation_id, members, tags in relations:
        if any(k not in ways for k, _ in members):
            raise ValueError(f"Missing member way in relation {relation_id}")
        outer = {k: ways[k] for k, role in members if role in {"outer", ""}}
        if not any(p and p[0] != p[-1] for p in outer.values()):
            continue
        inner = {k: ways[k] for k, role in members if role == "inner"}
        selected.append((relation_id, outer, join_rings(outer), join_rings(inner), tags))
    needed = {n for _, outer, rings, holes, _ in selected
              for p in list(outer.values()) + rings + holes for n in p}
    nodes = {}
    scale, lat0, lon0 = 40075036.0 / 360.0 * 100.0, lat_sum / count, lon_sum / count
    for e in elements(source):
        if e.tag == "node" and e.get("id") in needed:
            lat, lon = float(e.get("lat")), float(e.get("lon"))
            nodes[e.get("id")] = [(lon - lon0) * scale * math.cos(math.radians(lat)),
                                   -(lat - lat0) * scale]
    if needed - nodes.keys():
        raise ValueError("Missing nodes in relation boundaries")
    return {"version": 1, "projection": "StreetMap-node-average-cm",
            "source": str(source.resolve()), "latitude": lat0, "longitude": lon0,
            "relations": [{"id": rid, "tags": tags,
                           "fragments": [{"way": k, "points": [nodes[n] for n in
                               (p[:-1] if p[0] == p[-1] else p)]} for k, p in outer.items()],
                           "outer": [[nodes[n] for n in p] for p in rings],
                           "inner": [[nodes[n] for n in p] for p in holes]}
                          for rid, outer, rings, holes, tags in selected]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = build(args.source)
    # Exclusive creation: never silently replace a prior repair manifest.
    with args.output.open("x", encoding="utf-8") as stream:
        json.dump(data, stream, separators=(",", ":"))
    print(f"Saved {len(data['relations'])} multipart building corrections to {args.output}")
