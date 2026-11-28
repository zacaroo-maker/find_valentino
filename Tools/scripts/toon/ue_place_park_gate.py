import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    l = a.get_actor_label()
    if l == "DP_Gate":
        eas.destroy_actor(a)
    elif l in ("DP_GatePost_L", "DP_GatePost_R"):
        p = a.get_actor_location(); a.set_actor_location(unreal.Vector(p.x, p.y, 135.0), False, False)
g = eas.spawn_actor_from_class(unreal.FVParkGate, unreal.Vector(3125, 1498, 0), unreal.Rotator(0, 0, 0))
g.set_actor_label("DP_Gate"); g.set_folder_path("DogPark")
g.get_editor_property("sign").set_world_size(17.0)
o, e = g.get_actor_bounds(False)
print("gate bounds x", o.x-e.x, o.x+e.x, "y", o.y-e.y, o.y+e.y, "z", o.z-e.z, o.z+e.z)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
