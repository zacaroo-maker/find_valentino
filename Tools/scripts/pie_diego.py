# Diego play-test steps (step name in Downloads\_step.txt)
import unreal, math
step = open(r"C:\Users\calif\Documents\Finding Valentino\Downloads\_step.txt", encoding="utf-8-sig").read().strip()
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
GS = unreal.GameplayStatics
pc = GS.get_player_controller(w, 0); p = pc.get_controlled_pawn()
carry = p.get_components_by_class(unreal.FVCarryComponent)[0]
dog = GS.get_all_actors_of_class(w, unreal.FVDog)[0]

def face(target_loc, dist, side=unreal.Vector(-1, 0, 0)):
    pos = unreal.Vector(target_loc.x + side.x * dist, target_loc.y + side.y * dist, p.get_actor_location().z)
    p.set_actor_location(pos, False, True)
    yaw = math.degrees(math.atan2(target_loc.y - pos.y, target_loc.x - pos.x))
    p.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), True)
    pitch = math.degrees(math.atan2(target_loc.z - (pos.z + 60), dist))
    pc.set_control_rotation(unreal.Rotator(roll=0.0, pitch=max(pitch, -60.0), yaw=yaw))

if step == "go_dog":
    face(dog.get_actor_location(), 110)
elif step == "info":
    print("dog state", dog.get_editor_property("state"), "energy %.3f" % dog.energy, "held", dog.is_held(), "happy", dog.is_happy(), "thought:", dog.get_thought())
    print("prompt:", carry.get_prompt(), "| pet:", carry.get_pet_prompt(), "| toast:", carry.get_toast())
elif step == "side_view":
    # look at Dad + dog from the side (third person camera)
    pc.set_control_rotation(unreal.Rotator(roll=0.0, pitch=-15.0, yaw=p.get_actor_rotation().yaw - 70))
