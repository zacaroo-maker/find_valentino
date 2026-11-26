# Finding Valentino - food items, garden pond (with ramp), watering can + water stream. Static meshes, cm, origin at
# bottom-centre. Blender -Y ends up as Unreal +Y; Blender +X = Unreal +X (watering-can spout points along +X).
# Run: blender -b --factory-startup --python make_items.py -- <out_dir> [preview.png]
import bpy, bmesh, sys, os, math, random
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]; PREVIEW = argv[1] if len(argv) > 1 else None
os.makedirs(OUT, exist_ok=True)
random.seed(3)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.scale_length = 0.01

def reset():
    # clear objects/meshes between items but keep materials
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.meshes):
        bpy.data.meshes.remove(m)

MATS = {}
def mat(name, rgb, rough=0.6):
    m = bpy.data.materials.get(name)
    if not m:
        m = bpy.data.materials.new(name); m.use_nodes = True
        b = m.node_tree.nodes.get("Principled BSDF")
        b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough
        m.diffuse_color = (*rgb, 1)
    return m

def assign(o, m):
    o.data.materials.clear(); o.data.materials.append(m)
    for p in o.data.polygons:
        p.use_smooth = True
    return o

def blob(r, loc, scale, m, noise=0.0, sub=2):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=sub, radius=r, location=loc)
    o = bpy.context.object; o.scale = scale
    for v in o.data.vertices:
        v.co *= 1 + random.uniform(-noise, noise)
    return assign(o, m)

def cyl(r1, r2, a, b, m, verts=16):
    a, b = Vector(a), Vector(b); d = b - a
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r1, radius2=r2, depth=d.length, location=(a + b) / 2)
    o = bpy.context.object; o.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    return assign(o, m)

def export(name, objs, uv_cube=None):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if len(objs) > 1:
        bpy.ops.object.join()
    o = bpy.context.object; o.name = o.data.name = name
    if uv_cube:
        bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.cube_project(cube_size=uv_cube); bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True, object_types={'MESH'},
                             global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
                             axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', bake_anim=False)
    d = o.dimensions
    print(f"ITEM {name}: {d.x:.0f} x {d.y:.0f} x {d.z:.0f} cm, mats={[m.name for m in o.data.materials]}")
    return o

GREEN = mat("Leaf_Green", (0.22, 0.55, 0.12), 0.5); LIGHT = mat("Leaf_Light", (0.55, 0.78, 0.35), 0.5)
YELLOW = mat("Petal_Yellow", (0.95, 0.75, 0.05), 0.6); RED = mat("Berry_Red", (0.75, 0.05, 0.05), 0.25)
SEED = mat("Berry_Seed", (0.9, 0.8, 0.3), 0.5); WORM = mat("Worm_Pink", (0.75, 0.42, 0.42), 0.35)
STONE = mat("Stone", (0.45, 0.43, 0.4), 0.9); PEBBLE = mat("Pebbles", (0.35, 0.33, 0.3), 0.95)
WATER = mat("Water", (0.1, 0.25, 0.3), 0.05); CAN = mat("Can_Green", (0.15, 0.4, 0.25), 0.4)
STREAM = mat("WaterStream", (0.6, 0.8, 0.95), 0.05)

# ---------------- food ----------------
# lettuce leaf: wavy, slightly cupped, light rib
reset()
bpy.ops.mesh.primitive_circle_add(vertices=40, radius=1, fill_type='TRIFAN')
leaf = bpy.context.object
for v in leaf.data.vertices:
    a = math.atan2(v.co.y, v.co.x); r = v.co.length
    v.co.x *= 8.5; v.co.y *= 6.5
    v.co.z = 1.2 * r * r + 0.8 * r * math.sin(a * 7)
assign(leaf, LIGHT)
mod = leaf.modifiers.new("s", 'SOLIDIFY'); mod.thickness = 0.4
bpy.context.view_layer.objects.active = leaf; bpy.ops.object.modifier_apply(modifier="s")
leaf.location.z = 0.4
rib = cyl(0.35, 0.15, (-7.5, 0, 0.6), (7.5, 0, 1.6), GREEN, 6)
export("SM_FV_Food_Lettuce", [leaf, rib])

