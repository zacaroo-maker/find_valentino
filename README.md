# Finding Valentino

A relaxing Unreal Engine 5.8 exploration game: search a lush backyard for **Valentino**, a Hermann's tortoise with a mind of his own.

This repository holds the **code and project configuration only**:

| Folder | What |
|---|---|
| `Source/FindingValentino/` | C++ gameplay: Valentino's AI and needs, pick-up/carry, food, pond, watering can, the dog, camera switching, HUD |
| `Config/` | Project settings (input, navigation agents, maps) |
| `Tools/scripts/` | Blender and Unreal-Python scripts that generate/import models and build the backyard level |
| `Docs/handoff.md` | Design decisions, progress log and next steps |

Game assets (`Content/`) are not included: they contain third-party content (Fab/Quixel Megascans, Unreal starter content) whose licenses don't allow public redistribution.

## Building
- Unreal Engine 5.8, Visual Studio 2022 with the "Game development with C++" workload.
- `Build.bat FindingValentinoEditor Win64 Development -Project=<path>\FindingValentino.uproject`

## Controls (prototype)
WASD move - mouse look - Space jump - **V** first/third person - **E** pick up / put down / give food - **F** pet Diego - **hold left mouse** pour the watering can

