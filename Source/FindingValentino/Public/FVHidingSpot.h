// Finding Valentino - a place Valentino can choose to hide (under a bush, flowerpot, log...).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVHidingSpot.generated.h"

class UBillboardComponent;
class UArrowComponent;

UCLASS()
class FINDINGVALENTINO_API AFVHidingSpot : public AActor
{
	GENERATED_BODY()

public:
	AFVHidingSpot();

	/** How attractive this spot is (higher = chosen more often). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hiding")
	float Attractiveness = 1.f;

	/** True if it keeps rain off (used later by weather-driven behaviour). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hiding")
	bool bSheltersFromRain = true;

	/** Set while Valentino is using this spot. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Hiding")
	bool bOccupied = false;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Hiding")
	TObjectPtr<USceneComponent> Root;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Hiding")
	TObjectPtr<UBillboardComponent> Sprite;
	UPROPERTY(VisibleAnywhere, Category = "Hiding")
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
