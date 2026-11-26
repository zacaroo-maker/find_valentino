#include "FVDogAIController.h"

#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "FVDogPark.h"
#include "GameFramework/CharacterMovementComponent.h"
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

// ---------------------------------------------------------------- dog park

void AFVDogAIController::RequestChase(AFVDog* Target)
{
	if (!Dog || !Target || !bParkMode || Dog->IsHeld() || State == EFVDogState::Lying || State == EFVDogState::LyingDown || State == EFVDogState::GettingUp)
	{
		return;
	}
	ChaseTarget = Target;
	Dog->SetPoseLoop(nullptr);
	Dog->PlaySpeed = RunMultiplier;
	SetState(EFVDogState::Play);
	StopMovement();
	bMoveDone = false;
	ChaseDelay = 1.5f; // count to three, then chase
	StateTimeLeft = FMath::FRandRange(6.f, 10.f);
	Dog->Say(FText::FromString(TEXT("I'm it! 1.. 2.. 3..")), 1.6f);
}

void AFVDogAIController::ChooseParkActivity(AFVDogPark* Park)
{
	ChaseTarget = nullptr;
	Dog->SetPoseLoop(nullptr);
	if (Dog->Energy < 0.15f)
	{
		Dog->PlaySpeed = 1.f;
		StartLyingDown(); // worn out: flop down for a rest
		return;
	}
	TArray<AFVDog*> Others = Park->DogsInside();
	Others.Remove(Dog);
	const float R = FMath::FRand();
	if (Others.Num() > 0 && R < 0.45f)
	{
		// chase someone
		AFVDog* T = Others[FMath::RandHelper(Others.Num())];
		ChaseTarget = T;
		Dog->PlaySpeed = RunMultiplier;
		SetState(EFVDogState::Play);
		bMoveDone = false;
		MoveToActor(T, 40.f);
		StateTimeLeft = FMath::FRandRange(5.f, 9.f);
		return;
	}
	if (R < 0.65f)
	{
		// zoomies to somewhere far across the park
		FVector Dest = Park->RandomPoint();
		for (int32 i = 0; i < 4 && FVector::Dist2D(Dest, Dog->GetActorLocation()) < 600.f; ++i)
		{
			Dest = Park->RandomPoint();
		}
		Dog->PlaySpeed = RunMultiplier * 1.1f;
		SetState(EFVDogState::Play);
		bMoveDone = false;
		MoveToLocation(Dest, 40.f);
		StateTimeLeft = 8.f;
		return;
	}
	if (Others.Num() > 0 && R < 0.82f)
	{
		// bark at the nearest dog
		AFVDog* Near = nullptr;
		float Best = 900.f;
		for (AFVDog* O : Others)
		{
			const float D = FVector::Dist2D(O->GetActorLocation(), Dog->GetActorLocation());
			if (D < Best)
			{
				Best = D;
				Near = O;
			}
		}
		if (Near)
		{
			StopMovement();
			ChaseTarget = Near;
			Dog->PlaySpeed = 1.f;
			SetState(EFVDogState::Bark);
			StateTimeLeft = FMath::FRandRange(1.5f, 3.f);
			Dog->Bark();
			return;
		}
	}
	// sniff around a bit
	Dog->PlaySpeed = 1.f;
	StartSniff();
	StateTimeLeft = FMath::FRandRange(2.f, 4.f);
}

