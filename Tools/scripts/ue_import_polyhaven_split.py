# Import split Poly Haven glTFs listed in Downloads\PolyHavenSplit\_import_list.txt (one filename stem per line)
# into /Game/Environment/PolyHaven/<asset id>/ ; enable Nanite on heavy meshes.
import unreal, os, re
eal = unreal.EditorAssetLibrary
SRC = r"C:\Users\calif\Documents\Finding Valentino\Downloads\PolyHavenSplit"
stems = [l.strip() for l in open(os.path.join(SRC, "_import_list.txt"), encoding="utf-8-sig") if l.strip()]
tasks = []
for stem in stems:
    pid = re.sub(r"_v\d+$", "", stem)
    f = os.path.join(SRC, pid, stem + ".gltf")
    if not os.path.exists(f):
        print("missing", f); continue
    t = unreal.AssetImportTask()
    t.filename = f; t.destination_path = f"/Game/Environment/PolyHaven/{pid}"
    t.automated = True; t.replace_existing = True; t.save = False
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

done = set()
for t in tasks:
    if t.destination_path in done:
        continue
    done.add(t.destination_path)
    for p in eal.list_assets(t.destination_path, recursive=True):
        a = eal.load_asset(p)
        if isinstance(a, unreal.StaticMesh):
            tris = a.get_num_triangles(0)
            if tris > 20000:
                ns = a.get_editor_property("nanite_settings")
                if not ns.enabled:
                    ns.enabled = True
                    a.set_editor_property("nanite_settings", ns)
            b = a.get_bounds()
            print(f"MESH {a.get_name():30} {b.box_extent.x*2:5.0f}x{b.box_extent.y*2:5.0f}x{b.box_extent.z*2:5.0f} cm  tris={tris:8d} nanite={a.get_editor_property('nanite_settings').enabled}")
    eal.save_directory(t.destination_path, only_if_is_dirty=False, recursive=True)
print("IMPORT DONE", len(tasks))
