# Finding Valentino - foraging: Valentino's real favourites (romaine, chicory, flowers, berries, tomatoes).
# Food items you carry, plus the plants they grow on. Each plant is two meshes with the same origin:
#   SM_FV_Plant_<Name>          the plant itself (stays)
#   SM_FV_Plant_<Name>_Produce  the pickable part (hidden when picked, grows back)
# Static meshes, cm, origin at bottom-centre.
# Run: blender -b --factory-startup --python make_forage.py -- <out_dir> [preview.png]
import bpy, sys, os, math, random
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]; PREVIEW = argv[1] if len(argv) > 1 else None
os.makedirs(OUT, exist_ok=True)
random.seed(11)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.scale_length = 0.01

def reset():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.meshes):
        bpy.data.meshes.remove(m)

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

def blob(r, loc, scale, m, noise=0.0, sub=2, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=sub, radius=r, location=loc)
    o = bpy.context.object; o.scale = scale; o.rotation_euler = rot
    for v in o.data.vertices:
        v.co *= 1 + random.uniform(-noise, noise)
    return assign(o, m)

def cyl(r1, r2, a, b, m, verts=10):
    a, b = Vector(a), Vector(b); d = b - a
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r1, radius2=r2, depth=d.length, location=(a + b) / 2)
    o = bpy.context.object; o.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    return assign(o, m)

def leaf(base, ang, tilt, length, width, m, noise=0.04, thick=0.25):
    """A flat leaf starting at base, pointing at compass angle ang, tilted up by tilt (radians)."""
    d = Vector((math.cos(ang) * math.cos(tilt), math.sin(ang) * math.cos(tilt), math.sin(tilt)))
    c = Vector(base) + d * (length / 2)
    return blob(1, c, (length / 2, width / 2, thick), m, noise, 2, (0, -tilt, ang))

def export(name, objs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if len(objs) > 1:
        bpy.ops.object.join()
    o = bpy.context.object; o.name = o.data.name = name
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True, object_types={'MESH'},
                             global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
                             axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', bake_anim=False)
    d = o.dimensions
    print(f"ITEM {name}: {d.x:.0f} x {d.y:.0f} x {d.z:.0f} cm, mats={[m.name for m in o.data.materials]}")
    return o

GREEN = mat("Leaf_Green", (0.22, 0.55, 0.12), 0.5); LIGHT = mat("Leaf_Light", (0.55, 0.78, 0.35), 0.5)
DARK = mat("Leaf_Dark", (0.1, 0.32, 0.08), 0.55); ROMAINE = mat("Leaf_Romaine", (0.32, 0.62, 0.18), 0.45)
YELLOW = mat("Petal_Yellow", (0.95, 0.75, 0.05), 0.6); RED = mat("Berry_Red", (0.75, 0.05, 0.05), 0.25)
SEED = mat("Berry_Seed", (0.9, 0.8, 0.3), 0.5); TOMATO = mat("Tomato_Red", (0.85, 0.12, 0.04), 0.2)
BLUE = mat("Petal_Chicory", (0.35, 0.5, 0.95), 0.55); HIBISCUS = mat("Petal_Hibiscus", (0.95, 0.2, 0.35), 0.45)
STAMEN = mat("Stamen", (1.0, 0.85, 0.3), 0.5); WOOD = mat("Stake_Wood", (0.42, 0.3, 0.18), 0.85)
SOIL = mat("Soil_Dark", (0.16, 0.11, 0.07), 0.95)

# ---------------- parts used by both the carried food and the plants ----------------
def tomato(c, r, objs):
    objs.append(blob(r, c, (1, 1, 0.85), TOMATO, 0.03, 3))
    for i in range(5):  # little green star on top
        a = i / 5 * 2 * math.pi
        objs.append(leaf((c[0], c[1], c[2] + r * 0.8), a, -0.3, r * 0.8, r * 0.3, GREEN, 0, 0.12))

def hibiscus(c, r, up, objs, face=0.0):
    """Open flower at c, tipped 'up' radians from facing the sky, leaning towards compass angle 'face'."""
    from mathutils import Matrix
    flower = []
    for i in range(5):
        a = i / 5 * 2 * math.pi
        flower.append(leaf((0, 0, 0), a, 0.35, r, r * 0.8, HIBISCUS, 0.06, 0.12))
    flower.append(cyl(0.25, 0.15, (0, 0, 0), (0, 0, r * 0.9), STAMEN, 6))
    flower.append(blob(r * 0.12, (0, 0, r * 0.9), (1, 1, 1), STAMEN, 0, 1))
    bpy.context.view_layer.update()
    M = Matrix.Translation(Vector(c)) @ Matrix.Rotation(face, 4, 'Z') @ Matrix.Rotation(up, 4, 'Y')
    for o in flower:
        o.matrix_world = M @ o.matrix_world
    objs.extend(flower)

def chicory_flower(c, r, objs):
    for i in range(10):
        a = i / 10 * 2 * math.pi
        objs.append(leaf(c, a, 0.15, r, r * 0.32, BLUE, 0, 0.1))
    objs.append(blob(r * 0.18, c, (1, 1, 0.6), mat("Chicory_Centre", (0.25, 0.35, 0.8), 0.5), 0, 1))

def dandelion_head(c, r, objs):
    objs.append(blob(r, c, (1, 1, 0.55), YELLOW, 0.12, 2))

def strawberry(c, s, objs):
    reset_b = []
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=10, radius=1, location=(0, 0, 0))
    b = bpy.context.object
    for v in b.data.vertices:      # taper down to the tip (hanging berry, tip at -Z)
        t = (1 - v.co.z) / 2
        k = 1.0 - 0.5 * t
        v.co.x *= k; v.co.y *= k
    b.scale = (2.3 * s, 2.3 * s, 2.8 * s); b.location = c; assign(b, RED); objs.append(b)
    for i in range(5):
        a = i / 5 * 2 * math.pi
        objs.append(leaf((c[0], c[1], c[2] + 2.6 * s), a, 0.1, 1.6 * s, 0.7 * s, GREEN, 0, 0.12))

