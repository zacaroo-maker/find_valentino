#include "FVBallLauncher.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FVAudio.h"
#include "FVDogPark.h"
#include "FVPickupItem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float AimSeconds = 1.3f;
	constexpr float CooldownSeconds = 1.5f;
}

AFVBallLauncher::AFVBallLauncher()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
	Sign->SetupAttachment(RootComponent);
	Sign->SetRelativeLocation(FVector(0.f, 0.f, 165.f));
	Sign->SetHorizontalAlignment(EHTA_Center);
	Sign->SetVerticalAlignment(EVRTA_TextCenter);
	Sign->SetWorldSize(16.f);
	Sign->SetTextRenderColor(FColor(255, 225, 40));
	Sign->SetText(FText::FromString(TEXT("BALL LAUNCHER\nDROP A BALL HERE!")));
	Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

AFVPickupItem* AFVBallLauncher::FindBallToLoad() const
{
	const FVector Here = GetActorLocation();
	for (TActorIterator<AFVPickupItem> It(GetWorld()); It; ++It)
	{
		AFVPickupItem* B = *It;
		if (B->Kind != EFVItemKind::Ball || B->IsHeld() || B->IsInMouth())
		{
			continue;
		}
		const FVector P = B->GetActorLocation();
		if (FVector::Dist2D(P, Here) < LoadRadius && P.Z < Here.Z + 180.f && P.Z > Here.Z - 30.f
			&& B->Mesh->GetPhysicsLinearVelocity().Size() < 120.f)
		{
			return B;
		}
	}
	return nullptr;
}

FVector AFVBallLauncher::PickTarget() const
{
	const FVector Here = GetActorLocation();
	const AFVDogPark* Park = AFVDogPark::FindAt(this, Here);
	FVector Best = Here + GetActorForwardVector() * MinRange;
	for (int32 Try = 0; Try < 20; ++Try)
	{
		FVector P;
		if (Park)
		{
			P = Park->RandomPoint();
		}
		else
		{
			const float A = FMath::FRandRange(0.f, 2.f * PI);
			P = Here + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * FMath::FRandRange(MinRange, MaxRange);
		}
		P.Z = Here.Z;
		const float D = FVector::Dist2D(P, Here);
		if (D >= MinRange && D <= MaxRange)
		{
			return P;
		}
		Best = P;
	}
	return Best;
}

void AFVBallLauncher::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer -= DeltaSeconds;

	switch (State)
	{
	case EState::Idle:
		if (Timer <= 0.f)
		{
			Timer = 0.25f; // no need to look every frame
			if (AFVPickupItem* B = FindBallToLoad())
			{
				// gulp: the ball goes into the funnel
				Loaded = B;
				B->Mesh->SetSimulatePhysics(false);
				B->Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				B->SetActorLocation(GetActorLocation() + FVector(0.f, 0.f, 112.f));
				B->SetActorHiddenInGame(true);
				FVAudio::PlayAt(this, EFVSound::PutDown, GetActorLocation(), 0.8f, 0.7f);
				Target = PickTarget();
				AimFromYaw = GetActorRotation().Yaw;
				AimToYaw = AimFromYaw + FMath::FindDeltaAngleDegrees(AimFromYaw, (Target - GetActorLocation()).Rotation().Yaw);
				State = EState::Aiming;
				Timer = AimSeconds;
			}
		}
		break;

	case EState::Aiming:
	{
		// swivel to face the landing spot, with an excited wiggle
		const float A = FMath::Clamp(1.f - Timer / AimSeconds, 0.f, 1.f);
		const float Yaw = FMath::Lerp(AimFromYaw, AimToYaw, FMath::InterpEaseInOut(0.f, 1.f, FMath::Min(A * 1.4f, 1.f), 2.f));
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
		const float Wiggle = A > 0.6f ? FMath::Sin(A * 60.f) * 0.04f : 0.f;
		Body->SetRelativeScale3D(FVector(1.f - Wiggle, 1.f + Wiggle, 1.f + Wiggle));
		if (Timer <= 0.f)
		{
			Body->SetRelativeScale3D(FVector::OneVector);
			Fire();
			State = EState::Cooldown;
			Timer = CooldownSeconds;
		}
		break;
	}

	case EState::Cooldown:
		if (Timer <= 0.f)
		{
			State = EState::Idle;
		}
		break;
	}
}

void AFVBallLauncher::Fire()
{
	AFVPickupItem* B = Loaded.Get();
	Loaded = nullptr;
	if (!B)
	{
		return;
	}
	const FVector Start = GetActorTransform().TransformPosition(MuzzleOffset);
	FVector Velocity;
	const float GravityZ = GetWorld()->GetGravityZ();
	if (!UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, Velocity, Start, Target, GravityZ, 0.42f))
	{
		Velocity = (GetActorForwardVector() + FVector(0.f, 0.f, 0.8f)).GetSafeNormal() * 1100.f;
	}
	B->SetActorHiddenInGame(false);
	B->Throw(Start, Velocity, this);
	FVAudio::PlayAt(this, EFVSound::Pickup, Start, 1.f, 0.55f);
	FVAudio::PlayAt(this, EFVSound::UIStart, Start, 0.5f, 1.3f);
}
