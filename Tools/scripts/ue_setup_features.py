# Finding Valentino - import dog + items and set up: dog, pond, food spawner, plant beds, watering can,
# movable garden props, LMB "use" input, tree shade tags, dynamic navmesh. Re-runnable where practical.
import unreal, os, math, random
random.seed(77)
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary
DOC = r"C:\Users\calif\Documents\Finding Valentino"
LOG = []
def log(*a):
    LOG.append(" ".join(str(x) for x in a)); print(*a)

def R(yaw=0.0, pitch=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)

# ------------------------------------------------------------------ imports
def fbx_task(path, dest, name, skeletal, anims=False):
    ui = unreal.FbxImportUI(); ui.import_mesh = True; ui.import_as_skeletal = skeletal; ui.import_animations = anims
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else unreal.FBXImportType.FBXIT_STATIC_MESH
    ui.import_materials = True; ui.import_textures = False; ui.create_physics_asset = skeletal
    for d in (ui.skeletal_mesh_import_data, ui.static_mesh_import_data, ui.anim_sequence_import_data):
        d.set_editor_property("convert_scene", True); d.set_editor_property("convert_scene_unit", True)
    if not skeletal:
        ui.static_mesh_import_data.set_editor_property("combine_meshes", True)
        ui.static_mesh_import_data.set_editor_property("auto_generate_collision", False)
    t = unreal.AssetImportTask(); t.filename = path; t.destination_path = dest; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
    return t

DOG = "/Game/Characters/Dog"; ITEMS = "/Game/Environment/Items"
tasks = [fbx_task(os.path.join(DOC, "Dog", "Dog.fbx"), DOG, "SK_Dog", True, True)]
for f in sorted(os.listdir(os.path.join(DOC, "Items"))):
    if f.endswith(".fbx"):
        tasks.append(fbx_task(os.path.join(DOC, "Items", f), ITEMS, f[:-4], False))
tex = unreal.AssetImportTask(); tex.filename = os.path.join(DOC, "Dog", "T_DogVest_Plaid.png"); tex.destination_path = DOG
tex.automated = True; tex.replace_existing = True; tex.save = False
tasks.append(tex)
at.import_asset_tasks(tasks)
log("imported:", [a.split(".")[-1] for a in eal.list_assets(DOG) + eal.list_assets(ITEMS)])

def load(p):
    a = eal.load_asset(p)
    if a is None:
        raise RuntimeError("missing asset " + p)
    return a

# dog vest material: plaid texture on the facade master
FACADE = load("/Game/Environment/Houses/Materials/M_FV_Facade")
def facade_mi(name, folder, color_tex, tint=(1, 1, 1), uv=1.0):
    p = f"{folder}/{name}"
    mi = eal.load_asset(p) if eal.does_asset_exist(p) else at.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, FACADE)
    if color_tex:
        mel.set_material_instance_texture_parameter_value(mi, "BaseColorTex", color_tex)
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "UVScale", uv)
    mel.update_material_instance(mi); eal.save_loaded_asset(mi)
    return mi
vest = facade_mi("MI_DogVest", DOG, load(DOG + "/T_DogVest_Plaid"))
sk_dog = load(DOG + "/SK_Dog")
mats = list(sk_dog.get_editor_property("materials"))
for i, m in enumerate(mats):
    if str(m.material_slot_name).startswith("Vest"):
        m.set_editor_property("material_interface", vest); mats[i] = m
sk_dog.set_editor_property("materials", mats)

