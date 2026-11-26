import unreal
eal = unreal.EditorAssetLibrary
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem); SDL = unreal.SubobjectDataBlueprintFunctionLibrary
bp = eal.load_asset("/Game/Characters/Dad/BP_Dad")
for h in sds.k2_gather_subobject_data_for_blueprint(bp):
    o = SDL.get_object(SDL.get_data(h))
    if isinstance(o, unreal.FVCarryComponent):
        o.set_editor_property("carry_offset", unreal.Vector(34, 24, 22))
        o.set_editor_property("reach_distance", 190.0)
unreal.BlueprintEditorLibrary.compile_blueprint(bp); eal.save_loaded_asset(bp)
print("carry tuned")
