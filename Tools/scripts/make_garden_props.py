# Finding Valentino - blockout garden props (procedural, cm units). One FBX per prop.
# Run: blender -b --factory-startup --python make_garden_props.py -- <out_dir> [preview.png]
import bpy, bmesh, sys, os, math, random
from mathutils import Vector, Matrix, Euler

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]
PREVIEW = argv[1] if len(argv) > 1 else None
os.makedirs(OUT, exist_ok=True)
random.seed(7)

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.scale_length = 0.01

MATS = {}
def mat(name, rgb, rough=0.7):
    if name in MATS:
        return MATS[name]
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (*rgb, 1)
    b.inputs["Roughness"].default_value = rough
    m.diffuse_color = (*rgb, 1)
    MATS[name] = m
    return m

GRASS = mat("FV_Grass", (0.16, 0.30, 0.07))
GRASS_DRY = mat("FV_GrassDry", (0.40, 0.38, 0.14))
LEAF = mat("FV_Leaves", (0.10, 0.25, 0.06))
LEAF_LIGHT = mat("FV_LeavesLight", (0.22, 0.38, 0.10))
BARK = mat("FV_Bark", (0.20, 0.13, 0.08), 0.9)
WOOD = mat("FV_WoodWeathered", (0.42, 0.33, 0.23), 0.8)
TERRA = mat("FV_Terracotta", (0.62, 0.28, 0.15), 0.85)
SOIL = mat("FV_Soil", (0.13, 0.08, 0.05), 0.95)
STONE = mat("FV_Stone", (0.45, 0.44, 0.41), 0.9)
DEADLEAF = mat("FV_DeadLeaves", (0.45, 0.25, 0.08), 0.9)
STEM = mat("FV_Stem", (0.15, 0.32, 0.08))
PETALS = {"Red": mat("FV_PetalRed", (0.75, 0.06, 0.06)), "Yellow": mat("FV_PetalYellow", (0.95, 0.75, 0.08)),
          "Purple": mat("FV_PetalPurple", (0.45, 0.15, 0.65))}
CENTER = mat("FV_FlowerCentre", (0.9, 0.7, 0.1))

def clear():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)

def finish(name, objs):
    """Join objs into one mesh named name, apply transforms, origin at world 0, export."""
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if len(objs) > 1:
        bpy.ops.object.join()
    o = bpy.context.object
    o.name = o.data.name = name
    for p in o.data.polygons:
        p.use_smooth = False
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True, object_types={'MESH'},
                             global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
                             axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', bake_anim=False)
    dims = o.dimensions
    print(f"PROP {name}: {dims.x:.0f} x {dims.y:.0f} x {dims.z:.0f} cm, {len(o.data.polygons)} faces")
    return o

def with_mat(o, m):
    o.data.materials.clear(); o.data.materials.append(m); return o

def cyl(r1, r2, h, loc=(0, 0, 0), verts=16, m=None, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r1, radius2=r2, depth=h, location=loc, rotation=rot)
    o = bpy.context.object
    return with_mat(o, m) if m else o

def box(sx, sy, sz, loc, m, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc, rotation=rot)
    o = bpy.context.object; o.scale = (sx, sy, sz)
    return with_mat(o, m)

def blob(r, loc, m, squash=(1, 1, 1), noise=0.18, sub=2):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=sub, radius=r, location=loc)
    o = bpy.context.object; o.scale = squash
    for v in o.data.vertices:
        v.co *= 1 + random.uniform(-noise, noise)
    return with_mat(o, m)

def blades(count, radius, hmin, hmax, m, width=1.4, lean=0.35):
    me = bpy.data.meshes.new("blades"); bm = bmesh.new()
    for _ in range(count):
        a = random.uniform(0, 2 * math.pi); d = radius * math.sqrt(random.random())
        base = Vector((math.cos(a) * d, math.sin(a) * d, 0))
        h = random.uniform(hmin, hmax)
        yaw = random.uniform(0, 2 * math.pi)
        side = Vector((math.cos(yaw), math.sin(yaw), 0)) * width / 2
        tipdir = Vector((random.uniform(-lean, lean), random.uniform(-lean, lean), 1)).normalized()
        mid = base + tipdir * h * 0.55 + Vector((0, 0, 0))
        tip = base + tipdir * h + Vector((random.uniform(-4, 4), random.uniform(-4, 4), 0))
        v = [bm.verts.new(base - side), bm.verts.new(base + side), bm.verts.new(mid + side * 0.7), bm.verts.new(mid - side * 0.7), bm.verts.new(tip)]
        bm.faces.new((v[0], v[1], v[2], v[3])); bm.faces.new((v[3], v[2], v[4]))
    bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("blades", me); sc.collection.objects.link(o)
    return with_mat(o, m)

