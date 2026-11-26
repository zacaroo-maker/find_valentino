#include "FVValentino.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "FVAudio.h"
#include "FVValentinoAIController.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

AFVValentino::AFVValentino()
{
	PrimaryActorTick.bCanEverTick = true;

	// Small body: ~25 cm long, ~9 cm tall
	GetCapsuleComponent()->InitCapsuleSize(9.f, 9.f);
	GetMesh()->SetRelativeLocation(FVector(0, 0, -9.f));
	GetMesh()->SetRelativeRotation(FRotator(0, 270.f, 0)); // model faces +Y, actor forward is +X
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0, 60.f, 0); // tortoises turn slowly
	Move->MaxWalkSpeed = ComfortableWalkSpeed;
	Move->MaxAcceleration = 60.f;
	Move->BrakingDecelerationWalking = 80.f;
	Move->MaxStepHeight = 6.f;
	Move->SetWalkableFloorAngle(35.f);
	Move->bUseRVOAvoidance = false;
	Move->NavAgentProps.AgentRadius = 9.f;
	Move->NavAgentProps.AgentHeight = 18.f;

	AIControllerClass = AFVValentinoAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AFVValentino::BeginPlay()
{
	Super::BeginPlay();
	PlayAnim(IdleAnim, true);
}

float AFVValentino::GetWarmthFactor() const
{
	const float T = FMath::GetRangePct(ColdTemperature, ComfortTemperature, Temperature);
	return FMath::Clamp(0.2f + 0.8f * T, 0.2f, 1.3f);
}

void AFVValentino::SetRetracted(bool bInShell)
{
	if (bInShell == bRetracted)
	{
		return;
	}
	bRetracted = bInShell;
	if (bRetracted)
	{
		GetCharacterMovement()->StopMovementImmediately();
		PlayAnim(HideAnim, false);
	}
	else
	{
		PlayAnim(EmergeAnim, false);
		EmergeTimeLeft = EmergeAnim ? EmergeAnim->GetPlayLength() : 0.f;
	}
}

void AFVValentino::PickUp(AActor* Holder, FVector Offset)
{
	if (bHeld || !Holder)
	{
		return;
	}
	bHeld = true;
	HeldTime = 0.f;
	ShyTimeLeft = PickupShyTime;
	if (AController* C = GetController())
	{
		C->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachToActor(Holder, FAttachmentTransformRules::KeepWorldTransform);
	SetActorRelativeLocation(Offset);
	SetActorRelativeRotation(FRotator::ZeroRotator); // face the same way as the person holding him
	SetRetracted(true);                              // startled: tuck in first
}

void AFVValentino::PutDown(FVector DropLocation, float Yaw)
{
	if (!bHeld)
	{
		return;
	}
	bHeld = false;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocationAndRotation(DropLocation, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (bRetracted)
	{
		SetRetracted(false);
	}
	else
	{
		PlayAnim(IdleAnim, true, 1.f);
	}
}

bool AFVValentino::Eat(EFVFoodType Food, float EnergyValue)
{
	if (bHeld || bRetracted || IsEating())
	{
		return false;
	}
	Energy = FMath::Clamp(Energy + EnergyValue, 0.f, 1.f);
	EatTimeLeft = 3.5f;
	// three crunchy bites while he eats
	for (int32 k = 0; k < 3; ++k)
	{
		FTimerHandle H;
		TWeakObjectPtr<AFVValentino> Self(this);
		GetWorldTimerManager().SetTimer(H, [Self]()
		{
			if (Self.IsValid())
			{
				FVAudio::PlayAt(Self.Get(), EFVSound::Munch, Self->GetActorLocation(), 0.8f, FMath::FRandRange(0.9f, 1.1f));
			}
		}, 0.15f + k * 1.1f, false);
	}
	if (AController* C = GetController())
	{
		C->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();
	return true;
}

void AFVValentino::Drink(float Seconds)
{
	Thirst = FMath::Max(0.f, Thirst - Seconds * 0.08f);
	SipTimer -= Seconds;
	if (SipTimer <= 0.f)
	{
		SipTimer = 1.8f;
		FVAudio::PlayAt(this, EFVSound::Sip, GetActorLocation(), 0.7f, FMath::FRandRange(0.95f, 1.1f));
	}
}

void AFVValentino::PlayAnim(UAnimSequence* Anim, bool bLoop, float Rate)
{
	if (!Anim)
	{
		return;
	}
	if (Anim != CurrentAnim)
	{
		GetMesh()->PlayAnimation(Anim, bLoop);
		CurrentAnim = Anim;
	}
	GetMesh()->SetPlayRate(Rate);
}

void AFVValentino::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// needs
	const float Speed2D = GetVelocity().Size2D();
	if (!bHeld)
	{
		Energy = FMath::Clamp(Energy - (Speed2D * DeltaSeconds / 100.f) * EnergyPerMetre - EnergyPerSecond * DeltaSeconds, 0.f, 1.f);
	}
	Thirst = FMath::Clamp(Thirst + DeltaSeconds / FMath::Max(SecondsToThirsty, 1.f) * FMath::Max(GetWarmthFactor(), 0.3f), 0.f, 1.f);

	const float EnergyFactor = 0.45f + 0.55f * Energy;
	GetCharacterMovement()->MaxWalkSpeed = (bRetracted || IsEating()) ? 0.f : ComfortableWalkSpeed * GetWarmthFactor() * EnergyFactor;

	if (EatTimeLeft > 0.f && !bHeld)
	{
		EatTimeLeft -= DeltaSeconds;
		PlayAnim(IdleAnim, true, 2.5f); // quick head bobbing while munching
		return;
	}

	if (bHeld)
	{
		HeldTime += DeltaSeconds;
		if (ShyTimeLeft > 0.f)
		{
			ShyTimeLeft -= DeltaSeconds;
			if (ShyTimeLeft <= 0.f)
			{
				SetRetracted(false); // peek out
			}
			return;
		}
		if (EmergeTimeLeft > 0.f)
		{
			EmergeTimeLeft -= DeltaSeconds;
			return;
		}
		// legs paddle in the air; faster the longer he's held (impatience)
		const float Impatience = FMath::Clamp(HeldTime / FMath::Max(ImpatienceSeconds, 0.1f), 0.f, 1.f);
		PlayAnim(WalkAnim, true, FMath::Lerp(PaddleRateMin, PaddleRateMax, Impatience));
		return;
	}

	if (bRetracted)
	{
		return; // Hide anim holds its last frame
	}
	if (EmergeTimeLeft > 0.f)
	{
		EmergeTimeLeft -= DeltaSeconds;
		return;
	}

	if (Speed2D > 1.5f)
	{
		// match leg cadence to ground speed (walk cycle authored for ~18 cm/s)
		PlayAnim(WalkAnim, true, FMath::Clamp(Speed2D / 18.f, 0.3f, 2.f));
	}
	else
	{
		PlayAnim(IdleAnim, true, 1.f);
	}
}
