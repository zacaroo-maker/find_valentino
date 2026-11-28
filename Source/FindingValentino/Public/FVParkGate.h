// Finding Valentino - the gate into the dog park. It stays shut until enough garden points have been earned
// (all days added up), then swings open with a fanfare and stays open from then on.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVParkGate.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class FINDINGVALENTINO_API AFVParkGate : public AActor
{
	GENERATED_BODY()

public:
	AFVParkGate();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Garden points (earned in total, over all days) that open the gate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate")
	int32 PointsToOpen = 1000;

	/** The door swings around this (at the left post). */
	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<USceneComponent> Hinge;
	/** Tall invisible wall in the gateway while it's locked, so nobody hops over. */
	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UBoxComponent> Blocker;
	/** "OPENS AT 1000 POINTS / 380 TO GO", facing the garden. */
	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UTextRenderComponent> Sign;

	bool IsOpen() const { return OpenTarget > 0.5f; }
	/** Open the gate (bCelebrate: swing it open with a sound and a banner; otherwise it's simply open). */
	void Unlock(bool bCelebrate);
	/** Testing: shut and lock it again. */
	void Relock();

	static AFVParkGate* Find(const UObject* WorldContext);

private:
	void UpdateSign();
	int32 PointsToGo() const;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> DoorParts;
	float OpenAlpha = 0.f;
	float OpenTarget = 0.f;
	float SignTimer = 0.f;
	bool bPlayerWasNear = false;
};
