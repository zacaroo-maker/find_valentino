#include "FVAudio.h"

#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "FVGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

namespace
{
	TArray<const TCHAR*> PathsFor(EFVSound S)
	{
		switch (S)
		{
		case EFVSound::UIMove: return {TEXT("/Game/Audio/UI_Move.UI_Move")};
		case EFVSound::UISelect: return {TEXT("/Game/Audio/UI_Select.UI_Select")};
		case EFVSound::UIBack: return {TEXT("/Game/Audio/UI_Back.UI_Back")};
		case EFVSound::UIStart: return {TEXT("/Game/Audio/UI_Start.UI_Start")};
		case EFVSound::UIPause: return {TEXT("/Game/Audio/UI_Pause.UI_Pause")};
		case EFVSound::MenuMusic: return {TEXT("/Game/Audio/Music_Menu.Music_Menu")};
		case EFVSound::Birds: return {TEXT("/Game/StarterContent/Audio/Starter_Birds01.Starter_Birds01")};
		case EFVSound::Wind: return {TEXT("/Game/StarterContent/Audio/Starter_Wind05.Starter_Wind05")};
		case EFVSound::Step: return {TEXT("/Game/Audio/Step_Grass_1.Step_Grass_1"), TEXT("/Game/Audio/Step_Grass_2.Step_Grass_2"),
		                             TEXT("/Game/Audio/Step_Grass_3.Step_Grass_3"), TEXT("/Game/Audio/Step_Grass_4.Step_Grass_4")};
		case EFVSound::Pickup: return {TEXT("/Game/Audio/Pickup.Pickup")};
		case EFVSound::PutDown: return {TEXT("/Game/Audio/PutDown.PutDown")};
		case EFVSound::Munch: return {TEXT("/Game/Audio/Munch.Munch")};
		case EFVSound::Sip: return {TEXT("/Game/Audio/Sip.Sip")};
		case EFVSound::PetChime: return {TEXT("/Game/Audio/Pet_Chime.Pet_Chime")};
		case EFVSound::Sprout: return {TEXT("/Game/Audio/Sprout_Sparkle.Sprout_Sparkle")};
		case EFVSound::Nibble: return {TEXT("/Game/Audio/Nibble_Boing.Nibble_Boing")};
		case EFVSound::Bark: return {TEXT("/Game/Audio/Dog_Bark.Dog_Bark"), TEXT("/Game/Audio/Dog_Bark2.Dog_Bark2")};
		case EFVSound::PourLoop: return {TEXT("/Game/Audio/Water_Pour_Loop.Water_Pour_Loop")};
		case EFVSound::Splash: return {TEXT("/Game/Audio/Water_Splash.Water_Splash")};
		}
		return {};
	}

	USoundAttenuation* WorldAttenuation(bool bFar = false)
	{
		// normal: heard clearly up close, fades out across the garden
		// far: loud sounds (barking next door) that carry over the fence
		static TStrongObjectPtr<USoundAttenuation> Att[2];
		TStrongObjectPtr<USoundAttenuation>& A = Att[bFar ? 1 : 0];
		if (!A)
		{
			A.Reset(NewObject<USoundAttenuation>());
			A->Attenuation.bAttenuate = true;
			A->Attenuation.bSpatialize = true;
			A->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
			A->Attenuation.AttenuationShapeExtents = FVector(bFar ? 400.f : 250.f);
			A->Attenuation.FalloffDistance = bFar ? 6000.f : 2200.f;
		}
		return A.Get();
	}
}

