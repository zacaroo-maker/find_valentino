# Diego setup: IA_Pet on F, wire into BP_Dad's carry component, name the dog Diego.
import unreal
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary
P = "/Game/Core/Input"
ia = eal.load_asset(P + "/IA_Pet") if eal.does_asset_exist(P + "/IA_Pet") else at.create_asset("IA_Pet", P, unreal.InputAction, unreal.InputAction_Factory())
ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN); eal.save_loaded_asset(ia)
imc = eal.load_asset("/Game/ThirdPerson/Input/IMC_Default")
maps = list(imc.get_editor_property("mappings"))
if not any(m.action == ia for m in maps):
    k = unreal.Key(); k.import_text("F")
    m = unreal.EnhancedActionKeyMapping(); m.set_editor_property("action", ia); m.set_editor_property("key", k)
    maps.append(m); imc.set_editor_property("mappings", maps)
imc.modify(); eal.save_asset("/Game/ThirdPerson/Input/IMC_Default", only_if_is_dirty=False)
print("mappings:", [(x.action.get_name(), x.key.export_text()) for x in imc.get_editor_property("mappings") if x.action and x.action.get_name() in ("IA_Pet", "IA_Use", "IA_Interact")])
dad = eal.load_asset("/Game/Characters/Dad/BP_Dad")
for h in sds.k2_gather_subobject_data_for_blueprint(dad):
    o = SDL.get_object(SDL.get_data(h))
    if isinstance(o, unreal.FVCarryComponent):
        o.set_editor_property("pet_action", ia)
        print("pet_action set; use_action:", o.get_editor_property("use_action"))
unreal.BlueprintEditorLibrary.compile_blueprint(dad); eal.save_loaded_asset(dad)
dog = eal.load_asset("/Game/Characters/Dog/BP_Dog")
cdo = unreal.get_default_object(dog.generated_class())
cdo.set_editor_property("dog_name", unreal.Text("Diego"))
unreal.BlueprintEditorLibrary.compile_blueprint(dog); eal.save_loaded_asset(dog)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "FT_Dog":
        a.set_actor_label("FT_Diego")
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("dog name:", cdo.get_editor_property("dog_name"))
