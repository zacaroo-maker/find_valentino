#include "FVPickupItem.h"

#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "FVAudio.h"

AFVPickupItem::AFVPickupItem()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);

	WaterStream = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterStream"));
	WaterStream->SetupAttachment(Mesh);
	WaterStream->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterStream->SetCanEverAffectNavigation(false);
	WaterStream->SetCastShadow(false);
	WaterStream->SetVisibility(false);
}

void AFVPickupItem::BeginPlay()
{
	Super::BeginPlay();
	Water = WaterCapacity;
	ApplyCollision();
	WaterStream->SetVisibility(false);
}

void AFVPickupItem::ApplyCollision()
{
	if (Kind == EFVItemKind::Food)
	{
		// food never blocks anyone (Valentino walks over it)
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
	}
	else
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
		Mesh->SetCanEverAffectNavigation(Kind == EFVItemKind::Prop);
	}
}

FText AFVPickupItem::GetLabel() const
{
	if (Kind == EFVItemKind::Food)
	{
		return FVFood::Name(FoodType);
	}
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}
	return Kind == EFVItemKind::WateringCan ? NSLOCTEXT("FV", "Can", "watering can") : NSLOCTEXT("FV", "Item", "item");
}

void AFVPickupItem::PrepareForPickup()
{
	RestRotation = GetActorRotation();
	FVector Origin, Extent;
	Origin = Mesh->Bounds.Origin; Extent = Mesh->Bounds.BoxExtent; // own mesh only
	RestHalfExtent = Extent;
	BottomOffset = GetActorLocation().Z - (Origin.Z - Extent.Z);
}

void AFVPickupItem::AttachToHolder(AActor* Holder, const FVector& RelLocation, float RelYaw)
{
	bHeld = true;
	HeldRelYaw = RelYaw;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachToActor(Holder, FAttachmentTransformRules::KeepWorldTransform);
	SetActorRelativeLocation(RelLocation);
	SetActorRelativeRotation(FRotator(RestRotation.Pitch, RelYaw, RestRotation.Roll));
}

void AFVPickupItem::PlaceOnGround(const FVector& GroundPoint, float WorldYaw)
{
	SetPouring(false);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocationAndRotation(GroundPoint + FVector(0, 0, BottomOffset + 0.5f),
		FRotator(RestRotation.Pitch, WorldYaw, RestRotation.Roll), false, nullptr, ETeleportType::TeleportPhysics);
	bHeld = false;
	ApplyCollision();
}

void AFVPickupItem::SetPouring(bool bPour)
{
	bPour = bPour && bHeld && Kind == EFVItemKind::WateringCan && Water > 0.f;
	if (bPour == bPouring)
	{
		return;
	}
	bPouring = bPour;
	WaterStream->SetVisibility(bPouring);
	// the sound of water pouring, from the spout
	if (bPouring && !PourAudio)
	{
		PourAudio = FVAudio::PlayAttached(EFVSound::PourLoop, WaterStream, 0.6f);
	}
	else if (!bPouring && PourAudio)
	{
		PourAudio->FadeOut(0.25f, 0.f);
		PourAudio = nullptr;
	}
	if (bHeld)
	{
		// tip the can forward while pouring
		SetActorRelativeRotation(FRotator(RestRotation.Pitch + (bPouring ? -35.f : 0.f), HeldRelYaw, RestRotation.Roll));
	}
}


