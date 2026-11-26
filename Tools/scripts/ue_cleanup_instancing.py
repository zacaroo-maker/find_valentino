# Clean-up + performance: delete the hidden old foliage actors, and merge the many visible non-colliding
# plant actors into one instanced actor per mesh. Collision/gameplay actors are left alone.
import unreal
from collections import defaultdict
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary

before = len(eas.get_all_level_actors())
deleted = 0
groups = defaultdict(list)
for a in eas.get_all_level_actors():
    if type(a) is not unreal.StaticMeshActor:
        continue
    lab = a.get_actor_label()
    c = a.static_mesh_component
    if lab.startswith("AP_TrunkCollider"):
        continue                                   # invisible on purpose (tree trunk collision)
    if not c.is_visible():
        eas.destroy_actor(a); deleted += 1         # hidden leftovers from earlier art passes
        continue
    f = str(a.get_folder_path())
    plant = lab.startswith("AP_") or f.startswith("Garden/Flowers") or f.startswith("Garden/ArtPass")
    if not plant or c.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION or not c.static_mesh:
        continue
    if any(c.get_material(i) != c.static_mesh.get_material(i) for i in range(c.get_num_materials())):
        continue                                   # has a material override: keep as its own actor
    groups[c.static_mesh.get_path_name()].append(a)

merged = 0
for mesh_path, actors in groups.items():
    if len(actors) < 3:
        continue
    holder = eas.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    mesh = actors[0].static_mesh_component.static_mesh
    holder.set_actor_label("IN_" + mesh.get_name()); holder.set_folder_path("Garden/Instanced")
    root = sds.k2_gather_subobject_data_for_instance(holder)[0]
    h, _ = sds.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent))
    comp = SDL.get_object(SDL.get_data(h))
    comp.set_static_mesh(mesh)
    comp.set_collision_profile_name("NoCollision")
    comp.set_editor_property("can_ever_affect_navigation", False)
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    for a in actors:
        comp.add_instance(a.get_actor_transform(), world_space=True)
        eas.destroy_actor(a); merged += 1

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
after = len(eas.get_all_level_actors())
print(f"deleted hidden: {deleted} | merged into instances: {merged} ({len([g for g in groups.values() if len(g) >= 3])} instanced actors) | actors {before} -> {after}")
