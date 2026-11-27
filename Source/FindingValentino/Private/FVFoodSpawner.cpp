#include "FVFoodSpawner.h"

#include "Components/BillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
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

AFVFoodSpawner* AFVFoodSpawner::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AFVFoodSpawner> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

EFVFoodType AFVFoodSpawner::RandomType() const
{
	// fallen bits: mostly greens and flowers, the odd treat
	static const float W[(int32)EFVFoodType::Count] = { 3.f, 3.f, 1.f, 1.f, 2.f, 2.f };
	float Total = 0.f;
	for (float X : W) { Total += X; }
	float Pick = FMath::FRand() * Total;
	for (int32 i = 0; i < (int32)EFVFoodType::Count; ++i)
	{
		Pick -= W[i];
		if (Pick <= 0.f)
		{
			return (EFVFoodType)i;
		}
	}
	return EFVFoodType::Romaine;
}

AFVPickupItem* AFVFoodSpawner::SpawnFood(EFVFoodType Type, const FTransform& Where)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	AFVPickupItem* Item = World->SpawnActorDeferred<AFVPickupItem>(AFVPickupItem::StaticClass(), Where);
	if (!Item)
	{
		return nullptr;
	}
	Item->Kind = EFVItemKind::Food;
	Item->FoodType = Type;
	Item->EnergyValue = FVFood::Energy(Type);
	if (const TObjectPtr<UStaticMesh>* M = FoodMeshes.Find(Type))
	{
		Item->Mesh->SetStaticMesh(*M);
	}
	Item->Tags.Add(TEXT("FV_Food"));
	Item->FinishSpawning(Where);
	return Item;
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
	return SpawnFood(Type, FTransform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Ground));
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
