# Finding Valentino - Valentino's little wooden house, like the one the family built when he was little:
# single storey, flat top he can walk on, a ramp up one side, and a door with a zigzag top whose sides tilt.
# Two door versions to choose from:  A = narrower at the bottom,  B = narrower at the top.
# cm, origin bottom-centre, door faces +X.  Ramp along the +Y side (becomes -Y... see export axes; checked in UE).
# Run: blender -b --factory-startup --python make_tortoise_house.py -- <out_dir> [preview.png]
import bpy, bmesh, sys, os, math, random
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]; PREVIEW = argv[1] if len(argv) > 1 else None
os.makedirs(OUT, exist_ok=True)

L, W, H, T = 60.0, 46.0, 26.0, 2.0       # length (x), width (y), wall height, plank thickness
PLANK = 6.5                              # plank height
ROOF_Z = H + 2.0                         # top surface of the roof

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.scale_length = 0.01

def mat(name, rgb, rough=0.8):
    m = bpy.data.materials.get(name)
    if not m:
        m = bpy.data.materials.new(name); m.use_nodes = True
        b = m.node_tree.nodes.get("Principled BSDF")
        b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough
        m.diffuse_color = (*rgb, 1)
    return m
WOOD = [mat("House_Wood_A", (0.55, 0.38, 0.22)), mat("House_Wood_B", (0.47, 0.32, 0.18)), mat("House_Wood_C", (0.6, 0.43, 0.26))]
DARK = mat("House_Wood_Dark", (0.3, 0.2, 0.11))

def box(cx, cy, cz, sx, sy, sz, m, rz=0.0, ry=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=(cx, cy, cz))
    o = bpy.context.object; o.scale = (sx, sy, sz); o.rotation_euler = (0, ry, rz)
    o.data.materials.append(m)
    return o

def reset():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for me in list(bpy.data.meshes):
        bpy.data.meshes.remove(me)