# ---------------- carried food items ----------------
# romaine leaf: long, crinkly, pale rib
reset()
bpy.ops.mesh.primitive_circle_add(vertices=40, radius=1, fill_type='TRIFAN')
lf = bpy.context.object
for v in lf.data.vertices:
    a = math.atan2(v.co.y, v.co.x); r = v.co.length
    v.co.x *= 10.0; v.co.y *= 4.2
    v.co.z = 1.0 * r * r + 0.5 * r * math.sin(a * 9)
assign(lf, ROMAINE)
mod = lf.modifiers.new("s", 'SOLIDIFY'); mod.thickness = 0.4
bpy.context.view_layer.objects.active = lf; bpy.ops.object.modifier_apply(modifier="s")
lf.location.z = 0.4
rib = cyl(0.5, 0.2, (-9.5, 0, 0.7), (9.0, 0, 1.4), LIGHT, 6)
export("SM_FV_Food_Romaine", [lf, rib])

reset(); objs = []
tomato((0, 0, 2.6), 3.0, objs)
export("SM_FV_Food_Tomato", objs)

# chicory: a jagged leaf and a sprig with a blue flower, lying on the ground
reset(); objs = []
objs.append(leaf((-7, 0, 0.4), 0, 0.03, 13, 3.2, GREEN, 0.12, 0.25))
objs.append(cyl(0.25, 0.2, (-6, 0.5, 0.6), (5, 1.5, 1.2), LIGHT, 6))
chicory_flower((5.5, 1.6, 1.4), 2.4, objs)
export("SM_FV_Food_Chicory", objs)

reset(); objs = []
hibiscus((0, 0, 0.6), 5.5, 0.0, objs)
export("SM_FV_Food_Hibiscus", objs)

# ---------------- plants (base + produce) ----------------
def rosette(n, length, width, tilt, m, objs, noise=0.08, z=0.5, jitter=0.25):
    for i in range(n):
        a = i / n * 2 * math.pi + random.uniform(-jitter, jitter)
        objs.append(leaf((0, 0, z), a, tilt + random.uniform(-0.1, 0.1), length * random.uniform(0.85, 1.1), width, m, noise))

