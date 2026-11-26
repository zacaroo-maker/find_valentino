# Finding Valentino - build the blockout backyard level /Game/Maps/L_Backyard.
# Yard: X 0..4000 (house at X<0, back fence at X=4000), Y -1500..1500 (side fences). cm.
import unreal, random, math
random.seed(20261125)
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
MAP = "/Game/Maps/L_Backyard"
GP = "/Game/Environment/GardenProps/SM_FV_"
SC = "/Game/StarterContent/Props/SM_"
CUBE = "/Engine/BasicShapes/Cube"

if eal.does_asset_exist(MAP):
    les.load_level(MAP)
    for a in eas.get_all_level_actors():
        if a.get_actor_label().startswith("BY_"):
            eas.destroy_actor(a)
else:
    les.new_level(MAP)

def R(yaw=0.0, pitch=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)

COUNT = {"n": 0}
def mesh(path, x, y, z=0.0, yaw=0.0, scale=1.0, collide=True, folder="Garden", label=None, mat=None, pitch=0.0, roll=0.0):
    sm = eal.load_asset(path)
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), R(yaw, pitch, roll))
    c = a.static_mesh_component
    c.set_static_mesh(sm)
    a.set_actor_scale3d(unreal.Vector(*scale) if isinstance(scale, tuple) else unreal.Vector(scale, scale, scale))
    if mat:
        c.set_material(0, eal.load_asset(mat))
    if not collide:
        c.set_collision_profile_name("NoCollision")
        c.set_editor_property("can_ever_affect_navigation", False)
        c.set_editor_property("cast_shadow", True)
    COUNT["n"] += 1
    a.set_actor_label(f"BY_{label or path.split('/')[-1]}_{COUNT['n']}")
    a.set_folder_path(folder)
    return a

def block(x, y, z, sx, sy, sz, mat, folder="Garden/Structure", label="Block", collide=True):
    # Cube is 100 cm, pivot at centre
    return mesh(CUBE, x, y, z, 0, (sx / 100, sy / 100, sz / 100), collide, folder, label, mat)

def spot(x, y, z=2.0, attract=1.0, rain=True, label="HidingSpot"):
    a = eas.spawn_actor_from_class(unreal.FVHidingSpot, unreal.Vector(x, y, z))
    a.set_editor_property("attractiveness", attract); a.set_editor_property("shelters_from_rain", rain)
    COUNT["n"] += 1; a.set_actor_label(f"BY_{label}_{COUNT['n']}"); a.set_folder_path("Gameplay/HidingSpots")
    return a

def scatter(cx, cy, radius, n, paths, smin, smax, collide=False, folder="Garden/Plants", z=0.0, avoid=None):
    out = []
    for _ in range(n):
        for _try in range(10):
            a = random.uniform(0, 2 * math.pi); d = radius * math.sqrt(random.random())
            x, y = cx + math.cos(a) * d, cy + math.sin(a) * d
            if not avoid or not avoid(x, y):
                break
        out.append(mesh(random.choice(paths), x, y, z, random.uniform(0, 360), random.uniform(smin, smax), collide, folder))
    return out

# ---------------- lighting / sky ----------------
sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), R(yaw=-35, pitch=-42))
sun.set_actor_label("BY_Sun"); sun.set_folder_path("Lighting")
lc = sun.get_component_by_class(unreal.DirectionalLightComponent)
lc.set_editor_property("intensity", 9.0)
lc.set_editor_property("atmosphere_sun_light", True)
lc.set_mobility(unreal.ComponentMobility.MOVABLE)
for cls, lab in ((unreal.SkyAtmosphere, "BY_SkyAtmosphere"), (unreal.SkyLight, "BY_SkyLight"),
                 (unreal.ExponentialHeightFog, "BY_Fog"), (unreal.VolumetricCloud, "BY_Clouds")):
    a = eas.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0)); a.set_actor_label(lab); a.set_folder_path("Lighting")
    if cls == unreal.SkyLight:
        sl = a.get_component_by_class(unreal.SkyLightComponent)
        sl.set_mobility(unreal.ComponentMobility.MOVABLE)
        sl.set_editor_property("real_time_capture", True)
    if cls == unreal.ExponentialHeightFog:
        f = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
        f.set_editor_property("fog_density", 0.01)
pp = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(2000, 0, 200))
pp.set_editor_property("unbound", True); pp.set_actor_label("BY_PostProcess"); pp.set_folder_path("Lighting")

