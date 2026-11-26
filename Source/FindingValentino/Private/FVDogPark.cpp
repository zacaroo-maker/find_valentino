#include "FVDogPark.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FVDog.h"

AFVDogPark::AFVDogPark()
{
	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetBoxExtent(FVector(1400.f, 900.f, 300.f));
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->ShapeColor = FColor(255, 170, 40);
	RootComponent = Area;
}

bool AFVDogPark::Contains(const FVector& Location) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Location);
	const FVector E = Area->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= E.X && FMath::Abs(Local.Y) <= E.Y && FMath::Abs(Local.Z) <= E.Z;
}

FVector AFVDogPark::RandomPoint() const
{
	const FVector E = Area->GetUnscaledBoxExtent();
	const FVector Local(FMath::FRandRange(-E.X * 0.85f, E.X * 0.85f), FMath::FRandRange(-E.Y * 0.85f, E.Y * 0.85f), 0.f);
	return GetActorTransform().TransformPosition(Local);
}

TArray<AFVDog*> AFVDogPark::DogsInside() const
{
	TArray<AFVDog*> Out;
	for (TActorIterator<AFVDog> It(GetWorld()); It; ++It)
	{
		if (!It->IsHeld() && Contains(It->GetActorLocation()))
		{
			Out.Add(*It);
		}
	}
	return Out;
}

AFVDogPark* AFVDogPark::FindAt(const UObject* WorldContext, const FVector& Location)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AFVDogPark> It(World); It; ++It)
	{
		if (It->Contains(Location))
		{
			return *It;
		}
	}
	return nullptr;
}