# romaine: splayed outer leaves on a little soil mound; the produce is the tall upright heart
reset(); objs = []
objs.append(blob(1, (0, 0, 0), (16, 16, 3), SOIL, 0.1, 2))
rosette(7, 22, 9, 0.25, ROMAINE, objs, 0.05)
export("SM_FV_Plant_Romaine", objs)
reset(); objs = []
for i in range(8):
    a = i / 8 * 2 * math.pi
    objs.append(leaf((math.cos(a) * 1.5, math.sin(a) * 1.5, 1), a, 1.25, 26, 8, ROMAINE if i % 2 else LIGHT, 0.04, 0.3))
export("SM_FV_Plant_Romaine_Produce", objs)

# tomato: a staked vine; the produce is the ripe fruit
reset(); objs = []
objs.append(cyl(1.2, 1.0, (0, 0, 0), (0, 0, 95), WOOD, 6))
pts = [Vector((math.sin(z * 0.08) * 6, math.cos(z * 0.06) * 4, z)) for z in range(0, 90, 6)]
for p, q in zip(pts, pts[1:]):
    objs.append(cyl(0.8, 0.7, p, q, GREEN, 6))
for k, p in enumerate(pts[1:]):
    for s in (-1, 1):
        a = random.uniform(0, 2 * math.pi)
        objs.append(leaf(p, a, random.uniform(-0.2, 0.3), random.uniform(13, 17), 7, DARK if k % 2 else GREEN, 0.1, 0.25))
export("SM_FV_Plant_Tomato", objs)
reset(); objs = []
for (x, y, z, r) in ((7, 3, 32, 3.4), (5, -6, 38, 3.0), (-6, 5, 47, 3.3), (-4, -5, 58, 2.9), (7, 1, 66, 3.1)):
    objs.append(cyl(0.3, 0.3, (x * 0.4, y * 0.4, z + r + 3), (x, y, z + r * 0.8), GREEN, 5))
    tomato((x, y, z), r, objs)
export("SM_FV_Plant_Tomato_Produce", objs)

# dandelion: flat rosette of toothy leaves; the produce is the flowers on their stems
reset(); objs = []
rosette(9, 13, 3.4, 0.08, GREEN, objs, 0.14)
export("SM_FV_Plant_Dandelion", objs)
reset(); objs = []
for (a, h) in ((0.3, 14), (2.4, 11), (4.3, 16)):
    top = (math.cos(a) * 3, math.sin(a) * 3, h)
    objs.append(cyl(0.3, 0.25, (0, 0, 0.5), top, LIGHT, 6))
    dandelion_head((top[0], top[1], top[2] + 0.6), 2.4, objs)
export("SM_FV_Plant_Dandelion_Produce", objs)

# strawberry: low clumps of three-part leaves; the produce is the berries hanging near the ground
reset(); objs = []
for i in range(7):
    a = i / 7 * 2 * math.pi + random.uniform(-0.2, 0.2)
    r = random.uniform(6, 11); h = random.uniform(9, 14)
    top = (math.cos(a) * r, math.sin(a) * r, h)
    objs.append(cyl(0.3, 0.25, (0, 0, 0), top, LIGHT, 5))
    for k in (-0.7, 0, 0.7):
        objs.append(leaf(top, a + k, 0.15, 6.5, 4.8, GREEN, 0.06, 0.2))
export("SM_FV_Plant_Strawberry", objs)
reset(); objs = []
for i, a in enumerate((0.7, 2.2, 3.6, 5.2)):
    c = (math.cos(a) * 8, math.sin(a) * 8, 3.2)
    objs.append(cyl(0.2, 0.2, (math.cos(a) * 4, math.sin(a) * 4, 8), (c[0], c[1], c[2] + 3), LIGHT, 5))
    strawberry(c, 1.0, objs)
export("SM_FV_Plant_Strawberry_Produce", objs)

