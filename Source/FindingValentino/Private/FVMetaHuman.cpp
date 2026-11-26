#include "FVMetaHuman.h"

#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	/** Face mesh (its own skeleton, animated by ABP_Face) or any groom (hair, brows, lashes, beard, fuzz). */
	bool IsHeadPart(const UActorComponent* C)
	{
		if (const USkeletalMeshComponent* S = Cast<USkeletalMeshComponent>(C))
		{
			const USkeletalMesh* M = S->GetSkeletalMeshAsset();
			return M && M->GetSkeleton() && M->GetSkeleton()->GetName().Contains(TEXT("Face"));
		}
		return C && C->GetClass()->GetName().Contains(TEXT("Groom"));
	}
}

AActor* FVMetaHuman::AttachTo(ACharacter* Character, UClass* MetaHumanClass)
{
	if (!Character || !MetaHumanClass || !Character->GetWorld())
	{
		return nullptr;
	}
	USkeletalMeshComponent* Leader = Character->GetMesh();
	FActorSpawnParameters P;
	P.Owner = Character;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* MH = Character->GetWorld()->SpawnActor<AActor>(MetaHumanClass, Leader->GetComponentTransform(), P);
	if (!MH)
	{
		return nullptr;
	}
	MH->AttachToComponent(Leader, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MH->SetActorEnableCollision(false);

	TArray<USkeletalMeshComponent*> Meshes;
	MH->GetComponents(Meshes);
	for (USkeletalMeshComponent* S : Meshes)
	{
		S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (!IsHeadPart(S))
		{
			S->SetLeaderPoseComponent(Leader, true);
		}
	}
	// the mannequin keeps animating (for the MetaHuman to copy) but is never drawn
	Leader->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Leader->SetCastShadow(false);
	Leader->SetVisibility(false, false);
	return MH;
}

void FVMetaHuman::SetHeadHidden(ACharacter* Character, bool bHidden)
{
	if (!Character)
	{
		return;
	}
	TArray<AActor*> Attached;
	Character->GetAttachedActors(Attached, true, true);
	for (AActor* A : Attached)
	{
		if (!A || A->GetOwner() != Character)
		{
			continue;
		}
		for (UActorComponent* C : A->GetComponents())
		{
			if (IsHeadPart(C))
			{
				if (USceneComponent* SC = Cast<USceneComponent>(C))
				{
					SC->SetHiddenInGame(bHidden);
				}
			}
		}
	}
}

void FVMetaHuman::UseAnimFrame(ACharacter* Character, bool bFemale)
{
	if (!Character || !Character->GetMesh())
	{
		return;
	}
	const TCHAR* MeshPath = bFemale ? TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn.SKM_Quinn")
	                                : TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny");
	const TCHAR* AnimPath = bFemale ? TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C")
	                                : TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C");
	if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MeshPath))
	{
		Character->GetMesh()->SetSkeletalMesh(Mesh);
	}
	if (UClass* Anim = LoadObject<UClass>(nullptr, AnimPath))
	{
		Character->GetMesh()->SetAnimInstanceClass(Anim);
	}
}
