# Finding Valentino - placeholder Hermann's tortoise (procedural), rigged + 4 animations.
# Run: blender -b --factory-startup --python make_valentino.py -- <out.fbx> [preview.png]
# Units: centimetres (scene unit scale 0.01). Faces -Y (Blender convention), like Dad.
import bpy, bmesh, sys, math, mathutils
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]
PREVIEW = argv[1] if len(argv) > 1 else None

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.scale_length = 0.01
sc.render.fps = 30

def mat(name, rgb, rough=0.6):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (*rgb, 1)
    b.inputs["Roughness"].default_value = rough
    m.diffuse_color = (*rgb, 1)
    return m

M_SHELL = mat("Shell_DarkBrown", (0.16, 0.09, 0.035), 0.45)
M_SCUTE = mat("Shell_Yellow", (0.62, 0.45, 0.14), 0.45)
M_PLAST = mat("Plastron_Cream", (0.72, 0.62, 0.36))
M_SKIN = mat("Skin_OliveGrey", (0.36, 0.33, 0.22), 0.75)
M_EYE = mat("Eye_Black", (0.01, 0.01, 0.01), 0.15)
M_HEART = mat("Sticker_Pink", (0.95, 0.35, 0.55), 0.35)

parts = []  # (object, bone)

def add(obj, bone, m):
    obj.data.materials.append(m)
    for p in obj.data.polygons:
        p.use_smooth = True
    parts.append((obj, bone))
    return obj

def sphere(name, loc, scale, bone, m, seg=32, rings=16):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=rings, location=loc)
    o = bpy.context.object; o.name = name; o.scale = scale
    return add(o, bone, m)

def cyl(name, a, b, r, bone, m, verts=16):
    a, b = Vector(a), Vector(b)
    d = b - a
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=r, depth=d.length, location=(a + b) / 2)
    o = bpy.context.object; o.name = name
    o.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    return add(o, bone, m)

# --- carapace: upper half of an ellipsoid, domed ---
bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, location=(0, 0, 0))
shell = bpy.context.object; shell.name = "Carapace"
bm = bmesh.new(); bm.from_mesh(shell.data)
bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z < -0.02], context='VERTS')
for v in bm.verts:
    # flatter front/back, slightly higher at the back like Hermann's
    v.co.x *= 7.2; v.co.y *= 9.2; v.co.z = v.co.z * 5.2 * (1.0 + 0.06 * v.co.y)
bm.to_mesh(shell.data); bm.free()
shell.location = (0, 0, 3.4)
add(shell, "body", M_SHELL)

# rim / marginal ring
bpy.ops.mesh.primitive_torus_add(major_radius=1.0, minor_radius=0.09, location=(0, 0, 3.45))
rim = bpy.context.object; rim.name = "Marginal"; rim.scale = (7.2, 9.2, 6.0)
add(rim, "body", M_SHELL)

# scute highlights: yellow patches sitting on the dome
def dome_point(x, y):
    nx, ny = x / 7.2, y / 9.2
    zz = max(0.0, 1 - nx * nx - ny * ny)
    return Vector((x, y, 3.4 + math.sqrt(zz) * 5.2 * (1 + 0.06 * ny)))
def scute(name, x, y, sx, sy):
    p = dome_point(x, y)
    n = Vector((x / 7.2 ** 2, y / 9.2 ** 2, (p.z - 3.4) / (5.2 ** 2))).normalized()
    o = sphere(name, p - n * 0.25, (sx, sy, 0.35), "body", M_SCUTE, 20, 10)
    o.rotation_euler = n.to_track_quat('Z', 'Y').to_euler()
for i, y in enumerate((-5.6, -2.8, 0.0, 2.8, 5.4)):
    scute(f"Vertebral{i}", 0, y, 1.5, 1.2)
for side in (-1, 1):
    for i, y in enumerate((-4.2, -1.4, 1.4, 4.0)):
        scute(f"Costal{side}_{i}", side * 3.9, y, 1.3, 1.1)

# plastron
bpy.ops.mesh.primitive_cylinder_add(vertices=40, radius=1, depth=1.2, location=(0, 0, 3.0))
pl = bpy.context.object; pl.name = "Plastron"; pl.scale = (6.6, 8.6, 1)
add(pl, "body", M_PLAST)

