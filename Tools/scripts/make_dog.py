# Finding Valentino - placeholder family dog: small shaggy white dog, black floppy ears/head-top,
# fluffy white tail, red plaid fleece vest. Rigged (rigid parts) with 6 animations. cm units, faces -Y.
# Run: blender -b --factory-startup --python make_dog.py -- <out.fbx> <plaid.png> [preview.png]
import bpy, bmesh, sys, math, random
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT, PLAID = argv[0], argv[1]
PREVIEW = argv[2] if len(argv) > 2 else None
random.seed(5)

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.scale_length = 0.01
sc.render.fps = 30

# ---- plaid texture (red tartan with black, thin white/yellow/blue lines) ----
N = 512
img = bpy.data.images.new("T_DogVest_Plaid", N, N)
px = [0.0] * (N * N * 4)
def band(v):
    p = v % 128
    if p < 40: return "black"
    if 60 <= p < 63: return "white"
    if 90 <= p < 92: return "yellow"
    if 110 <= p < 112: return "blue"
    return "red"
COL = {"red": (0.62, 0.05, 0.05), "black": (0.05, 0.04, 0.04), "white": (0.95, 0.95, 0.92),
       "yellow": (0.9, 0.8, 0.3), "blue": (0.1, 0.2, 0.6)}
for y in range(N):
    for x in range(N):
        a, b = COL[band(x)], COL[band(y)]
        c = [(a[i] + b[i]) * 0.5 for i in range(3)]
        if band(x) == "black" and band(y) == "black":
            c = list(COL["black"])
        n = random.uniform(-0.03, 0.03)   # fleece noise
        i = (y * N + x) * 4
        px[i:i + 4] = [max(0, c[0] + n), max(0, c[1] + n), max(0, c[2] + n), 1.0]
img.pixels = px
img.filepath_raw = PLAID; img.file_format = 'PNG'; img.save()

def mat(name, rgb, rough=0.8, tex=None):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough
    if tex:
        t = m.node_tree.nodes.new("ShaderNodeTexImage"); t.image = tex
        m.node_tree.links.new(t.outputs["Color"], b.inputs["Base Color"])
    m.diffuse_color = (*rgb, 1)
    return m
M_WHITE = mat("Fur_White", (0.86, 0.84, 0.8), 0.95)
M_BLACK = mat("Fur_Black", (0.05, 0.045, 0.04), 0.9)
M_NOSE = mat("Nose", (0.02, 0.02, 0.02), 0.3)
M_EYE = mat("Eye", (0.03, 0.02, 0.015), 0.1)
M_VEST = mat("Vest", (0.6, 0.06, 0.06), 0.9, img)

parts = []
def add(o, bone, m, shaggy=0.0):
    o.data.materials.append(m)
    if shaggy:
        for v in o.data.vertices:
            v.co *= 1 + random.uniform(-shaggy, shaggy)
    for p in o.data.polygons:
        p.use_smooth = True
    parts.append((o, bone)); return o

def blob(name, loc, scale, bone, m, shaggy=0.0, seg=24, rings=12, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=rings, location=loc, rotation=rot)
    o = bpy.context.object; o.name = name; o.scale = scale
    return add(o, bone, m, shaggy)

def cyl(name, a, b, r, bone, m, shaggy=0.0, verts=12):
    a, b = Vector(a), Vector(b); d = b - a
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=r, depth=d.length, location=(a + b) / 2)
    o = bpy.context.object; o.name = name; o.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    return add(o, bone, m, shaggy)

# body + vest
blob("Body", (0, 0, 20), (10.5, 19, 9.5), "body", M_WHITE, 0.06)
bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, location=(0, -5, 21))
vest = bpy.context.object; vest.name = "Vest"; vest.scale = (11.4, 14.5, 10.6)
bm = bmesh.new(); bm.from_mesh(vest.data)
bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z < -0.55], context='VERTS')   # open belly
bm.to_mesh(vest.data); bm.free()
bpy.context.view_layer.objects.active = vest
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.sphere_project(); bpy.ops.object.mode_set(mode='OBJECT')
for l in vest.data.uv_layers.active.data:
    l.uv = (l.uv[0] * 3.0, l.uv[1] * 2.0)   # tile the plaid
