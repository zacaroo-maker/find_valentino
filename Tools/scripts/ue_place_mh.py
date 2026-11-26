import unreal
eal = unreal.EditorAssetLibrary
mom = unreal.load_asset("/Game/Characters/Family/Data/DA_Mom")
mom.set_editor_property("meta_human_class", unreal.load_asset("/Game/MetaHumans/Mom/BP_Mom").generated_class())
mom.set_editor_property("female_frame", True)
eal.save_loaded_asset(mom)
print("DA_Mom ->", mom.get_editor_property("meta_human_class"))
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
bed = None
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("FT_Jorge"):
        eas.destroy_actor(a)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "FT_PlantBed_vegetable_bed":
        bed = a
loc = bed.get_actor_location() + unreal.Vector(0, 260, 100) if bed else unreal.Vector(800, 600, 100)
j = eas.spawn_actor_from_class(unreal.FVGardener, loc, unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
j.set_actor_label("FT_Jorge"); j.set_folder_path("Gameplay/Features")
j.set_editor_property("meta_human_class", unreal.load_asset("/Game/MetaHumans/Jorge/BP_Jorge").generated_class())
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("Jorge placed at", loc)
