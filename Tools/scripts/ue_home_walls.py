# Home walls: plain painted render (flat cream colour + subtle plaster normal) instead of the green-stained scan.
import unreal
eal = unreal.EditorAssetLibrary; mel = unreal.MaterialEditingLibrary
mi = eal.load_asset("/Game/Environment/Houses/Materials/MI_FV_Walls_Home")
mel.set_material_instance_texture_parameter_value(mi, "BaseColorTex", eal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
mel.set_material_instance_texture_parameter_value(mi, "NormalTex", eal.load_asset("/Game/Environment/Houses/Textures/grey_plaster_nor_gl_2k"))
mel.set_material_instance_texture_parameter_value(mi, "ARMTex", eal.load_asset("/Game/Environment/Houses/Textures/grey_plaster_arm_2k"))
mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(0.78, 0.72, 0.6, 1))
mel.update_material_instance(mi); eal.save_loaded_asset(mi)
print("home walls: painted cream")
