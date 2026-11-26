// Finding Valentino - shared enums.
#pragma once

#include "CoreMinimal.h"
#include "FVTypes.generated.h"

UENUM(BlueprintType)
enum class EFVItemKind : uint8
{
	Prop,         // garden objects you can move (pots, benches, chairs...)
	Food,         // something Valentino eats
	WateringCan
};

UENUM(BlueprintType)
enum class EFVFoodType : uint8
{
	Lettuce,
	Dandelion,
	Strawberry,
	Worm
};

namespace FVFood
{
	inline FText Name(EFVFoodType T)
	{
		switch (T)
		{
		case EFVFoodType::Lettuce:    return NSLOCTEXT("FV", "Lettuce", "lettuce leaf");
		case EFVFoodType::Dandelion:  return NSLOCTEXT("FV", "Dandelion", "dandelion");
		case EFVFoodType::Strawberry: return NSLOCTEXT("FV", "Strawberry", "strawberry");
		case EFVFoodType::Worm:       return NSLOCTEXT("FV", "Worm", "worm");
		}
		return FText::GetEmpty();
	}
	/** Energy restored (0-1). Dandelions are a Hermann's favourite; the worm is a rare odd treat. */
	inline float Energy(EFVFoodType T)
	{
		switch (T)
		{
		case EFVFoodType::Dandelion:  return 0.35f;
		case EFVFoodType::Strawberry: return 0.30f;
		case EFVFoodType::Lettuce:    return 0.25f;
		case EFVFoodType::Worm:       return 0.15f;
		}
		return 0.2f;
	}
}