add(vest, "body", M_VEST)
# neck, head
cyl("Neck", (0, -14, 24), (0, -19, 31), 5.5, "neck", M_WHITE, 0.05)
blob("Head", (0, -22, 35), (7.5, 7.5, 7), "head", M_WHITE, 0.07)
blob("HeadTop", (0, -20.5, 39), (7.6, 6.8, 4.2), "head", M_BLACK, 0.08)
blob("Fringe", (0, -26.5, 40), (4.5, 3.5, 3), "head", M_WHITE, 0.15, 12, 6)
blob("Snout", (0, -29, 32), (3.4, 4.6, 3), "head", M_WHITE, 0.05)
blob("Beard", (0, -29.5, 29), (3.2, 3.5, 2.6), "head", M_WHITE, 0.15, 12, 6)
blob("Nose", (0, -33.4, 33), (1.4, 1.1, 1.1), "head", M_NOSE, 0, 12, 6)
for sx in (-1, 1):
    blob(f"Eye{sx}", (sx * 3.0, -28.2, 36.2), (1.1, 0.9, 1.1), "head", M_EYE, 0, 12, 6)
    blob(f"Ear{sx}", (sx * 7.2, -20.5, 31.5), (2.6, 3.8, 9.5), "head", M_BLACK, 0.12, 16, 8, rot=(0, sx * 0.25, 0))
# legs
LEGS = {"leg_FL": (5.5, -11), "leg_FR": (-5.5, -11), "leg_BL": (5.5, 12), "leg_BR": (-5.5, 12)}
for name, (x, y) in LEGS.items():
    cyl(name + "_m", (x, y, 16), (x, y, 2.5), 3.0, name, M_WHITE, 0.08)
    blob(name + "_paw", (x, y - 1.2, 2.2), (3.0, 3.8, 2.2), name, M_WHITE, 0.05, 12, 6)
# tail plume
blob("Tail", (0, 25, 27), (3.8, 8.5, 4.2), "tail", M_WHITE, 0.2, 16, 8, rot=(math.radians(-35), 0, 0))

# ---- armature ----
arm_data = bpy.data.armatures.new("Dog_Skeleton")
arm = bpy.data.objects.new("Armature", arm_data); sc.collection.objects.link(arm)
bpy.context.view_layer.objects.active = arm
bpy.ops.object.mode_set(mode='EDIT')
eb = arm_data.edit_bones
def bone(n, h, t, parent=None):
    b = eb.new(n); b.head = h; b.tail = t
    if parent: b.parent = eb[parent]
bone("root", (0, 0, 0), (0, 0, 6))
bone("body", (0, 0, 20), (0, -8, 20), "root")
bone("neck", (0, -14, 24), (0, -19, 31), "body")
bone("head", (0, -19, 31), (0, -27, 36), "neck")
for n, (x, y) in LEGS.items():
    bone(n, (x, y, 16), (x, y, 2.5), "body")
bone("tail", (0, 18, 23), (0, 28, 30), "body")
bpy.ops.object.mode_set(mode='OBJECT')

for o, b in parts:
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    vg = o.vertex_groups.new(name=b); vg.add(list(range(len(o.data.vertices))), 1.0, 'REPLACE')
bpy.ops.object.select_all(action='DESELECT')
for o, _ in parts:
    o.select_set(True)
bpy.context.view_layer.objects.active = parts[0][0]
bpy.ops.object.join()
body = bpy.context.object; body.name = body.data.name = "Dog"
body.parent = arm; m = body.modifiers.new("Armature", 'ARMATURE'); m.object = arm

# ---- animations ----
pb = arm.pose.bones
for p in pb:
    p.rotation_mode = 'XYZ'
arm.animation_data_create()
def new_action(name, frames):
    a = bpy.data.actions.new(name); a.use_fake_user = True; arm.animation_data.action = a
    for p in pb:
        p.location = (0, 0, 0); p.rotation_euler = (0, 0, 0)
    sc.frame_start, sc.frame_end = 0, frames
    return a
def key(p, f, loc=None, rot=None):
    if loc is not None:
        p.location = loc; p.keyframe_insert("location", frame=f)
    if rot is not None:
        p.rotation_euler = [math.radians(v) for v in rot]; p.keyframe_insert("rotation_euler", frame=f)
def rest(f):
    for p in pb:
        key(p, f, (0, 0, 0), (0, 0, 0))

