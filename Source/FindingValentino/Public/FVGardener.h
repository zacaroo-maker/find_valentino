// Finding Valentino - Jorge the gardener: a MetaHuman who strolls between the plant beds and pauses to tend them.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FVGardener.generated.h"

UCLASS()
class FINDINGVALENTINO_API AFVGardener : public ACharacter
{
	GENERATED_BODY()

public:
	AFVGardener();

	/** His assembled MetaHuman (BP_Jorge). Without one he shows as the plain mannequin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gardener")
	TSoftClassPtr<AActor> MetaHumanClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gardener")
	bool bFemaleFrame = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gardener")
	FText DisplayName = NSLOCTEXT("FV", "Jorge", "Jorge");

	/** How far from where he was placed he wanders when not at a bed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gardener")
	float WanderRadius = 1500.f;
	/** Chance that the next stop is a plant bed (otherwise a stroll somewhere nearby). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gardener", meta = (ClampMin = "0", ClampMax = "1"))
	float BedChance = 0.65f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gardener")
	float PauseMin = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gardener")
	float PauseMax = 12.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void PickNextStop();

	UPROPERTY(Transient)
	TObjectPtr<AActor> MetaHumanActor;
	UPROPERTY(Transient)
	TObjectPtr<AActor> FacingTarget;

	FVector Home = FVector::ZeroVector;
	bool bWalking = false;
	float PauseLeft = 2.f;
	float WalkTime = 0.f;
};
