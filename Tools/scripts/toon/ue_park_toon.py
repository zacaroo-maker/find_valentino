# Toon project: import the cartoon dog park props (Toon\Park\SM_Park_*.fbx) and dress the park with them.
# The realistic trees, bushes and steel bowl in the park are hidden (not deleted) and moved to DogPark/Replaced.
import unreal, os
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
at = unreal.AssetToolsHelpers.get_asset_tools()
SRC = r"C:\Users\calif\Documents\Finding Valentino\Toon\Park"
DEST = "/Game/Toon/Park"
V = unreal.Vector

tasks = []
for f in sorted(os.listdir(SRC)):
    if f.endswith(".fbx"):
        ui = unreal.FbxImportUI(); ui.import_mesh = True; ui.import_as_skeletal = False
        ui.import_materials = True; ui.import_textures = False
        d = ui.static_mesh_import_data
        d.set_editor_property("convert_scene", True); d.set_editor_property("convert_scene_unit", True)
        d.set_editor_property("combine_meshes", True); d.set_editor_property("auto_generate_collision", True)
        t = unreal.AssetImportTask(); t.filename = os.path.join(SRC, f); t.destination_path = DEST
        t.destination_name = f[:-4]; t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
        tasks.append(t)
at.import_asset_tasks(tasks)
eal.save_directory(DEST, only_if_is_dirty=False, recursive=True)
mesh = {t.destination_name[len("SM_Park_"):]: eal.load_asset(f"{DEST}/{t.destination_name}") for t in tasks}
print("imported:", sorted(mesh))

# clean a previous run
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("DP_Toon_"):
        eas.destroy_actor(a)

# hide the realistic things this replaces
for a in eas.get_all_level_actors():
    if a.get_actor_label() in ("NB_island_tree_011_12", "NB_island_tree_011_13", "NB_SM_Bush_18", "NB_SM_Bush_22",
                               "NB_SM_Bush_23", "DP_Bowl", "DP_BowlWater"):
        a.set_actor_hidden_in_game(True); a.set_is_temporarily_hidden_in_editor(True)
        a.set_actor_enable_collision(False); a.set_folder_path("DogPark/Replaced")

def place(label, key, x, y, yaw=0.0, s=1.0):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, V(x, y, 0), unreal.Rotator(0, 0, yaw))
    a.set_actor_label("DP_Toon_" + label); a.set_folder_path("DogPark/Toon")
    a.static_mesh_component.set_static_mesh(mesh[key]); a.set_actor_scale3d(V(s, s, s))
    return a

place("Tree1", "TreeRound", 1200, 2500, 20, 1.15)
place("Tree2", "TreePine", 3320, 2720, 0, 1.0)
place("Tree3", "TreeRound", 1380, 3230, 140, 0.95)
place("Tree4", "TreePine", 3820, 2120, 0, 0.85)
place("Tree5", "TreeRound", 2720, 3300, 260, 0.9)
place("Tree6", "TreePine", 3830, 2950, 0, 1.1)
for i, (x, y, yaw, s) in enumerate(((2009, 1653, 0, 1.2), (1739, 1654, 70, 1.0), (2727, 1627, 150, 1.25),
                                    (1080, 3080, 30, 1.1), (3880, 1650, 200, 1.0), (1650, 3420, 90, 0.9))):
    place(f"Bush{i}", "Bush", x, y, yaw, s)
place("Fountain", "Fountain", 3520, 1790, 180)
place("AFrame", "AFrame", 2000, 2450, 0)
place("WeavePoles", "WeavePoles", 2780, 3150, 0)

# the ball launcher (game actor), facing north into the park
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "DP_BallLauncher":
        eas.destroy_actor(a)
bl = eas.spawn_actor_from_class(unreal.FVBallLauncher, V(2650, 1850, 0), unreal.Rotator(0, 0, 90))
bl.set_actor_label("DP_BallLauncher"); bl.set_folder_path("DogPark")
bl.get_editor_property("body").set_static_mesh(mesh["BallLauncher"])
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("park dressed and saved")

# afterwards (run once the matte materials exist): glossier water, see FVBallLauncher LoadRadius 200
# Park_Water roughness 0.1, Park_White 0.4, Park_Black 0.3
