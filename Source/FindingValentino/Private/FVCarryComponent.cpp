#include "FVCarryComponent.h"

#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "FVAudio.h"
#include "EngineUtils.h"
#include "FVDay.h"
#include "FVDog.h"
#include "FVForagePlant.h"
#include "FVPickupItem.h"
#include "FVPlantBed.h"
#include "FVPond.h"
#include "FVValentino.h"
#include "GameFramework/Character.h"
#include "InputAction.h"

#define LOCTEXT_NAMESPACE "FVCarry"

UFVCarryComponent::UFVCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UFVCarryComponent::TryBindInput()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (bInputBound || !InteractAction || !Owner)
	{
		return bInputBound;
	}
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(Owner->InputComponent))
	{
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &UFVCarryComponent::Interact);
		if (PetAction)
		{
			EIC->BindAction(PetAction, ETriggerEvent::Started, this, &UFVCarryComponent::Pet);
		}
		if (UseAction)
		{
			EIC->BindAction(UseAction, ETriggerEvent::Started, this, &UFVCarryComponent::UseStarted);
			EIC->BindAction(UseAction, ETriggerEvent::Completed, this, &UFVCarryComponent::UseStopped);
			EIC->BindAction(UseAction, ETriggerEvent::Canceled, this, &UFVCarryComponent::UseStopped);
		}
		bInputBound = true;
	}
	return bInputBound;
}

void UFVCarryComponent::ShowToast(const FText& Message, float Seconds)
{
	Toast = Message;
	ToastTimeLeft = Seconds;
}

bool UFVCarryComponent::IsHoldingFood() const
{
	return HeldItem && HeldItem->Kind == EFVItemKind::Food;
}

FVector UFVCarryComponent::GetViewDir2D() const
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	FVector ViewDir = Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;
	if (Owner)
	{
		if (const AController* C = Owner->GetController())
		{
			ViewDir = C->GetControlRotation().Vector(); // works in first and third person
		}
	}
	return ViewDir.GetSafeNormal2D();
}

AFVValentino* UFVCarryComponent::FindReachableValentino() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	const FVector ViewDir = GetViewDir2D();
	const float CosLimit = FMath::Cos(FMath::DegreesToRadians(ReachHalfAngleDeg));
	AFVValentino* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AFVValentino> It(GetWorld()); It; ++It)
	{
		AFVValentino* V = *It;
		if (V->IsHeld())
		{
			continue;
		}
		const FVector To = V->GetActorLocation() - Owner->GetActorLocation();
		const float Dist = To.Size();
		if (Dist > ReachDistance)
		{
			continue;
		}
		const bool bInView = FVector::DotProduct(ViewDir, To.GetSafeNormal2D()) >= CosLimit;
		if ((bInView || To.Size2D() < AtFeetDistance) && Dist < BestDist)
		{
			Best = V;
			BestDist = Dist;
		}
	}
	return Best;
}

AFVPickupItem* UFVCarryComponent::FindReachableItem(float& OutDist) const
{
	const AActor* Owner = GetOwner();
	OutDist = TNumericLimits<float>::Max();
	if (!Owner)
	{
		return nullptr;
	}
	const FVector ViewDir = GetViewDir2D();
	const float CosLimit = FMath::Cos(FMath::DegreesToRadians(ReachHalfAngleDeg + 10.f));
	const FVector P = Owner->GetActorLocation();
	// full 3D view direction from the eyes, to pick what the player is actually aiming at
	FVector View3D = Owner->GetActorForwardVector();
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		if (const AController* C = Pawn->GetController())
		{
			View3D = C->GetControlRotation().Vector();
		}
	}
	const FVector Eye = P + FVector(0, 0, 60.f);
	float BestAim = -2.f;
	AFVPickupItem* Best = nullptr;
	for (TActorIterator<AFVPickupItem> It(GetWorld()); It; ++It)
	{
		AFVPickupItem* I = *It;
		if (I->IsHeld())
		{
			continue;
		}
		FVector O, E;
		O = I->Mesh->Bounds.Origin; E = I->Mesh->Bounds.BoxExtent; // the item's own mesh (food has no collision; ignore the can's water stream)
		// distance (2D) from us to the nearest edge of the item, so big things like benches work from either end
		const FVector2D Near(FMath::Clamp(P.X, O.X - E.X, O.X + E.X), FMath::Clamp(P.Y, O.Y - E.Y, O.Y + E.Y));
		const float Dist = FVector2D::Distance(FVector2D(P), Near);
		if (Dist > ItemReach || FMath::Abs(O.Z - P.Z) > 200.f)
		{
			continue;
		}
		const FVector To = FVector(Near.X - P.X, Near.Y - P.Y, 0.f);
		const bool bInView = To.IsNearlyZero(5.f) || FVector::DotProduct(ViewDir, To.GetSafeNormal()) >= CosLimit;
		if (!(bInView || Dist < 40.f))
		{
			continue;
		}
		// aim score: how close the item (its nearest point, at its own height) is to the centre of view
		const FVector AimPoint(Near.X, Near.Y, FMath::Clamp(Eye.Z, O.Z - E.Z, O.Z + E.Z));
		const float Aim = FVector::DotProduct(View3D, (AimPoint - Eye).GetSafeNormal());
		if (Aim > BestAim)
		{
			BestAim = Aim;
			Best = I;
			OutDist = Dist;
		}
	}
	return Best;
}

