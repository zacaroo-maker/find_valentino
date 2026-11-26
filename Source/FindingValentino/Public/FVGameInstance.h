// Finding Valentino - remembers which family member was picked on the main menu.
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FVGameInstance.generated.h"

class UFVFamilyMemberData;

UCLASS()
class FINDINGVALENTINO_API UFVGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** The family member chosen on the main menu (null = the level's default character). */
	UPROPERTY(BlueprintReadWrite, Category = "Family")
	TObjectPtr<UFVFamilyMemberData> SelectedMember;

	/** SOUND / MUSIC switches from the settings screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bSoundOn = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bMusicOn = true;

	/** Which day of the garden we're on (NEXT DAY counts up; a new game from the menu starts at 1). */
	UPROPERTY(BlueprintReadWrite, Category = "Day")
	int32 DayNumber = 1;

	/** The garden level. The main menu is this same level opened with "?Menu". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Family")
	FName GameLevel = TEXT("L_Backyard");
};