# --- three pink heart stickers on the back of the shell ---
def heart(name, x, y, size):
    p = dome_point(x, y)
    n = Vector((x / 7.2 ** 2, y / 9.2 ** 2, (p.z - 3.4) / (5.2 ** 2))).normalized()
    objs = []
    for dx in (-0.5, 0.5):
        bpy.ops.mesh.primitive_cylinder_add(vertices=20, radius=0.55 * size, depth=0.12, location=(dx * size * 0.75, 0.3 * size, 0))
        objs.append(bpy.context.object)
    bpy.ops.mesh.primitive_cube_add(size=1.05 * size, location=(0, -0.25 * size, 0))
    c = bpy.context.object; c.rotation_euler = (0, 0, math.radians(45)); c.scale = (1, 1, 0.115 / (1.05 * size)); objs.append(c)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.ops.object.join()
    h = bpy.context.object; h.name = name
    # heart points toward the tail (+Y); tilt to the shell surface
    h.rotation_euler = (n.to_track_quat('Z', 'Y') @ mathutils.Quaternion((0, 0, 1), math.pi)).to_euler()
    h.location = p + n * 0.05
    add(h, "body", M_HEART)
heart("Heart1", -1.6, 5.2, 1.0)
heart("Heart2", 1.5, 5.6, 0.85)
heart("Heart3", 0.0, 7.0, 0.75)

# --- head, neck, eyes ---
cyl("Neck", (0, -7.0, 4.2), (0, -10.0, 4.6), 1.35, "neck", M_SKIN)
sphere("Head", (0, -11.4, 4.8), (1.75, 2.4, 1.6), "head", M_SKIN)
for sx in (-1, 1):
    sphere(f"Eye{sx}", (sx * 1.35, -12.3, 5.35), (0.38, 0.38, 0.38), "head", M_EYE, 16, 8)
sphere("Beak", (0, -13.5, 4.5), (0.9, 0.6, 0.7), "head", M_SKIN, 16, 8)

# --- legs (stubby, angled out) and tail ---
LEGS = {"leg_FL": (5.0, -5.5), "leg_FR": (-5.0, -5.5), "leg_BL": (5.0, 5.5), "leg_BR": (-5.0, 5.5)}
for bone, (x, y) in LEGS.items():
    top = (x, y, 3.6); foot = (x * 1.22, y * 1.08, 0.9)
    cyl(bone + "_mesh", top, foot, 1.55, bone, M_SKIN)
    sphere(bone + "_foot", (foot[0], foot[1] - 0.4, 0.9), (1.7, 1.9, 0.9), bone, M_SKIN, 16, 8)
cyl("Tail", (0, 8.6, 3.4), (0, 10.6, 2.6), 0.6, "tail", M_SKIN, 10)

# --- armature ---
arm_data = bpy.data.armatures.new("Valentino_Skeleton")
arm = bpy.data.objects.new("Armature", arm_data)
sc.collection.objects.link(arm)
bpy.context.view_layer.objects.active = arm
bpy.ops.object.mode_set(mode='EDIT')
eb = arm_data.edit_bones
def bone(name, h, t, parent=None):
    b = eb.new(name); b.head = h; b.tail = t
    if parent: b.parent = eb[parent]
    return b
bone("root", (0, 0, 0), (0, 0, 2))
bone("body", (0, 0, 3.5), (0, 0, 7), "root")
bone("neck", (0, -7.0, 4.2), (0, -10.0, 4.6), "body")
bone("head", (0, -10.0, 4.6), (0, -13.5, 4.8), "neck")
for name, (x, y) in LEGS.items():
    bone(name, (x, y, 3.6), (x * 1.22, y * 1.08, 0.9), "body")
bone("tail", (0, 8.6, 3.4), (0, 10.6, 2.6), "body")
bpy.ops.object.mode_set(mode='OBJECT')

# --- rigid skin: join everything, each part 100% to its bone ---
for o, b in parts:
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    vg = o.vertex_groups.new(name=b)
    vg.add(list(range(len(o.data.vertices))), 1.0, 'REPLACE')
bpy.ops.object.select_all(action='DESELECT')
for o, _ in parts:
    o.select_set(True)
bpy.context.view_layer.objects.active = parts[0][0]
bpy.ops.object.join()
body = bpy.context.object; body.name = body.data.name = "Valentino"
body.parent = arm
mod = body.modifiers.new("Armature", 'ARMATURE'); mod.object = arm

# --- animations ---
pb = arm.pose.bones
for p in pb:
    p.rotation_mode = 'XYZ'
arm.animation_data_create()

def new_action(name, frames):
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    arm.animation_data.action = act
    for p in pb:
        p.location = (0, 0, 0); p.rotation_euler = (0, 0, 0); p.scale = (1, 1, 1)
    sc.frame_start, sc.frame_end = 0, frames
    return act

def key(p, f, loc=None, rot=None, scl=None):
    if loc is not None:
        p.location = loc; p.keyframe_insert("location", frame=f)
    if rot is not None:
        p.rotation_euler = [math.radians(a) for a in rot]; p.keyframe_insert("rotation_euler", frame=f)
    if scl is not None:
        p.scale = scl; p.keyframe_insert("scale", frame=f)

