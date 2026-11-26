# Art pass: swap blockout meshes in L_Backyard for Poly Haven scans and add ground cover.
# Positions of hiding-relevant props are kept. Re-runnable: removes previous "AP_" actors first,
# and re-dresses "BY_" actors from their folder category.
import unreal, random, math
random.seed(42)
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
PH = "/Game/Environment/PolyHaven/"

def meshes(folder):
    return [p.split(".")[0] for p in eal.list_assets(PH + folder, recursive=False)
            if isinstance(eal.load_asset(p), unreal.StaticMesh)]
def one(folder):
    return meshes(folder)[0]

GRASS_TALL = meshes("grass_medium_02")[-1:] * 3 + meshes("grass_medium_01")[:11]   # v3 is the big clump
GRASS_TUFT = meshes("grass_medium_01")
SHRUBS = meshes("shrub_02")
SMALL_PLANTS = meshes("periwinkle_plant") + meshes("nettle_plant") + meshes("shrub_03")
FLOWERS = meshes("flower_gazania") + meshes("celandine_01") + meshes("dandelion_01")
WEEDS = meshes("weed_plant_02") + meshes("dandelion_01")[:3]
FERNS = meshes("fern_02")
ROCKS_MID = meshes("rock_moss_set_02")
BRANCHES = meshes("dry_branches_medium_01")

# ---- collision setup on solid scans ----
for f in ("rock_moss_set_02", "rock_moss_set_01", "boulder_01", "rock_07", "tree_stump_01", "tree_stump_02",
          "dead_tree_trunk", "painted_wooden_bench", "planter_pot_clay", "potted_plant_01", "potted_plant_02", "planter_box_01"):
    for p in meshes(f):
        m = eal.load_asset(p); bs = m.get_editor_property("body_setup")
        if bs and bs.get_editor_property("collision_trace_flag") != unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            m.modify(); eal.save_loaded_asset(m)

def R(yaw=0.0, pitch=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)

def set_mesh(a, path, scale=1.0, yaw=None, collide=True, z=None):
    c = a.static_mesh_component
    c.set_static_mesh(eal.load_asset(path))
    n = c.get_num_materials()
    for i in range(n):
        c.set_material(i, None)   # use the scan's own materials
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    if yaw is not None:
        r = a.get_actor_rotation(); a.set_actor_rotation(R(yaw, r.pitch, r.roll), False)
    if z is not None:
        l = a.get_actor_location(); a.set_actor_location(unreal.Vector(l.x, l.y, z), False, False)
    if collide:
        c.set_collision_profile_name("BlockAll"); c.set_editor_property("can_ever_affect_navigation", True)
    else:
        c.set_collision_profile_name("NoCollision"); c.set_editor_property("can_ever_affect_navigation", False)

N = {"n": 0}
def spawn(path, x, y, z=0.0, yaw=None, scale=1.0, collide=False, folder="Garden/ArtPass"):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), R(random.uniform(0, 360) if yaw is None else yaw))
    set_mesh(a, path, scale, None, collide)
    N["n"] += 1; a.set_actor_label(f"AP_{path.split('/')[-1]}_{N['n']}"); a.set_folder_path(folder)
    return a

def collider(x, y, radius, height=400):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, height / 2))
    a.static_mesh_component.set_static_mesh(eal.load_asset("/Engine/BasicShapes/Cylinder"))
    a.set_actor_scale3d(unreal.Vector(radius / 50.0, radius / 50.0, height / 100.0))
    a.set_actor_hidden_in_game(True)
    a.static_mesh_component.set_editor_property("visible", False)
    N["n"] += 1; a.set_actor_label(f"AP_TrunkCollider_{N['n']}"); a.set_folder_path("Garden/ArtPass/Colliders")

# ---- clean previous art-pass actors ----
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith(("AP_", "TEMP_Preview")):
        eas.destroy_actor(a)

actors = [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith("BY_") and isinstance(a, unreal.StaticMeshActor)]
def folder(a):
    return str(a.get_folder_path())
def mname(a):
    sm = a.static_mesh_component.static_mesh
    return sm.get_name() if sm else ""