def door_cutter(variant):
    """Prism through the front wall: tilted sides, zigzag top."""
    if variant == "A":
        wb, wt = 15.0, 23.0      # narrower at the bottom
    else:
        wb, wt = 23.0, 15.0      # narrower at the top
    hs = 15.0                    # height of the sides
    teeth, amp = 4, 3.0
    pts = [(-wb / 2, -1.0), (wb / 2, -1.0), (wt / 2, hs)]
    for i in range(1, teeth * 2):  # zigzag across the top, right to left
        y = wt / 2 - wt * i / (teeth * 2)
        z = hs + (amp if i % 2 else 0.0)
        pts.append((y, z))
    pts.append((-wt / 2, hs))
    bm = bmesh.new()
    front = [bm.verts.new((L / 2 - 5, y, z)) for y, z in pts]
    back = [bm.verts.new((L / 2 + 5, y, z)) for y, z in pts]
    bm.faces.new(front); bm.faces.new(list(reversed(back)))
    n = len(pts)
    for i in range(n):
        bm.faces.new((front[i], front[(i + 1) % n], back[(i + 1) % n], back[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new("cut"); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("cut", me); bpy.context.collection.objects.link(o)
    return o

def build(variant, x_off=0.0):
    random.seed(4)
    objs = []
    rows = int(math.ceil(H / PLANK))
    # side walls (long, along x) and back wall: horizontal planks with little gaps and colour changes
    for r in range(rows):
        z = r * PLANK + PLANK / 2; h = PLANK - 0.4
        for s in (-1, 1):
            objs.append(box(0, s * (W / 2 - T / 2), z, L, T, h, random.choice(WOOD)))
        objs.append(box(-L / 2 + T / 2, 0, z, T, W - 2 * T, h, random.choice(WOOD)))
    # front wall planks, then the door cut through them
    front = []
    for r in range(rows):
        z = r * PLANK + PLANK / 2
        front.append(box(L / 2 - T / 2, 0, z, T, W - 2 * T, PLANK - 0.4, random.choice(WOOD)))
    bpy.ops.object.select_all(action='DESELECT')
    for o in front:
        o.select_set(True)
    bpy.context.view_layer.objects.active = front[0]
    bpy.ops.object.join(); fw = bpy.context.object
    cut = door_cutter(variant)
    mod = fw.modifiers.new("door", 'BOOLEAN'); mod.operation = 'DIFFERENCE'; mod.object = cut; mod.solver = 'EXACT'
    bpy.context.view_layer.objects.active = fw; bpy.ops.object.modifier_apply(modifier="door")
    bpy.data.objects.remove(cut, do_unlink=True)
    objs.append(fw)
    # corner posts
    for sx in (-1, 1):
        for sy in (-1, 1):
            objs.append(box(sx * (L / 2 - 1.5), sy * (W / 2 - 1.5), H / 2, 3.2, 3.2, H, DARK))
    # flat roof of boards he can walk on, with a low lip so he doesn't tumble off (gap where the ramp arrives)
    n = 7
    for i in range(n):
        x = -L / 2 - 2 + (L + 4) * (i + 0.5) / n
        objs.append(box(x, 0, H + 1, (L + 4) / n - 0.4, W + 4, 2, random.choice(WOOD)))
    lip_h = 2.5
    objs.append(box(L / 2 + 1, 0, ROOF_Z + lip_h / 2, 2, W + 4, lip_h, DARK))          # front
    objs.append(box(-L / 2 - 1, 0, ROOF_Z + lip_h / 2, 2, W + 4, lip_h, DARK))         # back
    objs.append(box(0, -(W / 2 + 1), ROOF_Z + lip_h / 2, L + 4, 2, lip_h, DARK))       # far side
    objs.append(box(10, W / 2 + 1, ROOF_Z + lip_h / 2, L - 16, 2, lip_h, DARK))        # ramp side, open at the back end
    # ramp up the +Y side: from the ground in front of the house to the roof at the back corner
    RW = 17.0; y = W / 2 + 2 + RW / 2
    x0, z0 = L / 2 + 22, 0.0            # bottom (on the ground, past the front)
    x1, z1 = -L / 2 + 4, ROOF_Z         # top (level with the roof)
    run = x0 - x1; ang = math.atan2(z1 - z0, run); length = math.hypot(run, z1 - z0)
    cx, cz = (x0 + x1) / 2, (z0 + z1) / 2
    objs.append(box(cx, y, cz - 1, length, RW, 2, random.choice(WOOD), ry=ang))
    # little cleats across the ramp for grip
    k = 9
    for i in range(1, k):
        t = i / k
        px = x0 + (x1 - x0) * t; pz = z0 + (z1 - z0) * t
        objs.append(box(px, y, pz + 0.5, 1.2, RW - 2, 1.0, DARK, ry=ang))
    # landing at the top, joining the ramp to the roof
    objs.append(box(x1 - 1, y - 2, ROOF_Z - 1, 10, RW + 4, 2, random.choice(WOOD)))
    # legs under the ramp
    for t in (0.45, 0.8):
        px = x0 + (x1 - x0) * t; pz = z0 + (z1 - z0) * t
        objs.append(box(px, y, pz / 2 - 1, 2.5, 2.5, pz, DARK))
    for o in objs:
        o.location.x += x_off
    return objs

def export(name, objs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.join()
    o = bpy.context.object; o.name = o.data.name = name
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.cube_project(cube_size=40); bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True, object_types={'MESH'},
                             global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
                             axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', bake_anim=False)
    d = o.dimensions
    print(f"ITEM {name}: {d.x:.0f} x {d.y:.0f} x {d.z:.0f} cm")
    return o

for v in ("A", "B"):
    reset()
    export(f"SM_FV_TortoiseHouse_{v}", build(v))

if PREVIEW:
    reset()
    build("A", x_off=0)
    objs_b = build("B", x_off=0)
    for o in objs_b:
        o.location.y += 140
    # Valentino-sized stand-in (25 cm shell) on the ramp for scale
    bpy.ops.mesh.primitive_uv_sphere_add(radius=1, location=(L / 2 + 30, W / 2 + 11, 4))
    s = bpy.context.object; s.scale = (12.5, 9, 7); s.data.materials.append(mat("Shell", (0.55, 0.45, 0.2)))
    sc = bpy.context.scene
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam)
    cam.data.type = 'ORTHO'; cam.data.ortho_scale = 300
    cam.location = (260, 190, 170); cam.rotation_euler = (math.radians(62), 0, math.radians(125)); sc.camera = cam
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.rotation_euler = (0.7, 0.2, 2.2); sun.data.energy = 4
    sc.collection.objects.link(sun)
    w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.75, 0.8, 0.85, 1)
    bpy.ops.mesh.primitive_plane_add(size=1000); g = bpy.context.object; g.data.materials.append(mat("Grass", (0.25, 0.4, 0.15)))
    for e in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
        try:
            sc.render.engine = e; break
        except Exception:
            pass
    sc.render.resolution_x, sc.render.resolution_y = 1600, 900; sc.render.filepath = PREVIEW
    bpy.ops.render.render(write_still=True)
