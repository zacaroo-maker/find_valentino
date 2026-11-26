# Art pass fixes: dense hedge/bushes, dense tall-grass clumps, stone & wood materials, natural lawn tint.
import unreal, random, math
random.seed(7)
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
PH = "/Game/Environment/PolyHaven/"
def meshes(folder):
    return [p.split(".")[0] for p in eal.list_assets(PH + folder, recursive=False) if isinstance(eal.load_asset(p), unreal.StaticMesh)]
TUFTS = meshes("grass_medium_01")[:11]
SHRUBS = meshes("shrub_02")
BUSH = eal.load_asset("/Game/StarterContent/Props/SM_Bush")
STONE_MAT = eal.load_asset(PH + "rock_07/rock_07")
WOOD_MAT = eal.load_asset(PH + "planter_box_01/planter_box_01")
BENCH = eal.load_asset("/Game/Environment/GardenProps/SM_FV_Bench")

def R(yaw):
    return unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)
N = {"n": 0}
def spawn(path, x, y, z, scale, folder):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), R(random.uniform(0, 360)))
    c = a.static_mesh_component; c.set_static_mesh(eal.load_asset(path))
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    c.set_collision_profile_name("NoCollision"); c.set_editor_property("can_ever_affect_navigation", False)
    N["n"] += 1; a.set_actor_label(f"AP_{path.split('/')[-1]}_fill{N['n']}"); a.set_folder_path(folder)

for a in eas.get_all_level_actors():
    if "_fill" in a.get_actor_label() and a.get_actor_label().startswith("AP_"):
        eas.destroy_actor(a)

# natural lawn material (darker, less yellow) - new asset so existing references stay valid
LAWN = "/Game/Environment/Materials/M_FV_LawnNatural"
if eal.does_asset_exist(LAWN) and not eal.find_package_referencers_for_asset(LAWN, False):
    eal.delete_asset(LAWN)   # rebuild if nothing uses it yet
if not eal.does_asset_exist(LAWN):
    m = at.create_asset("M_FV_LawnNatural", "/Game/Environment/Materials", unreal.Material, unreal.MaterialFactoryNew())
    T = "/Game/StarterContent/Textures/"
    wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1200, 0)
    mask = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -1000, 0)
    mask.set_editor_property("r", True); mask.set_editor_property("g", True); mask.set_editor_property("b", False)
    mel.connect_material_expressions(wp, "", mask, "")
    def sample(tile, y, normal=False):
        dv = mel.create_material_expression(m, unreal.MaterialExpressionDivide, -800, y); dv.set_editor_property("const_b", tile)
        mel.connect_material_expressions(mask, "", dv, "A")
        t = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, -550, y)
        t.set_editor_property("texture", eal.load_asset(T + ("T_Ground_Grass_N" if normal else "T_Ground_Grass_D")))
        if normal:
            t.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        mel.connect_material_expressions(dv, "", t, "UVs")
        return t
    d1 = sample(180.0, -200); d2 = sample(1330.0, 100); n1 = sample(180.0, 400, True)
    desat = mel.create_material_expression(m, unreal.MaterialExpressionDesaturation, -300, -200)
    mel.connect_material_expressions(d1, "RGB", desat, "")
    frac = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -450, -60); frac.set_editor_property("r", 0.3)
    mel.connect_material_expressions(frac, "", desat, "Fraction")
    tint = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -300, -350)
    tint.set_editor_property("constant", unreal.LinearColor(0.52, 0.62, 0.40, 1))
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -100, -250)
    mel.connect_material_expressions(desat, "", mul, "A"); mel.connect_material_expressions(tint, "", mul, "B")
    lerp = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -300, 100)
    lerp.set_editor_property("const_a", 0.75); lerp.set_editor_property("const_b", 1.25)
    mel.connect_material_expressions(d2, "G", lerp, "Alpha")
    mul2 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, 100, -150)
    mel.connect_material_expressions(mul, "", mul2, "A"); mel.connect_material_expressions(lerp, "", mul2, "B")
    mel.connect_material_property(mul2, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(n1, "RGB", unreal.MaterialProperty.MP_NORMAL)
    r = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -100, 500); r.set_editor_property("r", 0.95)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m); eal.save_loaded_asset(m)

counts = {}
for a in eas.get_all_level_actors():
    lab = a.get_actor_label()
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    f = str(a.get_folder_path()); c = a.static_mesh_component
    sm = c.static_mesh.get_name() if c.static_mesh else ""
    l = a.get_actor_location()
    k = None
    if lab.startswith("BY_Lawn"):
        c.set_material(0, eal.load_asset(LAWN)); k = "lawn"
    elif f == "Garden/Hedge":
        c.set_static_mesh(BUSH); a.set_actor_scale3d(unreal.Vector(1.5, 1.3, 1.2)); k = "hedge"
    elif f == "Garden/Bushes":
        c.set_static_mesh(BUSH); s = random.uniform(0.9, 1.4); a.set_actor_scale3d(unreal.Vector(s, s, s)); k = "bushes"
    elif f == "Garden/TallGrass":
        for _ in range(6):   # dense clump around each tall-grass spot
            ang = random.uniform(0, 2 * math.pi); dist = random.uniform(0, 38)
            spawn(random.choice(TUFTS), l.x + math.cos(ang) * dist, l.y + math.sin(ang) * dist, 0, random.uniform(2.0, 3.0), "Garden/ArtPass/TallGrassFill")
        k = "grass clumps"
    elif f == "Garden/Path":
        c.set_material(0, STONE_MAT); k = "stones"
    elif f == "Garden/Fence":
        c.set_material(0, WOOD_MAT); k = "fence"
    elif f == "Garden/Furniture" and ("bench" in sm.lower() or "Bench" in lab):
        c.set_static_mesh(BENCH); a.set_actor_scale3d(unreal.Vector(1, 1, 1)); c.set_material(0, WOOD_MAT)
        c.set_collision_profile_name("BlockAll"); k = "benches"
    elif f == "Garden/Beds":
        c.set_material(0, WOOD_MAT); k = "beds"
    if k:
        counts[k] = counts.get(k, 0) + 1

for i in range(14):   # weedy shrubs along the side fence
    spawn(random.choice(SHRUBS), random.uniform(500, 3800), 1420 + random.uniform(-20, 10), 0, random.uniform(0.7, 1.0), "Garden/ArtPass/WildShrubs")

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("updated:", counts, "| filler actors:", N["n"])