void AFVDogAIController::TickPark(AFVDogPark* Park, float DeltaSeconds)
{
	if (!bParkMode)
	{
		bParkMode = true;
		Dog->bInPark = true;
		StopMovement();
		StateTimeLeft = FMath::FRandRange(0.5f, 2.f);
		SetState(EFVDogState::Sniff);
		Dog->SetPoseLoop(nullptr);
		Dog->Say(FText::FromString(TEXT("THE DOG PARK!!")), 2.5f);
		Dog->Bark();
	}
	StateTimeLeft -= DeltaSeconds;
	TagImmune -= DeltaSeconds;

	switch (State)
	{
	case EFVDogState::Play:
		if (ChaseDelay > 0.f)
		{
			ChaseDelay -= DeltaSeconds;
			if (ChaseDelay <= 0.f && ChaseTarget)
			{
				Dog->Say(FText::FromString(TEXT("Ready or not!")), 1.2f);
				Dog->Bark();
				MoveToActor(ChaseTarget, 40.f);
			}
			break;
		}
		if (ChaseTarget)
		{
			// caught them? tag - they're it now
			AFVDogAIController* Other = Cast<AFVDogAIController>(ChaseTarget->GetController());
			const float Reach = 70.f * FMath::Max(Dog->GetActorScale3D().X, ChaseTarget->GetActorScale3D().X);
			if (FVector::Dist2D(ChaseTarget->GetActorLocation(), Dog->GetActorLocation()) < Reach + 40.f && (!Other || Other->CanBeTagged()))
			{
				Dog->Bark();
				Dog->Say(FText::FromString(TEXT("TAG! You're it!")), 1.8f);
				TagImmune = 4.f;
				if (Other)
				{
					Other->RequestChase(Dog);
				}
				// run away from whoever is it now
				ChaseTarget = nullptr;
				const FVector Away = Park->RandomPoint();
				bMoveDone = false;
				MoveToLocation(Away, 40.f);
				Dog->PlaySpeed = RunMultiplier * 1.15f;
				StateTimeLeft = 6.f;
				break;
			}
			if (!Park->Contains(ChaseTarget->GetActorLocation()) || ChaseTarget->IsHeld())
			{
				StateTimeLeft = 0.f; // they left (or got picked up)
			}
		}
		if ((bMoveDone && !ChaseTarget) || StateTimeLeft <= 0.f)
		{
			StopMovement();
			ChooseParkActivity(Park);
		}
		break;

	case EFVDogState::Bark:
		if (ChaseTarget)
		{
			const FVector To = (ChaseTarget->GetActorLocation() - Dog->GetActorLocation()).GetSafeNormal2D();
			Dog->SetActorRotation(FMath::RInterpTo(Dog->GetActorRotation(), To.Rotation(), DeltaSeconds, 6.f));
			if (FMath::FRand() < DeltaSeconds * 0.8f)
			{
				Dog->Bark(); // keeps barking now and then
			}
		}
		if (StateTimeLeft <= 0.f)
		{
			// often the barked-at dog starts a chase
			if (ChaseTarget && FMath::FRand() < 0.5f)
			{
				if (AFVDogAIController* Other = Cast<AFVDogAIController>(ChaseTarget->GetController()))
				{
					Other->RequestChase(Dog);
				}
			}
			ChooseParkActivity(Park);
		}
		break;

	case EFVDogState::LyingDown:
		if (StateTimeLeft <= 0.f)
		{
			SetState(EFVDogState::Lying);
			Dog->SetPoseLoop(Dog->LyingAnim);
			StateTimeLeft = FMath::FRandRange(12.f, 25.f);
		}
		break;

	case EFVDogState::Lying:
		if (StateTimeLeft <= 0.f || Dog->Energy > 0.6f)
		{
			SetState(EFVDogState::GettingUp);
			StateTimeLeft = Dog->PlayOnce(Dog->GetUpAnim);
		}
		break;

	case EFVDogState::GettingUp:
	case EFVDogState::Sniff:
	case EFVDogState::Wander:
	case EFVDogState::GoingToSun:
		if (StateTimeLeft <= 0.f || (State == EFVDogState::Wander && bMoveDone))
		{
			ChooseParkActivity(Park);
		}
		break;

	default:
		break;
	}
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

	// at the dog park: play instead of the garden routine
	if (AFVDogPark* Park = AFVDogPark::FindAt(this, Dog->GetActorLocation()))
	{
		TickPark(Park, DeltaSeconds);
		return;
	}
	if (bParkMode)
	{
		// carried home: back to normal life
		bParkMode = false;
		Dog->bInPark = false;
		Dog->PlaySpeed = 1.f;
		ChaseTarget = nullptr;
		WanderLegsLeft = 1;
		StartSniff();
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
