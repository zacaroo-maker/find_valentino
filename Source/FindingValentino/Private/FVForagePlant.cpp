#include "FVForagePlant.h"

#include "Components/StaticMeshComponent.h"
#include "FVAudio.h"
#include "FVFoodSpawner.h"
#include "FVPickupItem.h"

AFVForagePlant::AFVForagePlant()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only ticks while growing back

	PlantMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plant"));
	SetRootComponent(PlantMesh);
	ProduceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Produce"));
	ProduceMesh->SetupAttachment(PlantMesh);
	// plants never block anyone and don't cut holes in the navmesh (Valentino walks through the leaves)
	for (UStaticMeshComponent* C : { PlantMesh.Get(), ProduceMesh.Get() })
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
	}
}

void AFVForagePlant::BeginPlay()
{
	Super::BeginPlay();
	SparklePhase = FMath::FRand();
	bRipe = true;
	ProduceMesh->SetVisibility(true);
}

FVector AFVForagePlant::GetProduceCentre() const
{
	return ProduceMesh->GetStaticMesh() ? ProduceMesh->Bounds.Origin : GetActorLocation() + FVector(0, 0, 15.f);
}

AFVPickupItem* AFVForagePlant::Pick()
{
	if (!bRipe)
	{
		return nullptr;
	}
	AFVFoodSpawner* Spawner = AFVFoodSpawner::Get(this);
	if (!Spawner)
	{
		return nullptr;
	}
	AFVPickupItem* Item = Spawner->SpawnFood(FoodType, FTransform(GetActorRotation(), GetProduceCentre()));
	if (!Item)
	{
		return nullptr;
	}
	bRipe = false;
	RegrowLeft = RegrowSeconds * FMath::FRandRange(0.85f, 1.15f);
	ProduceMesh->SetVisibility(false);
	SetActorTickEnabled(true);
	FVAudio::PlayAt(this, EFVSound::Nibble, GetProduceCentre(), 0.35f, 1.6f); // a little "snip"
	return Item;
}

void AFVForagePlant::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bRipe)
	{
		RegrowLeft -= DeltaSeconds;
		if (RegrowLeft <= 0.f)
		{
			bRipe = true;
			GrowAnim = 0.f;
			ProduceMesh->SetVisibility(true);
			ProduceMesh->SetRelativeScale3D(FVector(0.05f));
		}
		return;
	}
	// grow back with a gentle pop
	GrowAnim = FMath::Min(1.f, GrowAnim + DeltaSeconds / 1.5f);
	const float S = FMath::InterpEaseOut(0.05f, 1.f, GrowAnim, 2.5f);
	ProduceMesh->SetRelativeScale3D(FVector(S));
	if (GrowAnim >= 1.f)
	{
		SetActorTickEnabled(false);
	}
}
