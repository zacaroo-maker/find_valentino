// Finding Valentino - chunky 80s-arcade pixel drawing on the HUD canvas: a 5x7 block font, a pixel tortoise, boxes, scanlines.
#pragma once

#include "CoreMinimal.h"

class AHUD;

namespace FVPixel
{
	/** Width in screen pixels of Text drawn with blocks of size Px (each letter is 5 blocks + 1 gap). */
	FINDINGVALENTINO_API float TextWidth(const FString& Text, float Px);
	/** Height of one line (7 blocks). */
	inline float TextHeight(float Px) { return 7.f * Px; }

	/** Draws Text in the block font. RowColors (optional, 7 entries) colour each row for a striped arcade look. */
	FINDINGVALENTINO_API void DrawText(AHUD* Hud, const FString& Text, float X, float Y, float Px, const FLinearColor& Color,
		const TArray<FLinearColor>* RowColors = nullptr, float ShadowOffset = 0.f, const FLinearColor& Shadow = FLinearColor::Black);

	/** Draws Text centred on X. */
	FINDINGVALENTINO_API void DrawTextCentred(AHUD* Hud, const FString& Text, float CentreX, float Y, float Px, const FLinearColor& Color,
		const TArray<FLinearColor>* RowColors = nullptr, float ShadowOffset = 0.f, const FLinearColor& Shadow = FLinearColor::Black);

	/** Splits Text into lines no wider than MaxWidth. */
	FINDINGVALENTINO_API TArray<FString> Wrap(const FString& Text, float Px, float MaxWidth);

	/** Valentino as a 16x7 sprite (Frame 0/1 = legs). Width is 16*Px. */
	FINDINGVALENTINO_API void DrawTortoise(AHUD* Hud, float X, float Y, float Px, int32 Frame, bool bFaceLeft = false);

	/** A chunky box: Fill with a Px-thick Border. */
	FINDINGVALENTINO_API void DrawBox(AHUD* Hud, float X, float Y, float W, float H, float Px, const FLinearColor& Fill, const FLinearColor& Border);

	/** A filled circle made of Px-sized steps. */
	FINDINGVALENTINO_API void DrawDisc(AHUD* Hud, float CX, float CY, float Radius, float Px, const FLinearColor& Color);

	/** A comic-book thought cloud (rounded, bumpy edge, outlined) around a W x H area at X,Y. */
	FINDINGVALENTINO_API void DrawThoughtCloud(AHUD* Hud, float X, float Y, float W, float H, float Px, const FLinearColor& Fill, const FLinearColor& Edge);

	/** CRT scanlines across the whole screen. */
	FINDINGVALENTINO_API void DrawScanlines(AHUD* Hud, float Width, float Height, float Spacing, float Alpha);
}
