import unreal, os
src = r"C:\Users\calif\Documents\Finding Valentino\Sounds\Barks"
tasks = []
for f in sorted(os.listdir(src)):
    if f.endswith(".wav"):
        t = unreal.AssetImportTask(); t.filename = os.path.join(src, f); t.destination_path = "/Game/Audio/Barks"
        t.automated = True; t.replace_existing = True; t.save = True; tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
print("barks:", sorted(unreal.EditorAssetLibrary.list_assets("/Game/Audio/Barks")))
