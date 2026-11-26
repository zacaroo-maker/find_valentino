// Finding Valentino - minimal HUD: interaction prompts.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FVHUD.generated.h"

UCLASS()
class FINDINGVALENTINO_API AFVHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;
	virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;
	virtual void NotifyHitBoxEndCursorOver(FName BoxName) override { if (Hover == BoxName) { Hover = NAME_None; } }

private:
	void DrawPauseMenu();
	void DrawPrompt(const FText& Text, float Y, const FLinearColor& TextColor);
	void DrawToast(const FText& Text, float Y);
	/** Pixel block size for this screen (same scale as the start screen). */
	float Px() const;
	FName Hover;

public:

	UPROPERTY(EditAnywhere, Category = "HUD")
	float PromptScale = 1.4f;
	/** Prompt height as a fraction of the screen (0 = top, 1 = bottom). */
	UPROPERTY(EditAnywhere, Category = "HUD")
	float PromptY = 0.82f;
};
