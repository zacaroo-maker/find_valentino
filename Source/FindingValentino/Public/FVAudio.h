// Finding Valentino - one place to play the game's sounds (original synthesized effects in /Game/Audio).
#pragma once

#include "CoreMinimal.h"

class USoundBase;
class UAudioComponent;
class USceneComponent;

enum class EFVSound : uint8
{
	UIMove,
	UISelect,
	UIBack,
	UIStart,
	UIPause,
	MenuMusic,
	Birds,
	Wind,
	Step,
	Pickup,
	PutDown,
	Munch,
	Sip,
	PetChime,
	Sprout,
	Nibble,
	Bark,
	PourLoop,
	Splash,
};

namespace FVAudio
{
	FINDINGVALENTINO_API USoundBase* Get(EFVSound Sound);
	FINDINGVALENTINO_API bool IsMusic(EFVSound Sound);
	/** Respect the SOUND / MUSIC settings (stored on the game instance). */
	FINDINGVALENTINO_API bool Enabled(const UObject* WorldContext, EFVSound Sound);

	/** A sound with no position (UI, music, ambience). Returns the component for loops so they can be stopped. */
	FINDINGVALENTINO_API UAudioComponent* Play2D(const UObject* WorldContext, EFVSound Sound, float Volume = 1.f, float Pitch = 1.f, bool bKeepPlayingIntoNextLevel = false);
	/** A sound in the world (gets quieter with distance). */
	FINDINGVALENTINO_API void PlayAt(const UObject* WorldContext, EFVSound Sound, const FVector& Location, float Volume = 1.f, float Pitch = 1.f);
	/** A looping sound that follows a component (e.g. water pouring from the can). */
	FINDINGVALENTINO_API UAudioComponent* PlayAttached(EFVSound Sound, USceneComponent* AttachTo, float Volume = 1.f);
}
