import unreal
eal = unreal.EditorAssetLibrary
SRC = r"C:\Users\calif\Documents\Finding Valentino\Valentino.fbx"
DEST = "/Game/Characters/Valentino"
if eal.does_directory_exist(DEST):
    eal.delete_directory(DEST)
ui = unreal.FbxImportUI()
ui.import_mesh = True; ui.import_as_skeletal = True
ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
ui.import_animations = True; ui.import_materials = True; ui.import_textures = False
ui.create_physics_asset = True
ui.skeletal_mesh_import_data.set_editor_property("convert_scene", True)
ui.skeletal_mesh_import_data.set_editor_property("convert_scene_unit", True)
ui.anim_sequence_import_data.set_editor_property("convert_scene", True)
ui.anim_sequence_import_data.set_editor_property("convert_scene_unit", True)
ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
t = unreal.AssetImportTask(); t.filename = SRC; t.destination_path = DEST; t.destination_name = "SK_Valentino"
t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
eal.save_directory(DEST, only_if_is_dirty=False, recursive=True)
for p in eal.list_assets(DEST):
    a = eal.load_asset(p)
    extra = ""
    if isinstance(a, unreal.AnimSequence):
        extra = f" frames={unreal.AnimationLibrary.get_num_frames(a)}"
    if isinstance(a, unreal.SkeletalMesh):
        b = a.get_bounds(); extra = f" extent=({b.box_extent.x:.1f},{b.box_extent.y:.1f},{b.box_extent.z:.1f}) origin_z={b.origin.z:.1f}"
        sk = a.skeleton; r = unreal.AnimPoseExtensions.get_bone_pose(unreal.AnimPoseExtensions.get_reference_pose(sk), "root", unreal.AnimPoseSpaces.LOCAL)
        extra += f" root_scale={r.scale3d.x:.2f}"
    print(f"{a.get_class().get_name():14} {p.split('.')[-1]}{extra}")
