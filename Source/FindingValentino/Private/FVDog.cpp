#include "FVDog.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "FVDogAIController.h"
#include "GameFramework/CharacterMovementComponent.h"

AFVDog::AFVDog()
{
	PrimaryActorTick.bCanEverTick = true;

	// small dog: ~43 cm tall, ~68 cm long incl. tail
	GetCapsuleComponent()->InitCapsuleSize(20.f, 22.f);
	GetMesh()->SetRelativeLocation(FVector(0, 0, -22.f));
	GetMesh()->SetRelativeRotation(FRotator(0, 270.f, 0)); // model faces +Y, actor forward is +X
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0, 300.f, 0);
	Move->MaxWalkSpeed = 110.f;
	Move->MaxAcceleration = 400.f;
	Move->BrakingDecelerationWalking = 500.f;
	Move->bUseRVOAvoidance = false;
	// use the "Human" navmesh (radius 35) so he keeps out of the narrow tortoise-only gaps
	Move->NavAgentProps.AgentRadius = 34.f;
	Move->NavAgentProps.AgentHeight = 60.f;

	AIControllerClass = AFVDogAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AFVDog::BeginPlay()
{
	Super::BeginPlay();
	PlayAnim(IdleAnim, true);
}

void AFVDog::PlayAnim(UAnimSequence* Anim, bool bLoop, float Rate)
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

float AFVDog::PlayOnce(UAnimSequence* Anim)
{
	PoseLoop = nullptr;
	if (!Anim)
	{
		return 0.f;
	}
	CurrentAnim = nullptr; // force restart
	PlayAnim(Anim, false);
	OneShotTimeLeft = Anim->GetPlayLength();
	return OneShotTimeLeft;
}

void AFVDog::SetPoseLoop(UAnimSequence* Anim)
{
	PoseLoop = Anim;
}

void AFVDog::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (OneShotTimeLeft > 0.f)
	{
		OneShotTimeLeft -= DeltaSeconds;
		return;
	}
	if (PoseLoop)
	{
		PlayAnim(PoseLoop, true);
		return;
	}
	const float Speed = GetVelocity().Size2D();
	if (Speed > 10.f)
	{
		PlayAnim(WalkAnim, true, FMath::Clamp(Speed / FMath::Max(WalkAnimSpeed, 1.f), 0.4f, 2.f));
	}
	else
	{
		PlayAnim(IdleAnim, true);
	}
}
