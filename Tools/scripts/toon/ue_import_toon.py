# Toon project: import the cartoon Valentino, Diego and the cartoon person; hook them up.
import unreal, os
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
T = r"C:\Users\calif\Documents\Finding Valentino\Toon"

def import_skel(fbx, dest, name):
    ui = unreal.FbxImportUI()
    ui.import_mesh = True; ui.import_as_skeletal = True
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    ui.import_animations = True; ui.import_materials = True; ui.import_textures = True
    ui.create_physics_asset = True
    for d in (ui.skeletal_mesh_import_data, ui.anim_sequence_import_data):
        d.set_editor_property("convert_scene", True); d.set_editor_property("convert_scene_unit", True)
    ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
    t = unreal.AssetImportTask(); t.filename = os.path.join(T, fbx); t.destination_path = dest; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
    at.import_asset_tasks([t])
    eal.save_directory(dest, only_if_is_dirty=False, recursive=True)

def hook(bp_path, mesh_path, anim_prefix, props):
    bp = eal.load_asset(bp_path)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.get_editor_property("mesh").set_editor_property("skeletal_mesh_asset", eal.load_asset(mesh_path))
    for prop, name in props:
        cdo.set_editor_property(prop, eal.load_asset(anim_prefix + name))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp)
    return bp

# --- Valentino ---
import_skel("Valentino_Toon.fbx", "/Game/Toon/Valentino", "SK_Valentino_Toon")
hook("/Game/Characters/Valentino/BP_Valentino", "/Game/Toon/Valentino/SK_Valentino_Toon", "/Game/Toon/Valentino/SK_Valentino_ToonArmature_",
     (("idle_anim", "Idle"), ("walk_anim", "Walk"), ("hide_anim", "Hide"), ("emerge_anim", "Emerge")))
# --- Diego (and the other dogs, which share the dog mesh) ---
import_skel("Dog_Toon.fbx", "/Game/Toon/Dog", "SK_Dog_Toon")
hook("/Game/Characters/Dog/BP_Dog", "/Game/Toon/Dog/SK_Dog_Toon", "/Game/Toon/Dog/SK_Dog_ToonArmature_",
     (("idle_anim", "Idle"), ("walk_anim", "Walk"), ("sniff_anim", "Sniff"), ("lie_down_anim", "LieDown"), ("lying_anim", "Lying"), ("get_up_anim", "GetUp")))
# level instances that override the mesh get the new one too
world = unreal.EditorLevelLibrary.get_editor_world()
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Character):
    cls = a.get_class().get_name()
    comp = a.get_editor_property("mesh")
    if cls.startswith("BP_Valentino"):
        comp.set_editor_property("skeletal_mesh_asset", eal.load_asset("/Game/Toon/Valentino/SK_Valentino_Toon"))
    elif cls.startswith("BP_Dog"):
        comp.set_editor_property("skeletal_mesh_asset", eal.load_asset("/Game/Toon/Dog/SK_Dog_Toon"))

# --- the cartoon person (Family Friend) ---
P = os.path.join(T, "Person")
tasks = []
for f in sorted(os.listdir(P)):
    if f.endswith(".fbx"):
        ui = unreal.FbxImportUI(); ui.import_mesh = True; ui.import_as_skeletal = False
        ui.import_materials = True; ui.import_textures = False
        d = ui.static_mesh_import_data
        d.set_editor_property("convert_scene", True); d.set_editor_property("convert_scene_unit", True)
        d.set_editor_property("combine_meshes", True); d.set_editor_property("auto_generate_collision", False)
        t = unreal.AssetImportTask(); t.filename = os.path.join(P, f); t.destination_path = "/Game/Toon/Person"
        t.destination_name = f[:-4]; t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
        tasks.append(t)
at.import_asset_tasks(tasks)
eal.save_directory("/Game/Toon/Person", only_if_is_dirty=False, recursive=True)
parts = []
for t in tasks:
    name = t.destination_name
    sm = eal.load_asset("/Game/Toon/Person/" + name)
    p = unreal.FVToonPart(); p.set_editor_property("bone", name[len("SM_Toon_"):]); p.set_editor_property("mesh", sm)
    parts.append(p)
    b = sm.get_bounds()
    print(f"{name:22} centre=({b.origin.x:6.1f},{b.origin.y:6.1f},{b.origin.z:6.1f})")
da = eal.load_asset("/Game/Characters/Family/Data/DA_Friend")
da.set_editor_property("toon_parts", parts)
da.set_editor_property("female_frame", False)
eal.save_loaded_asset(da)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("toon parts on DA_Friend:", len(parts))
