// Finding Valentino - game modes and the player controller (pause menu).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "FVAudio.h"
#include "FVGameModes.generated.h"

class UInputAction;
class UInputMappingContext;
class UFVFamilyMemberData;

/** The garden: spawns the family member picked on the main menu and applies their settings. */
UCLASS()
class FINDINGVALENTINO_API AFVGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFVGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;

	/** True when the garden is shown behind the main menu (opened with "?Menu", or a fresh packaged start). */
	UFUNCTION(BlueprintPure, Category = "Family")
	bool IsMenuMode() const { return bMenuMode; }

private:
	bool bMenuMode = false;

public:

	/** Who is playing (from the main menu); null if the level was started directly. */
	UFUNCTION(BlueprintPure, Category = "Family")
	UFVFamilyMemberData* GetSelectedMember() const;
};

/** The main menu: no player character, a camera looking at the garden, the character-select HUD. */
UCLASS()
class FINDINGVALENTINO_API AFVMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFVMenuGameMode();
	virtual void BeginPlay() override;
};

/** Adds the pause menu (Esc in a packaged game, P in the editor). */
UCLASS()
class FINDINGVALENTINO_API AFVPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> MenuMappingContext;

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void TogglePauseMenu();
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void GoToMainMenu();
	UFUNCTION(BlueprintPure, Category = "Menu")
	bool IsPauseMenuOpen() const { return bMenuOpen; }

	/** Console: "Weather 0".."Weather 4" = cool morning, mild, warm spring, hot summer, cloudy & breezy. */
	UFUNCTION(Exec)
	void Weather(int32 Index);
	/** Console: "SkipTime 3" jumps the day's clock forward 3 hours (testing). */
	UFUNCTION(Exec)
	void SkipTime(float Hours);
	/** Console (testing): "GardenPoints 1000" adds garden points; "BuyDecor 2" buys catalogue item 2. */
	UFUNCTION(Exec)
	void GardenPoints(int32 Points);
	UFUNCTION(Exec)
	void BuyDecor(int32 Index);

	/** End of the day: pause and show the summary (NEXT DAY / MAIN MENU). */
	void ShowDaySummary();
	UFUNCTION(BlueprintCallable, Category = "Day")
	void NextDay();
	bool IsSummaryOpen() const { return bSummaryOpen; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	/** Footsteps for whoever is being played. */
	virtual void PlayerTick(float DeltaTime) override;

private:
	bool bMenuOpen = false;
	bool bSummaryOpen = false;
	FFVFootsteps Footsteps;
};
