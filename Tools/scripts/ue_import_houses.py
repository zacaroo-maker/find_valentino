# Import neighbour houses + Poly Haven facade textures, build materials, assign by slot name.
import unreal, os
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
HOUSES = r"C:\Users\calif\Documents\Finding Valentino\Houses"
TEX = r"C:\Users\calif\Documents\Finding Valentino\Downloads\PolyHavenTex"
DEST = "/Game/Environment/Houses"
TDEST = DEST + "/Textures"
MDEST = DEST + "/Materials"

# ---- textures ----
tasks = []
for tid in sorted(os.listdir(TEX)):
    for f in os.listdir(os.path.join(TEX, tid)):
        t = unreal.AssetImportTask(); t.filename = os.path.join(TEX, tid, f); t.destination_path = TDEST
        t.automated = True; t.replace_existing = True; t.save = True; tasks.append(t)
at.import_asset_tasks(tasks)
for p in eal.list_assets(TDEST):
    tx = eal.load_asset(p)
    n = tx.get_name()
    if "_nor_gl_" in n:
        tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tx.set_editor_property("srgb", False)
    elif "_arm_" in n:
        tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tx.set_editor_property("srgb", False)
    eal.save_loaded_asset(tx)

# ---- master material ----
MASTER = MDEST + "/M_FV_Facade"
if eal.does_asset_exist(MASTER):
    master = eal.load_asset(MASTER)
else:
    master = at.create_asset("M_FV_Facade", MDEST, unreal.Material, unreal.MaterialFactoryNew())
    tc = mel.create_material_expression(master, unreal.MaterialExpressionTextureCoordinate, -1100, 0)
    sc = mel.create_material_expression(master, unreal.MaterialExpressionScalarParameter, -1100, 150)
    sc.set_editor_property("parameter_name", "UVScale"); sc.set_editor_property("default_value", 1.0)
    uv = mel.create_material_expression(master, unreal.MaterialExpressionMultiply, -900, 50)
    mel.connect_material_expressions(tc, "", uv, "A"); mel.connect_material_expressions(sc, "", uv, "B")
    def tparam(name, y, kind):
        t = mel.create_material_expression(master, unreal.MaterialExpressionTextureSampleParameter2D, -650, y)
        t.set_editor_property("parameter_name", name)
        if kind == "normal":
            t.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
            t.set_editor_property("texture", eal.load_asset("/Engine/EngineMaterials/DefaultNormal"))
        elif kind == "masks":
            t.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
            t.set_editor_property("texture", eal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
        else:
            t.set_editor_property("texture", eal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
        mel.connect_material_expressions(uv, "", t, "UVs")
        return t
    d = tparam("BaseColorTex", -250, "color"); n = tparam("NormalTex", 100, "normal"); a = tparam("ARMTex", 400, "masks")
    tint = mel.create_material_expression(master, unreal.MaterialExpressionVectorParameter, -650, -450)
    tint.set_editor_property("parameter_name", "Tint"); tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul = mel.create_material_expression(master, unreal.MaterialExpressionMultiply, -300, -300)
    mel.connect_material_expressions(d, "RGB", mul, "A"); mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(n, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(a, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    rs = mel.create_material_expression(master, unreal.MaterialExpressionScalarParameter, -650, 700)
    rs.set_editor_property("parameter_name", "RoughnessMul"); rs.set_editor_property("default_value", 1.0)
    rm = mel.create_material_expression(master, unreal.MaterialExpressionMultiply, -300, 500)
    mel.connect_material_expressions(a, "G", rm, "A"); mel.connect_material_expressions(rs, "", rm, "B")
    mel.connect_material_property(rm, "", unreal.MaterialProperty.MP_ROUGHNESS)
    met = mel.create_material_expression(master, unreal.MaterialExpressionScalarParameter, -300, 750)
    met.set_editor_property("parameter_name", "Metallic"); met.set_editor_property("default_value", 0.0)
    mel.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(master); eal.save_loaded_asset(master)

def tex(tid, kind):
    return eal.load_asset(f"{TDEST}/{tid}_{kind}_2k")

def inst(name, tid=None, tint=(1, 1, 1), uv=1.0, rough=1.0, metal=0.0):
    path = f"{MDEST}/{name}"
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(name, MDEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    if tid:
        mel.set_material_instance_texture_parameter_value(mi, "BaseColorTex", tex(tid, "diff"))
        mel.set_material_instance_texture_parameter_value(mi, "NormalTex", tex(tid, "nor_gl"))
        mel.set_material_instance_texture_parameter_value(mi, "ARMTex", tex(tid, "arm"))
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "UVScale", uv)
    mel.set_material_instance_scalar_parameter_value(mi, "RoughnessMul", rough)
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", metal)
    mel.update_material_instance(mi); eal.save_loaded_asset(mi)
    return mi

SLOT = {
    "Walls_Brick": inst("MI_FV_Walls_Brick", "brick_wall_001"),
    "Walls_BrownBrick": inst("MI_FV_Walls_BrownBrick", "brown_brick_02"),
    "Walls_Plaster": inst("MI_FV_Walls_Plaster", "grey_plaster", (1.0, 0.97, 0.9)),
    "Walls_BluePlaster": inst("MI_FV_Walls_BluePlaster", "blue_plaster_wall"),
    "Walls_ClayPlaster": inst("MI_FV_Walls_ClayPlaster", "clay_plaster"),
    "Roof_Clay": inst("MI_FV_Roof_Clay", "clay_roof_tiles_02", uv=1.3),
    "Roof_Grey": inst("MI_FV_Roof_Grey", "grey_roof_tiles", uv=1.3),
    "Chimney": inst("MI_FV_Chimney", "brick_wall_001", (0.85, 0.85, 0.85)),
    "Trim": inst("MI_FV_Trim", None, (0.85, 0.85, 0.82), rough=0.6),
    "Glass": inst("MI_FV_Glass", None, (0.02, 0.03, 0.05), rough=0.05, metal=0.6),
    "Door": inst("MI_FV_Door", None, (0.18, 0.08, 0.05), rough=0.5),
}

# ---- meshes ----
tasks = []
for f in sorted(os.listdir(HOUSES)):
    if not f.endswith(".fbx"):
        continue
    ui = unreal.FbxImportUI(); ui.import_mesh = True; ui.import_as_skeletal = False; ui.import_animations = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH; ui.import_materials = False; ui.import_textures = False
    sd = ui.static_mesh_import_data
    sd.set_editor_property("convert_scene", True); sd.set_editor_property("convert_scene_unit", True)
    sd.set_editor_property("combine_meshes", True); sd.set_editor_property("auto_generate_collision", True)
    t = unreal.AssetImportTask(); t.filename = os.path.join(HOUSES, f); t.destination_path = DEST
    t.destination_name = f[:-4]; t.automated = True; t.replace_existing = True; t.save = False; t.options = ui
    tasks.append(t)
at.import_asset_tasks(tasks)
for p in eal.list_assets(DEST, recursive=False):
    m = eal.load_asset(p)
    if isinstance(m, unreal.StaticMesh):
        mats = list(m.static_materials)
        for i, sm in enumerate(mats):
            slot = str(sm.material_slot_name).split(".")[0]
            if slot in SLOT:
                m.set_material(i, SLOT[slot])
        b = m.get_bounds()
        eal.save_loaded_asset(m)
        print(f"HOUSE {m.get_name()} {b.box_extent.x*2:.0f}x{b.box_extent.y*2:.0f}x{b.box_extent.z*2:.0f} slots={[str(s.material_slot_name) for s in mats]}")
