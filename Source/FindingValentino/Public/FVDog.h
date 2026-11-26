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
	GettingUp
};

UCLASS()
class FINDINGVALENTINO_API AFVDog : public ACharacter
{
	GENERATED_BODY()

public:
	AFVDog();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dog")
	FText DogName = NSLOCTEXT("FV", "DogName", "the dog");

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
};
