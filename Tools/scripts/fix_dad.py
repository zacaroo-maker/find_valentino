import bpy, sys, mathutils

args = sys.argv[sys.argv.index("--") + 1:]
src, dst = args[0], args[1]
TARGET_HEIGHT = 1.85

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=src)

arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
fallback = arm.data.bones[0].name  # 'root'

def owning_bone(o):
    p = o
    while p is not None:
        if p.parent == arm:
            return p.parent_bone if (p.parent_type == 'BONE' and p.parent_bone) else fallback
        p = p.parent
    return fallback

# 1) rigid skin: every vertex of a piece 100% weighted to its bone
assign = {}
for m in meshes:
    assign[m.name] = owning_bone(m)
for m in meshes:
    bone = assign[m.name]
    mw = m.matrix_world.copy()
    m.parent = None
    m.matrix_world = mw
    for md in list(m.modifiers):
        if md.type == 'ARMATURE':
            m.modifiers.remove(md)
    m.vertex_groups.clear()
    vg = m.vertex_groups.new(name=bone)
    vg.add(list(range(len(m.data.vertices))), 1.0, 'REPLACE')

# apply transforms on loose meshes, then join into one
bpy.ops.object.select_all(action='DESELECT')
for m in meshes:
    m.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
bpy.ops.object.join()
body = bpy.context.view_layer.objects.active
body.name = body.data.name = "Dad"

# 2) apply armature transform so the rig is clean at identity
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# 3) scale to target height and put feet on the ground (scale about origin, then shift)
pts = [mathutils.Vector(c) for c in body.bound_box]
zmin, zmax = min(p.z for p in pts), max(p.z for p in pts)
# Work in centimetres (Blender unit scale 0.01) so Unreal gets cm directly and the
# root bone carries no x100 scale (which breaks animation/retargeting).
bpy.context.scene.unit_settings.scale_length = 0.01
s = (TARGET_HEIGHT * 100.0) / (zmax - zmin)
M = mathutils.Matrix.Translation((0, 0, -zmin * s)) @ mathutils.Matrix.Scale(s, 4)
body.data.transform(M)
bpy.ops.object.mode_set(mode='EDIT')
ebs = arm.data.edit_bones
# snapshot first, disconnect, then assign: connected bones otherwise drag each other
snap = {eb.name: (M @ eb.head.copy(), M @ eb.tail.copy(), eb.roll, eb.use_connect) for eb in ebs}
for eb in ebs:
    eb.use_connect = False
for eb in ebs:
    h, t, r, _ = snap[eb.name]
    eb.head = h; eb.tail = t; eb.roll = r
for eb in ebs:
    eb.use_connect = snap[eb.name][3]
bpy.ops.object.mode_set(mode='OBJECT')

# 4) bind mesh to armature
body.parent = arm
body.matrix_parent_inverse.identity()
mod = body.modifiers.new("Armature", 'ARMATURE')
mod.object = arm

# 4b) original script labelled sides from the viewer's view: "L_" bones sit on Dad's right (-X).
#     Swap prefixes so L_/R_ match Dad's own left/right. Rename bones and vertex groups together.
def swap(n):
    return ("R_" + n[2:]) if n.startswith("L_") else ("L_" + n[2:]) if n.startswith("R_") else n
for b in arm.data.bones:
    if b.name[:2] in ("L_", "R_"):
        b.name = "TMP_" + b.name
for vg in body.vertex_groups:
    if vg.name[:2] in ("L_", "R_") and ("TMP_" + vg.name) not in [g.name for g in body.vertex_groups]:
        vg.name = "TMP_" + vg.name
for b in arm.data.bones:
    if b.name.startswith("TMP_"):
        b.name = swap(b.name[4:])
for vg in body.vertex_groups:
    if vg.name.startswith("TMP_"):
        vg.name = swap(vg.name[4:])

# sanity report: bone heads after fix
for b in arm.data.bones:
    print(f"FIXBONE {b.name:12} head=({b.head_local.x:6.3f},{b.head_local.y:6.3f},{b.head_local.z:6.3f})")
print("FIXVG", sorted(g.name for g in body.vertex_groups))

# Unreal treats the armature object as an extra root bone unless it's named "Armature"
arm.name = "Armature"
arm.data.name = "Dad_Skeleton"

# 5) export
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True); body.select_set(True)
bpy.ops.export_scene.fbx(
    filepath=dst, use_selection=True, object_types={'ARMATURE', 'MESH'},
    global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', bake_space_transform=True,
    axis_forward='-Z', axis_up='Y', add_leaf_bones=False,
    mesh_smooth_type='FACE', use_armature_deform_only=False, bake_anim=False)

from collections import Counter
print("=== FIX REPORT ===")
print("Pieces per bone:", dict(Counter(assign.values())))
print(f"Scale factor: {s:.4f}; verts: {len(body.data.vertices)}; materials: {len(body.material_slots)}")
print("Exported:", dst)