def hollow(o, thickness):
    mod = o.modifiers.new("solid", 'SOLIDIFY'); mod.thickness = thickness; mod.offset = -1
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return o

made = []

# 1) tall grass clumps
for tag, n, r, h0, h1, m in (("A", 70, 22, 30, 65, GRASS), ("B", 90, 30, 40, 85, GRASS), ("Dry", 60, 25, 30, 60, GRASS_DRY)):
    clear(); made.append(finish(f"SM_FV_TallGrass_{tag}", [blades(n, r, h0, h1, m)]))

# 2) flower clumps
for colour, pm in PETALS.items():
    clear(); objs = [blades(25, 18, 10, 25, STEM, width=2.2, lean=0.5)]
    for _ in range(14):
        a = random.uniform(0, 2 * math.pi); d = 18 * math.sqrt(random.random())
        x, y = math.cos(a) * d, math.sin(a) * d; h = random.uniform(22, 42)
        objs.append(cyl(0.35, 0.3, h, (x, y, h / 2), 4, STEM))
        objs.append(blob(3.2, (x, y, h + 0.5), pm, (1, 1, 0.35), 0.15, 1))
        objs.append(blob(1.0, (x, y, h + 1.4), CENTER, (1, 1, 0.6), 0.05, 1))
    made.append(finish(f"SM_FV_Flowers_{colour}", objs))

# 3) flowerpot (hollow terracotta, open top)
clear()
bpy.ops.mesh.primitive_cone_add(vertices=32, radius1=13, radius2=18, depth=30, location=(0, 0, 15), end_fill_type='NOTHING')
pot = with_mat(bpy.context.object, TERRA)
bm = bmesh.new(); bm.from_mesh(pot.data)
bottom = [v for v in bm.verts if v.co.z < 0]
bmesh.ops.contextual_create(bm, geom=bottom)
bm.to_mesh(pot.data); bm.free()
hollow(pot, 1.6)
rim = bpy.ops.mesh.primitive_torus_add(major_radius=18.2, minor_radius=1.6, location=(0, 0, 29.5)); rim = with_mat(bpy.context.object, TERRA)
made.append(finish("SM_FV_Flowerpot", [pot, rim]))

# 4) potted plant (pot + soil + leafy blob)
clear()
bpy.ops.mesh.primitive_cone_add(vertices=32, radius1=13, radius2=18, depth=30, location=(0, 0, 15)); p2 = with_mat(bpy.context.object, TERRA)
soil = cyl(17, 17, 1, (0, 0, 28.5), 32, SOIL)
leaves = [blob(14, (random.uniform(-6, 6), random.uniform(-6, 6), 38 + random.uniform(0, 12)), random.choice((LEAF, LEAF_LIGHT)), (1, 1, 0.9), 0.25) for _ in range(6)]
made.append(finish("SM_FV_PottedPlant", [p2, soil] + leaves))

# 5) hollow log (Valentino can crawl through) and solid log
clear()
bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=24, depth=170, location=(0, 0, 0), rotation=(0, math.radians(90), 0), end_fill_type='NOTHING')
lg = with_mat(bpy.context.object, BARK)
for v in lg.data.vertices:
    v.co *= 1 + random.uniform(-0.04, 0.04)
hollow(lg, 6)
lg.location = (0, 0, 21)
made.append(finish("SM_FV_LogHollow", [lg]))
clear()
lg2 = cyl(16, 14, 130, (0, 0, 14), 16, BARK, rot=(0, math.radians(90), 0))
for v in lg2.data.vertices:
    v.co *= 1 + random.uniform(-0.05, 0.05)
stub = cyl(3, 1.5, 25, (20, 0, 32), 6, BARK, rot=(math.radians(30), 0, 0))
made.append(finish("SM_FV_Log", [lg2, stub]))

# 6) tree with root flare and lumpy canopy
clear(); objs = [cyl(28, 16, 380, (0, 0, 190), 14, BARK)]
for i in range(5):
    a = i * 2 * math.pi / 5 + random.uniform(-0.3, 0.3)
    objs.append(cyl(14, 3, 90, (math.cos(a) * 38, math.sin(a) * 38, 8), 8, BARK, rot=(0, math.radians(80), a)))
