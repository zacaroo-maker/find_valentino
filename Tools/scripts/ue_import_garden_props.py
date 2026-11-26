import unreal, os
eal = unreal.EditorAssetLibrary
SRC = r"C:\Users\calif\Documents\Finding Valentino\GardenProps"
DEST = "/Game/Environment/GardenProps"
SOLID = ("Flowerpot", "PottedPlant", "LogHollow", "Log", "Tree", "Bench", "FencePanel", "RaisedBed", "SteppingStone")

tasks = []
for f in sorted(os.listdir(SRC)):
    if not f.endswith(".fbx"):
        continue
    ui = unreal.FbxImportUI()
    ui.import_mesh = True; ui.import_as_skeletal = False; ui.import_animations = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    ui.import_materials = True; ui.import_textures = False
    sd = ui.static_mesh_import_data
    sd.set_editor_property("convert_scene", True)
    sd.set_editor_property("convert_scene_unit", True)
    sd.set_editor_property("combine_meshes", True)
    sd.set_editor_property("auto_generate_collision", False)
    sd.set_editor_property("generate_lightmap_u_vs", True)
    t = unreal.AssetImportTask(); t.filename = os.path.join(SRC, f); t.destination_path = DEST
    t.destination_name = f[:-4]; t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for p in eal.list_assets(DEST, recursive=False):
    a = eal.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        name = a.get_name().replace("SM_FV_", "")
        bs = a.get_editor_property("body_setup")
        if bs:
            solid = name in SOLID
            bs.set_editor_property("collision_trace_flag",
                unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE if solid else unreal.CollisionTraceFlag.CTF_USE_DEFAULT)
        b = a.get_bounds()
        print(f"MESH {a.get_name():24} size=({b.box_extent.x*2:.0f},{b.box_extent.y*2:.0f},{b.box_extent.z*2:.0f})")
    elif isinstance(a, unreal.Material):
        if a.get_name() in ("FV_Grass", "FV_GrassDry", "FV_Stem", "FV_Leaves", "FV_LeavesLight", "FV_DeadLeaves"):
            a.set_editor_property("two_sided", True)
            unreal.MaterialEditingLibrary.recompile_material(a)
eal.save_directory(DEST, only_if_is_dirty=False, recursive=True)
print("DONE", len(eal.list_assets(DEST)))