bool UFVCarryComponent::FindValentinoDrop(FVector& OutLocation) const
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner || !Carried)
	{
		return false;
	}
	const float HalfHeight = Carried->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Base = Owner->GetActorLocation() + Owner->GetActorForwardVector() * PutDownDistance;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FVCarryDrop), false, Owner);
	Params.AddIgnoredActor(Carried);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Base + FVector(0, 0, 60), Base - FVector(0, 0, 250), ECC_Visibility, Params))
	{
		OutLocation = Hit.ImpactPoint + FVector(0, 0, HalfHeight + 1.f);
		const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
			OutLocation, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(Carried->GetCapsuleComponent()->GetScaledCapsuleRadius(), HalfHeight - 1.f), Params);
		return !bBlocked;
	}
	return false;
}

bool UFVCarryComponent::FindItemDrop(FVector& OutGround, float& OutYaw) const
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner || !HeldItem)
	{
		return false;
	}
	const FVector Half = HeldItem->GetRestHalfExtent();
	const float Size = FMath::Max(Half.X, Half.Y);
	const FVector Fwd = Owner->GetActorForwardVector();
	const FVector Base = Owner->GetActorLocation() + Fwd * (45.f + Size);
	OutYaw = HeldItem->GetActorRotation().Yaw;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FVItemDrop), true, Owner);
	Params.AddIgnoredActor(HeldItem);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Base + FVector(0, 0, 80), Base - FVector(0, 0, 300), ECC_Visibility, Params))
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	if (HeldItem->Kind == EFVItemKind::Food)
	{
		return true; // food lies on anything
	}
	// room check: a slightly shrunken box of the item's footprint, lifted just off the ground
	const FVector Box(Size * 0.85f, Size * 0.85f, Half.Z * 0.9f);
	const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
		OutGround + FVector(0, 0, Half.Z + 3.f), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeBox(Box), Params);
	return !bBlocked;
}

float UFVCarryComponent::AimAt(const FVector& Point) const
{
	// uses the camera pitch too, so looking down at a dandelion next to Valentino picks the dandelion
	const AActor* Owner = GetOwner();
	FVector View = Owner->GetActorForwardVector();
	if (const APawn* P = Cast<APawn>(Owner))
	{
		if (const AController* C = P->GetController())
		{
			View = C->GetControlRotation().Vector();
		}
	}
	const FVector Eye = Owner->GetActorLocation() + FVector(0, 0, 60.f);
	return FVector::DotProduct(View, (Point - Eye).GetSafeNormal());
}

AFVDog* UFVCarryComponent::FindReachableDog() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	const FVector ViewDir = GetViewDir2D();
	const float CosLimit = FMath::Cos(FMath::DegreesToRadians(ReachHalfAngleDeg));
	AFVDog* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AFVDog> It(GetWorld()); It; ++It)
	{
		if (It->IsHeld())
		{
			continue;
		}
		const FVector To = It->GetActorLocation() - Owner->GetActorLocation();
		const float Dist = To.Size();
		if (Dist > ReachDistance + 30.f)
		{
			continue;
		}
		const bool bInView = FVector::DotProduct(ViewDir, To.GetSafeNormal2D()) >= CosLimit;
		if ((bInView || To.Size2D() < AtFeetDistance + 20.f) && Dist < BestDist)
		{
			Best = *It;
			BestDist = Dist;
		}
	}
	return Best;
}