swapped = {}
tree_plan = [("island_tree_01", 1.25), ("tree_small_02", 1.35), ("island_tree_01", 1.05), ("jacaranda_tree", 0.42)]
ti = 0
for a in actors:
    f, mn = folder(a), mname(a)
    l = a.get_actor_location()
    if f == "Garden/TallGrass":
        p = random.choice(GRASS_TALL)
        set_mesh(a, p, random.uniform(1.4, 1.9) if "grass_medium_02" in p else random.uniform(1.9, 2.8), random.uniform(0, 360), False, 0)
    elif f in ("Garden/Hedge", "Garden/Bushes"):
        set_mesh(a, random.choice(SHRUBS), random.uniform(0.75, 1.0) if f == "Garden/Hedge" else random.uniform(0.6, 0.95), random.uniform(0, 360), False, 0)
    elif f == "Garden/Flowers":
        set_mesh(a, random.choice(FLOWERS), random.uniform(1.4, 2.0), random.uniform(0, 360), False)
    elif f == "Garden/Trees":
        tid, sc = tree_plan[ti % len(tree_plan)]; ti += 1
        set_mesh(a, one(tid), sc, random.uniform(0, 360), False, 0)
        collider(l.x, l.y, 22 if tid != "jacaranda_tree" else 35)
    elif f == "Garden/Rocks":
        s = a.get_actor_scale3d().x
        if s > 0.11:   # rock garden
            set_mesh(a, random.choice(ROCKS_MID), random.uniform(0.22, 0.4), random.uniform(0, 360), True, -3)
        else:
            set_mesh(a, one("rock_07") if random.random() < 0.6 else one("stone_01"), random.uniform(1.2, 2.2), random.uniform(0, 360), True, 0)
    elif f == "Garden/Logs" and mn == "SM_FV_Log":
        set_mesh(a, one("dead_tree_trunk"), 0.75, None, True, 0)
    elif f == "Garden/Logs" and mn == "SM_FV_LogHollow":
        a.static_mesh_component.set_material(0, eal.load_asset(PH + "dead_tree_trunk/dead_tree_trunk"))
    elif f == "Garden/Pots":
        if mn == "SM_FV_PottedPlant":
            set_mesh(a, one("potted_plant_02") if random.random() < 0.5 else one("potted_plant_01"), random.uniform(0.55, 0.7), None, True, l.z)
        elif mn == "SM_FV_Flowerpot" and abs(a.get_actor_rotation().pitch) < 10:
            set_mesh(a, one("planter_pot_clay"), random.uniform(1.2, 1.5), None, True, l.z)
        else:   # tipped pots keep their shape (hiding spots), get the clay scan's material
            a.static_mesh_component.set_material(0, eal.load_asset(PH + "planter_pot_clay/planter_pot_clay"))
    elif f == "Garden/Furniture" and mn == "SM_FV_Bench":
        set_mesh(a, one("painted_wooden_bench"), 1.15, None, True, l.z)
    else:
        continue
    swapped[f] = swapped.get(f, 0) + 1

# ---- new ground cover ----
def lawn_point():
    return random.uniform(500, 3900), random.uniform(-1400, 1400)
for _ in range(220):    # grass tufts all over the lawn
    x, y = lawn_point(); spawn(random.choice(GRASS_TUFT), x, y, 0, None, random.uniform(1.0, 1.8))
for _ in range(70):     # weeds and dandelions
    x, y = lawn_point(); spawn(random.choice(WEEDS), x, y, 0, None, random.uniform(1.0, 1.6))
for _ in range(40):     # small plants along fences and beds
    side = random.choice(((random.uniform(400, 3950), 1440), (random.uniform(400, 3950), -1440), (3940, random.uniform(-1400, 1400))))
    spawn(random.choice(SMALL_PLANTS), side[0] + random.uniform(-25, 25), side[1] + random.uniform(-25, 25), 0, None, random.uniform(1.2, 2.0))
TREES = [(1700, -950), (3200, -250), (2500, 1050), (3650, 1150)]
for (tx, ty) in TREES:  # ferns, branches under trees
    for _ in range(5):
        a_ = random.uniform(0, 2 * math.pi); d = random.uniform(80, 260)
        spawn(random.choice(FERNS), tx + math.cos(a_) * d, ty + math.sin(a_) * d, 0, None, random.uniform(0.7, 1.2))
    for _ in range(2):
        a_ = random.uniform(0, 2 * math.pi); d = random.uniform(100, 250)
        spawn(random.choice(BRANCHES), tx + math.cos(a_) * d, ty + math.sin(a_) * d, 0, None, random.uniform(0.8, 1.2))
spawn(one("root_cluster_01"), 3230, -230, -25, 30, 0.45, False)   # roots at the base of a tree
spawn(one("tree_stump_02"), 2700, -1050, 0, 15, 0.7, True)          # stump near the hedge
spawn(one("boulder_01"), 1350, 520, -15, 40, 0.45, True)           # centrepiece of the rock garden
spawn(one("planter_box_01"), 420, 330, 2, 90, 1.0, True)           # planter on the patio edge

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("swapped:", swapped)
print("new art-pass actors:", N["n"])
