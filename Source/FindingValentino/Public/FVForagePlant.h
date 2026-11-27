// Finding Valentino - a plant in the garden that Valentino's food grows on (romaine, tomato, dandelion,
// strawberry, chicory, hibiscus). Walk up and press E to pick; the picked part grows back after a while.
// Ripe plants twinkle now and then (drawn by the HUD) so you can spot them from across the garden.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVTypes.h"
#include "FVForagePlant.generated.h"

class UStaticMeshComponent;
class AFVPickupItem;

UCLASS()
class FINDINGVALENTINO_API AFVForagePlant : public AActor
{
	GENERATED_BODY()

public:
	AFVForagePlant();

	/** The plant itself (always there). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Forage")
	TObjectPtr<UStaticMeshComponent> PlantMesh;
	/** The pickable part: flowers, fruit, the heart of the lettuce. Hidden while it grows back. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Forage")
	TObjectPtr<UStaticMeshComponent> ProduceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forage")
	EFVFoodType FoodType = EFVFoodType::Dandelion;
	/** Seconds before it can be picked again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forage")
	float RegrowSeconds = 120.f;

	bool IsRipe() const { return bRipe; }
	/** Where the sparkle and the pick prompt aim (the middle of the produce). */
	FVector GetProduceCentre() const;
	/** Pick it: returns a food item for the player's hands (nullptr if not ripe). */
	AFVPickupItem* Pick();

	/** Each plant twinkles at its own moment (0..1). */
	float SparklePhase = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	bool bRipe = true;
	float RegrowLeft = 0.f;
	float GrowAnim = 1.f; // 0..1 pop-in when it comes back
};
