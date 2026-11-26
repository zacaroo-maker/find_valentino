#include "FVValentinoAIController.h"

#include "EngineUtils.h"
#include "FVAudio.h"
#include "FVCarryComponent.h"
#include "FVFamilyCharacter.h"
#include "FVFamilyMemberData.h"
#include "FVGameInstance.h"
#include "GameFramework/Character.h"
#include "FVHidingSpot.h"
#include "FVPond.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

AFVValentinoAIController::AFVValentinoAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFVValentinoAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Tortoise = Cast<AFVValentino>(InPawn);
	StartRest();
}

void AFVValentinoAIController::SetState(EFVValentinoState NewState)
{
	State = NewState;
	if (Tortoise)
	{
		Tortoise->State = NewState;
	}
	UE_LOG(LogTemp, Verbose, TEXT("Valentino -> %s"), *UEnum::GetValueAsString(NewState));
}

void AFVValentinoAIController::StartWander()
{
	ReleaseHidingSpot();
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Dest;
	// use the navmesh built for Valentino's (tortoise-sized) agent, not the default human one
	ANavigationData* NavData = (Tortoise && Nav) ? Nav->GetNavDataForProps(Tortoise->GetNavAgentPropertiesRef()) : nullptr;
	if (Tortoise && Nav && Nav->GetRandomReachablePointInRadius(Tortoise->GetActorLocation(), WanderRadius, Dest, NavData))
	{
		bMoveDone = false;
		SetState(EFVValentinoState::Wander);
		MoveToLocation(Dest.Location, 15.f);
	}
	else
	{
		StartRest();
	}
}

void AFVValentinoAIController::StartRest()
{
	StopMovement();
	SetState(EFVValentinoState::Rest);
	// cold or tired tortoises rest longer
	const float Warmth = Tortoise ? Tortoise->GetWarmthFactor() : 1.f;
	const float Energy = Tortoise ? Tortoise->Energy : 1.f;
	StateTimeLeft = FMath::RandRange(RestSeconds.X, RestSeconds.Y) / FMath::Max(Warmth, 0.25f) / (0.35f + 0.65f * Energy);
}

bool AFVValentinoAIController::StartGoingToPond()
{
	AFVPond* Pond = nullptr;
	for (TActorIterator<AFVPond> It(GetWorld()); It; ++It)
	{
		Pond = *It;
		break;
	}
	if (!Pond || !Tortoise)
	{
		return false;
	}
	ReleaseHidingSpot();
	bMoveDone = false;
	SetState(EFVValentinoState::GoingToPond);
	const EPathFollowingRequestResult::Type R = MoveToLocation(Pond->GetDrinkLocation(), 20.f);
	if (R == EPathFollowingRequestResult::Failed)
	{
		StartRest();
		return false;
	}
	return true;
}

void AFVValentinoAIController::NibbleToes(APawn* Player, const UFVFamilyMemberData* Member)
{
	ACharacter* Who = Cast<ACharacter>(Player);
	if (!Who || !Tortoise)
	{
		return;
	}
	// a little hop of surprise
	Who->LaunchCharacter(FVector(0.f, 0.f, 260.f), false, true);
	FVAudio::PlayAt(this, EFVSound::Nibble, Who->GetActorLocation(), 0.9f);
	const FString Name = Member ? Member->DisplayName.ToString() : FString(TEXT("Dad"));
	FString Line;
	if (Name.Contains(TEXT("Friend")))
	{
		// the family friend reacts most comically (design doc)
		const TCHAR* Lines[] = { TEXT("AAAAH! He's eating my toes! Someone help!"), TEXT("Ow ow ow! Why always ME?!"), TEXT("My toes are not lettuce, Valentino!") };
		Line = Lines[FMath::RandHelper(3)];
	}
	else
	{
		const TCHAR* Lines[] = { TEXT("Ouch! Valentino nibbled %s's toes!"), TEXT("Hey! Those toes aren't dandelions, Valentino!"), TEXT("Valentino gives %s's toes a cheeky nibble.") };
		Line = FString(Lines[FMath::RandHelper(3)]).Replace(TEXT("%s"), *Name);
	}
	if (UFVCarryComponent* C = Player->FindComponentByClass<UFVCarryComponent>())
	{
		C->ShowToast(FText::FromString(Line), 3.5f);
	}
}

bool AFVValentinoAIController::IsHoldingFood(APawn* Player)
{
	const UFVCarryComponent* C = Player ? Player->FindComponentByClass<UFVCarryComponent>() : nullptr;
	return C && C->IsHoldingFood();
}

