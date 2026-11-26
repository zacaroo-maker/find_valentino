#include "FVFoodSpawner.h"

#include "Components/BillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FVHidingSpot.h"
#include "FVPickupItem.h"
#include "Kismet/GameplayStatics.h"

AFVFoodSpawner::AFVFoodSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(Root);
	}
#endif
}

UStaticMesh* AFVFoodSpawner::MeshFor(EFVFoodType T) const
{
	switch (T)
	{
	case EFVFoodType::Lettuce:    return LettuceMesh;
	case EFVFoodType::Dandelion:  return DandelionMesh;
	case EFVFoodType::Strawberry: return StrawberryMesh;
	case EFVFoodType::Worm:       return WormMesh;
	}
	return nullptr;
}

EFVFoodType AFVFoodSpawner::RandomType() const
{
	const float W[4] = { (float)Weights.X, (float)Weights.Y, (float)Weights.Z, (float)Weights.W };
	const float Total = W[0] + W[1] + W[2] + W[3];
	float Pick = FMath::FRand() * FMath::Max(Total, 0.01f);
	for (int32 i = 0; i < 4; ++i)
	{
		Pick -= W[i];
		if (Pick <= 0.f)
		{
			return (EFVFoodType)i;
		}
	}
	return EFVFoodType::Lettuce;
}

AFVPickupItem* AFVFoodSpawner::SpawnFoodAtGround(EFVFoodType Type, FVector NearLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	// find the ground (ignores foliage, which has no collision)
	FVector Ground = NearLocation;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FVFoodGround), true);
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Params.AddIgnoredActor(P);
	}
	if (World->LineTraceSingleByChannel(Hit, NearLocation + FVector(0, 0, 120), NearLocation - FVector(0, 0, 300), ECC_Visibility, Params))
	{
		Ground = Hit.ImpactPoint;
	}
	const FTransform T(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Ground);
	AFVPickupItem* Item = World->SpawnActorDeferred<AFVPickupItem>(AFVPickupItem::StaticClass(), T);
	if (!Item)
	{
		return nullptr;
	}
	Item->Kind = EFVItemKind::Food;
	Item->FoodType = Type;
	Item->EnergyValue = FVFood::Energy(Type);
	Item->Mesh->SetStaticMesh(MeshFor(Type));
	Item->Tags.Add(TEXT("FV_Food"));
	Item->FinishSpawning(T);
	return Item;
}

void AFVFoodSpawner::BeginPlay()
{
	Super::BeginPlay();
	TArray<AFVHidingSpot*> Spots;
	for (TActorIterator<AFVHidingSpot> It(GetWorld()); It; ++It)
	{
		Spots.Add(*It);
	}
	if (Spots.Num() == 0)
	{
		return;
	}
	// different spots each game
	for (int32 i = Spots.Num() - 1; i > 0; --i)
	{
		Spots.Swap(i, FMath::RandHelper(i + 1));
	}
	for (int32 i = 0; i < StartCount; ++i)
	{
		const AFVHidingSpot* S = Spots[i % Spots.Num()];
		const float A = FMath::FRandRange(0.f, 2.f * PI);
		const float D = FMath::FRandRange(30.f, ScatterRadius);
		SpawnFoodAtGround(RandomType(), S->GetActorLocation() + FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, 0.f));
	}
}
