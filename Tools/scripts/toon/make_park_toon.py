# Finding Valentino (TOON) - chunky cartoon props for the dog park: two trees, a flowering bush, a dog drinking
# fountain, an agility A-frame, weave poles and the ball launcher. Each prop is exported as its own static mesh
# SM_Park_<Name>.fbx with its pivot on the ground at its centre (Blender units = cm, UE axes: X forward, Z up).
# Run: blender -b --factory-startup --python make_park_toon.py -- <outdir>
import bpy, sys, os, math
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.scale_length = 0.01

def U(x, y, z):
    return Vector((x, -y, z))

def mat(name, rgb, rough=0.85):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough
    m.diffuse_color = (*rgb, 1)
    return m
BARK = mat("Park_Bark", (0.36, 0.2, 0.09))
LEAF = mat("Park_Leaf", (0.28, 0.66, 0.18))
LEAF2 = mat("Park_LeafLight", (0.47, 0.8, 0.24))
PINE = mat("Park_Pine", (0.12, 0.48, 0.25))
PINK = mat("Park_Flower", (1.0, 0.45, 0.65))
STONE = mat("Park_Stone", (0.62, 0.72, 0.84))
WATER = mat("Park_Water", (0.2, 0.6, 1.0), 0.15)
YELLOW = mat("Park_Yellow", (1.0, 0.8, 0.1))
BLUE = mat("Park_Blue", (0.12, 0.38, 0.95))
RED = mat("Park_Red", (0.92, 0.18, 0.15))
WHITE = mat("Park_White", (0.97, 0.97, 0.95), 0.4)
BLACK = mat("Park_Black", (0.02, 0.02, 0.02), 0.3)
ORANGE = mat("Park_Orange", (1.0, 0.5, 0.1))
GREEN = mat("Park_Green", (0.3, 0.78, 0.3))

parts = []
def add(o, m, smooth=True):
    o.data.materials.append(m)
    if smooth:
        for p in o.data.polygons: p.use_smooth = True
    parts.append(o)
    return o
def sphere(c, r, m, sz=1.0):
    bpy.ops.mesh.primitive_uv_sphere_add(radius=r, location=U(*c), segments=32, ring_count=16)
    o = bpy.context.object; o.scale.z = sz
    return add(o, m)
def cyl(c, r, h, m, rot=(0, 0, 0), smooth=True, verts=32):
    bpy.ops.mesh.primitive_cylinder_add(radius=r, depth=h, location=U(*c), rotation=rot, vertices=verts)
    return add(bpy.context.object, m, smooth)
def cone(c, r1, r2, h, m):
    bpy.ops.mesh.primitive_cone_add(radius1=r1, radius2=r2, depth=h, location=U(*c), vertices=32)
    return add(bpy.context.object, m)
def box(c, size, m, rot=(0, 0, 0), bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=U(*c), rotation=rot)
    o = bpy.context.object; o.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel > 0:
        mod = o.modifiers.new("b", 'BEVEL'); mod.width = bevel; mod.segments = 4
        bpy.ops.object.modifier_apply(modifier="b")
    return add(o, m, smooth=bevel > 0)

def export(name):
    global parts
    for o in parts: o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    o = bpy.context.object; o.name = "SM_Park_" + name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    lo = [min((o.matrix_world @ Vector(c))[i] for c in o.bound_box) for i in range(3)]
    hi = [max((o.matrix_world @ Vector(c))[i] for c in o.bound_box) for i in range(3)]
    print("%-22s size %.0f x %.0f x %.0f  z %.0f..%.0f" % (o.name, hi[0]-lo[0], hi[1]-lo[1], hi[2]-lo[2], lo[2], hi[2]))
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, o.name + ".fbx"), use_selection=True, object_types={'MESH'},
                             global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
                             axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', bake_anim=False)
    bpy.data.objects.remove(o)
    parts = []

# ---- round tree: a stubby flared trunk and a cloud of leafy balls
cone((0, 0, 95), 30, 16, 190, BARK)
cyl((20, 0, 150), 7, 60, BARK, rot=(0, math.radians(40), 0))
for c, r, m in (((0, 0, 270), 115, LEAF), ((70, 35, 225), 82, LEAF2), ((-65, -30, 232), 88, LEAF),
                ((15, -60, 300), 78, LEAF2), ((-30, 55, 310), 72, LEAF), ((40, 10, 345), 64, LEAF2)):
    sphere(c, r, m, 0.9)
