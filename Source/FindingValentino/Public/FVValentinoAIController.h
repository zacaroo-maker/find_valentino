// Finding Valentino - Valentino's simple behaviour state machine (v1).
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FVValentino.h"
#include "FVValentinoAIController.generated.h"

class AFVHidingSpot;

UCLASS()
class FINDINGVALENTINO_API AFVValentinoAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFVValentinoAIController();

	// ---- Tunables (first-pass values; adjust after play-testing) ----
	UPROPERTY(EditAnywhere, Category = "Valentino|Wander")
	float WanderRadius = 500.f;
	UPROPERTY(EditAnywhere, Category = "Valentino|Rest")
	FVector2D RestSeconds = FVector2D(4.f, 12.f);
	UPROPERTY(EditAnywhere, Category = "Valentino|Hide")
	FVector2D HideSeconds = FVector2D(15.f, 40.f);
	UPROPERTY(EditAnywhere, Category = "Valentino|Hide")
	float HideSearchRadius = 2500.f;
	/** Chance (0-1) of heading to a hiding spot after finishing a wander leg. */
	UPROPERTY(EditAnywhere, Category = "Valentino|Hide")
	float HideChance = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Valentino|Senses")
	float SightRadius = 700.f;
	UPROPERTY(EditAnywhere, Category = "Valentino|Senses")
	float SightHalfAngleDeg = 70.f;
	/** Hears/feels someone this close regardless of where he's looking. */
	UPROPERTY(EditAnywhere, Category = "Valentino|Senses")
	float FeelRadius = 120.f;
	/** A person moving faster than this (cm/s) close by startles him. */
	UPROPERTY(EditAnywhere, Category = "Valentino|Senses")
	float StartleSpeed = 380.f;
	UPROPERTY(EditAnywhere, Category = "Valentino|Senses")
	float StartleRadius = 220.f;
	UPROPERTY(EditAnywhere, Category = "Valentino|Social")
	float ApproachChance = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Valentino|Social")
	float StopAtFeetDistance = 45.f;

	/** Chance per check (every 4 s at someone's feet) of a toe nibble when warm and energetic. */
	UPROPERTY(EditAnywhere, Category = "Valentino|Social")
	float NibbleChance = 0.35f;

	/** Thirst level at which he heads to the pond. */
	UPROPERTY(EditAnywhere, Category = "Valentino|Pond")
	float ThirstyAt = 0.7f;
	/** Seconds spent drinking/soaking (normal, and when it's hot). */
	UPROPERTY(EditAnywhere, Category = "Valentino|Pond")
	FVector2D PondSeconds = FVector2D(10.f, 20.f);
	UPROPERTY(EditAnywhere, Category = "Valentino|Pond")
	FVector2D HotPondSeconds = FVector2D(30.f, 60.f);
	/** Someone holding food is noticed from further away, and he's keener to come over. */
	UPROPERTY(EditAnywhere, Category = "Valentino|Social")
	float FoodSmellMultiplier = 1.6f;

	UFUNCTION(BlueprintPure, Category = "Valentino")
	EFVValentinoState GetState() const { return State; }

	/** Start of the day: he's already tucked away in a random hiding spot and stays there until found. */
	void HideAtRandomSpot();
	/** Found him: come out of hiding and carry on with the day. */
	void StopHiding();
	/** Bedtime: tucked into his shell and asleep (picking him up wakes him). */
	void SetAsleep(bool bSleep);
	bool IsAsleep() const { return bAsleep; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	void SetState(EFVValentinoState NewState);
	void StartWander();
	void StartRest();
	bool StartGoingToHide();
	void ReleaseHidingSpot();
	bool CanSeePlayer(APawn* Player, float RangeMultiplier = 1.f) const;
	bool StartGoingToPond();
	static bool IsHoldingFood(APawn* Player);
	void NibbleToes(APawn* Player, const class UFVFamilyMemberData* Member);
	float NibbleCooldown = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<AFVValentino> Tortoise;
	UPROPERTY(Transient)
	TObjectPtr<AFVHidingSpot> CurrentSpot;

	EFVValentinoState State = EFVValentinoState::Wander;
	float StateTimeLeft = 0.f;
	float ThinkCooldown = 0.f;
	float ApproachCooldown = 0.f;
	bool bMoveDone = false;
	bool bAsleep = false;
	/** Morning hide: stay put until found, however long it takes. */
	bool bHideUntilFound = false;
};
