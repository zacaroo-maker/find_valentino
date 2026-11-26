import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("MM_"):
        eas.destroy_actor(a)
# a calm view across the lawn toward the pond and trees, from the patio corner
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(350, 1250, 230), unreal.Rotator(roll=0.0, pitch=-10.0, yaw=-25.0))
cam.set_actor_label("MM_MenuCamera"); cam.set_folder_path("Gameplay"); cam.tags = [unreal.Name("MenuCamera")]
cam.camera_component.set_editor_property("field_of_view", 75.0)
cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("menu camera placed")
