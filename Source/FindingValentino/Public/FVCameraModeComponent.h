// Finding Valentino - first/third person camera switching for player characters.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FVCameraModeComponent.generated.h"

class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class ACharacter;

UENUM(BlueprintType)
enum class EFVCameraMode : uint8
{
	ThirdPerson,
	FirstPerson
};

/**
 * Add to a Character that has a SpringArm + Camera (like the Third Person template).
 * Press the ToggleAction to switch views. Third person: over-the-shoulder, character turns
 * toward movement. First person: camera at eye height, head hidden, character turns with the mouse.
 */
UCLASS(ClassGroup = (FindingValentino), meta = (BlueprintSpawnableComponent))
class FINDINGVALENTINO_API UFVCameraModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFVCameraModeComponent();

	/** Input action that toggles between first and third person (mapped to V by default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode")
	TObjectPtr<UInputAction> ToggleAction;

	/** Eye position relative to the capsule centre (cm). X = forward, Z = up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode")
	FVector FirstPersonEyeOffset = FVector(12.f, 0.f, 70.f);

	/** Bone hidden in first person so the camera isn't inside the head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode")
	FName HeadBoneName = TEXT("head");

	/** Seconds for the camera to move between views. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode", meta = (ClampMin = "0.0"))
	float TransitionTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode")
	EFVCameraMode StartMode = EFVCameraMode::ThirdPerson;

	UFUNCTION(BlueprintCallable, Category = "Camera Mode")
	void ToggleCameraMode();

	UFUNCTION(BlueprintCallable, Category = "Camera Mode")
	void SetCameraMode(EFVCameraMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Camera Mode")
	EFVCameraMode GetCameraMode() const { return Mode; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool TryBindInput();
	void ApplyMovementStyle();

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;
	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> Boom;
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	EFVCameraMode Mode = EFVCameraMode::ThirdPerson;
	bool bInputBound = false;

	// Third-person values captured from the boom at start
	float TPArmLength = 400.f;
	FVector TPBoomLocation = FVector::ZeroVector;
	FVector TPSocketOffset = FVector::ZeroVector;
	bool bTPCameraLag = false;

	// Blend state (0 = third person, 1 = first person)
	float Blend = 0.f;
};