# ---------------- ground, house, patio, fence ----------------
block(2000, 0, -10, 4400, 3400, 20, "/Game/Environment/Materials/M_FV_Lawn", label="Lawn")
block(-450, -200, 300, 800, 1900, 600, "/Game/Environment/Materials/M_FV_BrickWall", label="House")
mesh(SC + "Door", -48, -300, 0, 90, 1.0, True, "Garden/Structure", "BackDoor")
mesh(SC + "GlassWindow", -45, 250, 120, 90, 1.0, True, "Garden/Structure", "Window")
block(200, -250, 1, 500, 1300, 2, "/Game/Environment/Materials/M_FV_Patio", label="Patio")   # 2 cm high: tortoise can step on
# patio furniture
mesh(SC + "TableRound", 230, -450, 2, 0, 1.0, True, "Garden/Furniture", "PatioTable")
for i, (dx, dy, yaw) in enumerate(((-90, 0, 0), (90, 0, 180), (0, -90, 90), (0, 90, -90))):
    mesh(SC + "Chair", 230 + dx, -450 + dy, 2, yaw, 1.0, True, "Garden/Furniture", "PatioChair")
for i, y in enumerate((60, 140, 220, 300)):
    mesh(GP + ("PottedPlant" if i % 2 == 0 else "Flowerpot"), 30, y, 2, random.uniform(0, 360), random.uniform(0.9, 1.3), True, "Garden/Pots")
mesh(GP + "Bench", 60, -800, 2, 90, 1.0, True, "Garden/Furniture", "PatioBench")

# fence: back (X=4000) and sides (Y=+-1500); panels 250 wide
for i in range(16):
    y = -1500 + 125 + i * 250
    mesh(GP + "FencePanel", 4000, y, 0, 90, 1.0, True, "Garden/Fence")
for side in (-1, 1):
    for i in range(16):
        x = 0 + 125 + i * 250
        mesh(GP + "FencePanel", x, side * 1500, 0, 0, 1.0, True, "Garden/Fence")

# ---------------- paths ----------------
def path_points():
    pts = []
    for i in range(30):
        t = i / 29.0
        x = 450 + t * 3200
        y = -300 + 900 * math.sin(t * math.pi * 1.3) * (0.4 + 0.6 * t)
        pts.append((x, y))
    return pts
PATH = path_points()
for (x, y) in PATH:
    mesh(GP + "SteppingStone", x + random.uniform(-15, 15), y + random.uniform(-15, 15), 0, random.uniform(0, 360), random.uniform(0.9, 1.25), True, "Garden/Path")
def near_path(x, y, r=90):
    return any((x - px) ** 2 + (y - py) ** 2 < r * r for px, py in PATH) or x < 480
# gravel area around the shed corner
block(3600, -1200, 0.5, 600, 500, 1, "/Game/Environment/Materials/M_FV_Gravel", label="Gravel", collide=False)

# ---------------- trees, hedge, bushes ----------------
TREES = ((1700, -950, 0.9), (3200, -250, 1.0), (2500, 1050, 0.85), (3650, 1150, 0.75))
for (x, y, s) in TREES:
    mesh(GP + "Tree", x, y, 0, random.uniform(0, 360), s, True, "Garden/Trees")
    mesh(GP + "LeafPile", x + random.uniform(-150, 150), y + random.uniform(-150, 150), -2, random.uniform(0, 360), random.uniform(0.9, 1.3), False, "Garden/Ground")
# hedge along the left fence
for i in range(14):
    mesh(SC + "Bush", 1900 + i * 140, -1400, 0, random.uniform(0, 360), (1.5, 1.3, 1.2), False, "Garden/Hedge")
# loose bushes
scatter(1200, 1300, 250, 4, [SC + "Bush"], 0.8, 1.3, avoid=near_path, folder="Garden/Bushes")
scatter(3300, 600, 300, 4, [SC + "Bush"], 0.8, 1.4, avoid=near_path, folder="Garden/Bushes")
scatter(800, -1300, 200, 3, [SC + "Bush"], 0.9, 1.2, avoid=near_path, folder="Garden/Bushes")
scatter(3850, -400, 250, 3, [SC + "Bush"], 0.9, 1.3, avoid=near_path, folder="Garden/Bushes")

# ---------------- beds, flowers, grass ----------------
mesh(GP + "RaisedBed", 1000, 1350, 0, 0, 1.0, True, "Garden/Beds")
mesh(GP + "RaisedBed", 2350, -1330, 0, 0, 1.0, True, "Garden/Beds")
FL = [GP + "Flowers_Red", GP + "Flowers_Yellow", GP + "Flowers_Purple"]
for bx, by in ((1000, 1350), (2350, -1330)):
    for i in range(10):
        mesh(random.choice(FL), bx - 130 + i * 29, by + random.uniform(-35, 35), 24, random.uniform(0, 360), random.uniform(0.9, 1.2), False, "Garden/Flowers")
for _ in range(7):   # wild flower patches on the lawn
    cx, cy = random.uniform(700, 3700), random.uniform(-1200, 1200)
    scatter(cx, cy, 70, 4, FL, 0.8, 1.2, avoid=near_path, folder="Garden/Flowers")
