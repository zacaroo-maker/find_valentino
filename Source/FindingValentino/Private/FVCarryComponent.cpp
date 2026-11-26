#include "FVCarryComponent.h"

#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EngineUtils.h"
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
	AFVPickupItem* Best = nullptr;
	for (TActorIterator<AFVPickupItem> It(GetWorld()); It; ++It)
	{
		AFVPickupItem* I = *It;
		if (I->IsHeld())
		{
			continue;
		}
		FVector O, E;
		I->GetActorBounds(true, O, E);
		// distance (2D) from us to the nearest edge of the item, so big things like benches work from either end
		const FVector2D Near(FMath::Clamp(P.X, O.X - E.X, O.X + E.X), FMath::Clamp(P.Y, O.Y - E.Y, O.Y + E.Y));
		const float Dist = FVector2D::Distance(FVector2D(P), Near);
		if (Dist > ItemReach || FMath::Abs(O.Z - P.Z) > 200.f)
		{
			continue;
		}
		const FVector To = FVector(Near.X - P.X, Near.Y - P.Y, 0.f);
		const bool bInView = To.IsNearlyZero(5.f) || FVector::DotProduct(ViewDir, To.GetSafeNormal()) >= CosLimit;
		if ((bInView || Dist < 40.f) && Dist < OutDist)
		{
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
	// 1) carrying Valentino -> put him down
	if (Carried)
	{
		FVector Drop;
		if (FindValentinoDrop(Drop))
		{
			Carried->PutDown(Drop, Owner->GetActorRotation().Yaw);
			Carried = nullptr;
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
				if (V->Eat(HeldItem->FoodType, HeldItem->EnergyValue))
				{
					ShowToast(FText::Format(LOCTEXT("Ate", "Valentino munches the {0}!"), Food));
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
		}
		return;
	}
	// 3) hands free -> pick up whichever is closer: Valentino or an item
	float ItemDist;
	AFVPickupItem* Item = FindReachableItem(ItemDist);
	AFVValentino* V = FindReachableValentino();
	const float VDist = V ? FVector::Dist2D(V->GetActorLocation(), Owner->GetActorLocation()) : TNumericLimits<float>::Max();
	if (V && (!Item || VDist <= ItemDist + 20.f))
	{
		Carried = V;
		V->PickUp(Owner, CarryOffset);
	}
	else if (Item)
	{
		Item->PrepareForPickup();
		const FVector Half = Item->GetRestHalfExtent();
		const float Size = FMath::Max(Half.X, Half.Y);
		// hold small things at the hands, big things (benches) further out and lower
		const FVector Rel(30.f + Size, Item->Kind == EFVItemKind::WateringCan ? 22.f : 0.f, FMath::Clamp(15.f - Half.Z, -55.f, 10.f));
		const float RelYaw = FRotator::NormalizeAxis(Item->GetRestYaw() - Owner->GetActorRotation().Yaw);
		Item->AttachToHolder(Owner, Rel, RelYaw);
		HeldItem = Item;
	}
	CandidateItem = nullptr;
	CandidateValentino = nullptr;
}

void UFVCarryComponent::UseStarted()
{
	bUseHeld = true;
	if (!HeldItem || HeldItem->Kind != EFVItemKind::WateringCan)
	{
		return;
	}
	if (FindNearbyPond())
	{
		HeldItem->Water = HeldItem->WaterCapacity;
		bUseHeld = false;
		ShowToast(LOCTEXT("Refilled", "Watering can filled up at the pond."));
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
	if (CandidateItem)
	{
		return FText::Format(LOCTEXT("PickItem", "E  -  Pick up the {0}"), CandidateItem->GetLabel());
	}
	return FText::GetEmpty();
}

FText UFVCarryComponent::GetUsePrompt() const
{
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
	if (!Carried)
	{
		CandidateValentino = FindReachableValentino();
		if (!HeldItem)
		{
			float ItemDist;
			CandidateItem = FindReachableItem(ItemDist);
			if (CandidateValentino && CandidateItem)
			{
				const float VDist = FVector::Dist2D(CandidateValentino->GetActorLocation(), GetOwner()->GetActorLocation());
				if (VDist <= ItemDist + 20.f)
				{
					CandidateItem = nullptr;
				}
				else
				{
					CandidateValentino = nullptr;
				}
			}
		}
		else if (HeldItem->Kind != EFVItemKind::Food)
		{
			CandidateValentino = nullptr;
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
