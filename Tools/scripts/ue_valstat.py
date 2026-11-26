import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
vs = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.FVValentino) if w else []
for v in vs:
    l = v.get_actor_location(); sp = v.get_velocity().length()
    print(f"VAL state={v.get_editor_property('state')} pos=({l.x:.0f},{l.y:.0f},{l.z:.0f}) speed={sp:.1f} retracted={v.is_retracted()}")
if not vs: print("VAL none / no game world")
