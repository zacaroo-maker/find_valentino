#include "FVMetaHuman.h"

#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "FVFamilyMemberData.h"

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
	// cartoon heads (first-person view looks out from inside them)
	for (UActorComponent* C : Character->GetComponents())
	{
		if (C->ComponentHasTag(TEXT("FV_ToonHead")))
		{
			Cast<USceneComponent>(C)->SetHiddenInGame(bHidden);
		}
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

void FVMetaHuman::AttachToonParts(ACharacter* Character, const TArray<FFVToonPart>& Parts, TArray<TObjectPtr<UStaticMeshComponent>>& OutComponents)
{
	USkeletalMeshComponent* Leader = Character ? Character->GetMesh() : nullptr;
	const USkeletalMesh* SkelMesh = Leader ? Leader->GetSkeletalMeshAsset() : nullptr;
	if (!SkelMesh)
	{
		return;
	}
	const FReferenceSkeleton& Ref = SkelMesh->GetRefSkeleton();
	for (const FFVToonPart& Part : Parts)
	{
		const int32 Bone = Ref.FindBoneIndex(Part.Bone);
		if (!Part.Mesh || Bone == INDEX_NONE)
		{
			continue;
		}
		// the bone's reference pose in component space: the piece was modelled there, so undo it
		FTransform BoneRef = Ref.GetRefBonePose()[Bone];
		for (int32 P = Ref.GetParentIndex(Bone); P != INDEX_NONE; P = Ref.GetParentIndex(P))
		{
			BoneRef = BoneRef * Ref.GetRefBonePose()[P];
		}
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Character);
		C->SetStaticMesh(Part.Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		C->SetupAttachment(Leader, Part.Bone);
		C->SetRelativeTransform(BoneRef.Inverse());
		if (Part.Bone == TEXT("head"))
		{
			C->ComponentTags.Add(TEXT("FV_ToonHead"));
		}
		C->RegisterComponent();
		OutComponents.Add(C);
	}
	// the mannequin keeps animating (the pieces ride its bones) but is never drawn
	Leader->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Leader->SetCastShadow(false);
	Leader->SetVisibility(false, false);
}