bool AFVValentinoAIController::StartGoingToHide()
{
	if (!Tortoise)
	{
		return false;
	}
	// Weighted random pick among free spots in range
	TArray<AFVHidingSpot*> Spots;
	float Total = 0.f;
	for (TActorIterator<AFVHidingSpot> It(GetWorld()); It; ++It)
	{
		if (!It->bOccupied && FVector::Dist(It->GetActorLocation(), Tortoise->GetActorLocation()) < HideSearchRadius)
		{
			Spots.Add(*It);
			Total += FMath::Max(It->Attractiveness, 0.01f);
		}
	}
	if (Spots.Num() == 0)
	{
		return false;
	}
	float Pick = FMath::FRand() * Total;
	AFVHidingSpot* Chosen = Spots.Last();
	for (AFVHidingSpot* S : Spots)
	{
		Pick -= FMath::Max(S->Attractiveness, 0.01f);
		if (Pick <= 0.f)
		{
			Chosen = S;
			break;
		}
	}
	ReleaseHidingSpot();
	CurrentSpot = Chosen;
	CurrentSpot->bOccupied = true;
	bMoveDone = false;
	SetState(EFVValentinoState::GoingToHide);
	MoveToLocation(CurrentSpot->GetActorLocation(), 10.f);
	return true;
}

void AFVValentinoAIController::ReleaseHidingSpot()
{
	if (CurrentSpot)
	{
		CurrentSpot->bOccupied = false;
		CurrentSpot = nullptr;
	}
}

bool AFVValentinoAIController::CanSeePlayer(APawn* Player, float RangeMultiplier) const
{
	if (!Player || !Tortoise)
	{
		return false;
	}
	const FVector ToPlayer = Player->GetActorLocation() - Tortoise->GetActorLocation();
	const float Dist = ToPlayer.Size2D();
	if (Dist < FeelRadius * RangeMultiplier)
	{
		return true;
	}
	if (Dist > SightRadius * RangeMultiplier)
	{
		return false;
	}
	const float CosAngle = FVector::DotProduct(Tortoise->GetActorForwardVector().GetSafeNormal2D(), ToPlayer.GetSafeNormal2D());
	if (CosAngle < FMath::Cos(FMath::DegreesToRadians(SightHalfAngleDeg)))
	{
		return false;
	}
	return LineOfSightTo(Player);
}

void AFVValentinoAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	bMoveDone = true;
}

void AFVValentinoAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Tortoise)
	{
		return;
	}
	// --- Being carried: the person holding him is in charge ---
	if (Tortoise->IsHeld())
	{
		if (State != EFVValentinoState::Held)
		{
			StopMovement();
			ReleaseHidingSpot();
			SetState(EFVValentinoState::Held);
		}
		return;
	}
	if (State == EFVValentinoState::Held)
	{
		// just put down: get his bearings, then carry on with his day
		StartRest();
		StateTimeLeft = FMath::RandRange(2.f, 5.f);
		ApproachCooldown = 10.f;
	}

	// --- Munching something he was given ---
	if (Tortoise->IsEating())
	{
		if (State != EFVValentinoState::Eating)
		{
			StopMovement();
			ReleaseHidingSpot();
			SetState(EFVValentinoState::Eating);
		}
		return;
	}
	if (State == EFVValentinoState::Eating)
	{
		StartRest();
		StateTimeLeft = FMath::RandRange(2.f, 4.f);
		ApproachCooldown = 6.f;
	}

	StateTimeLeft -= DeltaSeconds;
	ApproachCooldown -= DeltaSeconds;

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const float PlayerDist = Player ? FVector::Dist2D(Player->GetActorLocation(), Tortoise->GetActorLocation()) : BIG_NUMBER;
	const float PlayerSpeed = Player ? Player->GetVelocity().Size2D() : 0.f;
	// who is it? (Valentino is more at ease with some family members than others)
	float Comfort = 1.f;
	float StartleSpeedNow = StartleSpeed;
	const UFVFamilyMemberData* Member = nullptr;
	if (const AFVFamilyCharacter* FC = Cast<AFVFamilyCharacter>(Player))
	{
		Member = FC->MemberData;
	}
	if (!Member)
	{
		if (const UFVGameInstance* GI = Cast<UFVGameInstance>(GetGameInstance()))
		{
			Member = GI->SelectedMember;
		}
	}
	if (Member)
	{
		Comfort = Member->ValentinoComfort;
		StartleSpeedNow *= Member->StartleSpeedMultiplier;
	}
	NibbleCooldown -= DeltaSeconds;

	// --- Startle: someone rushes up close -> into the shell ---
	if (State != EFVValentinoState::Startled && Player && PlayerDist < StartleRadius && PlayerSpeed > StartleSpeedNow)
	{
		StopMovement();
		ReleaseHidingSpot();
		Tortoise->SetRetracted(true);
		SetState(EFVValentinoState::Startled);
		StateTimeLeft = FMath::RandRange(4.f, 8.f);
		return;
	}

	switch (State)
	{
	case EFVValentinoState::Startled:
		if (StateTimeLeft <= 0.f && PlayerSpeed < StartleSpeed * 0.5f)
		{
			Tortoise->SetRetracted(false);
			StartRest();
			StateTimeLeft = 2.f; // let the emerge animation finish, then decide
		}
		break;

	case EFVValentinoState::Rest:
		if (StateTimeLeft <= 0.f)
		{
			const bool bWantsPond = Tortoise->Thirst >= ThirstyAt || (Tortoise->IsHot() && FMath::FRand() < 0.5f);
			if (bWantsPond && StartGoingToPond())
			{
				break;
			}
			if (FMath::FRand() < HideChance && StartGoingToHide())
			{
				break;
			}
			StartWander();
		}
		break;

	case EFVValentinoState::GoingToPond:
		if (bMoveDone)
		{
			StopMovement();
			SetState(EFVValentinoState::AtPond);
			const FVector2D Range = Tortoise->IsHot() ? HotPondSeconds : PondSeconds;
			StateTimeLeft = FMath::RandRange(Range.X, Range.Y);
		}
		break;

	case EFVValentinoState::AtPond:
		Tortoise->Drink(DeltaSeconds);
		if (StateTimeLeft <= 0.f)
		{
			StartWander();
		}
		break;

	case EFVValentinoState::Wander:
		if (bMoveDone)
		{
			StartRest();
		}
		break;

	case EFVValentinoState::GoingToHide:
		if (bMoveDone)
		{
			StopMovement();
			SetState(EFVValentinoState::Hiding);
			StateTimeLeft = FMath::RandRange(HideSeconds.X, HideSeconds.Y);
		}
		break;

	case EFVValentinoState::Hiding:
		if (StateTimeLeft <= 0.f)
		{
			StartWander();
		}
		break;

	case EFVValentinoState::Approach:
		if (!Player || PlayerDist > SightRadius * 1.2f)
		{
			StartRest();
		}
		else if (PlayerDist <= StopAtFeetDistance + 20.f)
		{
			StopMovement();
			// warm and full of energy: sometimes he nibbles a toe (from the design doc)
			if (NibbleCooldown <= 0.f)
			{
				NibbleCooldown = 4.f;
				if (Tortoise->GetWarmthFactor() >= 0.9f && Tortoise->Energy > 0.55f && FMath::FRand() < NibbleChance)
				{
					NibbleToes(Player, Member);
					NibbleCooldown = 15.f;
				}
			}
			if (StateTimeLeft <= 0.f)
			{
				ApproachCooldown = 20.f;
				StartWander();
			}
		}
		else if (bMoveDone || ThinkCooldown <= 0.f)
		{
			bMoveDone = false;
			ThinkCooldown = 1.0f;
			MoveToActor(Player, StopAtFeetDistance);
		}
		ThinkCooldown -= DeltaSeconds;
		break;
	}

	// --- Curiosity: calm, warm, and he sees a slow-moving person -> go over ---
	const bool bCalmState = (State == EFVValentinoState::Rest || State == EFVValentinoState::Wander);
	const bool bFood = IsHoldingFood(Player);
	const float Smell = bFood ? FoodSmellMultiplier : 1.f;
	if (bCalmState && ApproachCooldown <= 0.f && Player && PlayerSpeed < StartleSpeed * 0.6f && CanSeePlayer(Player, Smell))
	{
		ApproachCooldown = bFood ? 4.f : 8.f; // don't re-roll every frame
		if (FMath::FRand() < ApproachChance * Smell * Comfort * Tortoise->GetWarmthFactor())
		{
			bMoveDone = false;
			ThinkCooldown = 0.f;
			SetState(EFVValentinoState::Approach);
			StateTimeLeft = FMath::RandRange(6.f, 15.f); // how long he hangs around once at the feet
		}
	}
}
