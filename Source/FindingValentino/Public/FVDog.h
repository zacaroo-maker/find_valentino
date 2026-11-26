// Finding Valentino - the family dog: wanders, sniffs around and loves lying in the sun.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FVDog.generated.h"

class UAnimSequence;

UENUM(BlueprintType)
enum class EFVDogState : uint8
{
	Wander,
	Sniff,
	GoingToSun,
	LyingDown,
	Lying,
	GettingUp,
	Held,
	Play,  // chasing another dog / zoomies (dog park)
	Bark
};

UCLASS()
class FINDINGVALENTINO_API AFVDog : public ACharacter
{
	GENERATED_BODY()

public:
	AFVDog();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog")
	FText DogName = NSLOCTEXT("FV", "DogName", "Diego");

	// ---- Energy ----
	/** 0 = worn out, 1 = full of beans. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Energy", meta = (ClampMin = "0", ClampMax = "1"))
	float Energy = 0.6f;
	/** Drain while awake (per second) and per metre walked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Energy")
	float AwakeDrainPerSecond = 0.0015f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Energy")
	float DrainPerMetre = 0.004f;
	/** Recharge per second while lying in the sun. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Energy")
	float SunRechargePerSecond = 0.004f;
	/** Energy from one pat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Energy")
	float PetEnergy = 0.06f;

	/** Pat him. Returns false if it was too soon after the last pat. */
	UFUNCTION(BlueprintCallable, Category = "Dog")
	bool Pet();
	UFUNCTION(BlueprintPure, Category = "Dog")
	bool IsHappy() const { return HappyTimeLeft > 0.f; }

	// ---- Being carried ----
	UFUNCTION(BlueprintCallable, Category = "Dog")
	void PickUp(AActor* Holder, FVector Offset);
	UFUNCTION(BlueprintCallable, Category = "Dog")
	void PutDown(FVector DropLocation, float Yaw);
	UFUNCTION(BlueprintPure, Category = "Dog")
	bool IsHeld() const { return bHeld; }

	/** What he's thinking right now (shown in a bubble above his head). */
	UFUNCTION(BlueprintPure, Category = "Dog")
	FText GetThought() const { return Thought; }

	/** Show this dog's energy meter on the HUD (Diego yes, the dogs at the park no). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog")
	bool bShowMeter = true;

	/** Bark! A little hop and a WOOF (YIP for small dogs, BORK for big ones). */
	UFUNCTION(BlueprintCallable, Category = "Dog")
	void Bark();
	/** Say something in the thought bubble for a while (overrides his idle thoughts). */
	void Say(const FText& Text, float Seconds);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Sound")
	TObjectPtr<class USoundBase> BarkSound;

	/** Set by the AI: multiplies walking speed (running while playing). */
	float PlaySpeed = 1.f;
	/** Set by the AI: he's at the dog park (changes what he thinks about). */
	bool bInPark = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Animation")
	TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Animation")
	TObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Animation")
	TObjectPtr<UAnimSequence> SniffAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Animation")
	TObjectPtr<UAnimSequence> LieDownAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Animation")
	TObjectPtr<UAnimSequence> LyingAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Animation")
	TObjectPtr<UAnimSequence> GetUpAnim;

	/** Speed (cm/s) the walk cycle was authored for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog|Animation")
	float WalkAnimSpeed = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dog")
	EFVDogState State = EFVDogState::Wander;

	/** Plays a one-shot (lie down / get up) and returns its length in seconds. */
	float PlayOnce(UAnimSequence* Anim);
	/** Holds a looping pose animation (lying / sniffing) instead of walk/idle. */
	void SetPoseLoop(UAnimSequence* Anim);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void PlayAnim(UAnimSequence* Anim, bool bLoop, float Rate = 1.f);
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CurrentAnim;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> PoseLoop;
	float OneShotTimeLeft = 0.f;

	void UpdateThought(float DeltaSeconds);
	FText Thought;
	EFVDogState ThoughtState = EFVDogState::Wander;
	float ThoughtTimeLeft = 0.f;
	float HappyTimeLeft = 0.f;
	float PetCooldown = 0.f;
	float SayTimeLeft = 0.f;
	float BarkCooldown = 0.f;
	bool bHeld = false;
};
