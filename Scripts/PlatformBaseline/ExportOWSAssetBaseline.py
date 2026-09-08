"""Read-only Unreal Asset Registry exporter for the OWS platform baseline."""

from __future__ import annotations

import datetime
import json
import os
import re
from typing import Dict, Iterable, List

import unreal


OUTPUT_ARGUMENT = "OWSAssetBaselineOutput"
DEPENDENCY_KINDS = (
    "hard_package",
    "soft_package",
    "manage_hard",
    "manage_soft",
    "searchable_name",
)
ASSET_ROLES = (
    "blueprint_parent",
    "map",
    "hlod",
    "data_layer",
    "world_partition",
    "external_actor",
    "external_object",
)
CLASSIFICATION_CAPABILITIES = DEPENDENCY_KINDS + ASSET_ROLES


def _command_line_value(name: str) -> str:
    command_line = unreal.SystemLibrary.get_command_line()
    match = re.search(
        r"(?:^|\s)-{}=(?:\"([^\"]+)\"|(\S+))".format(re.escape(name)),
        command_line,
    )
    if not match:
        raise RuntimeError("Missing required -{}=<path> argument".format(name))
    return match.group(1) or match.group(2)


def _dependency_options(category: str):
    values = {
        "include_soft_package_references": category == "soft_package",
        "include_hard_package_references": category == "hard_package",
        "include_searchable_names": category == "searchable_name",
        "include_soft_management_references": category == "manage_soft",
        "include_hard_management_references": category == "manage_hard",
    }
    return unreal.AssetRegistryDependencyOptions(**values)


def _string(value) -> str:
    return str(value) if value is not None else ""


def _sorted_strings(values: Iterable) -> List[str]:
    return sorted({_string(value) for value in values if _string(value)})


def _tag_value(asset_data, key: str) -> str:
    """Return an Asset Registry tag without loading the asset."""
    try:
        value = asset_data.get_tag_value(key)
    except Exception:
        return ""
    if isinstance(value, tuple):
        for candidate in reversed(value):
            text = _string(candidate)
            if text and text.lower() not in ("true", "false"):
                return text
        return ""
    return _string(value)


def _asset_roles(asset_data) -> Dict[str, List[str]]:
    package_name = _string(asset_data.package_name)
    package_lower = package_name.lower()
    asset_class = _string(asset_data.asset_class_path)
    class_lower = asset_class.lower()
    level_is_partitioned = _tag_value(asset_data, "LevelIsPartitioned")
    roles: Dict[str, List[str]] = {name: [] for name in ASSET_ROLES}

    roles["blueprint_parent"] = _sorted_strings(
        _tag_value(asset_data, key)
        for key in ("ParentClass", "NativeParentClass")
    )
    if class_lower.endswith(".world") or class_lower == "/script/engine.world":
        roles["map"] = [package_name]
    if "hlod" in class_lower or "/_hlod/" in package_lower or "/hlod/" in package_lower:
        roles["hlod"] = [asset_class or package_name]
    if "datalayer" in class_lower or "/datalayers/" in package_lower:
        roles["data_layer"] = [asset_class or package_name]
    if level_is_partitioned.lower() in ("1", "true"):
        roles["world_partition"] = ["LevelIsPartitioned={}".format(level_is_partitioned)]
    elif "worldpartition" in class_lower or "/worldpartition/" in package_lower:
        roles["world_partition"] = [asset_class or package_name]
    if "/__externalactors__/" in package_lower:
        roles["external_actor"] = [package_name]
    if "/__externalobjects__/" in package_lower:
        roles["external_object"] = [package_name]
    return roles


def _asset_record(registry, asset_data) -> Dict[str, object]:
    package_name = _string(asset_data.package_name)
    dependencies = {}
    for category in DEPENDENCY_KINDS:
        dependencies[category] = _sorted_strings(
            registry.get_dependencies(package_name, _dependency_options(category))
        )
    return {
        "package_name": package_name,
        "package_path": _string(asset_data.package_path),
        "asset_name": _string(asset_data.asset_name),
        "asset_class": _string(asset_data.asset_class_path),
        "dependencies": dependencies,
        "classifications": _asset_roles(asset_data),
        "registry_tags": {
            "parent_class": _tag_value(asset_data, "ParentClass"),
            "native_parent_class": _tag_value(asset_data, "NativeParentClass"),
            "generated_class": _tag_value(asset_data, "GeneratedClass"),
            "level_is_partitioned": _tag_value(asset_data, "LevelIsPartitioned"),
        },
    }


def main() -> None:
    output_path = os.path.abspath(_command_line_value(OUTPUT_ARGUMENT))
    output_directory = os.path.dirname(output_path)
    if not output_directory:
        raise RuntimeError("Asset baseline output must have a parent directory")
    os.makedirs(output_directory, exist_ok=True)

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    assets = sorted(
        registry.get_all_assets(),
        key=lambda item: (_string(item.package_name), _string(item.asset_name)),
    )
    records = [_asset_record(registry, asset) for asset in assets]

    report = {
        "schema_version": "1.0.0",
        "generated_utc": datetime.datetime.now(datetime.timezone.utc)
        .replace(microsecond=0)
        .isoformat()
        .replace("+00:00", "Z"),
        "project": "OWS",
        "classification_capabilities": {
            capability: True for capability in CLASSIFICATION_CAPABILITIES
        },
        "asset_count": len(records),
        "assets": records,
    }

    temporary_path = output_path + ".tmp"
    with open(temporary_path, "w", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2, sort_keys=True)
        stream.write("\n")
    os.replace(temporary_path, output_path)
    unreal.log("OWS_BASELINE|PASS|asset registry records={}".format(len(records)))


if __name__ == "__main__":
    main()
