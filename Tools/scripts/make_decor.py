# Finding Valentino - decorations for Valentino's yard that don't exist yet: a shallow terracotta water dish.
# cm, origin bottom-centre.  Run: blender -b --factory-startup --python make_decor.py -- <out_dir>
import bpy, sys, os, math

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.scale_length = 0.01

def mat(name, rgb, rough):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough
    return m
CLAY = mat("Dish_Terracotta", (0.62, 0.3, 0.17), 0.85)
WATER = mat("Dish_Water", (0.1, 0.22, 0.26), 0.04)

# a low, wide saucer he can walk into: sloped rim 3.5 cm high, 36 cm across
R, H, T = 18.0, 3.5, 1.2
prof = [(0.0, 0.0), (R - 4, 0.0), (R, H), (R - T, H), (R - 4 - T * 0.5, T), (0.0, T)]
verts, faces = [], []
SEG = 40
for i in range(SEG):
    a = i / SEG * 2 * math.pi
    for (r, z) in prof:
        verts.append((math.cos(a) * r, math.sin(a) * r, z))
n = len(prof)
for i in range(SEG):
    j = (i + 1) % SEG
    for k in range(n - 1):
        faces.append((i * n + k, j * n + k, j * n + k + 1, i * n + k + 1))
me = bpy.data.meshes.new("dish"); me.from_pydata(verts, [], faces); me.update()
dish = bpy.data.objects.new("dish", me); bpy.context.collection.objects.link(dish)
dish.data.materials.append(CLAY)
for p in dish.data.polygons:
    p.use_smooth = True
# the water inside
bpy.ops.mesh.primitive_cylinder_add(vertices=40, radius=R - 2.2, depth=0.4, location=(0, 0, 2.6))
w = bpy.context.object; w.data.materials.append(WATER)

bpy.ops.object.select_all(action='DESELECT')
dish.select_set(True); w.select_set(True); bpy.context.view_layer.objects.active = dish
bpy.ops.object.join()
o = bpy.context.object; o.name = o.data.name = "SM_FV_WaterDish"
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, "SM_FV_WaterDish.fbx"), use_selection=True, object_types={'MESH'},
                         global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
                         axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', bake_anim=False)
print("ITEM SM_FV_WaterDish", tuple(round(x) for x in o.dimensions))
