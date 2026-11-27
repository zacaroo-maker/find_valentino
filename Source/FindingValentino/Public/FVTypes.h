// Finding Valentino - shared enums.
#pragma once

#include "CoreMinimal.h"
#include "FVTypes.generated.h"

UENUM(BlueprintType)
enum class EFVItemKind : uint8
{
	Prop,         // garden objects you can move (pots, benches, chairs...)
	Food,         // something Valentino eats
	WateringCan,
	Ball          // throw it (left click) and Diego fetches it
};

/** What the real Valentino likes: flowers, romaine, chicory, berries and tomatoes. */
UENUM(BlueprintType)
enum class EFVFoodType : uint8
{
	Romaine,
	Dandelion,
	Strawberry,
	Tomato,
	Chicory,
	Hibiscus,
	Count UMETA(Hidden)
};

namespace FVFood
{
	inline FText Name(EFVFoodType T)
	{
		switch (T)
		{
		case EFVFoodType::Romaine:    return NSLOCTEXT("FV", "Romaine", "romaine leaf");
		case EFVFoodType::Dandelion:  return NSLOCTEXT("FV", "Dandelion", "dandelion");
		case EFVFoodType::Strawberry: return NSLOCTEXT("FV", "Strawberry", "strawberry");
		case EFVFoodType::Tomato:     return NSLOCTEXT("FV", "Tomato", "tomato");
		case EFVFoodType::Chicory:    return NSLOCTEXT("FV", "Chicory", "chicory");
		case EFVFoodType::Hibiscus:   return NSLOCTEXT("FV", "Hibiscus", "hibiscus flower");
		default: break;
		}
		return FText::GetEmpty();
	}
	/** Short upper-case name for the pixel HUD. */
	inline FString Upper(EFVFoodType T)
	{
		switch (T)
		{
		case EFVFoodType::Romaine:    return TEXT("ROMAINE");
		case EFVFoodType::Dandelion:  return TEXT("DANDELION");
		case EFVFoodType::Strawberry: return TEXT("STRAWBERRY");
		case EFVFoodType::Tomato:     return TEXT("TOMATO");
		case EFVFoodType::Chicory:    return TEXT("CHICORY");
		case EFVFoodType::Hibiscus:   return TEXT("HIBISCUS FLOWER");
		default: break;
		}
		return TEXT("FOOD");
	}
	/** "A TOMATO", "SOME ROMAINE", "SOME CHICORY" */
	inline FString WithArticle(EFVFoodType T)
	{
		const bool bSome = T == EFVFoodType::Romaine || T == EFVFoodType::Chicory;
		return (bSome ? TEXT("SOME ") : TEXT("A ")) + Upper(T);
	}
	/** Berries and tomatoes are sweet treats; greens and flowers are everyday food. */
	inline bool IsTreat(EFVFoodType T)
	{
		return T == EFVFoodType::Strawberry || T == EFVFoodType::Tomato;
	}
	/** Energy restored (0-1). */
	inline float Energy(EFVFoodType T)
	{
		switch (T)
		{
		case EFVFoodType::Dandelion:  return 0.30f;
		case EFVFoodType::Hibiscus:   return 0.30f;
		case EFVFoodType::Chicory:    return 0.30f;
		case EFVFoodType::Romaine:    return 0.25f;
		case EFVFoodType::Strawberry: return 0.35f;
		case EFVFoodType::Tomato:     return 0.35f;
		default: break;
		}
		return 0.2f;
	}
}