# chicory: rosette of jagged leaves and two tall wiry stems; the produce is the sky-blue flowers
reset(); objs = []
rosette(8, 16, 3.6, 0.15, GREEN, objs, 0.16)
stems = []
for (dx, dy, h) in ((2, 1, 62), (-2, -1.5, 54)):
    p0 = Vector((0, 0, 1)); p1 = Vector((dx * 2, dy * 2, h * 0.55)); p2 = Vector((dx * 4, dy * 3, h))
    objs.append(cyl(0.5, 0.4, p0, p1, LIGHT, 6)); objs.append(cyl(0.4, 0.3, p1, p2, LIGHT, 6))
    stems.append((p0, p1, p2))
    for t in (0.3, 0.6):
        q = p0.lerp(p1, t)
        objs.append(leaf(q, random.uniform(0, 6.28), 0.3, 6, 1.4, GREEN, 0.1, 0.15))
export("SM_FV_Plant_Chicory", objs)
reset(); objs = []
for (p0, p1, p2) in stems:
    for t in (0.55, 0.8, 1.0):
        q = p1.lerp(p2, (t - 0.5) * 2) if t > 0.5 else p0.lerp(p1, t * 2)
        chicory_flower((q.x + 0.8, q.y, q.z), 3.6, objs)
export("SM_FV_Plant_Chicory_Produce", objs)

# hibiscus: a leafy shrub; the produce is the big open flowers on its surface
reset(); objs = []
for i in range(5):
    a = i / 5 * 2 * math.pi
    objs.append(cyl(1.6, 0.8, (0, 0, 0), (math.cos(a) * 14, math.sin(a) * 14, 55), WOOD, 6))
for i in range(16):
    a = random.uniform(0, 2 * math.pi); h = random.uniform(30, 90); r = 30 * math.sin(math.pi * h / 105) + 6
    objs.append(blob(1, (math.cos(a) * r * 0.55, math.sin(a) * r * 0.55, h), (random.uniform(17, 24),) * 2 + (random.uniform(14, 18),),
                     DARK if i % 3 else GREEN, 0.18, 2))
export("SM_FV_Plant_Hibiscus", objs)
reset(); objs = []
for (a, h) in ((0.2, 70), (1.6, 55), (2.9, 82), (4.1, 62), (5.3, 76)):
    r = (30 * math.sin(math.pi * h / 105) + 6) * 0.55 + 15
    hibiscus((math.cos(a) * r, math.sin(a) * r, h), 7.0, 0.9, objs, face=a)  # facing outwards
export("SM_FV_Plant_Hibiscus_Produce", objs)

# ---------------- preview ----------------
if PREVIEW:
    reset(); x = 0
    names = ["SM_FV_Food_Romaine", "SM_FV_Food_Tomato", "SM_FV_Food_Chicory", "SM_FV_Food_Hibiscus"]
    for n in ("Romaine", "Tomato", "Dandelion", "Strawberry", "Chicory", "Hibiscus"):
        names += [f"SM_FV_Plant_{n}", f"SM_FV_Plant_{n}_Produce"]
    for n in names:
        bpy.ops.import_scene.fbx(filepath=os.path.join(OUT, n + ".fbx"))
        o = bpy.context.selected_objects[0]
        s = 3.0 if "Food" in n else 1.0
        o.scale = (s, s, s)
        if n.endswith("_Produce"):
            o.location.x = x_prev   # same spot as its plant
        else:
            w = max(o.dimensions.x * s, 30); o.location.x = x + w / 2; x_prev = o.location.x; x += w + 30
    sc = bpy.context.scene
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam)
    cam.data.type = 'ORTHO'; cam.data.ortho_scale = x * 1.05
    cam.location = (x / 2, -900, 260); cam.rotation_euler = (math.radians(78), 0, 0); cam.data.clip_end = 10000; sc.camera = cam
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.rotation_euler = (0.8, 0.1, 0.4); sun.data.energy = 4
    sc.collection.objects.link(sun)
    w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.8, 0.85, 0.9, 1)
    for e in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
        try:
            sc.render.engine = e; break
        except Exception:
            pass
    sc.render.resolution_x, sc.render.resolution_y = 2400, 500; sc.render.filepath = PREVIEW
    bpy.ops.render.render(write_still=True)