# pond water material (translucent, non-Nanite plane so translucency renders)
WATER_MAT = ITEMS + "/M_FV_PondWater"
if not eal.does_asset_exist(WATER_MAT):
    wm = at.create_asset("M_FV_PondWater", ITEMS, unreal.Material, unreal.MaterialFactoryNew())
    wm.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    wm.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
    c = mel.create_material_expression(wm, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(0.03, 0.09, 0.1, 1))
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    op = mel.create_material_expression(wm, unreal.MaterialExpressionConstant, -400, 200); op.set_editor_property("r", 0.72)
    mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    ro = mel.create_material_expression(wm, unreal.MaterialExpressionConstant, -400, 300); ro.set_editor_property("r", 0.04)
    mel.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
    sp = mel.create_material_expression(wm, unreal.MaterialExpressionConstant, -400, 380); sp.set_editor_property("r", 1.0)
    mel.connect_material_property(sp, "", unreal.MaterialProperty.MP_SPECULAR)
    wn = eal.load_asset("/Game/StarterContent/Textures/T_Water_N")
    if wn:
        tc = mel.create_material_expression(wm, unreal.MaterialExpressionTextureCoordinate, -900, 500); tc.set_editor_property("u_tiling", 2.0); tc.set_editor_property("v_tiling", 2.0)
        pan = mel.create_material_expression(wm, unreal.MaterialExpressionPanner, -700, 500)
        pan.set_editor_property("speed_x", 0.02); pan.set_editor_property("speed_y", 0.015)
        mel.connect_material_expressions(tc, "", pan, "Coordinate")
        n = mel.create_material_expression(wm, unreal.MaterialExpressionTextureSample, -450, 500)
        n.set_editor_property("texture", wn); n.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        mel.connect_material_expressions(pan, "", n, "UVs")
        mel.connect_material_property(n, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.recompile_material(wm); eal.save_loaded_asset(wm)
water_mat = load(WATER_MAT)
rock_mat = load("/Game/Environment/PolyHaven/rock_07/rock_07")
gravel_mat = load("/Game/Environment/Materials/M_FV_Gravel")

def set_slots(mesh, mapping):
    for i, sm in enumerate(mesh.static_materials):
        n = str(sm.material_slot_name)
        for k, v in mapping.items():
            if n.startswith(k):
                mesh.set_material(i, v)
def collision(mesh, complex_as_simple):
    bs = mesh.get_editor_property("body_setup")
    if bs:
        bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE if complex_as_simple else unreal.CollisionTraceFlag.CTF_USE_DEFAULT)

pond = load(ITEMS + "/SM_FV_Pond"); set_slots(pond, {"Stone": rock_mat, "Pebbles": gravel_mat}); collision(pond, True)
pw = load(ITEMS + "/SM_FV_PondWater"); set_slots(pw, {"Water": water_mat})
stream = load(ITEMS + "/SM_FV_WaterStream"); set_slots(stream, {"WaterStream": water_mat})
can = load(ITEMS + "/SM_FV_WateringCan"); collision(can, True)
for p in eal.list_assets(ITEMS):
    a = eal.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        ns = a.get_editor_property("nanite_settings")
        if ns.enabled:
            ns.enabled = False; a.set_editor_property("nanite_settings", ns)
eal.save_directory(ITEMS, only_if_is_dirty=False, recursive=True)
eal.save_directory(DOG, only_if_is_dirty=False, recursive=True)

# BP_Dog
bp_path = DOG + "/BP_Dog"
if eal.does_asset_exist(bp_path):
    bp = load(bp_path)
else:
    f = unreal.BlueprintFactory(); f.set_editor_property("parent_class", unreal.FVDog)
    bp = at.create_asset("BP_Dog", DOG, unreal.Blueprint, f)
cdo = unreal.get_default_object(bp.generated_class())
cdo.get_editor_property("mesh").set_editor_property("skeletal_mesh_asset", sk_dog)
for prop, nm in (("idle_anim", "Idle"), ("walk_anim", "Walk"), ("sniff_anim", "Sniff"), ("lie_down_anim", "LieDown"),
                 ("lying_anim", "Lying"), ("get_up_anim", "GetUp")):
    cdo.set_editor_property(prop, load(f"{DOG}/SK_DogArmature_{nm}"))
unreal.BlueprintEditorLibrary.compile_blueprint(bp); eal.save_loaded_asset(bp)
log("BP_Dog ok")

