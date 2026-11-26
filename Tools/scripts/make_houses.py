# Finding Valentino - neighbour houses (background scenery). cm units, origin at front-centre of footprint, ground at z=0.
# Front faces -Y. Material slot names map to Unreal materials: Walls_<kind>, Roof_<kind>, Trim, Glass, Door, Chimney.
# Run: blender -b --factory-startup --python make_houses.py -- <out_dir> [preview.png]
import bpy, bmesh, sys, os, math, random
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]; PREVIEW = argv[1] if len(argv) > 1 else None
os.makedirs(OUT, exist_ok=True)

VARIANTS = [
    # name, width, depth, storeys, walls, roof, seed
    ("SM_FV_House_A", 1100, 900, 2, "Brick", "Clay", 1),
    ("SM_FV_House_B", 1250, 850, 1, "Plaster", "Grey", 2),
    ("SM_FV_House_C", 1000, 950, 2, "BluePlaster", "Grey", 3),
    ("SM_FV_House_D", 1300, 900, 2, "BrownBrick", "Clay", 4),
    ("SM_FV_House_E", 1150, 1000, 1, "ClayPlaster", "Clay", 5),
    ("SM_FV_House_F", 1050, 900, 2, "Plaster", "Clay", 6),
    ("SM_FV_House_Home", 1800, 1000, 2, "Plaster", "Grey", 7),   # the family's own house
]
STOREY = 290.0; FOUND = 35.0; OVERHANG = 45.0; PITCH = 35.0

def new_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.scale_length = 0.01

MATS = {}
def mat(name, rgb):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.diffuse_color = (*rgb, 1)
    return m

COLORS = {"Walls_Brick": (0.5, 0.2, 0.15), "Walls_BrownBrick": (0.4, 0.25, 0.18), "Walls_Plaster": (0.8, 0.78, 0.72),
          "Walls_BluePlaster": (0.5, 0.6, 0.7), "Walls_ClayPlaster": (0.75, 0.6, 0.45), "Roof_Clay": (0.55, 0.2, 0.12),
          "Roof_Grey": (0.25, 0.26, 0.28), "Trim": (0.92, 0.92, 0.9), "Glass": (0.1, 0.14, 0.2), "Door": (0.25, 0.15, 0.1),
          "Chimney": (0.45, 0.2, 0.15)}

def box(name, cx, cy, z0, sx, sy, sz, mname):
    bpy.ops.mesh.primitive_cube_add(size=1, location=(cx, cy, z0 + sz / 2))
    o = bpy.context.object; o.name = name; o.scale = (sx, sy, sz)
    bpy.ops.object.transform_apply(scale=True)
    o.data.materials.append(mat(mname, COLORS[mname]))
    return o

def prism(name, w, d, z0, h, mname, inset=0.0):
    """Gable fill (triangular prism along X)."""
    me = bpy.data.meshes.new(name); bm = bmesh.new()
    x0, x1 = -w / 2 + inset, w / 2 - inset
    pts = [(-d / 2, z0), (d / 2, z0), (0, z0 + h)]
    a = [bm.verts.new((x0, y, z)) for y, z in pts]; b = [bm.verts.new((x1, y, z)) for y, z in pts]
    bm.faces.new(a[::-1]); bm.faces.new(b)
    for i in range(3):
        j = (i + 1) % 3; bm.faces.new((a[i], a[j], b[j], b[i]))
    bm.normal_update(); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(o)
    o.data.materials.append(mat(mname, COLORS[mname]))
    return o

def roof_slab(name, w, d, z0, h, mname, thickness=14.0):
    """Two sloped slabs with overhang, ridge along X."""
    objs = []
    run = d / 2 + OVERHANG; rise = h * (run / (d / 2))
    for side in (-1, 1):
        me = bpy.data.meshes.new(name); bm = bmesh.new()
        xw = w / 2 + OVERHANG
        ridge = (0, z0 + h); eave = (side * run, z0 + h - rise)
        n = Vector((0, side * h, d / 2)).normalized() * thickness   # outward offset
        top = []; bot = []
        for x in (-xw, xw):
            for (y, z) in (ridge, eave):
                bot.append(bm.verts.new((x, y, z)))
                top.append(bm.verts.new((x, y + n.y * side * side, z + n.z)))
        # verts order: x-,ridge  x-,eave  x+,ridge  x+,eave
        def f(vs):
            try: bm.faces.new(vs)
            except ValueError: pass
        f((top[0], top[1], top[3], top[2])); f((bot[0], bot[2], bot[3], bot[1]))
        f((top[0], top[2], bot[2], bot[0])); f((top[1], bot[1], bot[3], top[3]))
        f((top[0], bot[0], bot[1], top[1])); f((top[2], top[3], bot[3], bot[2]))
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        bm.to_mesh(me); bm.free()
        o = bpy.data.objects.new(f"{name}_{side}", me); bpy.context.collection.objects.link(o)
        o.data.materials.append(mat(mname, COLORS[mname])); objs.append(o)
    return objs

