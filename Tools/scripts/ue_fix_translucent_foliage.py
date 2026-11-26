# UE 5.8: Nanite meshes don't render translucent materials. Switch glTF "Blend" foliage materials to masked.
import unreal
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
mask_ds = eal.load_asset("/InterchangeAssets/gltf/MaterialInstances/MI_Default_Mask_DS")
mask = eal.load_asset("/InterchangeAssets/gltf/MaterialInstances/MI_Default_Mask")
n = 0
for p in eal.list_assets("/Game/Environment/PolyHaven", recursive=True):
    a = eal.load_asset(p)
    if isinstance(a, unreal.MaterialInstanceConstant):
        par = a.get_editor_property("parent")
        if par and par.get_name().startswith("MI_Default_Blend"):
            mel.set_material_instance_parent(a, mask_ds if par.get_name().endswith("_DS") else mask)
            mel.update_material_instance(a)
            eal.save_loaded_asset(a); n += 1
            print("masked:", a.get_name())
print("fixed", n)
