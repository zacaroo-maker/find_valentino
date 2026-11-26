// Finding Valentino - hides food around the garden at the start of each game, and spawns food for plant beds.
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	TObjectPtr<UStaticMesh> LettuceMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	TObjectPtr<UStaticMesh> DandelionMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	TObjectPtr<UStaticMesh> StrawberryMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	TObjectPtr<UStaticMesh> WormMesh;

	/** How many food items get hidden at the start of a game. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	int32 StartCount = 8;
	/** Relative chances: lettuce, dandelion, strawberry, worm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	FVector4 Weights = FVector4(3.f, 3.f, 2.f, 1.f);
	/** Food is hidden near hiding spots, within this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	float ScatterRadius = 120.f;

	UFUNCTION(BlueprintCallable, Category = "Food")
	AFVPickupItem* SpawnFoodAtGround(EFVFoodType Type, FVector NearLocation);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Food")
	TObjectPtr<USceneComponent> Root;
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Food")
	TObjectPtr<UBillboardComponent> Sprite;
#endif

private:
	UStaticMesh* MeshFor(EFVFoodType T) const;
	EFVFoodType RandomType() const;
};
