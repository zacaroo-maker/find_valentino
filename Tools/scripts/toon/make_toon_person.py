# Finding Valentino (TOON) - a simple, chunky cartoon person ("Family Friend") made of rigid pieces that ride
# the UE mannequin's bones, so all the mannequin animations (walk, run, jump, carry) work unchanged.
# Each piece is modelled in the mannequin's reference (component) space and exported as its own static mesh
# SM_Toon_<bone>.fbx; the game attaches it to that bone (FVMetaHuman::AttachToonParts).
# Run: blender -b --factory-startup --python make_toon_person.py -- <manny_bones.json> <outdir> [preview.png]
import bpy, bmesh, sys, os, json, math
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
BONES = json.load(open(argv[0]))
OUT = argv[1]
PREVIEW = argv[2] if len(argv) > 2 else None
os.makedirs(OUT, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.scale_length = 0.01

def U(x, y, z):
    """UE component space -> Blender (the FBX export flips Y back)."""
    return Vector((x, -y, z))
def B(name):
    return U(*BONES[name])

def mat(name, rgb, rough=0.75):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough
    m.diffuse_color = (*rgb, 1)
    return m
SKIN = mat("Toon_PersonSkin", (0.93, 0.66, 0.48))
SHIRT = mat("Toon_Shirt", (0.98, 0.62, 0.12))      # sunny orange t-shirt
JEANS = mat("Toon_Jeans", (0.16, 0.30, 0.62))
SHOE = mat("Toon_Shoe", (0.95, 0.95, 0.93))
SOLE = mat("Toon_ShoeRed", (0.85, 0.15, 0.15))
HAIR = mat("Toon_Hair", (0.32, 0.18, 0.08))
WHITE = mat("Toon_EyeWhite", (0.97, 0.97, 0.95), 0.3)
BLACK = mat("Toon_Pupil", (0.02, 0.02, 0.02), 0.25)
BLUSH = mat("Toon_Blush", (1.0, 0.45, 0.45))
TURTLE = mat("Toon_ShirtTurtle", (0.35, 0.62, 0.22))

pieces = {}   # bone -> [objects]
def keep(o, bone, m):
    o.data.materials.append(m)
    for p in o.data.polygons:
        p.use_smooth = True
    pieces.setdefault(bone, []).append(o)
    return o

def ell(bone, c, r, m, rot=None, seg=32, rings=16):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=rings, location=c)
    o = bpy.context.object; o.scale = r
    if rot is not None:
        o.rotation_euler = rot
    return keep(o, bone, m)

def along(a, b):
    return (b - a).to_track_quat('Z', 'Y').to_euler()

def capsule(bone, a, b, ra, m, rb=None, seg=24):
    """Rounded tube from a to b (radius ra at a, rb at b)."""
    rb = ra if rb is None else rb
    d = b - a
    bpy.ops.mesh.primitive_cone_add(vertices=seg, radius1=ra, radius2=rb, depth=d.length, location=(a + b) / 2)
    o = bpy.context.object; o.rotation_euler = along(a, b); keep(o, bone, m)
    ell(bone, a, (ra, ra, ra), m, seg=seg, rings=seg // 2)
    ell(bone, b, (rb, rb, rb), m, seg=seg, rings=seg // 2)

# ---------------- body ----------------
ell("pelvis", U(0, 2, 92), (19, 14, 12), JEANS)                                   # hips / shorts top
ell("spine_02", U(0, 3, 111), (20.5, 15, 18), SHIRT)                                # belly
ell("spine_05", U(0, 2, 130), (21, 14.5, 18), SHIRT)                                  # chest + shoulders
# little green tortoise on the shirt
ell("spine_05", U(0, 16.0, 129), (6.0, 1.6, 4.6), TURTLE)
ell("spine_05", U(0, 15.4, 135.5), (2.0, 1.4, 1.8), TURTLE)
for tx, tz in ((-4.5, 125.5), (4.5, 125.5), (-4.5, 132), (4.5, 132)):
    ell("spine_05", U(tx, 15.0, tz), (1.4, 1.2, 1.2), TURTLE, seg=12, rings=6)
capsule("neck_01", B("neck_01") - Vector((0, 0, 3)), B("neck_02"), 7.5, SKIN)

# ---------------- head (big, round) ----------------
HC, HR = U(0, 3, 173), Vector((21, 19.5, 20))      # HR in Blender axes (x, y, z)
ell("head", HC, HR, SKIN, seg=48, rings=24)
def face(dx, dy_fwd, dz, out=0.0):
    """Point on the head surface in UE-ish directions (dy_fwd > 0 = face)."""
    d = Vector((dx, -dy_fwd, dz)).normalized()
    k = 1.0 / math.sqrt((d.x / HR.x) ** 2 + (d.y / HR.y) ** 2 + (d.z / HR.z) ** 2)
    return HC + d * (k + out), d
# hair: a cap over the top and back, plus a little swoop of fringe
bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, location=(0, 0, 0))
cap = bpy.context.object
bm = bmesh.new(); bm.from_mesh(cap.data)
bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z < -0.05 and v.co.y < 0.45], context='VERTS')   # keep top + back
bm.to_mesh(cap.data); bm.free()
cap.scale = HR * 1.06; cap.location = HC + Vector((0, 0.6, 0.8)); keep(cap, "head", HAIR)
p, d = face(0.25, 0.75, 0.62, -1.0)
ell("head", p, (10, 4.5, 4.0), HAIR, rot=(0.5, 0.25, 0.3))
for sx in (-1, 1):
    # big shiny eyes
    p, d = face(sx * 0.36, 0.9, 0.12)
    ell("head", p - d * 1.4, (4.0, 4.0, 4.8), WHITE, seg=24, rings=12)
    ell("head", p + d * 1.6, (2.4, 2.4, 2.9), BLACK, seg=20, rings=10)
    ell("head", p + d * 3.7 + Vector((0, 0, 1.1)), (0.75, 0.75, 0.75), WHITE, seg=10, rings=5)
    # eyebrows
    q, dq = face(sx * 0.38, 0.85, 0.42)
    ell("head", q, (2.6, 0.8, 0.7), HAIR, rot=(0, -sx * 0.15, 0) if False else None, seg=12, rings=6)
    # rosy cheeks
    c, dc = face(sx * 0.6, 0.75, -0.18)
    ell("head", c, (3.2, 3.2, 0.8), BLUSH, rot=dc.to_track_quat('Z', 'Y').to_euler(), seg=16, rings=8)
    # ears
    e, de = face(sx * 1.0, 0.0, 0.0, -0.6)
    ell("head", e, (2.0, 3.2, 4.2), SKIN, seg=16, rings=8)
