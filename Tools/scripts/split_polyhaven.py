# Split Poly Haven glTF files into one glTF per variant (spatially separate clusters),
# each re-centred with its base at the origin. Textures shared per asset in <out>/<id>/textures.
# Run: blender -b --factory-startup --python split_polyhaven.py -- <src_root> <out_root> [id1,id2,...]
import bpy, sys, os, glob, mathutils

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = argv[0], argv[1]
ONLY = set(argv[2].split(",")) if len(argv) > 2 and argv[2] else None

def world_bbox(o):
    pts = [o.matrix_world @ mathutils.Vector(c) for c in o.bound_box]
    return (min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts),
            max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts))

def overlap(a, b, pad):
    return not (a[3] + pad < b[0] or b[3] + pad < a[0] or a[4] + pad < b[1] or b[4] + pad < a[1])

for d in sorted(os.listdir(SRC)):
    if ONLY and d not in ONLY:
        continue
    files = glob.glob(os.path.join(SRC, d, "*.gltf"))
    if not files:
        continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=files[0])
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    # flatten hierarchy, bake transforms
    for o in meshes:
        mw = o.matrix_world.copy(); o.parent = None; o.matrix_world = mw
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.make_single_user(object=True, obdata=True)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for o in list(bpy.context.scene.objects):
        if o.type != 'MESH':
            bpy.data.objects.remove(o, do_unlink=True)

    # cluster by XY bbox overlap (union-find)
    boxes = {o.name: world_bbox(o) for o in meshes}
    size = max(max(b[3] - b[0], b[4] - b[1]) for b in boxes.values())
    pad = 0.02 * size
    parent = {o.name: o.name for o in meshes}
    def find(n):
        while parent[n] != n:
            parent[n] = parent[parent[n]]; n = parent[n]
        return n
    names = list(boxes)
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            if overlap(boxes[names[i]], boxes[names[j]], pad):
                parent[find(names[i])] = find(names[j])
    clusters = {}
    for n in names:
        clusters.setdefault(find(n), []).append(bpy.data.objects[n])
    groups = sorted(clusters.values(), key=lambda g: min(world_bbox(o)[0] for o in g))

    outdir = os.path.join(OUT, d)
    os.makedirs(outdir, exist_ok=True)
    for k, grp in enumerate(groups):
        bpy.ops.object.select_all(action='DESELECT')
        for o in grp:
            o.select_set(True)
        bpy.context.view_layer.objects.active = grp[0]
        if len(grp) > 1:
            bpy.ops.object.join()
        o = bpy.context.view_layer.objects.active
        b = world_bbox(o)
        shift = mathutils.Vector(((b[0] + b[3]) / 2, (b[1] + b[4]) / 2, b[2]))
        o.data.transform(mathutils.Matrix.Translation(-shift))
        o.location = (0, 0, 0)
        name = d if len(groups) == 1 else f"{d}_v{k + 1}"
        o.name = o.data.name = name
        bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
        bpy.ops.export_scene.gltf(filepath=os.path.join(outdir, name + ".gltf"), export_format='GLTF_SEPARATE',
                                  export_texture_dir="textures", use_selection=True, export_image_format='AUTO',
                                  export_apply=True, export_animations=False)
        dims = o.dimensions
        print(f"SPLIT {name}: {dims.x*100:.0f} x {dims.y*100:.0f} x {dims.z*100:.0f} cm, {len(o.data.polygons)} faces")
        o.location = shift  # keep others' positions irrelevant
print("SPLIT DONE")
