# Finding Valentino - foraging: import the forage meshes (make_forage.py), give the food spawner every food's mesh,
# update the plant beds to Valentino's real diet, and plant forage plants around the garden. Re-runnable.
import unreal, os, math, random
random.seed(5)
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
SRC = r"C:\Users\calif\Documents\Finding Valentino\Items\Forage"
DST = "/Game/Environment/Items/Forage"
ITEMS = "/Game/Environment/Items"
F = unreal.FVFoodType

# ---- import
tasks = []
for f in sorted(os.listdir(SRC)):
    if not f.endswith(".fbx"):
        continue
    ui = unreal.FbxImportUI(); ui.import_mesh = True; ui.import_as_skeletal = False; ui.import_animations = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    ui.import_materials = True; ui.import_textures = False
    d = ui.static_mesh_import_data
    d.set_editor_property("convert_scene", True); d.set_editor_property("convert_scene_unit", True)
    d.set_editor_property("combine_meshes", True); d.set_editor_property("auto_generate_collision", False)
    t = unreal.AssetImportTask(); t.filename = os.path.join(SRC, f); t.destination_path = DST; t.destination_name = f[:-4]
    t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
    tasks.append(t)
at.import_asset_tasks(tasks)
for p in eal.list_assets(DST):
    a = eal.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        ns = a.get_editor_property("nanite_settings")
        if ns.enabled:
            ns.enabled = False; a.set_editor_property("nanite_settings", ns)
eal.save_directory(DST, only_if_is_dirty=False, recursive=True)
print("imported", len(tasks))

def load(p):
    a = eal.load_asset(p)
    if a is None:
        raise RuntimeError("missing " + p)
    return a

# ---- food spawner: every food's carried mesh
food_mesh = {
    F.ROMAINE: load(DST + "/SM_FV_Food_Romaine"),
    F.DANDELION: load(ITEMS + "/SM_FV_Food_Dandelion"),
    F.STRAWBERRY: load(ITEMS + "/SM_FV_Food_Strawberry"),
    F.TOMATO: load(DST + "/SM_FV_Food_Tomato"),
    F.CHICORY: load(DST + "/SM_FV_Food_Chicory"),
    F.HIBISCUS: load(DST + "/SM_FV_Food_Hibiscus"),
}
actors = eas.get_all_level_actors()
for a in actors:
    if isinstance(a, unreal.FVFoodSpawner):
        a.set_editor_property("food_meshes", food_mesh)
        a.set_editor_property("start_count", 3)
        print("spawner meshes set")
    if isinstance(a, unreal.FVPlantBed):
        nm = str(a.get_editor_property("display_name"))
        types = {"vegetable bed": [F.ROMAINE, F.TOMATO, F.STRAWBERRY],
                 "wildflower patch": [F.DANDELION, F.CHICORY],
                 "flower bed": [F.HIBISCUS, F.DANDELION, F.STRAWBERRY]}.get(nm)
        if types:
            a.set_editor_property("sprout_types", types)
            print("bed", nm, "->", [str(t) for t in types])

# ---- forage plants
for a in actors:
    if a.get_actor_label().startswith("FP_"):
        eas.destroy_actor(a)

NAME = {F.ROMAINE: "Romaine", F.TOMATO: "Tomato", F.DANDELION: "Dandelion", F.STRAWBERRY: "Strawberry",
        F.CHICORY: "Chicory", F.HIBISCUS: "Hibiscus"}
REGROW = {F.ROMAINE: 150.0, F.TOMATO: 180.0, F.DANDELION: 90.0, F.STRAWBERRY: 160.0, F.CHICORY: 110.0, F.HIBISCUS: 130.0}
PLANTS = [
    # vegetable bed (raised, 300 x 120 at (2350,-1330))
    (F.ROMAINE, 2240, -1360), (F.ROMAINE, 2310, -1300), (F.TOMATO, 2420, -1345), (F.TOMATO, 2470, -1300),
    (F.STRAWBERRY, 2370, -1365),
    # flower bed (raised, at (1000,1350)) and hibiscus bushes beside it
    (F.STRAWBERRY, 930, 1360), (F.STRAWBERRY, 1060, 1330), (F.HIBISCUS, 760, 1480), (F.HIBISCUS, 1180, 1500),
    # wildflower patch on the lawn (1500,650)
    (F.DANDELION, 1450, 600), (F.DANDELION, 1560, 700), (F.DANDELION, 1410, 720), (F.CHICORY, 1620, 560), (F.CHICORY, 1360, 560),
    # a few wild ones around the lawn
    (F.DANDELION, 2000, -200), (F.DANDELION, 850, 300), (F.CHICORY, 2950, -1150), (F.DANDELION, 3050, -400),
]
count = {}
for (ft, x, y) in PLANTS:
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 400), unreal.Vector(x, y, -300),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    z = 0.0; on = "nothing"
    if hit:
        t = hit.to_tuple()
        z = t[4].z
        on = t[9].get_actor_label() if t[9] else "?"
    n = count.get(ft, 0) + 1; count[ft] = n
    p = eas.spawn_actor_from_class(unreal.FVForagePlant, unreal.Vector(x, y, z), unreal.Rotator(0, 0, random.uniform(0, 360)))
    p.set_actor_label(f"FP_{NAME[ft]}_{n}"); p.set_folder_path("Garden/Forage")
    p.set_editor_property("food_type", ft)
    p.set_editor_property("regrow_seconds", REGROW[ft])
    p.get_editor_property("plant_mesh").set_static_mesh(load(f"{DST}/SM_FV_Plant_{NAME[ft]}"))
    p.get_editor_property("produce_mesh").set_static_mesh(load(f"{DST}/SM_FV_Plant_{NAME[ft]}_Produce"))
    s = random.uniform(0.9, 1.15)
    p.set_actor_scale3d(unreal.Vector(s, s, s))
    print(f"FP_{NAME[ft]}_{n} at ({x},{y},{z:.0f}) on {on}")

les.save_current_level()
print("DONE")
