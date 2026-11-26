import unreal
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary

# 1) IA_Interact mapped to E
P = "/Game/Core/Input"
ia_path = P + "/IA_Interact"
ia = eal.load_asset(ia_path) if eal.does_asset_exist(ia_path) else at.create_asset("IA_Interact", P, unreal.InputAction, unreal.InputAction_Factory())
ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
eal.save_loaded_asset(ia)
imc_path = "/Game/ThirdPerson/Input/IMC_Default"
imc = eal.load_asset(imc_path)
maps = list(imc.get_editor_property("mappings"))
if not any(m.action == ia for m in maps):
    k = unreal.Key(); k.import_text("E")
    m = unreal.EnhancedActionKeyMapping(); m.set_editor_property("action", ia); m.set_editor_property("key", k)
    maps.append(m); imc.set_editor_property("mappings", maps)
imc.modify(); eal.save_asset(imc_path, only_if_is_dirty=False)
print("E mapped:", [x.key.export_text() for x in imc.get_editor_property("mappings") if x.action == ia])

# 2) Carry component on BP_Dad
bp = eal.load_asset("/Game/Characters/Dad/BP_Dad")
handles = sds.k2_gather_subobject_data_for_blueprint(bp)
comp = None
for h in handles:
    o = SDL.get_object(SDL.get_data(h))
    if isinstance(o, unreal.FVCarryComponent):
        comp = o
if comp is None:
    nh, fail = sds.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=unreal.FVCarryComponent, blueprint_context=bp))
    sds.rename_subobject(nh, unreal.Text("Carry"))
    comp = SDL.get_object(SDL.get_data(nh))
comp.set_editor_property("interact_action", ia)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp)
print("carry component:", comp.get_name(), comp.get_editor_property("interact_action").get_name())

# 3) HUD class on the game mode
gm = eal.load_asset("/Game/Core/BP_FV_GameMode")
unreal.get_default_object(gm.generated_class()).set_editor_property("hud_class", unreal.FVHUD)
unreal.BlueprintEditorLibrary.compile_blueprint(gm)
eal.save_loaded_asset(gm)
print("HUD:", unreal.get_default_object(gm.generated_class()).get_editor_property("hud_class").get_name())
