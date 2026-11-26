import unreal
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Characters/Dad"
SK_DAD = eal.load_asset(D + "/SK_Dad")
SKM_SRC = eal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
IK_SRC = eal.load_asset("/Game/Characters/Mannequins/Rigs/IK_Mannequin")

def fresh(name, cls, factory):
    path = f"{D}/{name}"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return at.create_asset(name, D, cls, factory)

# 1) IK Rig for Dad: retarget chains named like the mannequin's so they auto-map
ik = fresh("IK_Dad", unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
c = unreal.IKRigController.get_controller(ik)
c.set_skeletal_mesh(SK_DAD)
c.set_retarget_root("pelvis")
chains = [("Root", "root", "root"), ("Spine", "spine", "chest"), ("Neck", "neck", "neck"),
          ("Head", "head", "head"),
          ("LeftArm", "L_upper_arm", "L_hand"), ("RightArm", "R_upper_arm", "R_hand"),
          ("LeftLeg", "L_thigh", "L_foot"), ("RightLeg", "R_thigh", "R_foot")]
for n, a, b in chains:
    c.add_retarget_chain(n, a, b, "")
print("IK_Dad chains:", [str(ch.chain_name) for ch in c.get_retarget_chains()])
eal.save_loaded_asset(ik)

# 2) Retargeter Mannequin -> Dad
rtg = fresh("RTG_Mannequin_To_Dad", unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, IK_SRC)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, ik)
rc.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, SKM_SRC)
rc.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, SK_DAD)
rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
for ch in ("Spine", "LeftArm", "RightArm", "LeftLeg", "RightLeg", "Neck", "Head", "Root"):
    print("  map", ch, "<-", rc.get_source_chain(ch))
# match Dad's rest pose to the mannequin's A-pose where the API allows it
if hasattr(rc, "auto_align_all_bones"):
    rc.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
    print("auto-aligned target retarget pose")
# Root motion lives on the mannequin's root bone; carry it onto Dad's root (not his pelvis)
# so "Force Root Lock" strips it like it does for the mannequin.
st = rc.get_retarget_chain_settings("Root")
st.fk.translation_mode = unreal.RetargetTranslationMode.GLOBALLY_SCALED
rc.set_retarget_chain_settings("Root", st)
print("Root chain translation:", rc.get_retarget_chain_settings("Root").fk.translation_mode)
eal.save_loaded_asset(rtg)

# clear previous retargeted animations
if eal.does_directory_exist(D + "/Animations"):
    eal.delete_directory(D + "/Animations")

# 3) Duplicate + retarget the template's anim blueprint (and everything it uses) onto Dad
abp = eal.load_asset("/Game/Characters/Mannequins/Animations/ABP_Quinn")
out = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
    [eal.find_asset_data("/Game/Characters/Mannequins/Animations/ABP_Quinn")],
    SKM_SRC, SK_DAD, rtg, "", "", "", "_Dad", True)
print("retargeted:", [str(a.package_name) for a in out])
