#include "FVFamilyCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FVCameraModeComponent.h"
#include "FVCarryComponent.h"
#include "FVDog.h"
#include "FVFamilyMemberData.h"
#include "FVAudio.h"
#include "FVMetaHuman.h"
#include "FVPickupItem.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "FVValentino.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

AFVFamilyCharacter::AFVFamilyCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(35.f, 93.f);

	// third-person defaults like the template: body turns toward movement, camera orbits with the mouse
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 500.f, 0.f);
	Move->JumpZVelocity = 450.f;
	Move->AirControl = 0.35f;
	Move->MaxWalkSpeed = 300.f;
	Move->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CameraMode = CreateDefaultSubobject<UFVCameraModeComponent>(TEXT("CameraMode"));
	Carry = CreateDefaultSubobject<UFVCarryComponent>(TEXT("Carry"));
	Carry->ReachDistance = 190.f;
	Carry->CarryOffset = FVector(34.f, 24.f, 22.f);

	// meshes face +Y in Unreal; actor forward is +X
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -93.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, 270.f, 0.f));

	// defaults so the plain C++ class is playable without a Blueprint (a Blueprint child can override any of these)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC(TEXT("/Game/ThirdPerson/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAMove(TEXT("/Game/ThirdPerson/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IALook(TEXT("/Game/ThirdPerson/Input/Actions/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAJump(TEXT("/Game/ThirdPerson/Input/Actions/IA_Jump.IA_Jump"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAView(TEXT("/Game/Core/Input/IA_ToggleView.IA_ToggleView"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAInteract(TEXT("/Game/Core/Input/IA_Interact.IA_Interact"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAUse(TEXT("/Game/Core/Input/IA_Use.IA_Use"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAPet(TEXT("/Game/Core/Input/IA_Pet.IA_Pet"));
	if (MannyMesh.Succeeded()) GetMesh()->SetSkeletalMeshAsset(MannyMesh.Object);
	if (MannyAnim.Succeeded()) GetMesh()->SetAnimInstanceClass(MannyAnim.Class);
	DefaultMappingContext = IMC.Object;
	MoveAction = IAMove.Object;
	LookAction = IALook.Object;
	JumpAction = IAJump.Object;
	CameraMode->ToggleAction = IAView.Object;
	Carry->InteractAction = IAInteract.Object;
	Carry->UseAction = IAUse.Object;
	Carry->PetAction = IAPet.Object;
}

void AFVFamilyCharacter::ApplyMemberData(UFVFamilyMemberData* Member)
{
	MemberData = Member;
	if (!MemberData)
	{
		return;
	}
	GetCharacterMovement()->MaxWalkSpeed = MemberData->WalkSpeed;
	CameraMode->FirstPersonEyeOffset.Z = MemberData->FirstPersonEyeHeight;
	if (MemberData->ToonParts.Num() > 0)
	{
		if (ToonParts.Num() == 0)
		{
			FVMetaHuman::UseAnimFrame(this, MemberData->bFemaleFrame);
			FVMetaHuman::AttachToonParts(this, MemberData->ToonParts, ToonParts);
		}
		return;
	}
	if (!MetaHumanActor && !MemberData->MetaHumanClass.IsNull())
	{
		FVMetaHuman::UseAnimFrame(this, MemberData->bFemaleFrame);
		MetaHumanActor = FVMetaHuman::AttachTo(this, MemberData->MetaHumanClass.LoadSynchronous());
	}
}

void AFVFamilyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (MemberData)
	{
		ApplyMemberData(MemberData);
	}
}

void AFVFamilyCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (MetaHumanActor)
	{
		MetaHumanActor->Destroy();
		MetaHumanActor = nullptr;
	}
	Super::EndPlay(Reason);
}

void AFVFamilyCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Sub->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AFVFamilyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFVFamilyCharacter::Move);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFVFamilyCharacter::Look);
		}
	}
}

void AFVFamilyCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AFVFamilyCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

bool AFVFamilyCharacter::IsCarrying() const
{
	return Carry && (Carry->IsCarrying() || (Carry->GetHeldItem() && Carry->GetHeldItem()->Kind != EFVItemKind::WateringCan));
}

bool AFVFamilyCharacter::IsPouring() const
{
	return Carry && Carry->GetHeldItem() && Carry->GetHeldItem()->IsPouring();
}

bool AFVFamilyCharacter::IsPetting() const
{
	return Carry && Carry->GetLastPetTime() >= 0.f && GetWorld() && GetWorld()->GetTimeSeconds() - Carry->GetLastPetTime() < 0.6f;
}

bool AFVFamilyCharacter::GetHandTargets(FVector& LeftHand, FVector& RightHand) const
{
	if (!Carry)
	{
		return false;
	}
	// hold whatever it is by its sides; Right = +Y side of the character
	auto SidesOf = [&](const FVector& Centre, float HalfWidth)
	{
		const FVector Side = GetActorRightVector() * HalfWidth;
		LeftHand = Centre - Side;
		RightHand = Centre + Side;
	};
	if (const AActor* Held = Carry->GetCarriedActor())
	{
		FVector O, E;
		Held->GetActorBounds(false, O, E);
		SidesOf(O, FMath::Clamp(FMath::Max(E.X, E.Y) * 0.8f, 8.f, 45.f));
		return true;
	}
	if (const AFVPickupItem* Item = Carry->GetHeldItem())
	{
		const FBoxSphereBounds B = Item->Mesh->Bounds;
		if (Item->Kind == EFVItemKind::WateringCan || Item->Kind == EFVItemKind::Food)
		{
			// one-handed: right hand on the item (the can's handle sits near its top/back), left hand relaxed
			RightHand = B.Origin + FVector(0.f, 0.f, B.BoxExtent.Z * 0.6f);
			LeftHand = GetActorLocation() - GetActorRightVector() * 25.f;
			return true;
		}
		SidesOf(B.Origin, FMath::Clamp(FMath::Max(B.BoxExtent.X, B.BoxExtent.Y), 10.f, 60.f));
		return true;
	}
	if (IsPetting())
	{
		if (const AFVDog* Dog = Carry->GetLastPettedDog())
		{
			RightHand = Dog->GetActorLocation() + FVector(0.f, 0.f, 18.f);
			LeftHand = GetActorLocation() - GetActorRightVector() * 25.f;
			return true;
		}
	}
	return false;
}