def window(cx, y, z, w, h, facing, out):
    """facing: 'front'/'back' (normal -Y/+Y) or 'left'/'right' (normal -X/+X)."""
    t = 6.0
    if facing in ("front", "back"):
        s = -1 if facing == "front" else 1
        out.append(box("frame", cx, y + s * t / 2, z - 6, w + 16, t, h + 12, "Trim"))
        out.append(box("glass", cx, y + s * (t + 0.5), z, w, 1.0, h, "Glass"))
        out.append(box("sill", cx, y + s * (t + 4), z - 10, w + 24, 10, 5, "Trim"))
        out.append(box("mullion", cx, y + s * (t + 1.5), z, 5, 1.5, h, "Trim"))
    else:
        s = -1 if facing == "left" else 1
        out.append(box("frame", cx + s * t / 2, y, z - 6, t, w + 16, h + 12, "Trim"))
        out.append(box("glass", cx + s * (t + 0.5), y, z, 1.0, w, h, "Glass"))
        out.append(box("sill", cx + s * (t + 4), y, z - 10, 10, w + 24, 5, "Trim"))

def build(name, W, D, storeys, walls, roof, seed):
    random.seed(seed); new_scene()
    WM, RM = f"Walls_{walls}", f"Roof_{roof}"
    H = storeys * STOREY
    parts = []
    # footprint centred at (0, D/2): front face at y=0
    cy = D / 2
    parts.append(box("found", 0, cy, 0, W + 10, D + 10, FOUND, "Trim"))
    parts.append(box("walls", 0, cy, FOUND, W, D, H, WM))
    rh = math.tan(math.radians(PITCH)) * D / 2
    g = prism("gable", W, D, FOUND + H, rh, WM, inset=0)
    g.location.y = cy; parts.append(g)
    for o in roof_slab("roof", W, D, FOUND + H, rh, RM):
        o.location.y = cy; parts.append(o)
    parts.append(box("fascia", 0, cy, FOUND + H - 18, W + 2 * OVERHANG, D + 2 * OVERHANG, 18, "Trim"))
    # chimney
    chx = random.choice((-1, 1)) * (W / 2 - 120)
    parts.append(box("chimney", chx, cy + D * 0.2, FOUND + H, 70, 70, rh + 110, "Chimney"))
    parts.append(box("chimney_cap", chx, cy + D * 0.2, FOUND + H + rh + 110, 85, 85, 10, "Trim"))
    # windows per storey on all sides
    for s in range(storeys):
        zc = FOUND + s * STOREY + 165
        n_front = max(2, int(W / 300))
        for i in range(n_front):
            x = -W / 2 + W * (i + 0.5) / n_front
            if s == 0 and i == n_front // 2:
                continue  # door goes here on the front
            window(x, 0, zc, 110, 125, "front", parts)
            window(x, D, zc, 110, 125, "back", parts)
        n_side = max(1, int(D / 400))
        for i in range(n_side):
            y = D * (i + 0.5) / n_side
            window(-W / 2, y, zc, 90, 110, "left", parts)
            window(W / 2, y, zc, 90, 110, "right", parts)
    # front door (+ back door) with small canopy
    dx = -W / 2 + W * ((max(2, int(W / 300)) // 2) + 0.5) / max(2, int(W / 300))
    for y, s in ((0, -1), (D, 1)):
        parts.append(box("doorframe", dx, y + s * 3, FOUND, 115, 6, 225, "Trim"))
        parts.append(box("door", dx, y + s * 6.5, FOUND + 5, 95, 2, 210, "Door"))
        parts.append(box("step", dx, y + s * 40, 0, 160, 80, FOUND - 5, "Trim"))
    parts.append(box("canopy", dx, -45, FOUND + 245, 170, 90, 10, "Trim"))

    # join, UV (1 UV = 250 cm), export
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:
        o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.join()
    o = bpy.context.object; o.name = o.data.name = name
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.remove_doubles(threshold=0.01)
    bpy.ops.uv.cube_project(cube_size=250.0, correct_aspect=False, scale_to_bounds=False)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    for p in o.data.polygons:
        p.use_smooth = False
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True, object_types={'MESH'},
                             global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
                             axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', bake_anim=False)
    d = o.dimensions
    print(f"HOUSE {name}: {d.x:.0f} x {d.y:.0f} x {d.z:.0f} cm, {len(o.data.polygons)} faces, mats={[m.name for m in o.data.materials]}")

for v in VARIANTS:
    build(*v)

if PREVIEW:
    new_scene()
    x = 0
    for v in VARIANTS:
        bpy.ops.import_scene.fbx(filepath=os.path.join(OUT, v[0] + ".fbx"))
        o = bpy.context.selected_objects[0]; o.location.x = x + v[1] / 2; x += v[1] + 300
        for m in o.data.materials:
            m.use_nodes = True
            m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (*COLORS.get(m.name.split(".")[0], (0.5, 0.5, 0.5)), 1)
    sc = bpy.context.scene
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam)
    cam.location = (x / 2, -x * 0.75, x * 0.3); cam.rotation_euler = (Vector((x / 2, 0, 300)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.clip_end = 100000; sc.camera = cam
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.rotation_euler = (0.9, 0.2, -0.6); sun.data.energy = 4
    sc.collection.objects.link(sun)
    w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.7, 0.8, 0.95, 1)
    for e in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
        try:
            sc.render.engine = e; break
        except Exception:
            pass
    sc.render.resolution_x, sc.render.resolution_y = 2000, 700
    sc.render.filepath = PREVIEW; bpy.ops.render.render(write_still=True)
