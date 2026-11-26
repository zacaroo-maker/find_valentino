import bpy, sys
from collections import Counter
path = sys.argv[sys.argv.index("--") + 1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=path)
objs = list(bpy.context.scene.objects)
print("=== FBX REPORT ===")
print("Object types:", dict(Counter(o.type for o in objs)))
arms = [o for o in objs if o.type == 'ARMATURE']
for a in arms:
    print(f"Armature '{a.name}': {len(a.data.bones)} bones; roots:", [b.name for b in a.data.bones if not b.parent])
    print("Bones:", [b.name for b in a.data.bones])
meshes = [o for o in objs if o.type == 'MESH']
skinned = rigid = loose = 0
for m in meshes:
    has_mod = any(md.type == 'ARMATURE' for md in m.modifiers)
    if has_mod and m.vertex_groups: skinned += 1
    elif m.parent and m.parent.type == 'ARMATURE': rigid += 1
    else: loose += 1
print(f"Meshes: {len(meshes)} | skinned (armature modifier + vertex groups): {skinned} | bone/armature-parented only: {rigid} | not attached: {loose}")
print("Not attached:", [m.name for m in meshes if not any(md.type=='ARMATURE' for md in m.modifiers) and not (m.parent and m.parent.type=='ARMATURE')][:30])
print("Total verts:", sum(len(m.data.vertices) for m in meshes))
print("Materials:", sorted({s.material.name for m in meshes for s in m.material_slots if s.material}))
import mathutils
pts = [m.matrix_world @ mathutils.Vector(c) for m in meshes for c in m.bound_box]
if pts:
    mn = [min(p[i] for p in pts) for i in range(3)]; mx = [max(p[i] for p in pts) for i in range(3)]
    print("Bounds min:", [round(v,3) for v in mn], "max:", [round(v,3) for v in mx], "(Blender Z-up after import)")
print("=== END ===")