G = [GP + "TallGrass_A", GP + "TallGrass_B", GP + "TallGrass_Dry"]
scatter(3450, 1000, 420, 45, G, 0.9, 1.5, avoid=near_path, folder="Garden/TallGrass")   # overgrown back corner
scatter(2350, -450, 260, 22, G, 0.8, 1.3, avoid=near_path, folder="Garden/TallGrass")
scatter(1300, 600, 200, 14, G, 0.8, 1.2, avoid=near_path, folder="Garden/TallGrass")
for i in range(26):   # along the fences
    mesh(random.choice(G), random.uniform(500, 3900), 1440 + random.uniform(-30, 10), 0, random.uniform(0, 360), random.uniform(0.9, 1.4), False, "Garden/TallGrass")
for i in range(14):
    mesh(random.choice(G), 3930 + random.uniform(-20, 20), random.uniform(-1400, 1400), 0, random.uniform(0, 360), random.uniform(0.9, 1.4), False, "Garden/TallGrass")

# ---------------- rocks, logs, pots ----------------
for i in range(9):   # rock garden
    a = i * 2 * math.pi / 9
    mesh(SC + "Rock", 1450 + math.cos(a) * random.uniform(60, 160), 350 + math.sin(a) * random.uniform(60, 160), -5, random.uniform(0, 360), random.uniform(0.12, 0.3), True, "Garden/Rocks")
for i in range(6):
    mesh(SC + "Rock", random.uniform(900, 3700), random.uniform(-1300, 1300), -5, random.uniform(0, 360), random.uniform(0.08, 0.18), True, "Garden/Rocks")
mesh(GP + "LogHollow", 2900, 380, 0, 30, 1.0, True, "Garden/Logs", "HollowLog")
mesh(GP + "Log", 3700, -850, 0, -20, 1.0, True, "Garden/Logs")
mesh(GP + "Log", 1550, -1250, 0, 75, 1.1, True, "Garden/Logs")
mesh(GP + "Bench", 1750, -720, 0, 160, 1.0, True, "Garden/Furniture", "LawnBench")    # under the tree
scatter(1750, -720, 160, 8, G, 0.8, 1.2, folder="Garden/TallGrass")                  # bench half overgrown
# tipped-over flowerpots (opening faces sideways) - lying pot rests on its rim radius ~18 cm
mesh(GP + "Flowerpot", 1150, -600, 17, 40, 1.0, True, "Garden/Pots", "TippedPot", pitch=90)
mesh(GP + "Flowerpot", 3250, 1300, 17, 200, 1.0, True, "Garden/Pots", "TippedPot", pitch=90)
mesh(GP + "Flowerpot", 2050, 700, 0, 0, 1.2, True, "Garden/Pots")
mesh(GP + "PottedPlant", 2120, 740, 0, 0, 1.1, True, "Garden/Pots")

# ---------------- hiding spots ----------------
spot(1750, -720, label="Hide_UnderLawnBench", attract=1.5)
spot(2900, 380, 3, label="Hide_InHollowLog", attract=2.0)
spot(1160, -590, 3, label="Hide_InTippedPot", attract=1.5)
spot(3240, 1290, 3, label="Hide_InTippedPot", attract=1.5)
for (x, y, s) in TREES:
    spot(x + 60, y - 40, label="Hide_ByTreeRoots", attract=0.7)
spot(3450, 1000, label="Hide_TallGrassCorner", attract=1.5, rain=False)
spot(3600, 1250, label="Hide_TallGrassCorner", attract=1.2, rain=False)
spot(2350, -450, label="Hide_TallGrassPatch", attract=1.0, rain=False)
spot(2400, -1380, label="Hide_UnderHedge", attract=1.2)
spot(3000, -1390, label="Hide_UnderHedge", attract=1.2)
spot(1450, 350, label="Hide_RockGarden", attract=0.8, rain=False)
spot(3700, -800, label="Hide_BesideLog", attract=0.8, rain=False)
spot(230, -450, 3, label="Hide_UnderPatioTable", attract=0.6)
spot(60, -800, 3, label="Hide_UnderPatioBench", attract=0.8)
spot(2080, 720, label="Hide_BehindPots", attract=0.8, rain=False)
spot(1250, 1300, label="Hide_BushNearBed", attract=1.0)

# ---------------- gameplay actors ----------------
ps = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(250, -100, 100), R(yaw=0))
ps.set_actor_label("BY_PlayerStart"); ps.set_folder_path("Gameplay")
val = eas.spawn_actor_from_class(eal.load_asset("/Game/Characters/Valentino/BP_Valentino").generated_class(), unreal.Vector(2200, 200, 12), R(yaw=random.uniform(0, 360)))
val.set_actor_label("BY_Valentino"); val.set_folder_path("Gameplay")
nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(2000, 0, 150))
nav.set_actor_scale3d(unreal.Vector(4200 / 200, 3100 / 200, 400 / 200))
nav.set_actor_label("BY_NavBounds"); nav.set_folder_path("Gameplay")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", eal.load_asset("/Game/Core/BP_FV_GameMode").generated_class())
les.save_current_level()
print("BACKYARD BUILT: actors placed =", COUNT["n"])