# dandelion: leaf rosette + stem + yellow flower head
reset(); objs = []
for i in range(6):
    a = i / 6 * 2 * math.pi
    o = blob(1, (math.cos(a) * 5, math.sin(a) * 5, 0.4), (6, 1.6, 0.3), GREEN, 0.05, 2)
    o.rotation_euler = (0, 0, a); objs.append(o)
objs.append(cyl(0.3, 0.25, (0, 0, 0), (1.5, 0, 11), LIGHT, 6))
objs.append(blob(2.6, (1.5, 0, 11.5), (1, 1, 0.55), YELLOW, 0.12, 2))
export("SM_FV_Food_Dandelion", objs)

# strawberry (lying on its side) with green calyx
reset()
bpy.ops.mesh.primitive_uv_sphere_add(segments=20, ring_count=12, radius=1, location=(0, 0, 0))
b = bpy.context.object
for v in b.data.vertices:
    t = (v.co.x + 1) / 2   # taper toward +X tip
    s = 1.0 - 0.55 * t
    v.co.y *= s; v.co.z *= s
b.scale = (3.2, 2.6, 2.6); b.location = (0, 0, 2.2); assign(b, RED)
objs = [b]
for i in range(14):
    a = random.uniform(0, 2 * math.pi); x = random.uniform(-2.5, 2.5)
    rr = 2.6 * (1 - 0.55 * (x / 3.2 + 1) / 2)
    objs.append(blob(0.18, (x, math.cos(a) * rr, 2.2 + math.sin(a) * rr), (1, 1, 1), SEED, 0, 1))
for i in range(5):
    a = i / 5 * 2 * math.pi
    o = blob(1, (-3.2, math.cos(a) * 1.4, 2.2 + math.sin(a) * 1.4), (0.3, 1.5, 0.6), GREEN, 0, 1)
    o.rotation_euler = (a, 0, 0); objs.append(o)
export("SM_FV_Food_Strawberry", objs)

# worm: wiggly segmented tube on the ground
reset(); objs = []
pts = [Vector((i * 1.1 - 7, 1.6 * math.sin(i * 0.7), 0.7)) for i in range(14)]
for i, p in enumerate(pts):
    objs.append(blob(0.75 if 3 < i < 11 else 0.6, p, (1.0, 1, 0.9), WORM, 0.02, 2))
objs.append(blob(0.85, pts[4] + Vector((0.5, 0, 0.05)), (1.1, 1, 1), mat("Worm_Band", (0.85, 0.5, 0.5), 0.3), 0, 2))
export("SM_FV_Food_Worm", objs)

# ---------------- pond ----------------
reset(); objs = []
R_IN, R_OUT, H = 125.0, 168.0, 16.0
FLOOR, WATER_Z = 4.0, 9.5
# pebble floor
bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=R_IN + 8, depth=FLOOR, location=(0, 0, FLOOR / 2))
objs.append(assign(bpy.context.object, PEBBLE))
# rim of stones, with a gap for the ramp at -Y (becomes +Y in Unreal)
n = 30
for i in range(n):
    a = i / n * 2 * math.pi
    if abs(math.atan2(math.sin(a + math.pi / 2), math.cos(a + math.pi / 2))) < math.radians(13):
        continue
    r = (R_IN + R_OUT) / 2 + random.uniform(-4, 4)
    o = blob(1, (math.cos(a) * r, math.sin(a) * r, H * 0.55), (random.uniform(17, 22), random.uniform(13, 17), random.uniform(10, 13)), STONE, 0.12, 2)
    o.rotation_euler = (0, 0, a + math.pi / 2); objs.append(o)
# ramp: profile along Y (outside -> over the rim -> down to the floor), 54 cm wide
prof = [(-(R_OUT + 75), 0.0), (-R_OUT, H), (-R_IN + 5, H), (-(R_IN - 65), FLOOR)]
bm = bmesh.new()
top_l, top_r = [], []
for y, z in prof:
    top_l.append(bm.verts.new((-27, y, z))); top_r.append(bm.verts.new((27, y, z)))
