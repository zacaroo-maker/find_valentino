import unreal, os
src = r"C:\Users\calif\Documents\Finding Valentino\Sounds"
tasks = []
for f in sorted(os.listdir(src)):
    if f.endswith(".wav"):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(src, f); t.destination_path = "/Game/Audio"
        t.automated = True; t.replace_existing = True; t.save = True
        tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name in ("Music_Menu", "Water_Pour_Loop"):
    w = unreal.load_asset(f"/Game/Audio/{name}"); w.set_editor_property("looping", True); unreal.EditorAssetLibrary.save_loaded_asset(w)
for p in ("/Game/StarterContent/Audio/Starter_Birds01", "/Game/StarterContent/Audio/Starter_Wind05"):
    w = unreal.load_asset(p); w.set_editor_property("looping", True); unreal.EditorAssetLibrary.save_loaded_asset(w)
print("imported:", len(unreal.EditorAssetLibrary.list_assets("/Game/Audio")))
