# World-aligned tiling materials (texture repeats every N cm regardless of mesh size).
import unreal
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
P = "/Game/Environment/Materials"
T = "/Game/StarterContent/Textures/"

def world_mat(name, diffuse, normal, tile_cm, plane="XY", tint=(1, 1, 1), rough=0.9, macro=True):
    path = f"{P}/{name}"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    m = at.create_asset(name, P, unreal.Material, unreal.MaterialFactoryNew())
    wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1200, 0)
    mask = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -1000, 0)
    mask.set_editor_property("r", plane in ("XY", "XZ")); mask.set_editor_property("g", plane in ("XY", "YZ"))
    mask.set_editor_property("b", plane in ("XZ", "YZ")); mask.set_editor_property("a", False)
    mel.connect_material_expressions(wp, "", mask, "")
    div = mel.create_material_expression(m, unreal.MaterialExpressionDivide, -800, 0)
    div.set_editor_property("const_b", float(tile_cm))
    mel.connect_material_expressions(mask, "", div, "A")

    d = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, -550, -150)
    d.set_editor_property("texture", eal.load_asset(T + diffuse))
    mel.connect_material_expressions(div, "", d, "UVs")
    col = d; col_out = "RGB"
    tintc = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -550, -350)
    tintc.set_editor_property("constant", unreal.LinearColor(*tint, 1))
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, -200)
    mel.connect_material_expressions(d, "RGB", mul, "A"); mel.connect_material_expressions(tintc, "", mul, "B")
    col = mul; col_out = ""
    if macro:  # break up visible tiling with a large-scale second sample
        div2 = mel.create_material_expression(m, unreal.MaterialExpressionDivide, -800, 250)
        div2.set_editor_property("const_b", float(tile_cm) * 7.37)
        mel.connect_material_expressions(mask, "", div2, "A")
        d2 = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, -550, 250)
        d2.set_editor_property("texture", eal.load_asset(T + diffuse))
        mel.connect_material_expressions(div2, "", d2, "UVs")
        lerp = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -300, 150)
        lerp.set_editor_property("const_a", 0.7); lerp.set_editor_property("const_b", 1.6)
        mel.connect_material_expressions(d2, "G", lerp, "Alpha")
        mul2 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -100, -100)
        mel.connect_material_expressions(mul, "", mul2, "A"); mel.connect_material_expressions(lerp, "", mul2, "B")
        col = mul2
    mel.connect_material_property(col, col_out, unreal.MaterialProperty.MP_BASE_COLOR)

    if normal:
        n = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, -550, 500)
        n.set_editor_property("texture", eal.load_asset(T + normal))
        n.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        mel.connect_material_expressions(div, "", n, "UVs")
        mel.connect_material_property(n, "RGB", unreal.MaterialProperty.MP_NORMAL)
    r = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 650)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    print("material", name)

world_mat("M_FV_Lawn", "T_Ground_Grass_D", "T_Ground_Grass_N", 250, "XY", (0.85, 1.0, 0.75))
world_mat("M_FV_Patio", "T_Concrete_Tiles_D", "T_Concrete_Tiles_N", 200, "XY", (1, 1, 1), 0.8, macro=False)
world_mat("M_FV_Gravel", "T_Ground_Gravel_D", "T_Ground_Gravel_N", 150, "XY")
world_mat("M_FV_BrickWall", "T_Brick_Clay_New_D", "T_Brick_Clay_New_N", 200, "YZ", (1, 1, 1), 0.85, macro=False)
world_mat("M_FV_BrickWallSide", "T_Brick_Clay_New_D", "T_Brick_Clay_New_N", 200, "XZ", (1, 1, 1), 0.85, macro=False)
