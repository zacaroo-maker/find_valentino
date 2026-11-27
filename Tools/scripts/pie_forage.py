# Play-in-editor checks for foraging. Step name from Downloads\_step.txt.
import unreal, math
step = open(r"C:\Users\calif\Documents\Finding Valentino\Downloads\_step.txt", encoding="utf-8-sig").read().strip()
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
GS = unreal.GameplayStatics
pc = GS.get_player_controller(w, 0); p = pc.get_controlled_pawn()
carry = p.get_components_by_class(unreal.FVCarryComponent)[0]
val = GS.get_all_actors_of_class(w, unreal.FVValentino)[0]
plants = {a.get_actor_label(): a for a in GS.get_all_actors_of_class(w, unreal.FVForagePlant)}

def stand_facing(t, dist=90.0, from_dir=None):
    d = from_dir or unreal.Vector(-1, 0, 0)
    pos = unreal.Vector(t.x + d.x * dist, t.y + d.y * dist, max(t.z, 0) + 95.0)
    p.set_actor_location(pos, False, True)
    yaw = math.degrees(math.atan2(t.y - pos.y, t.x - pos.x))
    p.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), True)
    pitch = math.degrees(math.atan2(t.z - (pos.z + 60.0), max(dist, 1.0)))
    pc.set_control_rotation(unreal.Rotator(roll=0.0, pitch=max(pitch, -75.0), yaw=yaw))

def status():
    print("prompt:", carry.get_prompt(), "| toast:", carry.get_toast())
    print("valentino: state", val.get_editor_property("state"), "craving", val.get_editor_property("has_craving"),
          val.get_editor_property("craving"), "| eaten", val.get_editor_property("times_eaten"))

if step.startswith("look:"):
    name, dist, dx, dy = step[5:].split(",")
    a = plants[name]
    stand_facing(a.get_actor_location() + unreal.Vector(0, 0, 20), float(dist), unreal.Vector(float(dx), float(dy), 0))
    print("looking at", name, "ripe", a.get_editor_property("produce_mesh").is_visible())
elif step.startswith("pick:"):
    a = plants[step[5:]]
    stand_facing(a.get_actor_location() + unreal.Vector(0, 0, 20), 85)
    print("before:", carry.get_prompt())
    carry.interact()
    print("after:", carry.get_prompt(), "| toast:", carry.get_toast(), "| ripe now:", a.get_editor_property("produce_mesh").is_visible())
elif step == "find":
    l = val.get_actor_location()
    stand_facing(l, 150)
    print("near valentino at", l)
elif step == "feed":
    stand_facing(val.get_actor_location(), 70)
    print("before:", carry.get_prompt()); carry.interact(); status()
elif step == "status":
    status()




elif step == "watch":
    stand_facing(val.get_actor_location(), 320, unreal.Vector(0.5, 0.85, 0))
    status()

