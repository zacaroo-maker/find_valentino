# Main menu + pause setup: IA_Pause (Esc + P) in IMC_Menu, BP_FV_GameMode -> AFVGameMode,
# and L_MainMenu (persistent level that streams in the garden behind the menu, with a camera).
import unreal
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
P = "/Game/Core/Input"

# pause input (works while paused)
ia = eal.load_asset(P + "/IA_Pause") if eal.does_asset_exist(P + "/IA_Pause") else at.create_asset("IA_Pause", P, unreal.InputAction, unreal.InputAction_Factory())
ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
ia.set_editor_property("trigger_when_paused", True)
eal.save_loaded_asset(ia)
imc_path = P + "/IMC_Menu"
imc = eal.load_asset(imc_path) if eal.does_asset_exist(imc_path) else at.create_asset("IMC_Menu", P, unreal.InputMappingContext, unreal.InputMappingContext_Factory())
dk = imc.get_editor_property("default_key_mappings")
maps = list(dk.get_editor_property("mappings"))
for key in ("Escape", "P"):
    if not any(m.action == ia and m.key.export_text() == key for m in maps):
        k = unreal.Key(); k.import_text(key)
        m = unreal.EnhancedActionKeyMapping(); m.set_editor_property("action", ia); m.set_editor_property("key", k); maps.append(m)
dk.set_editor_property("mappings", maps); imc.set_editor_property("default_key_mappings", dk)
imc.modify(); eal.save_asset(imc_path, only_if_is_dirty=False)
print("IMC_Menu:", [(m.action.get_name(), m.key.export_text()) for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings")])

# garden game mode: reparent the Blueprint onto the C++ game mode (keeps BP_Dad as the default character)
gm = eal.load_asset("/Game/Core/BP_FV_GameMode")
unreal.BlueprintEditorLibrary.reparent_blueprint(gm, unreal.FVGameMode)
cdo = unreal.get_default_object(gm.generated_class())
cdo.set_editor_property("player_controller_class", unreal.FVPlayerController)
cdo.set_editor_property("hud_class", unreal.FVHUD)
unreal.BlueprintEditorLibrary.compile_blueprint(gm); eal.save_loaded_asset(gm)
print("BP_FV_GameMode parent ->", unreal.FVGameMode, "| pawn:", cdo.get_editor_property("default_pawn_class").get_name())

# main menu level
MENU = "/Game/Maps/L_MainMenu"
if not eal.does_asset_exist(MENU):
    les.new_level(MENU)
else:
    les.load_level(MENU)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("MM_"):
        eas.destroy_actor(a)
# stream the garden in behind the menu (Valentino and Diego carry on with their day in the background)
if not any(str(l.get_world_asset_package_name()).endswith("L_Backyard") for l in unreal.EditorLevelUtils.get_levels(world) if False):
    try:
        unreal.EditorLevelUtils.add_level_to_world(world, "/Game/Maps/L_Backyard", unreal.LevelStreamingAlwaysLoaded)
    except Exception as e:
        print("add level:", e)
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(-200, 1250, 260), unreal.Rotator(roll=0.0, pitch=-12.0, yaw=-30.0))
cam.set_actor_label("MM_MenuCamera"); cam.tags = [unreal.Name("MenuCamera")]
cam.camera_component.set_editor_property("field_of_view", 70.0)
world.get_world_settings().set_editor_property("default_game_mode", unreal.FVMenuGameMode)
les.save_current_level()
print("menu level saved")
les.load_level("/Game/Maps/L_Backyard")
