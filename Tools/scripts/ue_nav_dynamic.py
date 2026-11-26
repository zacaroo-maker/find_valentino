import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.RecastNavMesh):
        a.modify()
        a.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(w, "RebuildNavigation")
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print([(a.get_actor_label(), a.get_editor_property("runtime_generation")) for a in eas.get_all_level_actors() if isinstance(a, unreal.RecastNavMesh)])