AFVForagePlant* UFVCarryComponent::FindReachablePlant() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	const FVector ViewDir = GetViewDir2D();
	const float CosLimit = FMath::Cos(FMath::DegreesToRadians(ReachHalfAngleDeg + 10.f));
	const FVector P = Owner->GetActorLocation();
	AFVForagePlant* Best = nullptr;
	float BestAim = -2.f;
	for (TActorIterator<AFVForagePlant> It(GetWorld()); It; ++It)
	{
		if (!It->IsRipe())
		{
			continue;
		}
		const FVector C = It->GetProduceCentre();
		const FVector To(C.X - P.X, C.Y - P.Y, 0.f);
		// big plants (the hibiscus bush) can be reached from their edge
		const float Edge = FMath::Max(It->ProduceMesh->Bounds.BoxExtent.X, It->ProduceMesh->Bounds.BoxExtent.Y) * 0.6f;
		const float Dist = FMath::Max(0.f, To.Size() - Edge);
		if (Dist > ItemReach || FMath::Abs(C.Z - P.Z) > 200.f)
		{
			continue;
		}
		const bool bInView = To.IsNearlyZero(5.f) || FVector::DotProduct(ViewDir, To.GetSafeNormal()) >= CosLimit;
		if (!(bInView || Dist < 40.f))
		{
			continue;
		}
		const float Aim = AimAt(C);
		if (Aim > BestAim)
		{
			BestAim = Aim;
			Best = *It;
		}
	}
	return Best;
}

void UFVCarryComponent::ChooseCandidate(AFVValentino*& V, AFVDog*& D, AFVPickupItem*& I, AFVForagePlant*& Plant) const
{
	float ItemDist;
	V = FindReachableValentino();
	D = FindReachableDog();
	I = FindReachableItem(ItemDist);
	Plant = FindReachablePlant();
	const float AV = V ? AimAt(V->GetActorLocation()) : -3.f;
	const float AD = D ? AimAt(D->GetActorLocation()) : -3.f;
	const float AI = I ? AimAt(I->Mesh->Bounds.Origin) : -3.f;
	const float AP = Plant ? AimAt(Plant->GetProduceCentre()) : -3.f;
	// keep only the one aimed at most directly
	const float Best = FMath::Max(FMath::Max(AV, AD), FMath::Max(AI, AP));
	if (AV >= Best) { D = nullptr; I = nullptr; Plant = nullptr; }
	else if (AD >= Best) { V = nullptr; I = nullptr; Plant = nullptr; }
	else if (AI >= Best) { V = nullptr; D = nullptr; Plant = nullptr; }
	else { V = nullptr; D = nullptr; I = nullptr; }
}

void UFVCarryComponent::HoldItem(AFVPickupItem* Item)
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner || !Item)
	{
		return;
	}
	Item->PrepareForPickup();
	const FVector Half = Item->GetRestHalfExtent();
	const float Size = FMath::Max(Half.X, Half.Y);
	// hold small things at the hands, big things (benches) further out and lower
	const FVector Rel(30.f + Size, Item->Kind == EFVItemKind::WateringCan ? 22.f : 0.f, FMath::Clamp(15.f - Half.Z, -55.f, 10.f));
	// the watering can is always held spout-forward; other things keep the way they were facing
	const float RelYaw = Item->Kind == EFVItemKind::WateringCan ? 0.f : FRotator::NormalizeAxis(Item->GetRestYaw() - Owner->GetActorRotation().Yaw);
	Item->AttachToHolder(const_cast<ACharacter*>(Owner), Rel, RelYaw);
	HeldItem = Item;
}

bool UFVCarryComponent::FindCharacterDrop(ACharacter* Who, FVector& OutLocation) const
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner || !Who)
	{
		return false;
	}
	const UCapsuleComponent* Cap = Who->GetCapsuleComponent();
	const float HalfHeight = Cap->GetScaledCapsuleHalfHeight();
	const float Radius = Cap->GetScaledCapsuleRadius();
	const FVector Base = Owner->GetActorLocation() + Owner->GetActorForwardVector() * (PutDownDistance + Radius * 0.5f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FVCarryDrop), false, Owner);
	Params.AddIgnoredActor(Who);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Base + FVector(0, 0, 60), Base - FVector(0, 0, 250), ECC_Visibility, Params))
	{
		OutLocation = Hit.ImpactPoint + FVector(0, 0, HalfHeight + 1.f);
		const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
			OutLocation, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight - 1.f), Params);
		return !bBlocked;
	}
	return false;
}

