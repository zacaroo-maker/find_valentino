// Finding Valentino - "a day in the garden": the clock and the sun from morning to evening,
// little goals that pop up during the day, the score, bedtime, and the end-of-day summary.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FVTypes.h"
#include "FVDay.generated.h"

class AFVValentino;
class AFVDog;
class ADirectionalLight;

enum class EFVGoal : uint8
{
	FindValentino,
	FindAgain,
	Feed,
	WaterBed,
	DiegoPark,
	PetDiego,
	PondCool,
	Bedtime,
	Craving,
	Fetch,
};

struct FFVGoal
{
	EFVGoal Type = EFVGoal::FindValentino;
	EFVFoodType Food = EFVFoodType::Romaine; // for Craving
	FString Text;
	TWeakObjectPtr<AActor> Target;
	int32 Baseline = 0;
	float Progress = 0.f;   // seconds or count towards Needed
	float Needed = 0.f;     // 0 = no progress bar
	float Age = 0.f;
};

/** First-time tips, each shown once ever. */
enum class EFVHint : uint8
{
	Move, Search, PickUp, Sparkle, Goals, Craving, Ball, Can, Pet, Bedtime, Decorate,
	Count
};

struct FFVPopup
{
	FString Text;
	float Age = 0.f;
	bool bBig = false;
};

UCLASS()
class FINDINGVALENTINO_API UFVDaySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UFVDaySubsystem, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type T) const override { return T == EWorldType::Game || T == EWorldType::PIE; }
	virtual bool IsTickableWhenPaused() const override { return false; }

	/** Begins day N (called by the game mode when you start playing). */
	void StartDay(int32 Number);
	bool IsRunning() const { return bRunning; }

	// ---- for the HUD ----
	int32 GetDay() const { return Day; }
	int32 GetScore() const { return Score; }
	float GetHour() const { return Hour; }
	FString GetClockText() const;
	const TArray<FFVGoal>& GetGoals() const { return Goals; }
	const TArray<FFVPopup>& GetPopups() const { return Popups; }
	FString GetBanner() const { return BannerTime > 0.f ? Banner : FString(); }
	float GetBannerTime() const { return BannerTime; }

	// ---- intro card and tips ----
	bool IsIntroShowing() const { return bIntro; }
	float GetIntroAge() const { return IntroAge; }
	/** The tip on screen right now (empty if none) and how long it has been up. */
	FString GetHint() const { return HintText; }
	float GetHintAge() const { return HintAge; }
	/** Queue a first-time tip (ignored if it has been seen before). */
	void ShowHint(EFVHint Hint);

	// ---- for the weather's look ----
	/** 0 at midday .. 1 when the sun is low and golden. */
	float GetGolden() const { return Golden; }
	float GetSunElevation() const { return SunElevation; }

	// ---- end of day ----
	bool IsSummaryShowing() const { return bSummary; }
	const TArray<TPair<FString, int32>>& GetSummaryLines() const { return SummaryLines; }
	bool IsNewHighScore() const { return bNewHighScore; }
	int32 GetHighScore() const { return HighScore; }
	bool WasPutToBed() const { return bInBed; }

	/** The player picked food off a plant: the first of each kind each day earns a little bonus. */
	void OnForaged(EFVFoodType Food);
	/** Diego brought the ball back. */
	void OnFetch();

	/** Testing: jump the clock forward. */
	void SkipHours(float Hours) { Hour = FMath::Min(Hour + Hours, EndHour - 0.01f); }

	/** Real seconds a whole day (7:00 to 20:00) takes. */
	float DayLengthSeconds = 900.f;
	float StartHour = 7.f;
	float BedtimeHour = 18.5f;
	float EndHour = 20.f;

private:
	void AddScore(int32 Points, const FString& Why, const FString& Category, bool bBig = false);
	void Announce(const FString& Text, float Seconds = 3.f);
	void UpdateSun();
	void UpdateGoals(float DeltaTime);
	void MaybeAddGoal();
	bool HasGoal(EFVGoal Type) const;
	void AddGoal(EFVGoal Type, const FString& Text, AActor* Target = nullptr, int32 Baseline = 0, float Needed = 0.f);
	void EndDay();
	AFVValentino* GetValentino() const;
	AFVDog* GetDiego() const;
	class AFVValentinoAIController* GetValentinoAI() const;

	bool bRunning = false;
	bool bSummary = false;
	bool bInBed = false;
	bool bNewHighScore = false;
	bool bBedtimeAnnounced = false;
	int32 Day = 1;
	int32 Score = 0;
	int32 HighScore = 0;
	int32 PetPoints = 0;
	float Hour = 7.f;
	float FindTimer = 0.f;
	float NextGoalIn = 25.f;
	float SinceFound = 0.f;
	float EndDelay = -1.f;
	FString Banner;
	float BannerTime = 0.f;
	TArray<FFVGoal> Goals;
	TArray<FFVPopup> Popups;
	TMap<FString, int32> ByCategory;
	TArray<FString> CategoryOrder;
	TArray<TPair<FString, int32>> SummaryLines;
	int32 LastPetCount = 0;
	uint32 ForagedMask = 0;   // which foods were picked today
	int32 Fetches = 0;        // balls Diego brought back today
	int32 FetchPoints = 0;
	float NextCravingIn = 40.f;
	float Golden = 0.f;
	float SunElevation = 45.f;

	void UpdateIntroAndHints(float DeltaTime);
	bool bIntro = false;
	float IntroAge = 0.f;
	int32 HintsSeen = 0;
	TArray<EFVHint> HintQueue;
	FString HintText;
	float HintAge = 0.f;
	float HintGap = 0.f;
	float SearchTime = 0.f;
	void SetCraving(bool bOn, EFVFoodType Food = EFVFoodType::Romaine);

	UPROPERTY(Transient)
	TObjectPtr<ADirectionalLight> Sun;
	FRotator SunBase = FRotator::ZeroRotator;
	FLinearColor SunColorBase = FLinearColor::White;
};