void FFVFootsteps::Update(ACharacter* Character, float DeltaSeconds)
{
	if (!Character || !Character->GetMesh() || !Character->GetCharacterMovement())
	{
		return;
	}
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	const bool bWalking = Character->GetCharacterMovement()->IsMovingOnGround() && Character->GetVelocity().Size2D() > 20.f;
	const float Ground = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	// mannequin / MetaHuman naming, or Dad's own skeleton
	static const FName Bones[2][2] = {{TEXT("foot_l"), TEXT("L_foot")}, {TEXT("foot_r"), TEXT("R_foot")}};
	for (int32 i = 0; i < 2; ++i)
	{
		FName Bone = Bones[i][0];
		if (Mesh->GetBoneIndex(Bone) == INDEX_NONE)
		{
			Bone = Bones[i][1];
			if (Mesh->GetBoneIndex(Bone) == INDEX_NONE)
			{
				continue;
			}
		}
		const FVector Foot = Mesh->GetSocketLocation(Bone);
		const float H = Foot.Z - Ground;
		FFoot& F = Feet[i];
		// track the contact height, letting it creep back up (idle poses, slopes, different characters)
		F.Low = FMath::Min(F.Low + 6.f * DeltaSeconds, H);
		if (H > F.Low + 7.f)
		{
			F.bArmed = true; // the foot is up in the air
		}
		else if (F.bArmed && H < F.Low + 2.5f)
		{
			F.bArmed = false; // ...and has come down: a step
			if (bWalking)
			{
				FVAudio::PlayAt(Character, EFVSound::Step, FVector(Foot.X, Foot.Y, Ground), Volume, FMath::FRandRange(PitchRange.X, PitchRange.Y));
			}
		}
	}
}

USoundBase* FVAudio::Get(EFVSound Sound)
{
	static TMap<FString, TStrongObjectPtr<USoundBase>> Cache;
	const TArray<const TCHAR*> Paths = PathsFor(Sound);
	if (Paths.Num() == 0)
	{
		return nullptr;
	}
	const FString Path = Paths[FMath::RandHelper(Paths.Num())]; // pick a variant (footsteps, barks)
	if (TStrongObjectPtr<USoundBase>* Found = Cache.Find(Path))
	{
		return Found->Get();
	}
	USoundBase* S = LoadObject<USoundBase>(nullptr, *Path);
	Cache.Add(Path, TStrongObjectPtr<USoundBase>(S));
	return S;
}

bool FVAudio::IsMusic(EFVSound Sound)
{
	return Sound == EFVSound::MenuMusic;
}

bool FVAudio::Enabled(const UObject* WorldContext, EFVSound Sound)
{
	const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UFVGameInstance* GI = W ? Cast<UFVGameInstance>(W->GetGameInstance()) : nullptr;
	if (!GI)
	{
		return true;
	}
	return IsMusic(Sound) ? GI->bMusicOn : GI->bSoundOn;
}

UAudioComponent* FVAudio::Play2D(const UObject* WorldContext, EFVSound Sound, float Volume, float Pitch, bool bKeepPlayingIntoNextLevel)
{
	if (!Enabled(WorldContext, Sound))
	{
		return nullptr;
	}
	if (USoundBase* S = Get(Sound))
	{
		return UGameplayStatics::SpawnSound2D(WorldContext, S, Volume, Pitch, 0.f, nullptr, bKeepPlayingIntoNextLevel);
	}
	return nullptr;
}

void FVAudio::PlayAt(const UObject* WorldContext, EFVSound Sound, const FVector& Location, float Volume, float Pitch, bool bCarriesFar)
{
	if (!Enabled(WorldContext, Sound))
	{
		return;
	}
	if (USoundBase* S = Get(Sound))
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContext, S, Location, Volume, Pitch, 0.f, WorldAttenuation(bCarriesFar));
	}
}

UAudioComponent* FVAudio::PlayAttached(EFVSound Sound, USceneComponent* AttachTo, float Volume)
{
	if (!AttachTo || !Enabled(AttachTo, Sound))
	{
		return nullptr;
	}
	if (USoundBase* S = Get(Sound))
	{
		return UGameplayStatics::SpawnSoundAttached(S, AttachTo, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset,
			true, Volume, 1.f, 0.f, WorldAttenuation());
	}
	return nullptr;
}