void UFVCarryComponent::Pet()
{
	AFVDog* Dog = CarriedDog ? CarriedDog.Get() : PettableDog.Get();
	if (!Dog)
	{
		Dog = FindReachableDog();
	}
	if (Dog && Dog->Pet())
	{
		FVAudio::PlayAt(this, EFVSound::PetChime, Dog->GetActorLocation(), 0.5f, FMath::FRandRange(0.95f, 1.1f));
		if (FMath::FRand() < 0.3f)
		{
			Dog->Bark(); // a happy woof
		}
		++PetCount;
		LastPetTime = GetWorld()->GetTimeSeconds();
		LastPettedDog = Dog;
		if (PetCount % 4 == 1)
		{
			ShowToast(FText::Format(LOCTEXT("Petted", "{0} loves it! (+energy)"), Dog->DogName), 2.f);
		}
	}
}

AActor* UFVCarryComponent::GetCarriedActor() const
{
	if (Carried)
	{
		return Carried;
	}
	return CarriedDog;
}

FText UFVCarryComponent::GetPetPrompt() const
{
	const AFVDog* Dog = CarriedDog ? CarriedDog.Get() : PettableDog.Get();
	return Dog ? FText::Format(LOCTEXT("PetPrompt", "F  -  Pet {0}"), Dog->DogName) : FText::GetEmpty();
}

AFVPond* UFVCarryComponent::FindNearbyPond() const
{
	const AActor* Owner = GetOwner();
	for (TActorIterator<AFVPond> It(GetWorld()); It; ++It)
	{
		if (Owner && FVector::Dist2D(It->GetActorLocation(), Owner->GetActorLocation()) <= It->RefillRadius)
		{
			return *It;
		}
	}
	return nullptr;
}

void UFVCarryComponent::Interact()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner)
	{
		return;
	}
	// 0) carrying the dog -> put him down
	if (CarriedDog)
	{
		FVector Drop;
		if (FindCharacterDrop(CarriedDog, Drop))
		{
			CarriedDog->PutDown(Drop, Owner->GetActorRotation().Yaw);
			CarriedDog = nullptr;
			FVAudio::PlayAt(this, EFVSound::PutDown, Drop, 0.6f);
		}
		return;
	}
	// 1) carrying Valentino -> put him down
	if (Carried)
	{
		FVector Drop;
		if (FindValentinoDrop(Drop))
		{
			Carried->PutDown(Drop, Owner->GetActorRotation().Yaw);
			Carried = nullptr;
			FVAudio::PlayAt(this, EFVSound::PutDown, Drop, 0.5f, 1.2f);
		}
		return;
	}
	// 2) holding an item -> give food to Valentino, or put the item down
	if (HeldItem)
	{
		if (HeldItem->Kind == EFVItemKind::Food)
		{
			if (AFVValentino* V = FindReachableValentino())
			{
				const FText Food = HeldItem->GetLabel();
				const bool bWanted = V->bHasCraving && V->Craving == HeldItem->FoodType;
				if (V->Eat(HeldItem->FoodType, HeldItem->EnergyValue))
				{
					ShowToast(bWanted ? FText::Format(LOCTEXT("AteWanted", "Just what Valentino wanted!"), Food)
					                  : FVFood::IsTreat(HeldItem->FoodType) ? FText::Format(LOCTEXT("AteTreat", "Yum! Valentino gobbles the {0}!"), Food)
					                  : FText::Format(LOCTEXT("Ate", "Valentino munches the {0}!"), Food));
					HeldItem->Destroy();
					HeldItem = nullptr;
				}
				else
				{
					ShowToast(LOCTEXT("NotNow", "Valentino isn't interested right now."));
				}
				return;
			}
		}
		FVector Ground;
		float Yaw;
		if (FindItemDrop(Ground, Yaw))
		{
			bUseHeld = false;
			HeldItem->PlaceOnGround(Ground, Yaw);
			HeldItem = nullptr;
			FVAudio::PlayAt(this, EFVSound::PutDown, Ground, 0.7f);
		}
		return;
	}
	// 3) hands free -> pick up whatever you're looking at most directly: Valentino, the dog or an item
	AFVValentino* V;
	AFVDog* D;
	AFVPickupItem* Item;
	AFVForagePlant* Plant;
	ChooseCandidate(V, D, Item, Plant);
	if (Plant)
	{
		// pick food off the plant, straight into our hands
		Item = Plant->Pick();
		if (Item)
		{
			ShowToast(FText::Format(LOCTEXT("Picked", "Picked: {0}"), Item->GetLabel()), 2.f);
			if (UFVDaySubsystem* Day = GetWorld()->GetSubsystem<UFVDaySubsystem>())
			{
				Day->OnForaged(Item->FoodType);
			}
		}
	}
	if (V)
	{
		Carried = V;
		V->PickUp(Owner, CarryOffset);
	}
	else if (D)
	{
		CarriedDog = D;
		D->PickUp(Owner, DogCarryOffset);
	}
	else if (Item)
	{
		HoldItem(Item);
	}
	if (V || D || Item)
	{
		FVAudio::PlayAt(this, EFVSound::Pickup, Owner->GetActorLocation(), 0.6f, FMath::FRandRange(0.95f, 1.08f));
	}
	CandidateItem = nullptr;
	CandidateValentino = nullptr;
	CandidateDog = nullptr;
	CandidatePlant = nullptr;
}

