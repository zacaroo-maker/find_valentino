#include "FVPlantBed.h"

#include "Components/BillboardComponent.h"
#include "EngineUtils.h"
#include "FVCarryComponent.h"
#include "FVFoodSpawner.h"
#include "Kismet/GameplayStatics.h"

AFVPlantBed::AFVPlantBed()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(Root);
		Sprite->SetRelativeLocation(FVector(0, 0, 50));
	}
#endif
}

static void FVToast(UWorld* World, const FText& Msg)
{
	if (APawn* P = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		if (UFVCarryComponent* C = P->FindComponentByClass<UFVCarryComponent>())
		{
			C->ShowToast(Msg, 4.f);
		}
	}
}

bool AFVPlantBed::AddWater(float Seconds)
{
	const bool bWasFull = Wetness >= 1.f;
	Wetness = FMath::Min(1.f, Wetness + Seconds / FMath::Max(WaterNeededSeconds, 0.1f));
	if (!bWasFull && Wetness >= 1.f)
	{
		if (SproutTimeLeft < 0.f)
		{
			SproutTimeLeft = SproutDelay;
		}
		FVToast(GetWorld(), FText::Format(NSLOCTEXT("FV", "Watered", "The {0} is watered - something will grow soon!"), DisplayName));
		return true;
	}
	return false;
}

void AFVPlantBed::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Wetness = FMath::Max(0.f, Wetness - DeltaSeconds / FMath::Max(DrySeconds, 1.f));
	if (SproutTimeLeft >= 0.f)
	{
		SproutTimeLeft -= DeltaSeconds;
		if (SproutTimeLeft < 0.f)
		{
			Sprout();
		}
	}
}

void AFVPlantBed::Sprout()
{
	AFVFoodSpawner* Spawner = nullptr;
	for (TActorIterator<AFVFoodSpawner> It(GetWorld()); It; ++It)
	{
		Spawner = *It;
		break;
	}
	if (!Spawner || SproutTypes.Num() == 0)
	{
		return;
	}
	for (int32 i = 0; i < FoodPerWatering; ++i)
	{
		const float A = FMath::FRandRange(0.f, 2.f * PI);
		const float D = Radius * 0.6f * FMath::Sqrt(FMath::FRand());
		const FVector P = GetActorLocation() + FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, 0.f);
		Spawner->SpawnFoodAtGround(SproutTypes[FMath::RandHelper(SproutTypes.Num())], P);
	}
	FVToast(GetWorld(), FText::Format(NSLOCTEXT("FV", "Sprouted", "Something new grew in the {0}!"), DisplayName));
}
