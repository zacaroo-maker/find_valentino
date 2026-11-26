# Spawn one or more meshes in a row at a preview spot, frame the camera, take a HighResShot.
# Reads mesh paths from <scratch>\_preview.txt (one per line); first line may be "name=<shot name>".
import unreal, os
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
listfile = r"C:\Users\calif\Documents\Finding Valentino\Downloads\_preview.txt"
lines = [l.strip() for l in open(listfile, encoding="utf-8-sig") if l.strip()]
name = "PREVIEW"
if lines[0].startswith("name="):
    name = lines.pop(0)[5:]
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("TEMP_Preview"):
        eas.destroy_actor(a)
x = 600.0; y0 = 900.0; ymax = 0; heights = []
spawned = []
for p in lines:
    m = eal.load_asset(p)
    b = m.get_bounds()
    w = max(b.box_extent.x, b.box_extent.y) * 2
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y0, 0))
    a.static_mesh_component.set_static_mesh(m)
    a.set_actor_label("TEMP_Preview_" + m.get_name())
    # put the mesh bottom on the ground
    o, e = a.get_actor_bounds(False)
    a.set_actor_location(unreal.Vector(x, y0 + w / 2, -(o.z - e.z)), False, False)
    spawned.append(a); heights.append(e.z * 2)
    y0 += w + 60
    ymax = y0
span = max(ymax - 900, 100); h = max(heights) if heights else 100
cy = 900 + span / 2
dist = max(span, h * 1.6) * 0.95 + 150
cam = unreal.Vector(x - dist, cy - dist * 0.25, h * 0.6 + dist * 0.25)
ues.set_level_viewport_camera_info(cam, unreal.MathLibrary.find_look_at_rotation(cam, unreal.Vector(x, cy, h * 0.4)))
unreal.SystemLibrary.execute_console_command(ues.get_editor_world(), f"HighResShot 1600x900 filename={name}")
print("preview of", len(spawned), "meshes; heights", [round(v) for v in heights])
