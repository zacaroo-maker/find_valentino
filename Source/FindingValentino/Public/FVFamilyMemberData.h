// Finding Valentino - one family member's settings (used by the character-select screen and the character class).
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FVFamilyMemberData.generated.h"

class USkeletalMesh;
class UTexture2D;

UCLASS(BlueprintType)
class FINDINGVALENTINO_API UFVFamilyMemberData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Shown on the character-select screen, e.g. "Dad". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family")
	FText DisplayName;
	/** One line about their personality, e.g. "Curious explorer - checks under everything." */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family", meta = (MultiLine = true))
	FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family")
	TObjectPtr<UTexture2D> Portrait;

	/** The character Blueprint to spawn for this person (a child of AFVFamilyCharacter, e.g. a MetaHuman). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family")
	TSoftClassPtr<class AFVFamilyCharacter> CharacterClass;

	/** Their assembled MetaHuman (e.g. BP_Mom). Used when CharacterClass is empty: the standard family character wears it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family")
	TSoftClassPtr<AActor> MetaHumanClass;
	/** Animate with the female mannequin (Quinn) instead of the male one (Manny). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family")
	bool bFemaleFrame = false;

	// ---- personality / gameplay tuning (from the design: Dad walks faster, Grandpa is slower, kids are energetic...) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family|Movement")
	float WalkSpeed = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family|Movement")
	float RunSpeed = 500.f;
	/** Eye height for first person (cm above the capsule centre); children are lower. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family|Camera")
	float FirstPersonEyeHeight = 70.f;
	/** How readily Valentino comes over to this person (multiplies his approach chance). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family|Valentino")
	float ValentinoComfort = 1.f;
	/** Moving faster than this near Valentino startles him (kids rush, grandparents don't). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family|Valentino")
	float StartleSpeedMultiplier = 1.f;
};
