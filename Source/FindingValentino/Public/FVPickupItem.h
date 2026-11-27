// Finding Valentino - anything the player can pick up and carry: garden props, food, the watering can.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVTypes.h"
#include "FVPickupItem.generated.h"

class UStaticMeshComponent;

UCLASS()
class FINDINGVALENTINO_API AFVPickupItem : public AActor
{
	GENERATED_BODY()

public:
	AFVPickupItem();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EFVItemKind Kind = EFVItemKind::Prop;

	/** Shown in prompts, e.g. "flowerpot". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	// ---- Food ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Food")
	EFVFoodType FoodType = EFVFoodType::Romaine;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Food")
	float EnergyValue = 0.25f;

	// ---- Watering can ----
	/** Seconds of pouring in a full can. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Watering Can")
	float WaterCapacity = 12.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Item|Watering Can")
	float Water = 12.f;
	/** Visible only while pouring (a thin water stream mesh at the spout). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Watering Can")
	TObjectPtr<UStaticMeshComponent> WaterStream;

	// ---- Carrying ----
	/** Call before attaching: remembers how the item rests on the ground. */
	void PrepareForPickup();
	void AttachToHolder(AActor* Holder, const FVector& RelLocation, float RelYaw);
	void PlaceOnGround(const FVector& GroundPoint, float WorldYaw);
	bool IsHeld() const { return bHeld; }
	/** Half size of the item as it rests (axis-aligned, before pick-up). */
	FVector GetRestHalfExtent() const { return RestHalfExtent; }
	float GetRestYaw() const { return RestRotation.Yaw; }

	/** Bought for Valentino's yard: its entry in the save file (-1 = part of the level). */
	int32 DecorIndex = -1;

	// ---- Ball ----
	/** Throw it from where it is now (it must be held). */
	void Throw(const FVector& Start, const FVector& Velocity, AActor* By);
	/** A dog carries it in his mouth / drops it. */
	void HoldInMouth(AActor* DogActor, const FVector& RelLocation);
	void DropFromMouth(const FVector& Velocity);
	bool IsInMouth() const { return bInMouth; }
	/** World time it was last thrown (-1 if never) and by whom. */
	float GetLastThrownTime() const { return LastThrownTime; }
	AActor* GetThrownBy() const { return ThrownBy.Get(); }

	void SetPouring(bool bPour);
	bool IsPouring() const { return bPouring; }
	FText GetLabel() const;

protected:
	virtual void BeginPlay() override;

private:
	void ApplyCollision();

	bool bHeld = false;
	bool bPouring = false;
	bool bInMouth = false;
	float LastThrownTime = -1.f;
	TWeakObjectPtr<AActor> ThrownBy;
	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> PourAudio;
	FRotator RestRotation = FRotator::ZeroRotator;
	FVector RestHalfExtent = FVector(20.f);
	float BottomOffset = 0.f;
	float HeldRelYaw = 0.f;
};
