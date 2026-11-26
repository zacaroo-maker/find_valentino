import unreal
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
D = "/Game/Characters/Valentino"
A = D + "/SK_Valentino_Anim_Armature_"

# 1) BP_Valentino (child of the C++ class) with mesh + animations
bp_path = D + "/BP_Valentino"
if eal.does_asset_exist(bp_path):
    bp = eal.load_asset(bp_path)
else:
    f = unreal.BlueprintFactory(); f.set_editor_property("parent_class", unreal.FVValentino)
    bp = at.create_asset("BP_Valentino", D, unreal.Blueprint, f)
cdo = unreal.get_default_object(bp.generated_class())
cdo.get_editor_property("mesh").set_editor_property("skeletal_mesh_asset", eal.load_asset(D + "/SK_Valentino"))
for prop, name in (("idle_anim", "Idle"), ("walk_anim", "Walk"), ("hide_anim", "Hide"), ("emerge_anim", "Emerge")):
    cdo.set_editor_property(prop, eal.load_asset(A + name))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp)
print("BP_Valentino ready:", cdo.get_editor_property("walk_anim").get_name())

# 2) Level: navmesh, Valentino, hiding spots
actors = eas.get_all_level_actors()
start = [a for a in actors if isinstance(a, unreal.PlayerStart)][0].get_actor_location()
# floor bounds = union of big static meshes near ground
mn, mx = unreal.Vector(1e9, 1e9, 1e9), unreal.Vector(-1e9, -1e9, -1e9)
for a in actors:
    if isinstance(a, unreal.StaticMeshActor):
        o, e = a.get_actor_bounds(False)
        mn = unreal.Vector(min(mn.x, o.x - e.x), min(mn.y, o.y - e.y), min(mn.z, o.z - e.z))
        mx = unreal.Vector(max(mx.x, o.x + e.x), max(mx.y, o.y + e.y), max(mx.z, o.z + e.z))
print("level bounds", mn, mx, "player start", start)

for a in actors:
    if a.get_actor_label().startswith(("FV_", "TEMP_")):
        eas.destroy_actor(a)

if not any(isinstance(a, unreal.NavMeshBoundsVolume) for a in actors):
    c = (mn + mx) * 0.5
    nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, c)
    nav.set_actor_label("FV_NavBounds")
    size = mx - mn
    nav.set_actor_scale3d(unreal.Vector(size.x / 200.0 + 1, size.y / 200.0 + 1, max(size.z, 400) / 200.0 + 1))
    print("added NavMeshBoundsVolume")

floor_z = start.z - 92.0
val = eas.spawn_actor_from_class(bp.generated_class(), unreal.Vector(start.x + 350, start.y, floor_z + 12))
val.set_actor_label("FV_Valentino")
for i, (dx, dy) in enumerate(((900, 400), (-300, 900), (600, -700))):
    s = eas.spawn_actor_from_class(unreal.FVHidingSpot, unreal.Vector(start.x + dx, start.y + dy, floor_z + 5))
    s.set_actor_label(f"FV_HidingSpot_{i+1}")

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("placed Valentino at", val.get_actor_location())
