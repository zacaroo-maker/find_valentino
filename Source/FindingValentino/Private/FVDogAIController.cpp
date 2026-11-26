#include "FVDogAIController.h"

#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

AFVDogAIController::AFVDogAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFVDogAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Dog = Cast<AFVDog>(InPawn);
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		Sun = *It;
		break;
	}
	// sniff about briefly (navigation may still be building), then go and find the sun
	WanderLegsLeft = 0;
	StartSniff();
	StateTimeLeft = 1.5f;
}

void AFVDogAIController::SetState(EFVDogState S)
{
	State = S;
	if (Dog)
	{
		Dog->State = S;
	}
}

FVector AFVDogAIController::SunDirection() const
{
	// direction from the ground towards the sun
	return Sun ? -Sun->GetActorForwardVector() : FVector(0.3f, 0.3f, 0.9f).GetSafeNormal();
}

bool AFVDogAIController::IsSunny(const FVector& GroundPoint) const
{
	const FVector ToSun = SunDirection();
	if (ToSun.Z <= 0.05f)
	{
		return false; // sun below the horizon
	}
	// 1) solid things between him and the sun (house, fence, trunks, sheds...)
	const FVector Start = GroundPoint + FVector(0, 0, 15.f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FVDogSun), false, Dog);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + ToSun * 6000.f, ECC_Visibility, Params))
	{
		return false;
	}
	// 2) tree canopies (no collision): does the ray to the sun pass through a canopy at its height?
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(ShadeTag))
		{
			continue;
		}
		FVector O, E;
		It->GetActorBounds(false, O, E); // canopies have no collision, so include non-colliding parts
		const float CanopyZ = O.Z + E.Z * 0.3f;
		const float Up = CanopyZ - GroundPoint.Z;
		if (Up <= 0.f)
		{
			continue;
		}
		const FVector AtCanopy = GroundPoint + ToSun * (Up / ToSun.Z);
		const float Radius = FMath::Max(E.X, E.Y) * 0.7f;
		if (FVector::Dist2D(AtCanopy, O) < Radius)
		{
			return false;
		}
	}
	return true;
}

void AFVDogAIController::StartWander()
{
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Dest;
	if (Dog && Nav && Nav->GetRandomReachablePointInRadius(Dog->GetActorLocation(), WanderRadius, Dest))
	{
		Dog->SetPoseLoop(nullptr);
		bMoveDone = false;
		SetState(EFVDogState::Wander);
		MoveToLocation(Dest.Location, 30.f);
	}
	else
	{
		StartSniff();
	}
}

void AFVDogAIController::StartSniff()
{
	StopMovement();
	SetState(EFVDogState::Sniff);
	if (Dog)
	{
		Dog->SetPoseLoop(Dog->SniffAnim);
	}
	StateTimeLeft = FMath::RandRange(SniffSeconds.X, SniffSeconds.Y);
}

void AFVDogAIController::StartGoingToSun()
{
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Dog || !Nav)
	{
		return;
	}
	// sample spots, keep the nearest sunny one (dogs are lazy)
	FVector Best = Dog->GetActorLocation();
	float BestDist = TNumericLimits<float>::Max();
	bool bFound = false;
	for (int32 i = 0; i < 40; ++i)
	{
		FNavLocation P;
		if (!Nav->GetRandomReachablePointInRadius(Dog->GetActorLocation(), SunSearchRadius, P))
		{
			continue;
		}
		if (!IsSunny(P.Location))
		{
			continue;
		}
		const float D = FVector::Dist2D(P.Location, Dog->GetActorLocation()) + FMath::FRandRange(0.f, 300.f);
		if (D < BestDist)
		{
			BestDist = D;
			Best = P.Location;
			bFound = true;
		}
	}
	Dog->SetPoseLoop(nullptr);
	bMoveDone = false;
	SetState(EFVDogState::GoingToSun);
	if (!bFound || MoveToLocation(Best, 25.f) == EPathFollowingRequestResult::Failed)
	{
		bMoveDone = true; // nowhere sunny: just lie down here
	}
}

void AFVDogAIController::StartLyingDown()
{
	StopMovement();
	SetState(EFVDogState::LyingDown);
	StateTimeLeft = Dog ? Dog->PlayOnce(Dog->LieDownAnim) : 0.f;
}

void AFVDogAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	bMoveDone = true;
}

void AFVDogAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Dog)
	{
		return;
	}
	// being carried: the person holding him is in charge
	if (Dog->IsHeld())
	{
		if (State != EFVDogState::Held)
		{
			StopMovement();
			SetState(EFVDogState::Held);
		}
		return;
	}
	if (State == EFVDogState::Held)
	{
		WanderLegsLeft = 1;
		StartSniff();
		StateTimeLeft = 2.f;
	}
	// being petted while up and about: stop and enjoy it (tail wag)
	if (Dog->IsHappy() && (State == EFVDogState::Wander || State == EFVDogState::GoingToSun || State == EFVDogState::Sniff))
	{
		StopMovement();
		SetState(EFVDogState::Sniff);
		Dog->SetPoseLoop(nullptr);
		StateTimeLeft = FMath::Max(StateTimeLeft, 3.f);
	}

	StateTimeLeft -= DeltaSeconds;

	switch (State)
	{
	case EFVDogState::GoingToSun:
		if (bMoveDone)
		{
			StartLyingDown();
		}
		break;

	case EFVDogState::LyingDown:
		if (StateTimeLeft <= 0.f)
		{
			SetState(EFVDogState::Lying);
			Dog->SetPoseLoop(Dog->LyingAnim);
			// tired dogs sunbathe longer
			StateTimeLeft = FMath::RandRange(LieSeconds.X, LieSeconds.Y) * (1.6f - Dog->Energy);
			SunCheckTimeLeft = 15.f;
		}
		break;

	case EFVDogState::Lying:
		SunCheckTimeLeft -= DeltaSeconds;
		if (SunCheckTimeLeft <= 0.f)
		{
			SunCheckTimeLeft = 15.f;
			if (!IsSunny(Dog->GetActorLocation() - FVector(0, 0, Dog->GetCapsuleComponent()->GetScaledCapsuleHalfHeight())))
			{
				StateTimeLeft = 0.f; // shade reached him: move
			}
		}
		if (StateTimeLeft <= 0.f)
		{
			SetState(EFVDogState::GettingUp);
			StateTimeLeft = Dog->PlayOnce(Dog->GetUpAnim);
			WanderLegsLeft = FMath::RandRange(1, 3);
		}
		break;

	case EFVDogState::GettingUp:
		if (StateTimeLeft <= 0.f)
		{
			StartSniff();
		}
		break;

	case EFVDogState::Sniff:
		if (StateTimeLeft <= 0.f)
		{
			if (WanderLegsLeft-- > 0)
			{
				StartWander();
			}
			else
			{
				StartGoingToSun();
			}
		}
		break;

	case EFVDogState::Wander:
		if (bMoveDone)
		{
			StartSniff();
		}
		break;

	case EFVDogState::Held:
		break;
	}
}
