import unreal
eal = unreal.EditorAssetLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()
DST = "/Game/Characters/Dad/Animations"
moved = 0
for a in ar.get_assets_by_path("/Game", recursive=False):
    n = str(a.asset_name)
    if n.endswith("_Dad"):
        if eal.rename_asset(str(a.package_name), f"{DST}/{n}"):
            moved += 1
print("moved", moved)
eal.save_directory(DST, only_if_is_dirty=False, recursive=True)
eal.save_directory("/Game/Characters/Dad", only_if_is_dirty=False, recursive=False)

abp = eal.load_asset(DST + "/ABP_Manny_Dad")
unreal.BlueprintEditorLibrary.compile_blueprint(abp)
eal.save_loaded_asset(abp)
print("ABP target skeleton:", abp.get_editor_property("target_skeleton").get_name())

bp = eal.load_asset("/Game/Characters/Dad/BP_Dad")
mesh = unreal.get_default_object(bp.generated_class()).get_editor_property("mesh")
mesh.set_editor_property("anim_class", abp.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp)
print("BP_Dad anim class:", unreal.get_default_object(eal.load_asset("/Game/Characters/Dad/BP_Dad").generated_class()).get_editor_property("mesh").get_editor_property("anim_class"))
# fix redirectors left in /Game
reds = [a for a in ar.get_assets_by_path("/Game", recursive=False) if str(a.asset_class_path.asset_name) == "ObjectRedirector"]
print("redirectors left in /Game:", len(reds))
