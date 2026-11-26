import unreal, random
random.seed(7)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
V, R = unreal.Vector, lambda yaw=0.0, pitch=0.0, roll=0.0: unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)
FOLDER = "DogPark"

# clean previous run
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("DP_"):
        eas.destroy_actor(a)

def mi(name, parent_path, vec=None):
    path = f"/Game/Environment/DogPark/{name}"
    m = eal.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(name, "/Game/Environment/DogPark", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property("parent", unreal.load_asset(parent_path))
    for k, c in (vec or {}).items():
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(m, k, unreal.LinearColor(*c, 1.0))
    unreal.MaterialEditingLibrary.update_material_instance(m)
    eal.save_loaded_asset(m)
    return m

SHAPE = "/Engine/BasicShapes/BasicShapeMaterial"
mats = {n: mi(f"MI_Park_{n}", SHAPE, {"Color": c}) for n, c in {
    "Red": (0.8, 0.05, 0.04), "White": (0.9, 0.9, 0.88), "Blue": (0.05, 0.25, 0.8), "Yellow": (0.95, 0.75, 0.05),
    "Ball": (0.75, 0.95, 0.1), "Steel": (0.55, 0.57, 0.6), "Wood": (0.35, 0.2, 0.08)}.items()}

def mesh_actor(label, mesh, loc, rot, scale, mat=None, collide=True):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    a.set_actor_label(label); a.set_folder_path(FOLDER)
    c = a.static_mesh_component
    c.set_static_mesh(unreal.load_asset(mesh))
    a.set_actor_scale3d(scale)
    if mat: c.set_material(0, mat)
    if not collide: c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a

CUBE, CYL, SPH = "/Engine/BasicShapes/Cube", "/Engine/BasicShapes/Cylinder", "/Engine/BasicShapes/Sphere"
FENCE = "/Game/Environment/GardenProps/SM_FV_FencePanel"

# ---- ground and fence: x 1000..4000, y 1500..3500 (north of the garden)
mesh_actor("DP_Lawn", CUBE, V(2500, 2500, -10), R(), V(30, 20, 0.2), unreal.load_asset("/Game/Environment/Materials/M_FV_LawnNatural2"))
fm = unreal.load_asset(FENCE)
n = 0
for y in range(1625, 3500, 250):              # west side
    mesh_actor(f"DP_Fence_{n}", FENCE, V(1000, y, 0), R(90), V(1, 1, 1)); n += 1
for y in range(2625, 3500, 250):              # east side (garden fence already runs to y=2500)
    mesh_actor(f"DP_Fence_{n}", FENCE, V(4002, y, 0), R(90), V(1, 1, 1)); n += 1
for x in range(1125, 4000, 250):              # north side
    mesh_actor(f"DP_Fence_{n}", FENCE, V(x, 3500, 0), R(0), V(1, 1, 1)); n += 1
# match the garden fence's height/placement (its pivot may not be at the ground)
ref = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "BY_SM_FV_FencePanel_59"]
z = ref[0].get_actor_location().z if ref else 0.0
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("DP_Fence_"):
        l = a.get_actor_location(); a.set_actor_location(V(l.x, l.y, z), False, False)
# the gate: take out one garden fence panel
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "BY_SM_FV_FencePanel_60":
        eas.destroy_actor(a); print("gate opened at x=3125")
print("fence z", z)

# gate posts, crossbar and sign (readable from both sides)
mesh_actor("DP_GatePost_L", CUBE, V(2995, 1498, 0), R(), V(0.14, 0.14, 2.7), mats["Wood"])
mesh_actor("DP_GatePost_R", CUBE, V(3255, 1498, 0), R(), V(0.14, 0.14, 2.7), mats["Wood"])
mesh_actor("DP_GateBar", CUBE, V(3125, 1498, 245), R(), V(2.75, 0.14, 0.5), mats["Blue"], collide=False)
for side, y, yaw in (("S", 1488, -90.0), ("N", 1508, 90.0)):
    t = eas.spawn_actor_from_class(unreal.TextRenderActor, V(3125, y, 255), R(yaw))
    t.set_actor_label(f"DP_Sign_{side}"); t.set_folder_path(FOLDER)
    tr = t.text_render
    tr.set_text("DOG PARK"); tr.set_world_size(34.0)
    tr.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    tr.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    tr.set_text_render_color(unreal.Color(r=255, g=225, b=40, a=255))

