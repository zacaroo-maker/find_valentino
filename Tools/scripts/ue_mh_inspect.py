import unreal
eal = unreal.EditorAssetLibrary
els = unreal.EditorLoadingAndSavingUtils
els.save_dirty_packages(True, True)
print("unsaved after save:", len(els.get_dirty_content_packages()))
ar = unreal.AssetRegistryHelpers.get_asset_registry()
print("MetaHumans folders:", [str(p) for p in ar.get_sub_paths("/Game/MetaHumans", False)])
aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
m = unreal.load_asset("/Game/Characters/Family/Mom/MHC_Mom")
if m: aes.close_all_editors_for_asset(m)
if eal.does_directory_exist("/Game/MetaHumans/MHC_Mom"):
    print("folder:", eal.rename_directory("/Game/MetaHumans/MHC_Mom", "/Game/MetaHumans/Mom"))
    print("BP:", eal.rename_asset("/Game/MetaHumans/Mom/BP_MHC_Mom", "/Game/MetaHumans/Mom/BP_Mom"))
    els.save_dirty_packages(True, True)
for p in ("/Game/MetaHumans/Common/Animation", "/Game/MetaHumans/Common/Body", "/Game/MetaHumans/Common/Common"):
    print(p, [ (str(a.asset_name), str(a.asset_class_path.asset_name)) for a in ar.get_assets_by_path(p, False)])
bp = unreal.load_asset("/Game/MetaHumans/Jorge/BP_Jorge")
gc = unreal.load_object(None, bp.generated_class().get_path_name())
cdo = unreal.get_default_object(gc)
print("parent:", bp.generated_class().get_super_class() if hasattr(bp.generated_class(),'get_super_class') else '?')
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = sds.k2_gather_subobject_data_for_blueprint(bp)
for h in handles:
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if obj is None: continue
    extra = ""
    if isinstance(obj, unreal.SkeletalMeshComponent):
        sk = obj.get_editor_property("skeletal_mesh_asset")
        extra = f" mesh={sk.get_path_name() if sk else None} skel={sk.skeleton.get_path_name() if sk else None} anim={obj.get_editor_property('anim_class')}"
    print("  comp:", obj.get_name(), obj.get_class().get_name(), extra)
