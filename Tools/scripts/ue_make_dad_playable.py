import unreal
eal = unreal.EditorAssetLibrary
BPL = unreal.BlueprintEditorLibrary

def dup(src, dst):
    if eal.does_asset_exist(dst):
        return eal.load_asset(dst)
    return eal.duplicate_asset(src, dst)

# 1) Character blueprint for Dad (copy of the template character)
bp = dup("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter", "/Game/Characters/Dad/BP_Dad")
cdo = unreal.get_default_object(bp.generated_class())
mesh = cdo.get_editor_property("mesh")
cap = cdo.get_editor_property("capsule_component")
mesh.set_editor_property("skeletal_mesh_asset", eal.load_asset("/Game/Characters/Dad/SK_Dad"))
mesh.set_editor_property("anim_class", None)          # Manny/Quinn anims don't fit Dad's skeleton yet
cap.set_editor_property("capsule_half_height", 93.0)   # Dad is 185 cm
mesh.set_editor_property("relative_location", unreal.Vector(0, 0, -93.0))
mesh.set_editor_property("relative_rotation", unreal.Rotator(roll=0.0, pitch=0.0, yaw=270.0))  # Dad faces +Y; character forward is +X
BPL.compile_blueprint(bp)
eal.save_loaded_asset(bp)

# 2) Game mode that spawns Dad
gm = dup("/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode", "/Game/Core/BP_FV_GameMode")
gcdo = unreal.get_default_object(gm.generated_class())
gcdo.set_editor_property("default_pawn_class", bp.generated_class())
BPL.compile_blueprint(gm)
eal.save_loaded_asset(gm)

# 3) Use it in the current map
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", gm.generated_class())
les.save_current_level()

# verify by reading back
c2 = unreal.get_default_object(eal.load_asset("/Game/Characters/Dad/BP_Dad").generated_class())
m2 = c2.get_editor_property("mesh")
print("BP_Dad mesh:", m2.get_editor_property("skeletal_mesh_asset").get_name(),
      "| anim:", m2.get_editor_property("anim_class"),
      "| capsule:", c2.get_editor_property("capsule_component").get_editor_property("capsule_half_height"),
      "| yaw:", m2.get_editor_property("relative_rotation").yaw)
print("GameMode pawn:", unreal.get_default_object(gm.generated_class()).get_editor_property("default_pawn_class").get_name())
print("Map override:", world.get_world_settings().get_editor_property("default_game_mode").get_name())