# button nose and smile
n, dn = face(0, 1.0, -0.12)
ell("head", n, (2.0, 2.0, 1.7), SKIN, seg=16, rings=8)
m_, dm = face(0, 1.0, -0.42)
bpy.ops.mesh.primitive_torus_add(major_radius=4.2, minor_radius=0.55, major_segments=32, minor_segments=8)
sm = bpy.context.object
bm = bmesh.new(); bm.from_mesh(sm.data)
bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.y > -1.2], context='VERTS')
bm.to_mesh(sm.data); bm.free()
sm.rotation_euler = (math.radians(90), 0, 0); sm.location = m_ + Vector((0, -0.2, 3.2))
keep(sm, "head", BLACK)

# ---------------- arms ----------------
for s in ("l", "r"):
    sh, el, wr, mid = B("upperarm_" + s), B("lowerarm_" + s), B("hand_" + s), B("middle_01_" + s)
    capsule("upperarm_" + s, sh, el, 7.8, SHIRT, 6.6)                 # sleeve
    capsule("lowerarm_" + s, el, wr, 5.6, SKIN, 5.0)                   # forearm
    hc = wr + (mid - wr) * 0.8
    ell("hand_" + s, hc, (5.6, 4.2, 7.0), SKIN, rot=along(wr, mid))   # mitten hand
    side = 1 if s == "l" else -1
    ell("hand_" + s, wr + (mid - wr) * 0.45 + Vector((0, -3.0, 0.5)), (1.7, 1.7, 2.8), SKIN, seg=12, rings=6)   # thumb

# ---------------- legs ----------------
for s in ("l", "r"):
    hip, knee, ank, ball = B("thigh_" + s), B("calf_" + s), B("foot_" + s), B("ball_" + s)
    capsule("thigh_" + s, hip + Vector((0, 0, -2)), knee, 10.8, JEANS, 9.0)
    capsule("calf_" + s, knee, ank + Vector((0, 0, 4)), 8.6, JEANS, 7.8)
    # chunky sneaker: white upper, red sole
    toe = ball + (ball - ank).normalized() * 2.0
    c = (ank + toe) / 2
    ell("foot_" + s, Vector((c.x, c.y, 6.5)), (8.2, 14, 7.0), SHOE, rot=(0, 0, math.atan2(-(toe - ank).x, -(toe - ank).y)))
    ell("foot_" + s, Vector((c.x, c.y, 2.0)), (8.6, 14.6, 2.6), SOLE, rot=(0, 0, math.atan2(-(toe - ank).x, -(toe - ank).y)))

# ---------------- join per bone, export ----------------
joined = {}
for bone, objs in pieces.items():
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.ops.object.join()
    o = bpy.context.object
    # keep vertices in component space: move the object origin to the world origin
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    o.name = o.data.name = "SM_Toon_" + bone
    joined[bone] = o
for bone, o in joined.items():
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, o.name + ".fbx"), use_selection=True, object_types={'MESH'},
                             global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
                             axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', bake_anim=False)
print("PARTS:", sorted(joined))
lo = min(min((o.matrix_world @ Vector(c)).z for c in o.bound_box) for o in joined.values())
hi = max(max((o.matrix_world @ Vector(c)).z for c in o.bound_box) for o in joined.values())
print("HEIGHT cm: %.1f .. %.1f" % (lo, hi))

if PREVIEW:
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam)
    cam.location = (120, -330, 140); cam.rotation_euler = (Vector((0, 0, 105)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = 50; cam.data.clip_start = 1; cam.data.clip_end = 5000; sc.camera = cam
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.rotation_euler = (0.8, 0.2, 0.4); sun.data.energy = 4
    sc.collection.objects.link(sun)
    bpy.ops.mesh.primitive_plane_add(size=600); g = bpy.context.object; g.data.materials.append(mat("Ground", (0.3, 0.42, 0.2)))
    w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.6, 0.72, 0.9, 1)
    for e in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
        try:
            sc.render.engine = e; break
        except Exception:
            pass
    sc.render.resolution_x, sc.render.resolution_y = 800, 1000; sc.render.filepath = PREVIEW
    bpy.ops.render.render(write_still=True)