for i in range(3):
    a = random.uniform(0, 2 * math.pi)
    objs.append(cyl(10, 5, 160, (math.cos(a) * 60, math.sin(a) * 60, 360), 8, BARK, rot=(math.radians(55) * math.sin(a), math.radians(-55) * math.cos(a), 0)))
for _ in range(9):
    objs.append(blob(random.uniform(110, 160), (random.uniform(-120, 120), random.uniform(-120, 120), random.uniform(430, 560)), random.choice((LEAF, LEAF_LIGHT)), (1, 1, 0.75), 0.2))
made.append(finish("SM_FV_Tree", objs))

# 7) garden bench (gap underneath ~38 cm)
clear(); objs = []
for x in (-65, 65):
    for y in (-18, 18):
        objs.append(box(6, 6, 42, (x, y, 21), WOOD))
for i in range(4):
    objs.append(box(150, 9, 3, (0, -15 + i * 10, 43.5), WOOD))
for i in range(3):
    objs.append(box(150, 3, 9, (0, 21, 55 + i * 12), WOOD, rot=(math.radians(-12), 0, 0)))
objs.append(box(6, 6, 45, (-65, 22, 65), WOOD)); objs.append(box(6, 6, 45, (65, 22, 65), WOOD))
made.append(finish("SM_FV_Bench", objs))

# 8) fence panel 250 x 180 (posts at ends)
clear(); objs = [box(10, 10, 190, (-125, 0, 95), WOOD), box(10, 10, 190, (125, 0, 95), WOOD),
                 box(250, 4, 8, (0, 4, 40), WOOD), box(250, 4, 8, (0, 4, 150), WOOD)]
for i in range(20):
    objs.append(box(11, 2, 175 + random.uniform(-3, 3), (-117 + i * 12.3, 7, 90), WOOD))
made.append(finish("SM_FV_FencePanel", objs))

# 9) raised bed 300 x 120 x 30 with soil
clear(); objs = [box(300, 5, 30, (0, -57.5, 15), WOOD), box(300, 5, 30, (0, 57.5, 15), WOOD),
                 box(5, 120, 30, (-147.5, 0, 15), WOOD), box(5, 120, 30, (147.5, 0, 15), WOOD),
                 box(290, 110, 24, (0, 0, 12), SOIL)]
made.append(finish("SM_FV_RaisedBed", objs))

# 10) stepping stone (low enough to walk over)
clear(); st = cyl(26, 24, 4, (0, 0, 2), 12, STONE)
for v in st.data.vertices:
    v.co.x *= 1 + random.uniform(-0.15, 0.15); v.co.y *= 1 + random.uniform(-0.15, 0.15)
made.append(finish("SM_FV_SteppingStone", [st]))

# 11) leaf pile (walk-through, he can burrow under it)
clear(); objs = [blob(55, (0, 0, 0), DEADLEAF, (1.1, 1, 0.28), 0.3, 3)]
for _ in range(25):
    objs.append(blob(random.uniform(4, 8), (random.uniform(-80, 80), random.uniform(-70, 70), random.uniform(0, 3)), DEADLEAF, (1, 0.7, 0.12), 0.2, 1))
made.append(finish("SM_FV_LeafPile", objs))

print("PROPS DONE", len(made))

if PREVIEW:
    clear()
    # re-import the exported FBX files in a row for a contact sheet
    files = sorted(f for f in os.listdir(OUT) if f.endswith(".fbx"))
    x = 0
    for f in files:
        bpy.ops.import_scene.fbx(filepath=os.path.join(OUT, f))
        o = bpy.context.selected_objects[0]
        w = max(o.dimensions.x, 60)
        o.location.x = x + w / 2; x += w + 40
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam)
    cam.data.type = 'ORTHO'; cam.data.ortho_scale = x * 1.02
    cam.location = (x / 2, -2500, 300); cam.rotation_euler = (math.radians(84), 0, 0)
    cam.data.clip_end = 10000; sc.camera = cam
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.rotation_euler = (0.8, 0.1, 0.4); sun.data.energy = 4
    sc.collection.objects.link(sun)
    w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.75, 0.8, 0.9, 1)
    sc.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE'
    sc.render.resolution_x, sc.render.resolution_y = 2400, 600
    sc.render.filepath = PREVIEW
    bpy.ops.render.render(write_still=True)
