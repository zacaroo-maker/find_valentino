import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.RecastNavMesh) and a.get_editor_property("agent_radius") < 20:
        a.modify()
        ps = list(a.get_editor_property("nav_mesh_resolution_params"))
        for p in ps:
            p.set_editor_property("cell_size", 5.0); p.set_editor_property("cell_height", 5.0)
        a.set_editor_property("nav_mesh_resolution_params", ps)
# nudge the two spots that now sit under/inside solid objects onto open ground beside them
moves = {"BY_Hide_UnderHedge_344": unreal.Vector(2400, -1430, 2), "BY_Hide_BesideLog_347": unreal.Vector(3700, -750, 2)}
for a in eas.get_all_level_actors():
    if a.get_actor_label() in moves:
        a.set_actor_location(moves[a.get_actor_label()], False, False)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(w, "RebuildNavigation")
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("tortoise cells:", [[round(p.get_editor_property("cell_size"), 1) for p in a.get_editor_property("nav_mesh_resolution_params")] for a in eas.get_all_level_actors() if isinstance(a, unreal.RecastNavMesh)])
