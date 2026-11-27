# Play-in-editor checks for fetch. Step name from Downloads\_step.txt.
import unreal, math
step = open(r"C:\Users\calif\Documents\Finding Valentino\Downloads\_step.txt", encoding="utf-8-sig").read().strip()
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
GS = unreal.GameplayStatics
pc = GS.get_player_controller(w, 0); p = pc.get_controlled_pawn()
carry = p.get_components_by_class(unreal.FVCarryComponent)[0]
dog = [d for d in GS.get_all_actors_of_class(w, unreal.FVDog) if d.get_editor_property("show_meter")][0]
ball = [b for b in GS.get_all_actors_of_class(w, unreal.FVPickupItem) if b.get_actor_label() == "FT_Ball_Diego"][0]

def face(yaw, pitch):
    p.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), True)
    pc.set_control_rotation(unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))

def status():
    pl = p.get_actor_location(); bl = ball.get_actor_location(); dl = dog.get_actor_location()
    print(f"dog {str(dog.get_editor_property('state')).split('.')[-1][:-1]:10s} at ({dl.x:.0f},{dl.y:.0f}) energy {dog.energy:.2f} | "
          f"ball ({bl.x:.0f},{bl.y:.0f},{bl.z:.0f}) in mouth {ball.is_in_mouth() if hasattr(ball,'is_in_mouth') else '?'} "
          f"parent {ball.get_attach_parent_actor().get_actor_label() if ball.get_attach_parent_actor() else '-'} | "
          f"dog-player {math.hypot(dl.x-pl.x, dl.y-pl.y):.0f} | prompt '{carry.get_prompt()}' use '{carry.get_use_prompt()}'")

if step == "grab":
    bl = ball.get_actor_location()
    p.set_actor_location(unreal.Vector(bl.x - 75, bl.y, bl.z + 90), False, True)
    face(0.0, -40.0)
    print("before:", carry.get_prompt()); carry.interact(); status()
elif step.startswith("throw"):
    yaw = float(step.split(":")[1]) if ":" in step else 0.0
    face(yaw, 5.0)
    carry.use_started(); carry.use_stopped(); status()
elif step == "status":
    status()
elif step == "dog_near":
    dl = dog.get_actor_location()
    p.set_actor_location(unreal.Vector(dl.x - 300, dl.y, dl.z + 80), False, True)
    face(0.0, -15.0); status()
