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
SELECTOR_LEVEL_BUTTONS = {
    "Button_Widget",
    "Button_Widget2",
    "Button_Widget3",
    "Button_Color_1",
    "Button_Color_2",
    "Button_Color_3",
}
LEVEL_BUTTON_CLASS = "/GASPALS/LevelPrototyping/LevelButton.LevelButton_C"


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
        selectors = character.get_components_by_class(unreal.OWSSelectorComponent)
        require(
            len(selectors) == 1,
            "{} has {} OWS selector components".format(
                character.get_actor_label(), len(selectors)
            ),
        )
        require(
            selectors[0].has_valid_selector_stacks(),
            "{} has an invalid selector stack".format(character.get_actor_label()),
        )

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


def validate_level_button_routing():
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    level_buttons = [
        actor
        for actor in actors
        if actor.get_class().get_path_name() == LEVEL_BUTTON_CLASS
    ]
    found_selector_buttons = set()
    for button in level_buttons:
        label = button.get_actor_label()
        targets = button.get_components_by_class(unreal.OWSInteractionTargetComponent)
        adapters = button.get_components_by_class(
            unreal.OWSLevelButtonInteractionComponent
        )
        if label in SELECTOR_LEVEL_BUTTONS:
            found_selector_buttons.add(label)
            require(
                len(targets) == 1 and len(adapters) == 1,
                "{} must have one OWS target and one LevelButton adapter".format(label),
            )
            unreal.log("OWS_SMOKE|SELECTOR_BUTTON|{}".format(label))
        else:
            require(
                not targets and not adapters,
                "Unscoped LevelButton {} received OWS selector routing".format(label),
            )
    require(
        found_selector_buttons == SELECTOR_LEVEL_BUTTONS,
        "Missing selector LevelButtons: {}".format(
            sorted(SELECTOR_LEVEL_BUTTONS - found_selector_buttons)
        ),
    )


load_required_assets()
canonical_world = load_map(CANONICAL_MAP)
unreal.SystemLibrary.execute_console_command(canonical_world, "MAP CHECK")
load_map(COURSE_MAP)
validate_characters()
validate_level_button_routing()
unreal.log("OWS_SMOKE|PASS|clean-start validation passed")