# input: IA_Use on left mouse, wire into BP_Dad's carry component
P = "/Game/Core/Input"
ia_use = eal.load_asset(P + "/IA_Use") if eal.does_asset_exist(P + "/IA_Use") else at.create_asset("IA_Use", P, unreal.InputAction, unreal.InputAction_Factory())
ia_use.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN); eal.save_loaded_asset(ia_use)
imc = load("/Game/ThirdPerson/Input/IMC_Default")
maps = list(imc.get_editor_property("mappings"))
if not any(m.action == ia_use for m in maps):
    k = unreal.Key(); k.import_text("LeftMouseButton")
    m = unreal.EnhancedActionKeyMapping(); m.set_editor_property("action", ia_use); m.set_editor_property("key", k)
    maps.append(m); imc.set_editor_property("mappings", maps)
imc.modify(); eal.save_asset("/Game/ThirdPerson/Input/IMC_Default", only_if_is_dirty=False)
dad = load("/Game/Characters/Dad/BP_Dad")
for h in sds.k2_gather_subobject_data_for_blueprint(dad):
    o = SDL.get_object(SDL.get_data(h))
    if isinstance(o, unreal.FVCarryComponent):
        o.set_editor_property("use_action", ia_use)
unreal.BlueprintEditorLibrary.compile_blueprint(dad); eal.save_loaded_asset(dad)
log("IA_Use -> LMB, BP_Dad carry use_action set")

# ------------------------------------------------------------------ level
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("FT_"):
        eas.destroy_actor(a)

def spawn(cls, loc, rot=None, label="", folder="Gameplay/Features"):
    a = eas.spawn_actor_from_class(cls, loc, rot or R())
    a.set_actor_label("FT_" + label); a.set_folder_path(folder)
    return a

# pond at (2700,-700): clear plants around it
PC = unreal.Vector(2700, -700, 0); CLEAR = 300.0
removed = 0
for a in eas.get_all_level_actors():
    lab = a.get_actor_label()
    if lab.startswith("MS_"):
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            idx = [i for i in range(c.get_instance_count())
                   if (c.get_instance_transform(i, True).translation - PC).length() < CLEAR]
            for i in sorted(idx, reverse=True):
                c.remove_instance(i); removed += 1
    elif lab.startswith(("AP_", "BY_SM_FV_TallGrass", "BY_SM_FV_Flowers", "BY_SM_Rock")) and isinstance(a, unreal.StaticMeshActor):
        l = a.get_actor_location()
        if unreal.Vector(l.x - PC.x, l.y - PC.y, 0).length() < CLEAR:
            eas.destroy_actor(a); removed += 1
log("cleared around pond:", removed)
pa = spawn(unreal.StaticMeshActor, PC, label="Pond", folder="Garden/Pond"); pa.static_mesh_component.set_static_mesh(pond)
wa = spawn(unreal.StaticMeshActor, PC, label="PondWater", folder="Garden/Pond"); wa.static_mesh_component.set_static_mesh(pw)
wa.static_mesh_component.set_collision_profile_name("NoCollision"); wa.static_mesh_component.set_editor_property("can_ever_affect_navigation", False)
pm = spawn(unreal.FVPond, PC, label="PondLogic", folder="Garden/Pond")
pm.get_editor_property("drink_point").set_editor_property("relative_location", unreal.Vector(0, 25, 5))

# food spawner
fs = spawn(unreal.FVFoodSpawner, unreal.Vector(400, 0, 50), label="FoodSpawner")
fs.set_editor_property("lettuce_mesh", load(ITEMS + "/SM_FV_Food_Lettuce"))
fs.set_editor_property("dandelion_mesh", load(ITEMS + "/SM_FV_Food_Dandelion"))
fs.set_editor_property("strawberry_mesh", load(ITEMS + "/SM_FV_Food_Strawberry"))
fs.set_editor_property("worm_mesh", load(ITEMS + "/SM_FV_Food_Worm"))

