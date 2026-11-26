# Create the seven family-member data assets (placeholder tuning from the design archetypes - adjust in play-testing).
import unreal
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
P = "/Game/Characters/Family/Data"
FAMILY = [
    # id, name, description, walk, run, eye height, Valentino comfort, startle speed multiplier
    ("Dad", "Dad", "Curious explorer. Checks under everything, walks fast, makes silly comments. Valentino sometimes follows him.", 340, 540, 70, 1.1, 1.0),
    ("Mom", "Mom", "Observant. Notices small movements and details, and approaches Valentino gently.", 300, 500, 66, 1.15, 1.15),
    ("Grandpa", "Grandpa", "Mischievous and jokey, slower and easily distracted. Valentino is very comfortable with him.", 240, 380, 66, 1.6, 1.3),
    ("Grandma", "Grandma", "Patient and calm, great at spotting hiding places, gently encourages Valentino.", 250, 390, 62, 1.35, 1.3),
    ("OlderKid", "Older Kid", "Adventurous and energetic - explores everywhere, but may rush up and startle Valentino.", 360, 600, 55, 0.9, 0.7),
    ("YoungerKid", "Younger Kid", "Playful, sits and watches Valentino, easily distracted by bugs and flowers. Valentino is curious about them.", 320, 520, 40, 1.25, 0.8),
    ("Friend", "Family Friend", "Clumsy and goofy. Funny noises, sometimes overlooks Valentino right in front of them - watch your toes!", 300, 500, 70, 0.95, 0.9),
]
for fid, name, desc, walk, run, eye, comfort, startle in FAMILY:
    path = f"{P}/DA_{fid}"
    if eal.does_asset_exist(path):
        a = eal.load_asset(path)
    else:
        fac = unreal.DataAssetFactory(); fac.set_editor_property("data_asset_class", unreal.FVFamilyMemberData)
        a = at.create_asset(f"DA_{fid}", P, unreal.FVFamilyMemberData, fac)
    a.set_editor_property("display_name", unreal.Text(name))
    a.set_editor_property("description", unreal.Text(desc))
    a.set_editor_property("walk_speed", float(walk)); a.set_editor_property("run_speed", float(run))
    a.set_editor_property("first_person_eye_height", float(eye))
    a.set_editor_property("valentino_comfort", float(comfort)); a.set_editor_property("startle_speed_multiplier", float(startle))
    eal.save_loaded_asset(a)
    print("ok", path)
