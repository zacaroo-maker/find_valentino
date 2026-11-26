import unreal
eal = unreal.EditorAssetLibrary
imc = eal.load_asset("/Game/ThirdPerson/Input/IMC_Default")
ia = eal.load_asset("/Game/Core/Input/IA_ToggleView")
maps = list(imc.get_editor_property("mappings"))
if not any(m.action == ia for m in maps):
    k = unreal.Key(); k.import_text("V")
    m = unreal.EnhancedActionKeyMapping(); m.set_editor_property("action", ia); m.set_editor_property("key", k)
    maps.append(m)
    imc.set_editor_property("mappings", maps)
imc.modify()
print("saved:", eal.save_asset("/Game/ThirdPerson/Input/IMC_Default", only_if_is_dirty=False))
print([(x.action.get_name(), x.key.export_text()) for x in imc.get_editor_property("mappings") if x.action == ia])
