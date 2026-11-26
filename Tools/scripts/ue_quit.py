import unreal
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print('saved')
unreal.SystemLibrary.quit_editor()
