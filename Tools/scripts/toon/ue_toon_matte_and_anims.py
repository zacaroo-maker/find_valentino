# Toon project: matte cartoon material for all /Game/Toon models (replaces the shiny FBX Phong import material),
# and hook the toon animations (Interchange named them <Mesh>Armature_<Action>) onto BP_Valentino and BP_Dog.
import unreal
eal = unreal.EditorAssetLibrary; mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
PATH = "/Game/Toon/Materials/M_Toon_Matte"
if eal.does_asset_exist(PATH):
    m = eal.load_asset(PATH)
else:
    m = at.create_asset("M_Toon_Matte", "/Game/Toon/Materials", unreal.Material, unreal.MaterialFactoryNew())
    col = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, -100)
    col.set_editor_property("parameter_name", "Color"); col.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -600, 100)
    tex.set_editor_property("parameter_name", "Tex"); tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 0)
    mel.connect_material_expressions(col, "", mul, "A"); mel.connect_material_expressions(tex, "RGB", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -300, 250)
    rough.set_editor_property("parameter_name", "Roughness"); rough.set_editor_property("default_value", 0.85)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -300, 400)
    spec.set_editor_property("parameter_name", "Specular"); spec.set_editor_property("default_value", 0.3)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(m); eal.save_loaded_asset(m)
GLOSSY = {"Eye": 0.3, "Nose": 0.35, "Toon_Pupil": 0.3, "Toon_EyeWhite": 0.35, "Toon_Tongue": 0.55}
n = 0
for a in eal.list_assets("/Game/Toon", recursive=True):
    if str(eal.find_asset_data(a).asset_class_path.asset_name) != "MaterialInstanceConstant":
        continue
    mi = eal.load_asset(a)
    par = mi.get_editor_property("parent")
    if par and par.get_name() == "M_Toon_Matte":
        continue
    name = mi.get_name()
    c = mel.get_material_instance_vector_parameter_value(mi, "DiffuseColor")
    t = None
    for p in mi.get_editor_property("texture_parameter_values"):
        if "Diffuse" in str(p.get_editor_property("parameter_info").get_editor_property("name")):
            t = p.get_editor_property("parameter_value")
    mel.set_material_instance_parent(mi, m)
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(1, 1, 1, 1) if t else c)
    if t:
        mel.set_material_instance_texture_parameter_value(mi, "Tex", t)
    mel.set_material_instance_scalar_parameter_value(mi, "Roughness", GLOSSY.get(name, 0.85))
    mel.update_material_instance(mi); eal.save_loaded_asset(mi); n += 1
    print(f"{a.split('.')[0]:40} color={c.r:.2f},{c.g:.2f},{c.b:.2f} tex={t.get_name() if t else '-'}")
print("materials converted:", n)

def hook(bp_path, prefix, props):
    bp = eal.load_asset(bp_path)
    cdo = unreal.get_default_object(bp.generated_class())
    for prop, action in props:
        anim = eal.load_asset(prefix + action)
        print(prop, "->", anim.get_name() if anim else "MISSING")
        if anim: cdo.set_editor_property(prop, anim)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp); eal.save_loaded_asset(bp)
hook("/Game/Characters/Valentino/BP_Valentino", "/Game/Toon/Valentino/SK_Valentino_ToonArmature_",
     (("idle_anim", "Idle"), ("walk_anim", "Walk"), ("hide_anim", "Hide"), ("emerge_anim", "Emerge")))
hook("/Game/Characters/Dog/BP_Dog", "/Game/Toon/Dog/SK_Dog_ToonArmature_",
     (("idle_anim", "Idle"), ("walk_anim", "Walk"), ("sniff_anim", "Sniff"), ("lie_down_anim", "LieDown"), ("lying_anim", "Lying"), ("get_up_anim", "GetUp")))
