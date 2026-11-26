import unreal
eal = unreal.EditorAssetLibrary
aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
m = unreal.load_asset("/Game/Characters/Family/Mom/MHC_Mom")
if m: aes.close_all_editors_for_asset(m)
if eal.does_directory_exist("/Game/MetaHumans/MHC_Mom"):
    print("folder:", eal.rename_directory("/Game/MetaHumans/MHC_Mom", "/Game/MetaHumans/Mom"))
    print("BP:", eal.rename_asset("/Game/MetaHumans/Mom/BP_MHC_Mom", "/Game/MetaHumans/Mom/BP_Mom"))
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

def comps(path):
    bp = unreal.load_asset(path)
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    seen = set()
    for h in sds.k2_gather_subobject_data_for_blueprint(bp):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
        if obj is None or obj.get_name() in seen: continue
        seen.add(obj.get_name())
        extra = ""
        par = obj.get_attach_parent().get_name() if isinstance(obj, unreal.SceneComponent) and obj.get_attach_parent() else ""
        if isinstance(obj, unreal.SkeletalMeshComponent):
            sk = obj.get_editor_property("skeletal_mesh_asset")
            extra = f" mesh={sk.get_name() if sk else None} skel={sk.skeleton.get_path_name() if sk else None} anim={obj.get_editor_property('anim_class')}"
        print(f"  {obj.get_name()} [{obj.get_class().get_name()}] parent={par}{extra}")
for p in ("/Game/MetaHumans/Mom/BP_Mom", "/Game/Characters/Dad/BP_Dad"):
    print(p); comps(p)
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for cls in ("AnimBlueprint", "Skeleton", "IKRetargeter", "IKRigDefinition"):
    f = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine" if cls in ("AnimBlueprint","Skeleton") else "/Script/IKRig", cls)], package_paths=["/Game"], recursive_paths=True)
    print(cls, [str(a.package_name) for a in ar.get_assets(f)][:25])
