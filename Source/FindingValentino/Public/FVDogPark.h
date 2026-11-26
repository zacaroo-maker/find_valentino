// Finding Valentino - the dog park next door: an area where dogs (Diego included, once you bring him) play, chase and bark.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVDogPark.generated.h"

class UBoxComponent;
class AFVDog;

UCLASS()
class FINDINGVALENTINO_API AFVDogPark : public AActor
{
	GENERATED_BODY()

public:
	AFVDogPark();

	/** The play area (no collision; dogs inside switch to park behaviour). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dog Park")
	TObjectPtr<UBoxComponent> Area;

	UFUNCTION(BlueprintPure, Category = "Dog Park")
	bool Contains(const FVector& Location) const;

	/** A random point inside the park, kept away from the fence. */
	FVector RandomPoint() const;

	/** Dogs currently inside (not being carried). */
	TArray<AFVDog*> DogsInside() const;

	/** The park containing Location, if any. */
	static AFVDogPark* FindAt(const UObject* WorldContext, const FVector& Location);
};
