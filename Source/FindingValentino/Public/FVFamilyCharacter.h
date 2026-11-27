// Finding Valentino - base class for every playable family member (MetaHumans or other meshes).
// Movement, look, jump, 1st/3rd person camera, carrying/petting/watering, and hand-reach targets for animation.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FVFamilyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UFVCameraModeComponent;
class UFVCarryComponent;
class UFVFamilyMemberData;
struct FInputActionValue;

UCLASS()
class FINDINGVALENTINO_API AFVFamilyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFVFamilyCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Family")
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Family")
	TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Family")
	TObjectPtr<UFVCameraModeComponent> CameraMode;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Family")
	TObjectPtr<UFVCarryComponent> Carry;

	/** Who this is (name, speed, how Valentino feels about them). Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Family")
	TObjectPtr<UFVFamilyMemberData> MemberData;

	// ---- input (assign the same assets the template uses) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Family|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Family|Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Family|Input")
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Family|Input")
	TObjectPtr<UInputAction> JumpAction;

	// ---- for the animation blueprint: what the hands should be doing ----
	/** True while holding something in both arms (Valentino, Diego, a pot, a bench...). */
	UFUNCTION(BlueprintPure, Category = "Family|Animation")
	bool IsCarrying() const;
	/** True while pouring the watering can. */
	UFUNCTION(BlueprintPure, Category = "Family|Animation")
	bool IsPouring() const;
	/** True for a moment after a pat (for a petting arm motion). */
	UFUNCTION(BlueprintPure, Category = "Family|Animation")
	bool IsPetting() const;
	/** World-space points the left/right hand should reach for. Returns false when the hands are free. */
	UFUNCTION(BlueprintPure, Category = "Family|Animation")
	bool GetHandTargets(FVector& LeftHand, FVector& RightHand) const;

	/** Applies a family member: speed, eye height, and puts on their MetaHuman if they have one. */
	UFUNCTION(BlueprintCallable, Category = "Family")
	void ApplyMemberData(UFVFamilyMemberData* Member);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(Transient)
	TObjectPtr<AActor> MetaHumanActor;
	/** Cartoon body pieces (when the member has ToonParts). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UStaticMeshComponent>> ToonParts;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
};
