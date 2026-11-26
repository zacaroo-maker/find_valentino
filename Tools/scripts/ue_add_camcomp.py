import unreal
eal = unreal.EditorAssetLibrary
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary
bp = eal.load_asset("/Game/Characters/Dad/BP_Dad")
ia = eal.load_asset("/Game/Core/Input/IA_ToggleView")
cls = unreal.FVCameraModeComponent

handles = sds.k2_gather_subobject_data_for_blueprint(bp)
existing = None
for h in handles:
    obj = SDL.get_object(SDL.get_data(h))
    if obj and isinstance(obj, cls):
        existing = obj
if existing is None:
    params = unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=cls, blueprint_context=bp)
    new_handle, fail = sds.add_new_subobject(params)
    if not fail.is_empty():
        print("add failed:", fail)
    sds.rename_subobject(new_handle, unreal.Text("CameraMode"))
    existing = SDL.get_object(SDL.get_data(new_handle))
existing.set_editor_property("toggle_action", ia)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp)
print("component:", existing.get_name(), "| toggle action:", existing.get_editor_property("toggle_action").get_name())
