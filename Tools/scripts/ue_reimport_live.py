import unreal
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
SRC = r"C:\Users\calif\Documents\Finding Valentino\Dad_fixed.fbx"
DEST = "/Game/Characters/Dad"

# remove temp preview actor so the asset can be replaced
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "TEMP_DadPreview":
        eas.destroy_actor(a)
aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
old = eal.load_asset(DEST + "/SK_Dad") if eal.does_asset_exist(DEST + "/SK_Dad") else None
if old:
    aes.close_all_editors_for_asset(old)
    old = None
if eal.does_directory_exist(DEST):
    eal.delete_directory(DEST)

ui = unreal.FbxImportUI()
ui.import_mesh = True; ui.import_as_skeletal = True
ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
ui.import_animations = False; ui.import_materials = True; ui.import_textures = True
ui.create_physics_asset = True
sd = ui.skeletal_mesh_import_data
sd.set_editor_property("convert_scene", True)
sd.set_editor_property("convert_scene_unit", True)
t = unreal.AssetImportTask(); t.filename = SRC; t.destination_path = DEST; t.destination_name = "SK_Dad"
t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])

m = eal.load_asset(DEST + "/SK_Dad")
empty = [str(s.material_slot_name) for s in m.materials if not s.material_interface]
print("empty slots after import:", len(empty))
eal.save_directory(DEST, only_if_is_dirty=False, recursive=True)
for p in eal.list_assets(DEST):
    print(" ", eal.load_asset(p).get_class().get_name(), p.split(".")[-1])
b = m.get_bounds(); print("height cm:", round(b.box_extent.z * 2, 1))
