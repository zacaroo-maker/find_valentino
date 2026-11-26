import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
V = unreal.Vector
def R(yaw=0.0, pitch=0.0, roll=0.0): return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("VH_"):
        eas.destroy_actor(a)
def mi(name, color):
    path = f"/Game/Environment/DogPark/{name}"
    m = eal.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(name, "/Game/Environment/DogPark", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property("parent", unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(m, "Color", unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.update_material_instance(m); eal.save_loaded_asset(m)
    return m
wood = unreal.load_asset("/Game/Environment/DogPark/MI_Park_Wood")
roofm = mi("MI_Hutch_Roof", (0.45, 0.12, 0.08))
straw = mi("MI_Hutch_Straw", (0.85, 0.7, 0.3))
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube")
C = V(450, 1150, 0)   # on the lawn by the house, near the north fence
def box(label, rel, size, mat, rot=None, collide=True):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, C + rel, rot or R())
    a.set_actor_label("VH_" + label); a.set_folder_path("Gameplay/ValentinoHouse")
    c = a.static_mesh_component; c.set_static_mesh(CUBE); c.set_material(0, mat)
    a.set_actor_scale3d(V(size[0] / 100, size[1] / 100, size[2] / 100))
    if not collide: c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a
# hutch 100 (x) by 80 (y), open towards the garden (+X)
box("Floor", V(0, 0, 1.5), (100, 80, 3), wood)
box("Straw", V(0, 0, 3.5), (90, 70, 1.5), straw, collide=False)
box("Back", V(-48, 0, 25), (4, 80, 48), wood)
box("SideL", V(0, -38, 25), (100, 4, 48), wood)
box("SideR", V(0, 38, 25), (100, 4, 48), wood)
box("RoofL", V(0, -22, 58), (110, 50, 4), roofm, R(roll=-28.0), collide=False)
box("RoofR", V(0, 22, 58), (110, 50, 4), roofm, R(roll=28.0), collide=False)
t = eas.spawn_actor_from_class(unreal.TextRenderActor, C + V(52, 0, 44), R(0.0))
t.set_actor_label("VH_Sign"); t.set_folder_path("Gameplay/ValentinoHouse")
t.text_render.set_text("VALENTINO"); t.text_render.set_world_size(9.0)
t.text_render.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
t.text_render.set_text_render_color(unreal.Color(r=255, g=240, b=200, a=255))
s = eas.spawn_actor_from_class(unreal.FVSleepSpot, C + V(5, 0, 5), R())
s.set_actor_label("VH_SleepSpot"); s.set_folder_path("Gameplay/ValentinoHouse")
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("Valentino's house built at", C)
