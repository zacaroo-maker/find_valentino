// Finding Valentino - decorating Valentino's yard: garden points saved up from every day's score buy rocks,
// a basking stone, a water dish, a hollow log to hide in, and plants that grow his food. Bought things appear in
// front of you (props can be carried with E and put anywhere), Valentino uses them, and they stay from day to day.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FVTypes.h"
#include "FVDecor.generated.h"

class AFVPickupItem;

struct FFVDecorDef
{
	FName Id;
	const TCHAR* Name;      // shop name (upper case)
	const TCHAR* Blurb;     // one line about it
	int32 Price = 0;
	const TCHAR* Mesh = nullptr;
	float Scale = 1.f;
	bool bPlant = false;    // planted where you stand (a forage plant), not carried
	EFVFoodType Food = EFVFoodType::Dandelion;
	FName Tag;              // what Valentino recognises it as (FV_BaskStone, FV_WaterDish)
	bool bHide = false;     // has a hiding spot inside
};

UCLASS()
class FINDINGVALENTINO_API UFVDecorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type T) const override { return T == EWorldType::Game || T == EWorldType::PIE; }

	static const TArray<FFVDecorDef>& Catalog();

	int32 GetSavings() const { return Savings; }
	/** Points earned today also go into savings (written to the save file). */
	void AddSavings(int32 Points);
	/** All points earned (spending doesn't lower it); reaching the gate's target opens the dog park. */
	int32 GetTotalEarned() const { return TotalEarned; }
	bool IsParkUnlocked() const { return bParkUnlocked; }
	/** Testing: open the park now, or lock it again and start the count over. */
	void SetParkUnlocked(bool bOpen);
	/** Buy something and put it in front of Who. Returns a message for the player (empty = couldn't afford it). */
	FString Buy(int32 CatalogIndex, APawn* Who);
	/** A bought prop was carried somewhere new. */
	void OnMoved(AFVPickupItem* Item);

private:
	AActor* SpawnDecor(const FFVDecorDef& Def, const FTransform& Where, int32 SaveIndex);
	int32 Savings = 0;
	int32 TotalEarned = 0;
	bool bParkUnlocked = false;
};
