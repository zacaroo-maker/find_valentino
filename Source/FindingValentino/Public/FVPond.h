// Finding Valentino - the garden pond: Valentino drinks/soaks here; the watering can refills here.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVPond.generated.h"

class UBillboardComponent;

UCLASS()
class FINDINGVALENTINO_API AFVPond : public AActor
{
	GENERATED_BODY()

public:
	AFVPond();

	/** Where Valentino goes to drink/soak (inside the basin, reachable via the ramp). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pond")
	TObjectPtr<USceneComponent> DrinkPoint;

	/** Player within this distance of the pond centre can refill the watering can. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pond")
	float RefillRadius = 280.f;

	FVector GetDrinkLocation() const { return DrinkPoint->GetComponentLocation(); }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Pond")
	TObjectPtr<USceneComponent> Root;
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Pond")
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};