# ---- park furniture
bench = "/Game/Environment/GardenProps/SM_FV_Bench"
mesh_actor("DP_Bench_1", bench, V(1250, 1900, 0), R(0), V(1, 1, 1))
mesh_actor("DP_Bench_2", bench, V(3750, 3250, 0), R(180), V(1, 1, 1))
mesh_actor("DP_Bench_3", bench, V(2200, 3330, 0), R(90), V(1, 1, 1))
# water bowl
mesh_actor("DP_Bowl", CYL, V(3500, 1750, 4), R(), V(0.55, 0.55, 0.08), mats["Steel"])
mesh_actor("DP_BowlWater", CYL, V(3500, 1750, 8), R(), V(0.48, 0.48, 0.02), mats["Blue"], collide=False)
# agility hurdles (red/white), dogs can run through the bars
for i, x in enumerate((1700, 2100, 2500)):
    for dy in (-60, 60):
        mesh_actor(f"DP_Hurdle{i}_Post{dy}", CYL, V(x, 2900 + dy, 0), R(), V(0.07, 0.07, 0.75), mats["White"])
    mesh_actor(f"DP_Hurdle{i}_Bar", CYL, V(x, 2900, 38), R(pitch=0.0, roll=90.0), V(0.05, 0.05, 1.25), mats["Red" if i % 2 == 0 else "Yellow"], collide=False)
# tunnel
mesh_actor("DP_Tunnel", CYL, V(3000, 2500, 40), R(roll=90.0), V(0.85, 0.85, 2.4), mats["Blue"], collide=False)
# tennis balls the dogs bump around
for i in range(6):
    b = mesh_actor(f"DP_Ball_{i}", SPH, V(random.uniform(1300, 3700), random.uniform(1800, 3300), 10), R(), V(0.13, 0.13, 0.13), mats["Ball"])
    c = b.static_mesh_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_simulate_physics(True)
    c.set_linear_damping(0.6); c.set_angular_damping(0.6)

# the park area itself
park = eas.spawn_actor_from_class(unreal.FVDogPark, V(2500, 2500, 100), R())
park.set_actor_label("DP_Area"); park.set_folder_path(FOLDER)
park.area.set_box_extent(V(1460, 960, 400))

# ---- the dogs
fur_white = "/Game/Characters/Dog/Fur_White"
fur_black = "/Game/Characters/Dog/Fur_Black"
vest = "/Game/Characters/Dog/MI_DogVest"
dogs = [  # name, size, coat, accents (ears etc.), vest tint or None
    ("Biscuit", 0.6, (0.95, 0.78, 0.5), (0.4, 0.22, 0.1), None),
    ("Pepper", 0.8, (0.06, 0.05, 0.05), (0.02, 0.02, 0.02), (0.9, 0.1, 0.1)),
    ("Daisy", 1.0, (0.95, 0.95, 0.92), (0.05, 0.05, 0.05), (1.0, 0.4, 0.7)),
    ("Rocky", 1.25, (0.45, 0.25, 0.12), (0.15, 0.08, 0.04), None),
    ("Luna", 1.5, (0.5, 0.52, 0.56), (0.15, 0.15, 0.17), (0.1, 0.35, 1.0)),
    ("Bruno", 2.0, (0.85, 0.58, 0.25), (0.5, 0.3, 0.12), None),
]
dog_bp = unreal.load_asset("/Game/Characters/Dog/BP_Dog").generated_class()
spots = [(1500, 2200), (2000, 2000), (2700, 2100), (3300, 2800), (1800, 3150), (3500, 2200)]
for (name, size, coat, accent, vtint), (x, y) in zip(dogs, spots):
    m_coat = mi(f"MI_Dog_{name}_Coat", fur_white, {"DiffuseColor": coat})
    m_acc = mi(f"MI_Dog_{name}_Accent", fur_black, {"DiffuseColor": accent})
    m_vest = mi(f"MI_Dog_{name}_Vest", vest, {"Tint": vtint}) if vtint else m_coat
    d = eas.spawn_actor_from_class(dog_bp, V(x, y, 22 * size + 6), R(random.uniform(0, 360)))
    d.set_actor_label(f"DP_Dog_{name}"); d.set_folder_path(FOLDER)
    d.set_actor_scale3d(V(size, size, size))
    d.set_editor_property("dog_name", name)
    d.set_editor_property("show_meter", False)
    d.set_editor_property("energy", random.uniform(0.6, 1.0))
    sk = d.mesh
    sk.set_material(0, m_coat); sk.set_material(1, m_vest); sk.set_material(2, m_acc)

# ---- navigation must cover the park too
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "BY_NavBounds":
        a.set_actor_location(V(2000, 1025, 150), False, False)
        a.set_actor_scale3d(V(21.5, 25.9, 2.5))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("dog park built:", len([a for a in eas.get_all_level_actors() if a.get_actor_label().startswith("DP_")]), "actors")

