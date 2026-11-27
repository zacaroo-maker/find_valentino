# Finding Valentino - Valentino's yard: import the water dish, and start him off with a basking stone and a couple
# of rocks by his house (like the real one had). Re-runnable.
import unreal, os
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

SRC = r"C:\Users\calif\Documents\Finding Valentino\Items\Decor\SM_FV_WaterDish.fbx"
DST = "/Game/Environment/Items/Decor"
ui = unreal.FbxImportUI(); ui.import_mesh = True; ui.import_as_skeletal = False; ui.import_materials = True; ui.import_textures = False
ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
d = ui.static_mesh_import_data
d.set_editor_property("convert_scene", True); d.set_editor_property("convert_scene_unit", True)
d.set_editor_property("combine_meshes", True); d.set_editor_property("auto_generate_collision", False)
t = unreal.AssetImportTask(); t.filename = SRC; t.destination_path = DST; t.destination_name = "SM_FV_WaterDish"
t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
at.import_asset_tasks([t])
dish = eal.load_asset(DST + "/SM_FV_WaterDish")
ns = dish.get_editor_property("nanite_settings")
if ns.enabled:
    ns.enabled = False; dish.set_editor_property("nanite_settings", ns)
eal.save_directory(DST, only_if_is_dirty=False, recursive=True)
print("dish imported")

for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("VY_"):
        eas.destroy_actor(a)

def ground(x, y):
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 300), unreal.Vector(x, y, -200),
                                                 unreal.TraceTypeQuery.ECC_VISIBILITY if hasattr(unreal.TraceTypeQuery, "ECC_VISIBILITY") else unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                                                 False, [], unreal.DrawDebugTrace.NONE, True)
    return hit.to_tuple()[4].z if hit else 0.0

def item(label, mesh_path, x, y, yaw, scale, name, tag=None):
    a = eas.spawn_actor_from_class(unreal.FVPickupItem, unreal.Vector(x, y, ground(x, y)), unreal.Rotator(0, 0, yaw))
    a.set_actor_label(label); a.set_folder_path("Garden/ValentinoYard")
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    a.get_editor_property("mesh").set_static_mesh(eal.load_asset(mesh_path))
    a.set_editor_property("kind", unreal.FVItemKind.PROP)
    a.set_editor_property("display_name", unreal.Text(name))
    if tag:
        a.tags = [unreal.Name(tag)]
    print(label, "at", a.get_actor_location())
    return a

# his house is at (450,1150), door facing +X
item("VY_BaskStone", "/Game/Environment/GardenProps/SM_FV_SteppingStone", 640, 1030, 20, 1.4, "basking stone", "FV_BaskStone")
item("VY_Rock_1", "/Game/Environment/PolyHaven/rock_moss_set_02/rock_moss_set_02_v3", 330, 1000, 70, 1.0, "mossy rock")
item("VY_Rock_2", "/Game/Environment/PolyHaven/rock_moss_set_02/rock_moss_set_02_v1", 600, 1300, 140, 0.8, "mossy rock")
les.save_current_level()
print("DONE")
