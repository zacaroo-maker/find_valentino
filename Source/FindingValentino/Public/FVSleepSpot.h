// Finding Valentino - Valentino's bed: put him here at bedtime.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVSleepSpot.generated.h"

UCLASS()
class FINDINGVALENTINO_API AFVSleepSpot : public AActor
{
	GENERATED_BODY()

public:
	AFVSleepSpot();

	/** Put down within this distance (cm) of the spot counts as "in bed". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep")
	float Radius = 110.f;

	UFUNCTION(BlueprintPure, Category = "Sleep")
	bool Contains(const FVector& Location) const { return FVector::Dist2D(Location, GetActorLocation()) <= Radius; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Sleep")
	TObjectPtr<USceneComponent> Root;
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<class UBillboardComponent> Sprite;
#endif
};
