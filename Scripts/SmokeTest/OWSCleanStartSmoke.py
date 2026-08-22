"""Unreal-side assertions for the OWS clean-start smoke test."""

import unreal


CANONICAL_MAP = "/Game/OWS/Levels/OWS_CombinedDemo"
COURSE_MAP = "/Game/OWS/Levels/OWS_CourseSection"
REQUIRED_ASSETS = (
    "/Game/OWS/GameModes/GM_OWSCharacterDemo",
    "/Game/OWS/Controllers/PC_OWSCharacterDemo",
    "/Game/OWS/Characters/Core/CBP_OWSCharacter_Base",
    "/Game/OWS/Input/IMC_OWSCharacter",
    "/GASPALS/Blueprints/CBP_SandboxCharacter",
    "/GASPALS/Blueprints/ABP_SandboxCharacter",
    "/KinetiForge/Template/Vehicles/DefaultCar/DefaultVehicle",
    "/KinetiForge/Template/Vehicles/RWD/DriftCar/BP_DriftCar",
)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load_required_assets():
    unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
    for asset_path in REQUIRED_ASSETS:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        require(asset is not None, "Required asset did not load: {}".format(asset_path))
        unreal.log("OWS_SMOKE|ASSET|{}".format(asset_path))


def load_map(map_path):
    world = unreal.EditorLoadingAndSavingUtils.load_map(map_path)
    require(world is not None, "Required map did not load: {}".format(map_path))
    unreal.log("OWS_SMOKE|MAP|{}".format(map_path))
    return world


def validate_characters():
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    characters = [
        actor
        for actor in actors
        if "CBP_OWSCharacter" in actor.get_class().get_path_name()
    ]
    require(
        len(characters) == 3,
        "Expected three OWS character instances in {}; found {}".format(
            COURSE_MAP, len(characters)
        ),
    )

    for character in characters:
        meshes = [
            component
            for component in character.get_components_by_class(
                unreal.SkeletalMeshComponent
            )
            if component.get_name() == "CharacterMesh0"
        ]
        require(
            len(meshes) == 1,
            "{} has {} CharacterMesh0 components".format(
                character.get_actor_label(), len(meshes)
            ),
        )
        tags = list(meshes[0].get_editor_property("component_tags"))
        require(
            unreal.Name("Force SM Setup") in tags,
            "{} is missing Force SM Setup".format(character.get_actor_label()),
        )
        unreal.log("OWS_SMOKE|CHARACTER|{}".format(character.get_actor_label()))


load_required_assets()
canonical_world = load_map(CANONICAL_MAP)
unreal.SystemLibrary.execute_console_command(canonical_world, "MAP CHECK")
load_map(COURSE_MAP)
validate_characters()
unreal.log("OWS_SMOKE|PASS|clean-start validation passed")
