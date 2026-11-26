# Replace the brick-block house in L_Backyard with SM_FV_House_Home.
# Mesh pivot = front-centre, front faces +Y, depth 1000 extends to -Y. With yaw 90 the front faces -X (street)
# and the back wall (with back door) faces +X (the garden). Back wall must sit at X = -50 (old block face).
import unreal
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
removed = []
for a in eas.get_all_level_actors():
    lab = a.get_actor_label()
    if lab.startswith(("BY_House_", "BY_BackDoor", "BY_Window", "HM_")):
        removed.append(lab); eas.destroy_actor(a)
DEPTH = 1000.0
a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(-50.0 - DEPTH, -200.0, 0.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0))
a.static_mesh_component.set_static_mesh(eal.load_asset("/Game/Environment/Houses/SM_FV_House_Home"))
a.set_actor_label("HM_Home"); a.set_folder_path("Garden/Structure")
o, e = a.get_actor_bounds(False)
print("removed:", removed)
print(f"home bounds X {o.x - e.x:.0f}..{o.x + e.x:.0f}  Y {o.y - e.y:.0f}..{o.y + e.y:.0f}  Z top {o.z + e.z:.0f}")
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(w, "RebuildNavigation")
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
