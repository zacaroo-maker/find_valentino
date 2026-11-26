// Finding Valentino - main menu with character select, drawn on the HUD canvas (no widget assets needed).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FVMenuHUD.generated.h"

class UFVFamilyMemberData;

UCLASS()
class FINDINGVALENTINO_API AFVMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** The seven family members, in display order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TArray<TObjectPtr<UFVFamilyMemberData>> Family;

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;
	virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;
	virtual void NotifyHitBoxEndCursorOver(FName BoxName) override;

private:
	void Button(const FString& Label, FName Id, float X, float Y, float W, float H, bool bSelected, float Scale = 1.3f);
	int32 Selected = 0;
	FName Hover;
};