bot_l = [bm.verts.new((-27, y, 0)) for y, _ in prof]; bot_r = [bm.verts.new((27, y, 0)) for y, _ in prof]
for i in range(len(prof) - 1):
    bm.faces.new((top_l[i], top_r[i], top_r[i + 1], top_l[i + 1]))
    bm.faces.new((bot_l[i], bot_l[i + 1], bot_r[i + 1], bot_r[i]))
    bm.faces.new((top_l[i], top_l[i + 1], bot_l[i + 1], bot_l[i]))
    bm.faces.new((top_r[i], bot_r[i], bot_r[i + 1], top_r[i + 1]))
bm.faces.new((top_l[0], bot_l[0], bot_r[0], top_r[0])); bm.faces.new((top_l[-1], top_r[-1], bot_r[-1], bot_l[-1]))
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
me = bpy.data.meshes.new("ramp"); bm.to_mesh(me); bm.free()
ramp = bpy.data.objects.new("ramp", me); bpy.context.collection.objects.link(ramp); objs.append(assign(ramp, STONE))
for s in (-1, 1):   # edging stones along the ramp
    for y in (-(R_OUT + 40), -(R_OUT + 5)):
        objs.append(blob(1, (s * 34, y, 6), (9, 14, 8), STONE, 0.15, 2))
# flat basking stones outside
for a_deg in (25, 150, 205):
    a = math.radians(a_deg)
    bpy.ops.mesh.primitive_cylinder_add(vertices=14, radius=1, depth=6, location=(math.cos(a) * 215, math.sin(a) * 215, 3))
    o = bpy.context.object; o.scale = (random.uniform(26, 32), random.uniform(20, 26), 1)
    for v in o.data.vertices:
        v.co.x *= 1 + random.uniform(-0.08, 0.08)
    objs.append(assign(o, STONE))
export("SM_FV_Pond", objs, uv_cube=80)
# water surface (separate: no collision, its own material)
reset()
bpy.ops.mesh.primitive_circle_add(vertices=48, radius=R_IN + 6, fill_type='NGON', location=(0, 0, WATER_Z))
w = bpy.context.object; assign(w, WATER)
export("SM_FV_PondWater", [w], uv_cube=200)

# ---------------- watering can (spout along +X) ----------------
reset(); objs = []
objs.append(cyl(11, 10, (0, 0, 0), (0, 0, 24), CAN, 24))
objs.append(cyl(10.2, 10.2, (0, 0, 24), (0, 0, 25.5), CAN, 24))
objs.append(cyl(1.6, 1.0, (8, 0, 5), (36, 0, 30), CAN, 10))           # spout
objs.append(cyl(3.0, 3.6, (35, 0, 29.4), (38, 0, 31.6), CAN, 12))     # rose
bpy.ops.mesh.primitive_torus_add(major_radius=11, minor_radius=1.3, location=(-4, 0, 26), rotation=(math.pi / 2, 0, 0))
h = bpy.context.object; h.scale = (1, 1, 0.9); objs.append(assign(h, CAN))  # handle arch
export("SM_FV_WateringCan", objs)
# water stream from the rose, falling forward/down (attached to the can)
reset()
o = cyl(1.4, 0.6, (38, 0, 30.5), (52, 0, -35), STREAM, 10)
export("SM_FV_WaterStream", [o])

if PREVIEW:
    reset(); x = 0
    for f in sorted(os.listdir(OUT)):
        if not f.endswith(".fbx") or "Stream" in f:
            continue
        bpy.ops.import_scene.fbx(filepath=os.path.join(OUT, f))
        o = bpy.context.selected_objects[0]
        s = 1.0 if o.dimensions.x > 100 else (6.0 if o.dimensions.x < 25 else 3.0)
        o.scale = (s, s, s); o.location.x = x + o.dimensions.x * s / 2; x += o.dimensions.x * s + 60
    sc = bpy.context.scene
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam)
    cam.data.type = 'ORTHO'; cam.data.ortho_scale = x * 1.05
    cam.location = (x / 2, -1500, 700); cam.rotation_euler = (math.radians(65), 0, 0); cam.data.clip_end = 10000; sc.camera = cam
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.rotation_euler = (0.8, 0.1, 0.4); sun.data.energy = 4
    sc.collection.objects.link(sun)
    for e in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
        try:
            sc.render.engine = e; break
        except Exception:
            pass
    sc.render.resolution_x, sc.render.resolution_y = 2000, 600; sc.render.filepath = PREVIEW
    bpy.ops.render.render(write_still=True)
