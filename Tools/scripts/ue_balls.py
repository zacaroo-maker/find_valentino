# Finding Valentino - fetch: turn the dog park's tennis balls into throwable balls, and put Diego's ball in the garden.
# Re-runnable.
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eal = unreal.EditorAssetLibrary
SPH = eal.load_asset("/Engine/BasicShapes/Sphere")

def make_ball(label, loc, scale, mat, folder):
    b = eas.spawn_actor_from_class(unreal.FVPickupItem, loc, unreal.Rotator(0, 0, 0))
    b.set_actor_label(label); b.set_folder_path(folder)
    b.set_actor_scale3d(scale)
    m = b.get_editor_property("mesh"); m.set_static_mesh(SPH)
    if mat:
        m.set_material(0, mat)
    b.set_editor_property("kind", unreal.FVItemKind.BALL)
    b.set_editor_property("display_name", unreal.Text("ball"))
    return b

ball_mat = None
for a in list(eas.get_all_level_actors()):
    lab = a.get_actor_label()
    if lab.startswith("FT_Ball"):
        eas.destroy_actor(a)
    elif lab.startswith("DP_Ball_") and isinstance(a, unreal.StaticMeshActor):
        c = a.static_mesh_component
        ball_mat = c.get_material(0)
        make_ball(lab, a.get_actor_location(), a.get_actor_scale3d(), ball_mat, str(a.get_folder_path()))
        eas.destroy_actor(a)
        print("converted", lab)
if ball_mat is None:
    for a in eas.get_all_level_actors():
        if a.get_actor_label().startswith("DP_Ball_") and isinstance(a, unreal.FVPickupItem):
            ball_mat = a.get_editor_property("mesh").get_material(0); break

# Diego's ball: on the lawn near the patio
make_ball("FT_Ball_Diego", unreal.Vector(700, 220, 12), unreal.Vector(0.13, 0.13, 0.13), ball_mat, "Garden/Items")
print("garden ball placed, material:", ball_mat.get_name() if ball_mat else None)
les.save_current_level()
print("DONE")
