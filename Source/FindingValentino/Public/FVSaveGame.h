// Finding Valentino - what's remembered between sessions: the high score.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FVSaveGame.generated.h"

UCLASS()
class FINDINGVALENTINO_API UFVSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Best single day's score (the HI-SCORE on the start screen). */
	UPROPERTY()
	int32 HighScore = 0;
	/** Score of the most recent day (the 1UP number on the start screen). */
	UPROPERTY()
	int32 LastScore = 0;
	UPROPERTY()
	int32 DaysPlayed = 0;

	static constexpr const TCHAR* Slot = TEXT("FindingValentino");
	/** Loads the save (or a fresh one). */
	static UFVSaveGame* Load();
	void Save();
};
