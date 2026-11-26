# Import downloaded Poly Haven glTF models into /Game/Environment/PolyHaven/<id>/
# Reads ids (one per line) from <SRC>\_import_ids.txt, or imports every downloaded folder.
import unreal, os, glob
eal = unreal.EditorAssetLibrary
SRC = r"C:\Users\calif\Documents\Finding Valentino\Downloads\PolyHaven"
idfile = os.path.join(SRC, "_import_ids.txt")
ids = [l.strip() for l in open(idfile, encoding="utf-8-sig")] if os.path.exists(idfile) else []
ids = [i for i in ids if i] or sorted(d for d in os.listdir(SRC) if os.path.isdir(os.path.join(SRC, d)))
tasks = []
for pid in ids:
    files = glob.glob(os.path.join(SRC, pid, "*.gltf"))
    if not files:
        print("no gltf for", pid); continue
    dest = f"/Game/Environment/PolyHaven/{pid}"
    if eal.does_directory_exist(dest):
        eal.delete_directory(dest)
    t = unreal.AssetImportTask()
    t.filename = files[0]; t.destination_path = dest
    t.automated = True; t.replace_existing = True; t.save = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    dest = t.destination_path
    meshes = []
    for p in eal.list_assets(dest, recursive=True):
        a = eal.load_asset(p)
        if isinstance(a, unreal.StaticMesh):
            b = a.get_bounds()
            meshes.append(f"{a.get_name()} ({b.box_extent.x*2:.0f}x{b.box_extent.y*2:.0f}x{b.box_extent.z*2:.0f} cm, {a.get_num_triangles(0)} tris)")
    eal.save_directory(dest, only_if_is_dirty=False, recursive=True)
    print("IMPORTED", dest, "|", "; ".join(meshes) if meshes else "NO MESH")
