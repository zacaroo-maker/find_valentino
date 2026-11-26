# UE 5.8: key mappings live in IMC.default_key_mappings.mappings (the old IMC.mappings list is deprecated and not used).
import unreal
eal = unreal.EditorAssetLibrary
PATH = "/Game/ThirdPerson/Input/IMC_Default"
WANT = {"/Game/Core/Input/IA_Use": "LeftMouseButton", "/Game/Core/Input/IA_Pet": "F",
        "/Game/Core/Input/IA_Interact": "E", "/Game/Core/Input/IA_ToggleView": "V"}
imc = eal.load_asset(PATH)
dk = imc.get_editor_property("default_key_mappings")
maps = list(dk.get_editor_property("mappings"))
for ia_path, key in WANT.items():
    ia = eal.load_asset(ia_path)
    if not any(m.action == ia and m.key.export_text() == key for m in maps):
        k = unreal.Key(); k.import_text(key)
        m = unreal.EnhancedActionKeyMapping(); m.set_editor_property("action", ia); m.set_editor_property("key", k)
        maps.append(m); print("added", ia.get_name(), key)
dk.set_editor_property("mappings", maps)
imc.set_editor_property("default_key_mappings", dk)
imc.modify(); eal.save_asset(PATH, only_if_is_dirty=False)
print("now:", [(m.action.get_name(), m.key.export_text()) for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings") if m.action.get_name().startswith(("IA_Use", "IA_Pet", "IA_Inter", "IA_Toggle"))])
