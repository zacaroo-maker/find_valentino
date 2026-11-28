#include "FVParkGate.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FVAudio.h"
#include "FVDay.h"
#include "FVDecor.h"
#include "FVSaveGame.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The gateway in the garden fence: posts at x = -130 and +130 from the actor, door 240 wide.
	constexpr float HingeX = -120.f;
	constexpr float DoorWidth = 240.f;
	constexpr float OpenYaw = 100.f;     // swings into the park
	constexpr float SwingSeconds = 1.6f;
	constexpr float NearDistance = 380.f;
}

AFVParkGate::AFVParkGate()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(RootComponent);
	Hinge->SetRelativeLocation(FVector(HingeX, 0.f, 0.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/Environment/DogPark/MI_Park_Wood.MI_Park_Wood"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Yellow(TEXT("/Game/Environment/DogPark/MI_Park_Yellow.MI_Park_Yellow"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Red(TEXT("/Game/Environment/DogPark/MI_Park_Red.MI_Park_Red"));

	// a cube is 100 units on a side with its pivot in the middle
	auto Part = [&](const TCHAR* Name, const FVector& Centre, const FVector& Size, UMaterialInterface* Mat, bool bCollide)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Hinge);
		C->SetStaticMesh(Cube.Object);
		C->SetRelativeLocation(Centre);
		C->SetRelativeScale3D(Size / 100.f);
		if (Mat)
		{
			C->SetMaterial(0, Mat);
		}
		C->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		C->SetCanEverAffectNavigation(bCollide);
		DoorParts.Add(C);
		return C;
	};
	// six chunky slats, two cross rails, and a big yellow padlock with a red shackle
	for (int32 i = 0; i < 6; ++i)
	{
		const float H = (i == 0 || i == 5) ? 150.f : 140.f;
		Part(*FString::Printf(TEXT("Slat%d"), i), FVector(20.f + i * 40.f, 0.f, 10.f + H * 0.5f), FVector(30.f, 7.f, H), Wood.Object, true);
	}
	Part(TEXT("RailLow"), FVector(DoorWidth * 0.5f, -6.f, 45.f), FVector(DoorWidth - 4.f, 5.f, 14.f), Wood.Object, false);
	Part(TEXT("RailHigh"), FVector(DoorWidth * 0.5f, -6.f, 125.f), FVector(DoorWidth - 4.f, 5.f, 14.f), Wood.Object, false);
	Part(TEXT("Lock"), FVector(DoorWidth - 20.f, -12.f, 82.f), FVector(18.f, 7.f, 20.f), Yellow.Object, false);
	Part(TEXT("Shackle"), FVector(DoorWidth - 20.f, -12.f, 97.f), FVector(10.f, 4.f, 10.f), Red.Object, false);

	Blocker = CreateDefaultSubobject<UBoxComponent>(TEXT("Blocker"));
	Blocker->SetupAttachment(RootComponent);
	Blocker->SetBoxExtent(FVector(125.f, 12.f, 250.f));
	Blocker->SetRelativeLocation(FVector(0.f, 0.f, 250.f));
	Blocker->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Blocker->SetCanEverAffectNavigation(false);

	Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
	Sign->SetupAttachment(RootComponent);
	Sign->SetRelativeLocation(FVector(0.f, -16.f, 192.f));
	Sign->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // readable from the garden (south)
	Sign->SetHorizontalAlignment(EHTA_Center);
	Sign->SetVerticalAlignment(EVRTA_TextCenter);
	Sign->SetWorldSize(20.f);
	Sign->SetTextRenderColor(FColor(255, 225, 40));
	Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// a dark wooden board behind the words
	UStaticMeshComponent* Board = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SignBoard"));
	Board->SetupAttachment(RootComponent);
	Board->SetStaticMesh(Cube.Object);
	Board->SetRelativeLocation(FVector(0.f, -11.f, 192.f));
	Board->SetRelativeScale3D(FVector(3.2f, 0.05f, 0.52f));
	if (Wood.Object)
	{
		Board->SetMaterial(0, Wood.Object);
	}
	Board->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Board->SetCanEverAffectNavigation(false);
}

AFVParkGate* AFVParkGate::Find(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (World)
	{
		for (TActorIterator<AFVParkGate> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void AFVParkGate::BeginPlay()
{
	Super::BeginPlay();
	const UFVSaveGame* Save = UFVSaveGame::Load();
	if (Save && Save->bParkUnlocked)
	{
		Unlock(false);
	}
	UpdateSign();
}

int32 AFVParkGate::PointsToGo() const
{
	const UFVDecorSubsystem* Decor = GetWorld() ? GetWorld()->GetSubsystem<UFVDecorSubsystem>() : nullptr;
	const int32 Earned = Decor ? Decor->GetTotalEarned() : 0;
	return FMath::Max(0, PointsToOpen - Earned);
}

void AFVParkGate::UpdateSign()
{
	if (IsOpen())
	{
		Sign->SetText(FText::FromString(TEXT("COME ON IN!")));
		return;
	}
	Sign->SetText(FText::FromString(FString::Printf(TEXT("OPENS AT %d GARDEN POINTS\n%d TO GO"), PointsToOpen, PointsToGo())));
}

void AFVParkGate::Unlock(bool bCelebrate)
{
	if (IsOpen())
	{
		return;
	}
	OpenTarget = 1.f;
	Blocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (!bCelebrate)
	{
		OpenAlpha = 1.f;
		Hinge->SetRelativeRotation(FRotator(0.f, OpenYaw, 0.f));
	}
	else
	{
		FVAudio::Play2D(this, EFVSound::UIStart, 0.6f);
		FVAudio::PlayAt(this, EFVSound::Bark, GetActorLocation() + FVector(0.f, 400.f, 50.f), 0.9f, 1.f, true);
		if (UFVDaySubsystem* Day = GetWorld()->GetSubsystem<UFVDaySubsystem>())
		{
			Day->Notice(TEXT("THE DOG PARK IS OPEN!"), 4.f);
			Day->AddPopup(TEXT("TAKE DIEGO TO PLAY!"), true);
		}
	}
	UpdateSign();
}

void AFVParkGate::Relock()
{
	OpenTarget = 0.f;
	OpenAlpha = 0.f;
	Hinge->SetRelativeRotation(FRotator::ZeroRotator);
	Blocker->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	UpdateSign();
}

void AFVParkGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!FMath::IsNearlyEqual(OpenAlpha, OpenTarget))
	{
		OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, OpenTarget, DeltaSeconds, 1.f / SwingSeconds);
		const float Eased = FMath::InterpEaseInOut(0.f, 1.f, OpenAlpha, 2.f);
		Hinge->SetRelativeRotation(FRotator(0.f, OpenYaw * Eased, 0.f));
	}

	SignTimer -= DeltaSeconds;
	if (SignTimer <= 0.f)
	{
		SignTimer = 0.5f;
		UpdateSign();
	}

	// walking up to the locked gate tells you how far off it is
	if (!IsOpen())
	{
		const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
		const bool bNear = Player && FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) < NearDistance;
		if (bNear && !bPlayerWasNear)
		{
			if (UFVDaySubsystem* Day = GetWorld()->GetSubsystem<UFVDaySubsystem>())
			{
				Day->AddPopup(FString::Printf(TEXT("LOCKED - %d MORE GARDEN POINTS OPENS THE DOG PARK"), PointsToGo()));
			}
			FVAudio::Play2D(this, EFVSound::UIBack, 0.35f);
		}
		bPlayerWasNear = bNear;
	}
}
