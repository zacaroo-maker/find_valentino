# Play-in-editor test steps for the new features. Step name is read from Downloads\_step.txt.
import unreal, math
step = open(r"C:\Users\calif\Documents\Finding Valentino\Downloads\_step.txt", encoding="utf-8-sig").read().strip()
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
GS = unreal.GameplayStatics
pc = GS.get_player_controller(w, 0); p = pc.get_controlled_pawn()
carry = p.get_components_by_class(unreal.FVCarryComponent)[0]
val = GS.get_all_actors_of_class(w, unreal.FVValentino)[0]
items = GS.get_all_actors_of_class(w, unreal.FVPickupItem)

def stand_facing(target, dist=90.0, from_dir=None):
    t = target.get_actor_location() if hasattr(target, "get_actor_location") else target
    d = from_dir or unreal.Vector(-1, 0, 0)
    pos = unreal.Vector(t.x + d.x * dist, t.y + d.y * dist, p.get_actor_location().z)
    p.set_actor_location(pos, False, True)
    yaw = math.degrees(math.atan2(t.y - pos.y, t.x - pos.x))
    p.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), True)
    eye_z = pos.z + 60.0   # look at the target like a player would
    pitch = math.degrees(math.atan2(t.z - eye_z, max(dist, 1.0)))
    pc.set_control_rotation(unreal.Rotator(roll=0.0, pitch=max(pitch, -75.0), yaw=yaw))

def held():
    h = carry.get_editor_property("held_item") if False else None
    return carry.get_prompt()

def kind_label(i):
    return f"{i.get_editor_property('kind')} '{i.get_label() if hasattr(i, 'get_label') else ''}'"

if step == "status":
    foods = [i for i in items if i.get_editor_property("kind") == unreal.FVItemKind.FOOD]
    print("food items:", len(foods), [str(i.get_editor_property("food_type")).split(".")[-1].rstrip(">") for i in foods])
    print("props/can:", len(items) - len(foods))
    dog = GS.get_all_actors_of_class(w, unreal.FVDog)
    if dog:
        print("dog:", dog[0].get_editor_property("state"), dog[0].get_actor_location())
    print("valentino:", val.get_editor_property("state"), "energy %.2f thirst %.2f" % (val.energy, val.thirst), val.get_actor_location())
    print("prompt:", carry.get_prompt(), "| use:", carry.get_use_prompt(), "| toast:", carry.get_toast())
elif step == "grab_can":
    can = [i for i in items if i.get_editor_property("kind") == unreal.FVItemKind.WATERING_CAN][0]
    stand_facing(can, 80)
    print("before:", carry.get_prompt()); carry.interact(); print("after:", carry.get_prompt(), "|", carry.get_use_prompt())
elif step == "to_bed":
    beds = GS.get_all_actors_of_class(w, unreal.FVPlantBed)
    bed = [b for b in beds if "wildflower" in str(b.get_editor_property("display_name"))][0]
    stand_facing(bed, 110)
    carry.use_started(); print("pouring started:", carry.get_use_prompt())
elif step == "stop_pour":
    carry.use_stopped()
    for b in GS.get_all_actors_of_class(w, unreal.FVPlantBed):
        pass
    print("toast:", carry.get_toast(), "| use:", carry.get_use_prompt())
elif step == "to_pond":
    pond = GS.get_all_actors_of_class(w, unreal.FVPond)[0]
    stand_facing(pond, 200)
    print("use prompt at pond:", carry.get_use_prompt()); carry.use_started(); carry.use_stopped()
    print("toast:", carry.get_toast(), "| use:", carry.get_use_prompt())
elif step == "drop":
    print("prompt:", carry.get_prompt()); carry.interact(); print("after:", carry.get_prompt())
elif step == "feed":
    foods = [i for i in items if i.get_editor_property("kind") == unreal.FVItemKind.FOOD and i.get_attach_parent_actor() is None]
    def clear(i):
        l = i.get_actor_location(); best = 1e9
        for pr in [x for x in items if x.get_editor_property("kind") != unreal.FVItemKind.FOOD]:
            o, e = pr.get_actor_bounds(False)
            best = min(best, (max(abs(l.x - o.x) - e.x, 0) ** 2 + max(abs(l.y - o.y) - e.y, 0) ** 2) ** 0.5)
        return best
    foods = [i for i in foods if clear(i) > 150 and (i.get_actor_location() - val.get_actor_location()).length() > 300]
    f = foods[0]; print("target food:", i_name(f) if False else f.get_editor_property("food_type"), f.get_actor_location())
    stand_facing(f, 60)
    print("near food:", carry.get_prompt()); carry.interact(); print("holding:", carry.get_prompt())
    e0 = val.energy
    stand_facing(val, 80)
    print("near valentino:", carry.get_prompt()); carry.interact()
    print("toast:", carry.get_toast(), "| eating:", val.is_eating(), "| energy %.2f -> %.2f" % (e0, val.energy))
elif step == "move_pot":
    pots = [i for i in items if i.get_editor_property("kind") == unreal.FVItemKind.PROP and "flowerpot" in str(i.get_editor_property("display_name"))]
    pot = pots[0]; l0 = pot.get_actor_location()
    stand_facing(pot, 70)
    print("near pot:", carry.get_prompt()); carry.interact(); print("holding:", carry.get_prompt())
    p.set_actor_location(p.get_actor_location() + unreal.Vector(0, 250, 0), False, True)
    carry.interact(); print("dropped; moved from", l0, "to", pot.get_actor_location(), "| children:", [c.get_actor_label() for c in pot.get_attached_actors()])
elif step == "thirsty":
    val.set_editor_property("thirst", 1.0)
    print("valentino thirst set to 1; state", val.get_editor_property("state"))
elif step == "show_dog":
    dog = GS.get_all_actors_of_class(w, unreal.FVDog)[0]
    stand_facing(dog, 260, unreal.Vector(-0.7, -0.7, 0).normal())
    print("dog", dog.get_editor_property("state"), dog.get_actor_location())
elif step == "show_pond":
    pond = GS.get_all_actors_of_class(w, unreal.FVPond)[0]
    stand_facing(pond, 380, unreal.Vector(0, 1, 0))
    print("valentino", val.get_editor_property("state"), val.get_actor_location(), "thirst %.2f" % val.thirst)