def key_all_rest(f):
    for p in pb:
        key(p, f, (0, 0, 0), (0, 0, 0), (1, 1, 1))

actions = []
# Idle: slow breathing, small head look-around (4 s loop)
a = new_action("Idle", 120); key_all_rest(0)
for f, yaw, nod in ((0, 0, 0), (30, 12, -4), (60, 0, 3), (90, -12, -2), (120, 0, 0)):
    key(pb["head"], f, rot=(nod, 0, yaw))
for f, s in ((0, 1.0), (60, 1.015), (120, 1.0)):
    key(pb["body"], f, scl=(s, 1, s))
actions.append(a)

# Walk: diagonal leg pairs, gentle body roll + head bob (1.2 s loop)
a = new_action("Walk", 36); key_all_rest(0)
for f in (0, 9, 18, 27, 36):
    ph = (f / 36.0) * 2 * math.pi
    sw = 28 * math.sin(ph)
    lift = max(0.0, math.sin(ph)) * 0.6
    key(pb["leg_FL"], f, loc=(0, 0, lift), rot=(sw, 0, 0))
    key(pb["leg_BR"], f, loc=(0, 0, lift), rot=(sw, 0, 0))
    key(pb["leg_FR"], f, loc=(0, 0, max(0.0, -math.sin(ph)) * 0.6), rot=(-sw, 0, 0))
    key(pb["leg_BL"], f, loc=(0, 0, max(0.0, -math.sin(ph)) * 0.6), rot=(-sw, 0, 0))
    key(pb["body"], f, rot=(0, 3 * math.sin(ph), 0))
    key(pb["neck"], f, rot=(4 * math.sin(2 * ph), 0, 0))
    key(pb["tail"], f, rot=(0, 0, 10 * math.sin(ph)))
actions.append(a)

# Hide: pull head, legs and tail into the shell (0.6 s, holds at the end)
a = new_action("Hide", 18); key_all_rest(0)
key(pb["neck"], 18, loc=(0, -3.2, 0), scl=(0.85, 0.85, 0.85))
key(pb["head"], 18, rot=(-10, 0, 0))
for n in LEGS:
    key(pb[n], 18, loc=(0, -1.6, 0), scl=(0.9, 0.7, 0.9))
key(pb["tail"], 18, loc=(0, -1.5, 0))
key(pb["body"], 18, loc=(0, 0, -1.4))
actions.append(a)

# Emerge: reverse of Hide
a = new_action("Emerge", 24)
for p in pb:
    key(p, 24, (0, 0, 0), (0, 0, 0), (1, 1, 1))
key(pb["neck"], 0, loc=(0, -3.2, 0), scl=(0.85, 0.85, 0.85)); key(pb["head"], 0, rot=(-10, 0, 0))
for n in LEGS:
    key(pb[n], 0, loc=(0, -1.6, 0), scl=(0.9, 0.7, 0.9))
key(pb["tail"], 0, loc=(0, -1.5, 0)); key(pb["body"], 0, loc=(0, 0, -1.4))
actions.append(a)

arm.animation_data.action = actions[0]
sc.frame_set(0)

pts = [body.matrix_world @ Vector(c) for c in body.bound_box]
print("VALENTINO size cm: L=%.1f W=%.1f H=%.1f  verts=%d" % (
    max(p.y for p in pts) - min(p.y for p in pts), max(p.x for p in pts) - min(p.x for p in pts),
    max(p.z for p in pts), len(body.data.vertices)))
print("ACTIONS:", [x.name for x in bpy.data.actions])

if PREVIEW:
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam)
    cam.location = (24, -30, 22); cam.rotation_euler = (Vector((0, 0, 4)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.clip_start = 0.5; cam.data.clip_end = 5000; sc.camera = cam
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.rotation_euler = (0.7, 0.2, 0.5); sun.data.energy = 4
    sc.collection.objects.link(sun)
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, 0)); g = bpy.context.object
    g.data.materials.append(mat("Ground", (0.25, 0.32, 0.18)))
    w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.6, 0.7, 0.85, 1); w.node_tree.nodes["Background"].inputs[1].default_value = 0.8
    for e in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
        try:
            sc.render.engine = e; break
        except Exception:
            pass
    sc.render.resolution_x, sc.render.resolution_y = 1000, 700
    sc.render.filepath = PREVIEW
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(g); bpy.data.objects.remove(cam); bpy.data.objects.remove(sun)

bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True); body.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.export_scene.fbx(
    filepath=OUT, use_selection=True, object_types={'ARMATURE', 'MESH'},
    global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
    axis_forward='-Z', axis_up='Y', add_leaf_bones=False, mesh_smooth_type='FACE',
    bake_anim=True, bake_anim_use_all_actions=True, bake_anim_use_nla_strips=False,
    bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0)
print("EXPORTED", OUT)
