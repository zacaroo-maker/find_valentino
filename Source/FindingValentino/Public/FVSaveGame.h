// Finding Valentino - what's remembered between sessions: the high score.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FVSaveGame.generated.h"

/** Something bought for Valentino's yard, and where it was put. */
USTRUCT()
struct FFVDecorSave
{
	GENERATED_BODY()
	UPROPERTY()
	FName Type;
	UPROPERTY()
	FTransform Transform;
};

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
	/** First-time tips already shown (bit per tip). */
	UPROPERTY()
	int32 HintsSeen = 0;
	/** Garden points to spend on decorations: every point earned, minus what's been spent. */
	UPROPERTY()
	int32 Savings = 0;
	/** Every garden point earned since the dog park gate was added (spending doesn't lower it). Opens the gate. */
	UPROPERTY()
	int32 TotalEarned = 0;
	/** The dog park gate has been unlocked. */
	UPROPERTY()
	bool bParkUnlocked = false;
	/** Decorations bought for Valentino's yard. */
	UPROPERTY()
	TArray<FFVDecorSave> Decor;

	static constexpr const TCHAR* Slot = TEXT("FindingValentino");
	/** Loads the save (or a fresh one). */
	static UFVSaveGame* Load();
	void Save();
};
