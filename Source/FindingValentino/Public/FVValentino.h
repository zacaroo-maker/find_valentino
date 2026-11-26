// Finding Valentino - Valentino the Hermann's tortoise (pawn side: body, speed, animation).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FVTypes.h"
#include "FVValentino.generated.h"

class UAnimSequence;

UENUM(BlueprintType)
enum class EFVValentinoState : uint8
{
	Wander,
	Rest,
	GoingToHide,
	Hiding,
	Approach,
	Startled,
	Held,
	Eating,
	GoingToPond,
	AtPond
};

UCLASS()
class FINDINGVALENTINO_API AFVValentino : public ACharacter
{
	GENERATED_BODY()

public:
	AFVValentino();

	// ---- Environment (placeholder values until the weather system exists) ----

	/** Air temperature around Valentino in °C. Drives speed and activity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Environment")
	float Temperature = 24.f;

	/** Walking speed (cm/s) at a comfortable temperature. Real Hermann's tortoises do roughly 5-15 cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Movement")
	float ComfortableWalkSpeed = 18.f;

	/** Below this temperature he is sluggish; at/above ComfortTemperature he moves at full speed. (Tunable placeholders.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Environment")
	float ColdTemperature = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Environment")
	float ComfortTemperature = 24.f;

	/** 0..1.3 activity multiplier from temperature. */
	UFUNCTION(BlueprintPure, Category = "Valentino")
	float GetWarmthFactor() const;

	// ---- Animation ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Animation")
	TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Animation")
	TObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Animation")
	TObjectPtr<UAnimSequence> HideAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Animation")
	TObjectPtr<UAnimSequence> EmergeAnim;

	/** Pull into / come out of the shell. */
	UFUNCTION(BlueprintCallable, Category = "Valentino")
	void SetRetracted(bool bInShell);
	UFUNCTION(BlueprintPure, Category = "Valentino")
	bool IsRetracted() const { return bRetracted; }

	// ---- Being carried ----

	/** Pick up (attach to Holder at Offset, in the holder's space) or put down (place at DropLocation). */
	UFUNCTION(BlueprintCallable, Category = "Valentino")
	void PickUp(AActor* Holder, FVector Offset);
	UFUNCTION(BlueprintCallable, Category = "Valentino")
	void PutDown(FVector DropLocation, float Yaw);
	UFUNCTION(BlueprintPure, Category = "Valentino")
	bool IsHeld() const { return bHeld; }
	UFUNCTION(BlueprintPure, Category = "Valentino")
	float GetHeldSeconds() const { return HeldTime; }

	/** Seconds he stays tucked in his shell right after being lifted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Carry")
	float PickupShyTime = 1.2f;
	/** Leg-paddling speed while held: starts at Min, reaches Max after ImpatienceSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Carry")
	float PaddleRateMin = 1.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Carry")
	float PaddleRateMax = 3.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Carry")
	float ImpatienceSeconds = 12.f;

	// ---- Needs ----

	/** 0 = exhausted, 1 = full of energy. Low energy makes him slow and sleepy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Needs", meta = (ClampMin = "0", ClampMax = "1"))
	float Energy = 0.55f;
	/** 0 = not thirsty, 1 = very thirsty. Rises faster when warm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Needs", meta = (ClampMin = "0", ClampMax = "1"))
	float Thirst = 0.3f;
	/** Energy used per metre walked, plus a slow drain over time (placeholder tuning). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Needs")
	float EnergyPerMetre = 0.02f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Needs")
	float EnergyPerSecond = 0.0006f;
	/** Seconds from 0 to full thirst at a comfortable temperature. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Needs")
	float SecondsToThirsty = 360.f;
	/** At/above this temperature he heads for the pond to cool off (placeholder). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valentino|Environment")
	float HotTemperature = 30.f;

	/** Feed him. Returns false if he can't eat right now (in his shell, being carried, already eating). */
	UFUNCTION(BlueprintCallable, Category = "Valentino")
	bool Eat(EFVFoodType Food, float EnergyValue);
	UFUNCTION(BlueprintPure, Category = "Valentino")
	bool IsEating() const { return EatTimeLeft > 0.f; }
	/** Drinking / being watered: lowers thirst. */
	UFUNCTION(BlueprintCallable, Category = "Valentino")
	void Drink(float Seconds);
	UFUNCTION(BlueprintPure, Category = "Valentino")
	bool IsHot() const { return Temperature >= HotTemperature; }

	/** How many times he has eaten today, and what he ate last (for the day's goals). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Valentino|Needs")
	int32 TimesEaten = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Valentino|Needs")
	EFVFoodType LastFood = EFVFoodType::Lettuce;

	/** Current behaviour, set by the AI controller (shown for debugging). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Valentino")
	EFVValentinoState State = EFVValentinoState::Wander;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void PlayAnim(UAnimSequence* Anim, bool bLoop, float Rate = 1.f);

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CurrentAnim;
	bool bRetracted = false;
	float EmergeTimeLeft = 0.f;
	bool bHeld = false;
	float HeldTime = 0.f;
	float ShyTimeLeft = 0.f;
	float EatTimeLeft = 0.f;
	float SipTimer = 0.f;
};
