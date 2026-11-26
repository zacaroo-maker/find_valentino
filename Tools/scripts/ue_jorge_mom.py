import unreal
eal = unreal.EditorAssetLibrary
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("saved; still unsaved:", len(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()))
aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
g = unreal.load_asset("/Game/Characters/Gardener/MHC_Gardener")
if g: aes.close_all_editors_for_asset(g)
# Jorge: the creator asset, the assembled folder and its blueprint
print("MHC:", eal.rename_asset("/Game/Characters/Gardener/MHC_Gardener", "/Game/Characters/Jorge/MHC_Jorge"))
print("folder:", eal.rename_directory("/Game/MetaHumans/MHC_Gardener", "/Game/MetaHumans/Jorge"))
print("BP:", eal.rename_asset("/Game/MetaHumans/Jorge/BP_MHC_Gardener", "/Game/MetaHumans/Jorge/BP_Jorge"))
at = unreal.AssetToolsHelpers.get_asset_tools()
reds = [d.package_name for d in unreal.AssetRegistryHelpers.get_asset_registry().get_assets(unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/CoreUObject", "ObjectRedirector")], package_paths=["/Game/Characters", "/Game/MetaHumans"], recursive_paths=True))]
if reds:
    at.fixup_referencers([unreal.load_asset(str(r)) for r in reds if unreal.load_asset(str(r))])
eal.delete_directory("/Game/Characters/Gardener") if not eal.list_assets("/Game/Characters/Gardener") else None
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("Jorge BP exists:", eal.does_asset_exist("/Game/MetaHumans/Jorge/BP_Jorge"), "| redirectors fixed:", len(reds))
# Mom: a fresh MetaHuman
path = "/Game/Characters/Family/Mom/MHC_Mom"
mom = eal.load_asset(path) if eal.does_asset_exist(path) else at.create_asset("MHC_Mom", "/Game/Characters/Family/Mom", unreal.MetaHumanCharacter, unreal.MetaHumanCharacterFactoryNew())
eal.save_loaded_asset(mom)
unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).open_editor_for_assets([mom])
print("Mom:", mom.get_path_name())

