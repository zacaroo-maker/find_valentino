// Finding Valentino - helpers for putting a MetaHuman (an assembled BP_* actor) on a character.
// The character keeps an invisible mannequin (Quinn or Manny) that runs the normal walk/run/jump animation;
// the MetaHuman's body and clothes copy that pose (leader pose), its face and hair follow the body.
#pragma once

#include "CoreMinimal.h"

class ACharacter;
class AActor;

namespace FVMetaHuman
{
	/** Spawns the MetaHuman blueprint, attaches it to the character's mesh, makes it follow that mesh's animation, and hides the mesh. */
	FINDINGVALENTINO_API AActor* AttachTo(ACharacter* Character, UClass* MetaHumanClass);

	/** Hides/shows the head parts (face, hair, eyebrows, beard...) of MetaHumans attached to the character (for first person). */
	FINDINGVALENTINO_API void SetHeadHidden(ACharacter* Character, bool bHidden);

	/** Gives the character the mannequin + animation blueprint to drive a MetaHuman: female frame (Quinn) or male (Manny). */
	FINDINGVALENTINO_API void UseAnimFrame(ACharacter* Character, bool bFemale);
}