# Idle: tail wag, head looks around, breathing (3 s)
new_action("Idle", 90); rest(0)
for f in range(0, 91, 6):
    key(pb["tail"], f, rot=(0, 0, 25 * math.sin(f / 90 * 2 * math.pi * 4)))
for f, yaw, nod in ((0, 0, 0), (30, 20, -5), (60, -15, 5), (90, 0, 0)):
    key(pb["head"], f, rot=(nod, 0, yaw))
for f, z in ((0, 0), (45, 0.5), (90, 0)):
    key(pb["body"], f, loc=(0, 0, z))
# Walk: trot, diagonal pairs (0.6 s cycle, ~100 cm/s)
new_action("Walk", 18); rest(0)
for f in range(0, 19, 3):
    ph = f / 18 * 2 * math.pi; sw = 32 * math.sin(ph)
    key(pb["leg_FL"], f, rot=(sw, 0, 0)); key(pb["leg_BR"], f, rot=(sw, 0, 0))
    key(pb["leg_FR"], f, rot=(-sw, 0, 0)); key(pb["leg_BL"], f, rot=(-sw, 0, 0))
    key(pb["body"], f, loc=(0, 0, 0.8 * abs(math.sin(ph))))
    key(pb["head"], f, rot=(3 * math.sin(2 * ph), 0, 0))
    key(pb["tail"], f, rot=(0, 0, 18 * math.sin(2 * ph)))
# Sniff: nose to the ground, small side sweeps (2 s)
new_action("Sniff", 60); rest(0)
for f in range(0, 61, 6):
    key(pb["neck"], f, rot=(-38, 0, 0))
    key(pb["head"], f, rot=(-20 + 4 * math.sin(f), 0, 18 * math.sin(f / 60 * 2 * math.pi)))
    key(pb["tail"], f, rot=(0, 0, 12 * math.sin(f / 60 * 2 * math.pi * 3)))
# lying pose helper: body lowered, front legs forward, back legs folded
def lying(f, breathe=0.0):
    key(pb["body"], f, loc=(0, 0, -10 + breathe))
    key(pb["leg_FL"], f, rot=(85, 0, 0)); key(pb["leg_FR"], f, rot=(85, 0, 0))
    key(pb["leg_BL"], f, rot=(-80, 0, 15)); key(pb["leg_BR"], f, rot=(-80, 0, -15))
    key(pb["neck"], f, rot=(-15, 0, 0)); key(pb["head"], f, rot=(5, 0, 10))
    key(pb["tail"], f, rot=(30, 0, 40))
new_action("LieDown", 30); rest(0); lying(30)
new_action("Lying", 120)
for f in (0, 40, 80, 120):
    lying(f, 0.6 if f in (40, 120) else 0.0)
key(pb["tail"], 60, rot=(30, 0, 55)); key(pb["head"], 90, rot=(8, 0, -5))
new_action("GetUp", 24); lying(0); rest(24)

arm.animation_data.action = bpy.data.actions["Idle"]; sc.frame_set(0)
pts = [body.matrix_world @ Vector(c) for c in body.bound_box]
print("DOG size cm: L=%.0f W=%.0f H=%.0f verts=%d" % (max(p.y for p in pts) - min(p.y for p in pts),
      max(p.x for p in pts) - min(p.x for p in pts), max(p.z for p in pts), len(body.data.vertices)))

if PREVIEW:
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam)
    cam.location = (70, -80, 55); cam.rotation_euler = (Vector((0, -4, 26)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.clip_start = 1; sc.camera = cam
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.rotation_euler = (0.7, 0.2, 0.6); sun.data.energy = 4
    sc.collection.objects.link(sun)
    w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.6, 0.7, 0.85, 1)
    for e in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
        try:
            sc.render.engine = e; break
        except Exception:
            pass
    sc.render.resolution_x, sc.render.resolution_y = 1000, 750; sc.render.filepath = PREVIEW
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam); bpy.data.objects.remove(sun)

bpy.ops.object.select_all(action='DESELECT'); arm.select_set(True); body.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.export_scene.fbx(filepath=OUT, use_selection=True, object_types={'ARMATURE', 'MESH'}, global_scale=1.0,
    apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True, axis_forward='-Z', axis_up='Y',
    add_leaf_bones=False, mesh_smooth_type='FACE', bake_anim=True, bake_anim_use_all_actions=True,
    bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0)
print("EXPORTED", OUT)

