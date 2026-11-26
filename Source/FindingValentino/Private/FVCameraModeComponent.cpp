#include "FVCameraModeComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"

UFVCameraModeComponent::UFVCameraModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFVCameraModeComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("FVCameraMode: owner is not a Character"));
		SetComponentTickEnabled(false);
		return;
	}
	Boom = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
	Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!Boom || !Camera)
	{
		UE_LOG(LogTemp, Warning, TEXT("FVCameraMode: %s needs a SpringArm and a Camera"), *OwnerCharacter->GetName());
		SetComponentTickEnabled(false);
		return;
	}

	TPArmLength = Boom->TargetArmLength;
	TPBoomLocation = Boom->GetRelativeLocation();
	TPSocketOffset = Boom->SocketOffset;
	bTPCameraLag = Boom->bEnableCameraLag;

	Mode = StartMode;
	Blend = (Mode == EFVCameraMode::FirstPerson) ? 1.f : 0.f;
	ApplyMovementStyle();
	TryBindInput();
}

bool UFVCameraModeComponent::TryBindInput()
{
	if (bInputBound || !ToggleAction || !OwnerCharacter)
	{
		return bInputBound;
	}
	// The input component only exists once a player controller possesses the pawn.
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(OwnerCharacter->InputComponent))
	{
		EIC->BindAction(ToggleAction, ETriggerEvent::Started, this, &UFVCameraModeComponent::ToggleCameraMode);
		bInputBound = true;
	}
	return bInputBound;
}

void UFVCameraModeComponent::ToggleCameraMode()
{
	SetCameraMode(Mode == EFVCameraMode::ThirdPerson ? EFVCameraMode::FirstPerson : EFVCameraMode::ThirdPerson);
}

void UFVCameraModeComponent::SetCameraMode(EFVCameraMode NewMode)
{
	if (NewMode == Mode)
	{
		return;
	}
	Mode = NewMode;
	ApplyMovementStyle();
}

void UFVCameraModeComponent::ApplyMovementStyle()
{
	if (!OwnerCharacter)
	{
		return;
	}
	const bool bFP = (Mode == EFVCameraMode::FirstPerson);

	// First person: body follows the mouse. Third person: body turns toward movement.
	OwnerCharacter->bUseControllerRotationYaw = bFP;
	if (UCharacterMovementComponent* Move = OwnerCharacter->GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = !bFP;
	}
	if (Boom)
	{
		Boom->bEnableCameraLag = bFP ? false : bTPCameraLag;
		Boom->bDoCollisionTest = !bFP;
	}
	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
	{
		if (bFP)
		{
			Mesh->HideBoneByName(HeadBoneName, EPhysBodyOp::PBO_None);
		}
		else
		{
			Mesh->UnHideBoneByName(HeadBoneName);
		}
	}
}

void UFVCameraModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInputBound)
	{
		TryBindInput();
	}
	if (!Boom)
	{
		return;
	}

	const float Target = (Mode == EFVCameraMode::FirstPerson) ? 1.f : 0.f;
	if (TransitionTime <= KINDA_SMALL_NUMBER)
	{
		Blend = Target;
	}
	else
	{
		Blend = FMath::FInterpConstantTo(Blend, Target, DeltaTime, 1.f / TransitionTime);
	}
	const float A = FMath::SmoothStep(0.f, 1.f, Blend);

	Boom->TargetArmLength = FMath::Lerp(TPArmLength, 0.f, A);
	Boom->SocketOffset = FMath::Lerp(TPSocketOffset, FVector::ZeroVector, A);
	Boom->SetRelativeLocation(FMath::Lerp(TPBoomLocation, FirstPersonEyeOffset, A));
}
