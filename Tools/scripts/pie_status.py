# Quick play-in-editor status: game mode, pawn, HUD, pause state, animals.
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
GS = unreal.GameplayStatics
if not w:
    print("NO GAME WORLD")
else:
    gm = GS.get_game_mode(w); pc = GS.get_player_controller(w, 0)
    print("map:", w.get_name(), "| game mode:", gm.get_class().get_name() if gm else None,
          "| controller:", pc.get_class().get_name() if pc else None,
          "| pawn:", pc.get_controlled_pawn().get_class().get_name() if pc and pc.get_controlled_pawn() else None,
          "| hud:", pc.get_hud().get_class().get_name() if pc and pc.get_hud() else None)
    if isinstance(pc, unreal.FVPlayerController):
        print("pause menu open:", pc.is_pause_menu_open(), "| game paused:", GS.is_game_paused(w))
    gi = GS.get_game_instance(w)
    print("game instance:", gi.get_class().get_name(), "| selected:", gi.get_editor_property("selected_member").get_name() if isinstance(gi, unreal.FVGameInstance) and gi.get_editor_property("selected_member") else None)
    for v in GS.get_all_actors_of_class(w, unreal.FVValentino):
        print("valentino:", v.get_editor_property("state"), "energy %.2f" % v.energy)
    for d in GS.get_all_actors_of_class(w, unreal.FVDog):
        print("diego:", d.get_editor_property("state"), "energy %.2f" % d.energy, "|", d.get_thought())
    print("food items:", len([i for i in GS.get_all_actors_of_class(w, unreal.FVPickupItem) if i.get_editor_property("kind") == unreal.FVItemKind.FOOD]))
