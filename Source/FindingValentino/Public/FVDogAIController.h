// Finding Valentino - the dog's behaviour: find a sunny spot, lie there a long time, potter about, repeat.
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FVDog.h"
#include "FVDogAIController.generated.h"

class ADirectionalLight;

UCLASS()
class FINDINGVALENTINO_API AFVDogAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFVDogAIController();

	UPROPERTY(EditAnywhere, Category = "Dog")
	FVector2D LieSeconds = FVector2D(45.f, 150.f);
	UPROPERTY(EditAnywhere, Category = "Dog")
	FVector2D SniffSeconds = FVector2D(3.f, 7.f);
	UPROPERTY(EditAnywhere, Category = "Dog")
	float WanderRadius = 700.f;
	/** How far he'll look for a sunny spot. */
	UPROPERTY(EditAnywhere, Category = "Dog")
	float SunSearchRadius = 1800.f;
	/** Actors with this tag cast shade (tree canopies have no collision, so they're checked by shape). */
	UPROPERTY(EditAnywhere, Category = "Dog")
	FName ShadeTag = TEXT("FV_Shade");

	/** True if this ground point is in direct sunlight. */
	UFUNCTION(BlueprintPure, Category = "Dog")
	bool IsSunny(const FVector& GroundPoint) const;

	/** Dog park game of tag: "you're it" - chase Target. Ignored if not at the park or resting. */
	void RequestChase(AFVDog* Target);

	/** How fast dogs run while playing (multiplies walking speed). */
	UPROPERTY(EditAnywhere, Category = "Dog|Park")
	float RunMultiplier = 2.6f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	void SetState(EFVDogState S);
	void StartWander();
	void StartSniff();
	void StartGoingToSun();
	void StartLyingDown();
	FVector SunDirection() const;

	// ---- fetch ----
	/** Chase thrown balls / bring them back. Returns true while fetching (skips the normal routine). */
	bool TickFetch(float DeltaSeconds);
	void EndFetch(bool bDropBall);
	UPROPERTY(Transient)
	TObjectPtr<class AFVPickupItem> FetchBall;
	float HandledThrowTime = -1.f;
	bool bKeepAway = false;
	float FetchTimeLeft = 0.f;
public:
	/** Fetch the ball he'll notice from this far away. Only the family dog plays fetch. */
	UPROPERTY(EditAnywhere, Category = "Dog|Fetch")
	float FetchNoticeRadius = 2500.f;
	/** Chance he runs off with the ball instead of bringing it back. */
	UPROPERTY(EditAnywhere, Category = "Dog|Fetch")
	float KeepAwayChance = 0.15f;
private:

	// ---- dog park ----
	void TickPark(class AFVDogPark* Park, float DeltaSeconds);
	void ChooseParkActivity(class AFVDogPark* Park);
	bool bParkMode = false;
	/** Tagged dogs count to three before giving chase. */
	float ChaseDelay = 0.f;
	/** Just tagged someone: can't be tagged straight back. */
	float TagImmune = 0.f;
public:
	bool CanBeTagged() const { return TagImmune <= 0.f; }
private:
	UPROPERTY(Transient)
	TObjectPtr<AFVDog> ChaseTarget;

	UPROPERTY(Transient)
	TObjectPtr<AFVDog> Dog;
	UPROPERTY(Transient)
	TObjectPtr<ADirectionalLight> Sun;

	EFVDogState State = EFVDogState::Wander;
	float StateTimeLeft = 0.f;
	float SunCheckTimeLeft = 0.f;
	int32 WanderLegsLeft = 0;
	bool bMoveDone = false;
};