void UFVCarryComponent::UseStarted()
{
	bUseHeld = true;
	if (HeldItem && HeldItem->Kind == EFVItemKind::Ball)
	{
		// throw it where you're looking, in a nice arc
		const ACharacter* Owner = Cast<ACharacter>(GetOwner());
		FRotator Aim = Owner->GetActorRotation();
		if (const AController* C = Owner->GetController())
		{
			Aim = C->GetControlRotation();
		}
		Aim.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Aim.Pitch) + 18.f, 5.f, 60.f);
		const FVector Start = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 45.f + FVector(0, 0, 45.f);
		HeldItem->Throw(Start, Aim.Vector() * ThrowSpeed + Owner->GetVelocity() * 0.5f, GetOwner());
		FVAudio::PlayAt(this, EFVSound::Pickup, Start, 0.6f, 1.4f);
		HeldItem = nullptr;
		bUseHeld = false;
		return;
	}
	if (!HeldItem || HeldItem->Kind != EFVItemKind::WateringCan)
	{
		return;
	}
	if (FindNearbyPond())
	{
		HeldItem->Water = HeldItem->WaterCapacity;
		bUseHeld = false;
		ShowToast(LOCTEXT("Refilled", "Watering can filled up at the pond."));
		FVAudio::PlayAt(this, EFVSound::Splash, HeldItem->GetActorLocation(), 0.9f);
		return;
	}
	if (HeldItem->Water <= 0.f)
	{
		ShowToast(LOCTEXT("Empty", "The watering can is empty - fill it at the pond."));
		return;
	}
	HeldItem->SetPouring(true);
}

void UFVCarryComponent::UseStopped()
{
	bUseHeld = false;
	if (HeldItem)
	{
		HeldItem->SetPouring(false);
	}
}

