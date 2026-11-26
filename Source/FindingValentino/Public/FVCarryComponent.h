// Finding Valentino - lets a player character pick up, carry and put down Valentino and garden items,
// give food to Valentino, and use the watering can.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FVCarryComponent.generated.h"

class UInputAction;
class ACharacter;
class AFVValentino;
class AFVPickupItem;
class AFVPond;

UCLASS(ClassGroup = (FindingValentino), meta = (BlueprintSpawnableComponent))
class FINDINGVALENTINO_API UFVCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFVCarryComponent();

	/** Pick up / put down / give (mapped to E by default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
	TObjectPtr<UInputAction> InteractAction;
	/** Use the held item - e.g. pour the watering can (mapped to left mouse). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
	TObjectPtr<UInputAction> UseAction;

	/** How far (cm, from the character's centre) Valentino can be reached. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
	float ReachDistance = 170.f;
	/** Things must be within this angle of where you're looking (unless right at your feet). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
	float ReachHalfAngleDeg = 55.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
	float AtFeetDistance = 70.f;
	/** Items can be grabbed when their nearest edge is within this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
	float ItemReach = 110.f;

	/** Where Valentino is held, relative to the character's capsule centre (X forward, Z up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
	FVector CarryOffset = FVector(38.f, 0.f, 6.f);
	/** How far in front of the character Valentino is set down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
	float PutDownDistance = 75.f;
	/** Watering reaches beds/Valentino this far in front of you. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
	float WaterReach = 170.f;

	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Interact();
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void UseStarted();
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void UseStopped();

	UFUNCTION(BlueprintPure, Category = "Carry")
	bool IsCarrying() const { return Carried != nullptr; }
	UFUNCTION(BlueprintPure, Category = "Carry")
	bool IsHoldingFood() const;
	AFVPickupItem* GetHeldItem() const { return HeldItem; }
	AFVValentino* GetCarriedValentino() const { return Carried; }

	/** Prompt for E; empty when there's nothing to do. */
	UFUNCTION(BlueprintPure, Category = "Carry")
	FText GetPrompt() const;
	/** Prompt for the use key (left mouse); empty when not applicable. */
	UFUNCTION(BlueprintPure, Category = "Carry")
	FText GetUsePrompt() const;
	/** Short message shown for a few seconds ("Valentino munches the dandelion!"). */
	UFUNCTION(BlueprintPure, Category = "Carry")
	FText GetToast() const { return ToastTimeLeft > 0.f ? Toast : FText::GetEmpty(); }
	void ShowToast(const FText& Message, float Seconds = 3.f);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool TryBindInput();
	AFVValentino* FindReachableValentino() const;
	AFVPickupItem* FindReachableItem(float& OutDist) const;
	bool FindValentinoDrop(FVector& OutLocation) const;
	bool FindItemDrop(FVector& OutGround, float& OutYaw) const;
	AFVPond* FindNearbyPond() const;
	bool PreferValentino(const AFVValentino* V, const AFVPickupItem* Item) const;
	FVector GetViewDir2D() const;

	UPROPERTY(Transient)
	TObjectPtr<AFVValentino> Carried;
	UPROPERTY(Transient)
	TObjectPtr<AFVPickupItem> HeldItem;
	UPROPERTY(Transient)
	TObjectPtr<AFVValentino> CandidateValentino;
	UPROPERTY(Transient)
	TObjectPtr<AFVPickupItem> CandidateItem;

	bool bInputBound = false;
	bool bUseHeld = false;
	FText Toast;
	float ToastTimeLeft = 0.f;
	float ShowerToastCooldown = 0.f;
};
