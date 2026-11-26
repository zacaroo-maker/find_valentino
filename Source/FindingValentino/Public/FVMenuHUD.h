// Finding Valentino - 80s-arcade style start screen, main menu, character select and settings,
// drawn on the HUD canvas with chunky pixel blocks (no widget assets needed). Keyboard and mouse.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FVMenuHUD.generated.h"

class UFVFamilyMemberData;
class ACameraActor;

UENUM()
enum class EFVMenuScreen : uint8
{
	Title,
	Main,
	Who,
	Settings
};

UCLASS()
class FINDINGVALENTINO_API AFVMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** The seven family members, in display order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TArray<TObjectPtr<UFVFamilyMemberData>> Family;

	/** Render the garden behind the menu at low resolution for a chunky pixel look. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	bool bPixelLook = true;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;
	virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;

private:
	// screens
	void DrawTitle();
	void DrawMain();
	void DrawWho();
	void DrawSettings();
	void DrawTopBar();
	void DrawLogo(float CentreY, float Px);

	/** A chunky menu button; returns its height. Index is its position in the current screen's list. */
	float MenuButton(const FString& Label, int32 Index, float CentreX, float Y, float MinWidth, float Px);

	int32 ItemCount() const;
	void Activate(int32 Index);
	void Back();
	void GoTo(EFVMenuScreen S, int32 StartCursor = 0);
	void HandleKeys();
	void StartGame();
	void ApplyPixelLook(bool bOn);

	EFVMenuScreen Screen = EFVMenuScreen::Title;
	int32 Cursor = 0;
	int32 Selected = 0;
	float Time = 0.f;
	float ScreenTime = 0.f;
	float LastDrawTime = -1.f;

	// pixel size for this frame (scales with screen height)
	float P = 6.f;
	float CW = 0.f;
	float CH = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> MenuCamera;
	FRotator CameraBase = FRotator::ZeroRotator;

	bool bPixelApplied = false;
	float SavedScreenPercentage = 100.f;
	int32 SavedAA = 4;
	int32 SavedUpscale = 3;
};
