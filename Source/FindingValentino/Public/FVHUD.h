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

	UPROPERTY(EditAnywhere, Category = "HUD")
	float PromptScale = 1.4f;
	/** Prompt height as a fraction of the screen (0 = top, 1 = bottom). */
	UPROPERTY(EditAnywhere, Category = "HUD")
	float PromptY = 0.82f;
};
