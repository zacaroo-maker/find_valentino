// Finding Valentino - knows what each food looks like, spawns food items (for plant beds and forage plants),
// and drops a few fallen bits of food around the garden at the start of each game.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVTypes.h"
#include "FVFoodSpawner.generated.h"

class AFVPickupItem;
class UStaticMesh;
class UBillboardComponent;

UCLASS()
class FINDINGVALENTINO_API AFVFoodSpawner : public AActor
{
	GENERATED_BODY()

public:
	AFVFoodSpawner();

	/** The carried/ground mesh for each kind of food. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	TMap<EFVFoodType, TObjectPtr<UStaticMesh>> FoodMeshes;

	/** How many bits of food lie around at the start of a game (most food now comes from foraging). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	int32 StartCount = 3;
	/** Food is hidden near hiding spots, within this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	float ScatterRadius = 120.f;

	UFUNCTION(BlueprintCallable, Category = "Food")
	AFVPickupItem* SpawnFoodAtGround(EFVFoodType Type, FVector NearLocation);
	/** Spawn a food item exactly here (no ground trace), e.g. straight into the player's hands. */
	AFVPickupItem* SpawnFood(EFVFoodType Type, const FTransform& Where);

	/** The spawner in this world (there's one per level). */
	static AFVFoodSpawner* Get(const UObject* WorldContext);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Food")
	TObjectPtr<USceneComponent> Root;
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Food")
	TObjectPtr<UBillboardComponent> Sprite;
#endif

private:
	EFVFoodType RandomType() const;
};