export("TreeRound")

# ---- cartoon pine: three plump stacked cones
cyl((0, 0, 45), 18, 90, BARK)
for z, r, h in ((75, 120, 150), (170, 92, 130), (255, 64, 115)):
    cone((0, 0, z + h * 0.5), r, 8, h, PINE)
sphere((0, 0, 372), 10, YELLOW)
export("TreePine")

# ---- flowering bush
for c, r in (((0, 0, 42), 55), ((45, 20, 32), 42), ((-42, -15, 34), 45), ((10, -38, 30), 38)):
    sphere(c, r, LEAF, 0.85)
for i in range(9):
    a = i * 2.4
    sphere((48 * math.cos(a), 48 * math.sin(a), 55 + 18 * math.sin(i * 1.7)), 7, PINK)
export("Bush")

# ---- dog drinking fountain: a stone post with a basin on top and a low water bowl for dogs
cyl((0, 0, 4), 70, 8, STONE)
cyl((0, 0, 50), 20, 92, STONE)
cyl((0, 0, 100), 40, 16, STONE)
cyl((0, 0, 108.5), 34, 2, WATER)
cyl((0, 0, 118), 5, 20, WHITE)
sphere((0, 0, 130), 9, WATER)
cyl((55, 0, 16), 38, 14, STONE)
cyl((55, 0, 23.5), 32, 2, WATER)
for dx in (-12, 12):
    sphere((-22, dx, 70), 6, WHITE, 0.5)   # a little paw print on the post
export("Fountain")

# ---- agility A-frame: two yellow ramps with blue contact stripes and side braces
ANG = math.radians(32)
L = 170.0
for s in (1, -1):
    cx = s * (L * 0.5 * math.cos(ANG))
    cz = L * 0.5 * math.sin(ANG) + 4
    box((cx, 0, cz), (L, 95, 7), YELLOW, rot=(0, s * ANG, 0))
    for t in (0.15, 0.4, 0.65):   # fraction of the way up the ramp
        box((s * L * math.cos(ANG) * (1 - t), 0, L * math.sin(ANG) * t + 8.5), (14, 97, 2), BLUE, rot=(0, s * ANG, 0))
    box((s * L * 0.42 * math.cos(ANG), 44, 26), (6, 6, 52), RED)
    box((s * L * 0.42 * math.cos(ANG), -44, 26), (6, 6, 52), RED)
export("AFrame")

# ---- weave poles: a row of eight striped poles on a low base rail
box((0, 0, 3), (560, 14, 6), WHITE, bevel=2)
for i in range(8):
    x = -245 + i * 70
    cyl((x, 0, 55), 3.2, 100, RED if i % 2 == 0 else YELLOW)
    sphere((x, 0, 107), 5.5, WHITE)
export("WeavePoles")

# ---- ball launcher: a chunky orange box with googly eyes, a green funnel on top and a blue barrel facing +X
box((0, 0, 40), (90, 80, 80), ORANGE, bevel=10)
cone((0, 0, 100), 18, 46, 40, GREEN)                  # funnel (wide end up)
cyl((0, 0, 82), 18, 8, GREEN)
BA = math.radians(38)
bl = 70.0
bc = Vector((38, 0, 62)) + Vector((math.cos(BA), 0, math.sin(BA))) * (bl * 0.5)
cyl(tuple(bc), 15, bl, BLUE, rot=(0, math.radians(90) - BA, 0))
mouth = Vector((38, 0, 62)) + Vector((math.cos(BA), 0, math.sin(BA))) * bl
print("LAUNCHER MOUTH (UE, from pivot):", tuple(round(v, 1) for v in mouth))
for y in (-20, 20):
    sphere((46, y, 52), 13, WHITE)
    sphere((55, y, 54), 6, BLACK)
box((47, 0, 26), (4, 30, 6), BLACK, bevel=2)          # a little smile
for x in (-30, 30):
    for y in (-28, 28):
        cyl((x, y, 2), 10, 4, BLACK)
export("BallLauncher")
print("done")
