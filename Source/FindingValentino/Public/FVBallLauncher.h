// Finding Valentino - the dog park's ball launcher: put a ball down at it (or let Diego drop one at your feet
// while you stand beside it) and it swivels round, wiggles, and fires the ball across the park for him to chase.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVBallLauncher.generated.h"

class AFVPickupItem;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class FINDINGVALENTINO_API AFVBallLauncher : public AActor
{
	GENERATED_BODY()

public:
	AFVBallLauncher();
	virtual void Tick(float DeltaSeconds) override;

	/** The launcher's body (barrel along +X). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Launcher")
	TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Launcher")
	TObjectPtr<UTextRenderComponent> Sign;

	/** A ball resting this close (sideways) gets loaded. */
	UPROPERTY(EditAnywhere, Category = "Launcher")
	float LoadRadius = 200.f;
	/** Where the ball leaves the barrel, relative to the launcher. */
	UPROPERTY(EditAnywhere, Category = "Launcher")
	FVector MuzzleOffset = FVector(100.f, 0.f, 110.f);
	/** How far the ball lands (never closer than Min). */
	UPROPERTY(EditAnywhere, Category = "Launcher")
	float MinRange = 700.f;
	UPROPERTY(EditAnywhere, Category = "Launcher")
	float MaxRange = 1400.f;

private:
	enum class EState : uint8 { Idle, Aiming, Cooldown };
	EState State = EState::Idle;
	float Timer = 0.f;
	float AimFromYaw = 0.f;
	float AimToYaw = 0.f;
	FVector Target = FVector::ZeroVector;
	TWeakObjectPtr<AFVPickupItem> Loaded;

	AFVPickupItem* FindBallToLoad() const;
	FVector PickTarget() const;
	void Fire();
};