FText UFVCarryComponent::GetPrompt() const
{
	if (CarriedDog)
	{
		FVector Unused;
		return FindCharacterDrop(CarriedDog, Unused)
			? FText::Format(LOCTEXT("PutDog", "E  -  Put {0} down"), CarriedDog->DogName)
			: FText::Format(LOCTEXT("NoRoomDog", "No room to put {0} down here"), CarriedDog->DogName);
	}
	if (Carried)
	{
		FVector Unused;
		return FindValentinoDrop(Unused)
			? LOCTEXT("PutDown", "E  -  Put Valentino down")
			: LOCTEXT("NoRoom", "No room to put Valentino down here");
	}
	if (HeldItem)
	{
		const FText Name = HeldItem->GetLabel();
		if (HeldItem->Kind == EFVItemKind::Food && CandidateValentino)
		{
			return FText::Format(LOCTEXT("Give", "E  -  Give the {0} to Valentino"), Name);
		}
		FVector G;
		float Y;
		return FindItemDrop(G, Y)
			? FText::Format(LOCTEXT("PutItem", "E  -  Put down the {0}"), Name)
			: FText::Format(LOCTEXT("NoRoomItem", "No room to put the {0} down here"), Name);
	}
	if (CandidateValentino)
	{
		return LOCTEXT("PickUp", "E  -  Pick up Valentino");
	}
	if (CandidateDog)
	{
		return FText::Format(LOCTEXT("PickDog", "E  -  Pick up {0}"), CandidateDog->DogName);
	}
	if (CandidateItem)
	{
		return FText::Format(LOCTEXT("PickItem", "E  -  Pick up the {0}"), CandidateItem->GetLabel());
	}
	if (CandidatePlant)
	{
		return FText::Format(LOCTEXT("PickPlant", "E  -  Pick {0}"), FVFood::Name(CandidatePlant->FoodType));
	}
	return FText::GetEmpty();
}

FText UFVCarryComponent::GetUsePrompt() const
{
	if (HeldItem && HeldItem->Kind == EFVItemKind::Ball)
	{
		return LOCTEXT("UseThrow", "Left click  -  Throw the ball");
	}
	if (!HeldItem || HeldItem->Kind != EFVItemKind::WateringCan)
	{
		return FText::GetEmpty();
	}
	if (FindNearbyPond())
	{
		return LOCTEXT("UseRefill", "Left click  -  Fill the watering can");
	}
	const int32 Pct = FMath::RoundToInt(100.f * HeldItem->Water / FMath::Max(HeldItem->WaterCapacity, 0.01f));
	return FText::Format(LOCTEXT("UseWater", "Hold left click  -  Water   (can {0}% full)"), FText::AsNumber(Pct));
}

void UFVCarryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bInputBound)
	{
		TryBindInput();
	}
	ToastTimeLeft -= DeltaTime;
	ShowerToastCooldown -= DeltaTime;

	// what could E act on right now?
	CandidateValentino = nullptr;
	CandidateItem = nullptr;
	CandidateDog = nullptr;
	CandidatePlant = nullptr;
	PettableDog = nullptr;
	if (!Carried && !CarriedDog)
	{
		if (!HeldItem)
		{
			AFVValentino* V;
			AFVDog* D;
			AFVPickupItem* I;
			AFVForagePlant* Plant;
			ChooseCandidate(V, D, I, Plant);
			CandidateValentino = V;
			CandidateDog = D;
			CandidateItem = I;
			CandidatePlant = Plant;
			PettableDog = FindReachableDog(); // can pet him even if E would pick up something else
		}
		else if (HeldItem->Kind == EFVItemKind::Food)
		{
			CandidateValentino = FindReachableValentino();
		}
	}

	// pouring the watering can
	if (HeldItem && HeldItem->IsPouring())
	{
		HeldItem->Water = FMath::Max(0.f, HeldItem->Water - DeltaTime);
		if (HeldItem->Water <= 0.f)
		{
			HeldItem->SetPouring(false);
			ShowToast(LOCTEXT("RanOut", "The watering can is empty - fill it at the pond."));
			return;
		}
		const AActor* Owner = GetOwner();
		const FVector Target = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 90.f;
		for (TActorIterator<AFVPlantBed> It(GetWorld()); It; ++It)
		{
			if (FVector::Dist2D(It->GetActorLocation(), Target) <= It->Radius + 40.f)
			{
				It->AddWater(DeltaTime);
			}
		}
		for (TActorIterator<AFVValentino> It(GetWorld()); It; ++It)
		{
			if (!It->IsHeld() && FVector::Dist2D(It->GetActorLocation(), Target) <= WaterReach * 0.6f)
			{
				It->Drink(DeltaTime * 2.f);
				if (ShowerToastCooldown <= 0.f)
				{
					ShowToast(LOCTEXT("Shower", "Valentino enjoys a little shower!"));
					ShowerToastCooldown = 8.f;
				}
			}
		}
	}
	else if (bUseHeld && HeldItem && HeldItem->Kind == EFVItemKind::WateringCan && HeldItem->Water > 0.f && !FindNearbyPond())
	{
		HeldItem->SetPouring(true);
	}
}

#undef LOCTEXT_NAMESPACE


