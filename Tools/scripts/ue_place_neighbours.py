# Place neighbour houses, a wide ground plane and trees around the yard in L_Backyard.
# House meshes: pivot at front-centre, front faces +Y, body extends to -Y.
# Re-runnable: removes previous "NB_" actors.
import unreal, random
random.seed(11)
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
H = "/Game/Environment/Houses/SM_FV_House_"
PH = "/Game/Environment/PolyHaven/"

for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("NB_"):
        eas.destroy_actor(a)

N = {"n": 0}
def place(path, x, y, z, yaw, scale=1.0, folder="Neighbours", collide=True, mat=None):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    c = a.static_mesh_component; c.set_static_mesh(eal.load_asset(path))
    a.set_actor_scale3d(unreal.Vector(*scale) if isinstance(scale, tuple) else unreal.Vector(scale, scale, scale))
    if mat:
        c.set_material(0, eal.load_asset(mat))
    if not collide:
        c.set_collision_profile_name("NoCollision"); c.set_editor_property("can_ever_affect_navigation", False)
    N["n"] += 1; a.set_actor_label(f"NB_{path.split('/')[-1]}_{N['n']}"); a.set_folder_path(folder)
    return a

# wide ground so the world doesn't end at our fence (top at z=-2, under our lawn)
place("/Engine/BasicShapes/Cube", 2500, 0, -12, 0, (260, 200, 0.2), "Neighbours/Ground", True,
      "/Game/Environment/Materials/M_FV_LawnNatural2")

# back row (behind the back fence at X=4000): backs face our yard, fronts face +X (their street)
for letter, y in (("A", -2950), ("B", -950), ("D", 1050), ("F", 3000)):
    place(H + letter, 6300, y, 0, -90)
# side neighbours, level with our house: fronts face -X (the street in front of our house)
place(H + "C", -1150, -2900, 0, 90)
place(H + "E", -1150, 2850, 0, 90)

# trees in the neighbours' gardens
def mesh_in(folder):
    return [p.split(".")[0] for p in eal.list_assets(PH + folder, recursive=False) if isinstance(eal.load_asset(p), unreal.StaticMesh)][0][len(PH):]
trees = [(mesh_in("island_tree_01"), 1.2), (mesh_in("tree_small_02"), 1.3)]
for (x, y) in ((4700, -2100), (5000, 300), (4600, 2200), (2600, -2400), (1200, 2500), (3300, 2700), (500, -2500)):
    p, s = random.choice(trees)
    place(PH + p, x, y, 0, random.uniform(0, 360), s * random.uniform(0.85, 1.15), "Neighbours/Trees", False)
# hedges/bushes along the neighbours' side of the fences
bush = "/Game/StarterContent/Props/SM_Bush"
for i in range(10):
    place(bush, random.uniform(500, 3800), random.choice((-1, 1)) * random.uniform(1620, 1700), 0, random.uniform(0, 360), random.uniform(1.2, 1.7), "Neighbours/Bushes", False)
for i in range(6):
    place(bush, random.uniform(4120, 4200), random.uniform(-1400, 1400), 0, random.uniform(0, 360), random.uniform(1.2, 1.7), "Neighbours/Bushes", False)

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("placed", N["n"], "neighbour actors")
