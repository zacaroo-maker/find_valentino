# Replace blockout/Poly Haven foliage in L_Backyard with Fab Megascans, using one instanced-mesh actor per plant type.
# Re-runnable: deletes previous "MS_" actors. Hides (doesn't delete) the replaced older actors so it's reversible.
import unreal, random, math
random.seed(2026)
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary
P = "/Game/Fab/Megascans/Plants/"

def meshes(folder):
    return sorted(p.split(".")[0] for p in eal.list_assets(P + folder, recursive=True) if "/StaticMeshes/" in p)
WILD = meshes("Wild_Grass_vlkhcbxia"); TALL = meshes("Tall_Grass_rbEkI"); CLUMP = meshes("Grass_Clumps_rbdoo")
SORREL = meshes("Violet_Wood_Sorrel_uchkajuia"); ELDER = meshes("Elderberry_wfzobb2ia")
SPINDLE = meshes("European_Spindle_wk1ncbxja"); RASP = meshes("Raspberry_wf0oefeja")
WILD_SMALL = [m for m in WILD if m[-1] in "BCEFGH"]

for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("MS_"):
        eas.destroy_actor(a)

ISM = {}   # mesh path -> component
def ism_for(mesh_path, folder, cast_shadow=True):
    if mesh_path in ISM:
        return ISM[mesh_path]
    actor = eas.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    actor.set_actor_label("MS_" + mesh_path.split("/")[-1]); actor.set_folder_path("Garden/Megascans/" + folder)
    root = sds.k2_gather_subobject_data_for_instance(actor)[0]
    h, fail = sds.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent))
    comp = SDL.get_object(SDL.get_data(h))
    comp.set_static_mesh(eal.load_asset(mesh_path))
    comp.set_collision_profile_name("NoCollision")
    comp.set_editor_property("can_ever_affect_navigation", False)
    comp.set_editor_property("cast_shadow", cast_shadow)
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    ISM[mesh_path] = comp
    return comp

COUNT = {"n": 0}
def plant(mesh_list, x, y, smin=0.9, smax=1.2, folder="Plants", z=0.0, shadow=True):
    m = random.choice(mesh_list); s = random.uniform(smin, smax)
    t = unreal.Transform(location=unreal.Vector(x, y, z), rotation=unreal.Rotator(roll=0.0, pitch=0.0, yaw=random.uniform(0, 360)), scale=unreal.Vector(s, s, s))
    ism_for(m, folder, shadow).add_instance(t, world_space=True)
    COUNT["n"] += 1

def disc(cx, cy, r):
    a = random.uniform(0, 2 * math.pi); d = r * math.sqrt(random.random())
    return cx + math.cos(a) * d, cy + math.sin(a) * d

# ---- hide the actors being replaced ----
hidden = {}
spots_tall, spots_bush, spots_hedge = [], [], []
for a in eas.get_all_level_actors():
    lab = a.get_actor_label(); f = str(a.get_folder_path())
    hide = False
    if f == "Garden/TallGrass":
        spots_tall.append(a.get_actor_location()); hide = True
    elif f in ("Garden/ArtPass/TallGrassFill", "Garden/ArtPass/WildShrubs"):
        hide = True
    elif f == "Garden/Bushes":
        spots_bush.append(a.get_actor_location()); hide = True
    elif f == "Garden/Hedge":
        spots_hedge.append(a.get_actor_location()); hide = True
    elif f == "Garden/ArtPass" and lab.startswith("AP_grass_medium_01"):
        hide = True
    if hide:
        a.set_actor_hidden_in_game(True); a.set_is_temporarily_hidden_in_editor(True)
        a.static_mesh_component.set_editor_property("visible", False)
        hidden[f] = hidden.get(f, 0) + 1

# ---- tall/wild grass where the tall-grass clumps were ----
for p in spots_tall:
    for _ in range(2):
        x, y = disc(p.x, p.y, 35); plant(TALL, x, y, 1.0, 1.4, "TallGrass")
    for _ in range(6):
        x, y = disc(p.x, p.y, 70); plant(WILD, x, y, 1.2, 1.9, "TallGrass")
    x, y = disc(p.x, p.y, 60); plant(CLUMP, x, y, 1.0, 1.4, "TallGrass")

# ---- bushes: mix of spindle / elderberry / raspberry ----
for p in spots_bush:
    plant(random.choice((SPINDLE, ELDER, ELDER)), p.x, p.y, 1.2, 1.6, "Bushes")
    x, y = disc(p.x, p.y, 70); plant(RASP, x, y, 1.0, 1.4, "Bushes")
for p in spots_hedge:   # hedge line: elderberry + spindle, with raspberry filling the gaps (Valentino can still crawl under)
    plant(random.choice((ELDER, SPINDLE)), p.x, p.y, 1.3, 1.7, "Hedge")
    plant(random.choice((ELDER, SPINDLE)), p.x + 70, p.y + random.uniform(-30, 30), 1.2, 1.6, "Hedge")
    for _ in range(2):
        x, y = disc(p.x + 35, p.y, 60); plant(RASP, x, y, 1.0, 1.4, "Hedge")
for _ in range(16):     # wild shrubs along the side fence
    plant(random.choice((RASP, ELDER)), random.uniform(500, 3800), 1420 + random.uniform(-20, 5), 0.7, 1.0, "Bushes")

# ---- lawn: scattered wild grass, sorrel patches, clumps ----
def on_lawn():
    while True:
        x, y = random.uniform(450, 3950), random.uniform(-1450, 1450)
        if not (x < 650 and -900 < y < 400):   # keep the patio edge clear
            return x, y
for _ in range(900):
    x, y = on_lawn(); plant(WILD_SMALL, x, y, 0.8, 1.3, "Lawn", shadow=False)
for _ in range(14):
    cx, cy = on_lawn()
    for _ in range(random.randint(5, 10)):
        x, y = disc(cx, cy, 90); plant(SORREL, x, y, 0.9, 1.3, "Sorrel", shadow=False)
for _ in range(60):
    x, y = on_lawn(); plant(CLUMP, x, y, 0.7, 1.1, "Lawn")
# fence bases
for _ in range(120):
    side = random.choice(("L", "R", "B"))
    if side == "B":
        x, y = 3960 + random.uniform(-25, 15), random.uniform(-1450, 1450)
    else:
        x, y = random.uniform(300, 3950), (1460 if side == "L" else -1460) + random.uniform(-20, 20)
    plant(random.choice((WILD, TALL, CLUMP)), x, y, 0.8, 1.2, "FenceBase")

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("hidden:", hidden)
print("instances:", COUNT["n"], "in", len(ISM), "instanced actors")