# plant beds on the two raised beds + a wildflower patch on the lawn
for (x, y, nm, r) in ((1000, 1350, "flower bed", 170), (2350, -1330, "vegetable bed", 170), (1500, 650, "wildflower patch", 140)):
    b = spawn(unreal.FVPlantBed, unreal.Vector(x, y, 30), label="PlantBed_" + nm.replace(" ", "_"))
    b.set_editor_property("radius", float(r)); b.set_editor_property("display_name", unreal.Text(nm))
    if nm == "vegetable bed":
        b.set_editor_property("sprout_types", [unreal.FVFoodType.LETTUCE, unreal.FVFoodType.STRAWBERRY])
    if nm == "wildflower patch":
        b.set_editor_property("sprout_types", [unreal.FVFoodType.DANDELION, unreal.FVFoodType.WORM])

# watering can on the patio
wc = spawn(unreal.FVPickupItem, unreal.Vector(380, 120, 2), R(yaw=200), label="WateringCan", folder="Garden/Items")
wc.set_editor_property("kind", unreal.FVItemKind.WATERING_CAN)
wc.set_editor_property("display_name", unreal.Text("watering can"))
wc.get_editor_property("mesh").set_static_mesh(can)
wc.get_editor_property("water_stream").set_static_mesh(stream)

# dog: starts on the lawn
dg = spawn(load(bp_path).generated_class(), unreal.Vector(1800, 300, 30), R(yaw=40), label="Dog")

# ------------------------------------------------------------------ movable garden props
NAMES = {"PottedPlant": "potted plant", "potted_plant": "potted plant", "Flowerpot": "flowerpot", "planter_pot": "flowerpot",
         "Bench": "bench", "painted_wooden_bench": "bench", "Chair": "chair", "planter_box": "planter box", "LeafPile": "leaf pile",
         "TableRound": "table"}
def friendly(mesh_name):
    for k, v in NAMES.items():
        if k in mesh_name:
            return v
    return None
converted = 0
spots = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.FVHidingSpot)]
for a in list(eas.get_all_level_actors()):
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    lab = a.get_actor_label(); f = str(a.get_folder_path())
    if not (f in ("Garden/Pots", "Garden/Furniture", "Garden/Ground") or lab.startswith("AP_planter_box")):
        continue
    if lab.startswith("BY_PatioTable"):
        continue   # the table stays put
    c = a.static_mesh_component; sm = c.static_mesh
    if not sm or not c.is_visible():
        continue
    name = friendly(sm.get_name())
    if not name:
        continue
    t = a.get_actor_transform()
    it = eas.spawn_actor_from_class(unreal.FVPickupItem, t.translation, t.rotation.rotator())
    it.set_actor_scale3d(t.scale3d)
    m = it.get_editor_property("mesh"); m.set_static_mesh(sm)
    for i in range(c.get_num_materials()):
        if c.get_material(i):
            m.set_material(i, c.get_material(i))
    it.set_editor_property("kind", unreal.FVItemKind.PROP)
    it.set_editor_property("display_name", unreal.Text(name))
    it.set_actor_label("FT_Item_" + name.replace(" ", "_") + "_" + str(converted)); it.set_folder_path("Garden/Items")
    # hiding spots in/under this prop travel with it
    o, e = a.get_actor_bounds(False)
    for s in spots:
        l = s.get_actor_location()
        if abs(l.x - o.x) <= e.x + 10 and abs(l.y - o.y) <= e.y + 10:
            s.attach_to_actor(it, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    eas.destroy_actor(a); converted += 1
log("movable props:", converted)

# trees cast shade for the dog
shade = 0
for a in eas.get_all_level_actors():
    if str(a.get_folder_path()) in ("Garden/Trees", "Neighbours/Trees"):
        tags = list(a.tags)
        if "FV_Shade" not in [str(t) for t in tags]:
            tags.append("FV_Shade"); a.tags = tags
        shade += 1
log("shade trees tagged:", shade)

# navmesh: rebuild at runtime when props move
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.RecastNavMesh):
        a.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(w, "RebuildNavigation")
les.save_current_level()
log("DONE")

