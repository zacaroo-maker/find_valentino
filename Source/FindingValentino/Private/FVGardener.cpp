#include "FVGardener.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "FVAudio.h"
#include "FVMetaHuman.h"
#include "FVPlantBed.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

AFVGardener::AFVGardener()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(35.f, 93.f);
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -93.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, 270.f, 0.f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny"));
	if (MannyMesh.Succeeded()) GetMesh()->SetSkeletalMeshAsset(MannyMesh.Object);
	if (MannyAnim.Succeeded()) GetMesh()->SetAnimInstanceClass(MannyAnim.Class);

	// an unhurried walk that turns toward where he's going
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 240.f, 0.f);
	Move->MaxWalkSpeed = 130.f;
	Move->bUseRVOAvoidance = true;
	// path on the "Human" navmesh
	// accelerate along paths like a player would - the walk animation only plays while accelerating
	Move->GetNavMovementProperties()->bUseAccelerationForPaths = true;
	Move->GetNavMovementProperties()->bUpdateNavAgentWithOwnersCollision = false;
	Move->MaxAcceleration = 600.f;
	Move->NavAgentProps.AgentRadius = 35.f;
	Move->NavAgentProps.AgentHeight = 144.f;

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AFVGardener::BeginPlay()
{
	Super::BeginPlay();
	Home = GetActorLocation();
	if (!MetaHumanClass.IsNull())
	{
		FVMetaHuman::UseAnimFrame(this, bFemaleFrame);
		MetaHumanActor = FVMetaHuman::AttachTo(this, MetaHumanClass.LoadSynchronous());
	}
	PauseLeft = FMath::FRandRange(1.f, 3.f);
}

void AFVGardener::EndPlay(const EEndPlayReason::Type Reason)
{
	if (MetaHumanActor)
	{
		MetaHumanActor->Destroy();
		MetaHumanActor = nullptr;
	}
	Super::EndPlay(Reason);
}

void AFVGardener::PickNextStop()
{
	AAIController* AI = Cast<AAIController>(GetController());
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!AI || !Nav)
	{
		PauseLeft = 3.f;
		return;
	}
	ANavigationData* NavData = const_cast<ANavigationData*>(Nav->GetNavDataForProps(GetCharacterMovement()->NavAgentProps));

	FVector Target = FVector::ZeroVector;
	bool bFound = false;
	FacingTarget = nullptr;

	// usually: go stand at the edge of one of the plant beds
	TArray<AFVPlantBed*> Beds;
	for (TActorIterator<AFVPlantBed> It(GetWorld()); It; ++It)
	{
		Beds.Add(*It);
	}
	if (Beds.Num() > 0 && FMath::FRand() < BedChance)
	{
		AFVPlantBed* Bed = Beds[FMath::RandHelper(Beds.Num())];
		for (int32 Try = 0; Try < 8 && !bFound; ++Try)
		{
			const float A = FMath::FRandRange(0.f, 2.f * PI);
			const FVector P = Bed->GetActorLocation() + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * (Bed->Radius + 70.f);
			FNavLocation Out;
			if (Nav->ProjectPointToNavigation(P, Out, FVector(80.f, 80.f, 300.f), NavData))
			{
				Target = Out.Location;
				FacingTarget = Bed;
				bFound = true;
			}
		}
	}
	// otherwise: a stroll somewhere near home
	if (!bFound)
	{
		FNavLocation Out;
		if (Nav->GetRandomReachablePointInRadius(Home, WanderRadius, Out, NavData))
		{
			Target = Out.Location;
			bFound = true;
		}
	}
	if (!bFound)
	{
		PauseLeft = 3.f;
		return;
	}
	FAIMoveRequest Req(Target);
	Req.SetAcceptanceRadius(40.f);
	AI->MoveTo(Req);
	bWalking = true;
	WalkTime = 0.f;
}

void AFVGardener::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI)
	{
		return;
	}
	if (bWalking)
	{
		WalkTime += DeltaSeconds;
		// his footsteps, a little quieter
		const float Speed = GetVelocity().Size2D();
		StepDistance += Speed * DeltaSeconds;
		if (Speed > 30.f && StepDistance >= 70.f)
		{
			StepDistance = 0.f;
			FVAudio::PlayAt(this, EFVSound::Step, GetActorLocation() - FVector(0.f, 0.f, 90.f), 0.25f, FMath::FRandRange(0.8f, 0.95f));
		}
		const bool bArrived = AI->GetMoveStatus() == EPathFollowingStatus::Idle;
		if (bArrived || WalkTime > 30.f)
		{
			AI->StopMovement();
			bWalking = false;
			PauseLeft = FMath::FRandRange(PauseMin, PauseMax);
		}
		return;
	}
	// pausing: turn to look at the bed he's tending
	if (FacingTarget)
	{
		const FVector To = (FacingTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!To.IsNearlyZero())
		{
			const FRotator Want(0.f, To.Rotation().Yaw, 0.f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Want, DeltaSeconds, 2.5f));
		}
	}
	PauseLeft -= DeltaSeconds;
	if (PauseLeft <= 0.f)
	{
		PickNextStop();
	}
}
