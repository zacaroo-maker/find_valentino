// Finding Valentino - a flower bed / patch that grows food when watered.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVTypes.h"
#include "FVPlantBed.generated.h"

class UBillboardComponent;

UCLASS()
class FINDINGVALENTINO_API AFVPlantBed : public AActor
{
	GENERATED_BODY()

public:
	AFVPlantBed();

	/** Water landing within this radius counts. Food sprouts within 60% of it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plant Bed")
	float Radius = 150.f;
	/** Seconds of pouring for a full watering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plant Bed")
	float WaterNeededSeconds = 3.f;
	/** Seconds after a full watering before food appears. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plant Bed")
	float SproutDelay = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plant Bed")
	TArray<EFVFoodType> SproutTypes = { EFVFoodType::Dandelion, EFVFoodType::Strawberry, EFVFoodType::Romaine };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plant Bed")
	int32 FoodPerWatering = 1;
	/** How long the soil stays wet (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plant Bed")
	float DrySeconds = 180.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plant Bed")
	FText DisplayName = NSLOCTEXT("FV", "Bed", "flower bed");

	/** Add pouring time. Returns true at the moment the bed becomes fully watered. */
	bool AddWater(float Seconds);
	float GetWetness() const { return Wetness; }

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Plant Bed")
	TObjectPtr<USceneComponent> Root;
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Plant Bed")
	TObjectPtr<UBillboardComponent> Sprite;
#endif

private:
	void Sprout();
	float Wetness = 0.f;
	float SproutTimeLeft = -1.f;
};
