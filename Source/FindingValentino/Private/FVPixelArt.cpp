#include "FVPixelArt.h"

#include "GameFramework/HUD.h"

namespace
{
	// 5x7 glyphs, '#' = lit block
	struct FGlyphTable
	{
		TMap<TCHAR, TArray<uint8>> Rows; // 7 rows, bit 4 = leftmost column

		void Add(TCHAR C, std::initializer_list<const char*> Lines)
		{
			TArray<uint8> R;
			for (const char* L : Lines)
			{
				uint8 Bits = 0;
				for (int32 i = 0; i < 5 && L[i]; ++i)
				{
					if (L[i] == '#')
					{
						Bits |= (1 << (4 - i));
					}
				}
				R.Add(Bits);
			}
			Rows.Add(C, R);
		}

		FGlyphTable()
		{
			Add('A', {" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"});
			Add('B', {"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "});
			Add('C', {" ####", "#    ", "#    ", "#    ", "#    ", "#    ", " ####"});
			Add('D', {"#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "});
			Add('E', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"});
			Add('F', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "});
			Add('G', {" ####", "#    ", "#    ", "#  ##", "#   #", "#   #", " ####"});
			Add('H', {"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"});
			Add('I', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####"});
			Add('J', {"  ###", "   # ", "   # ", "   # ", "   # ", "#  # ", " ##  "});
			Add('K', {"#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"});
			Add('L', {"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"});
			Add('M', {"#   #", "## ##", "# # #", "# # #", "#   #", "#   #", "#   #"});
			Add('N', {"#   #", "##  #", "# # #", "#  ##", "#   #", "#   #", "#   #"});
			Add('O', {" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "});
			Add('P', {"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "});
			Add('Q', {" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"});
			Add('R', {"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"});
			Add('S', {" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "});
			Add('T', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "});
			Add('U', {"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "});
			Add('V', {"#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "});
			Add('W', {"#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #"});
			Add('X', {"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"});
			Add('Y', {"#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "});
			Add('Z', {"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"});
			Add('0', {" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "});
			Add('1', {"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "});
			Add('2', {" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"});
			Add('3', {"#### ", "    #", "    #", " ### ", "    #", "    #", "#### "});
			Add('4', {"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "});
			Add('5', {"#####", "#    ", "#### ", "    #", "    #", "#   #", " ### "});
			Add('6', {" ### ", "#    ", "#    ", "#### ", "#   #", "#   #", " ### "});
			Add('7', {"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "});
			Add('8', {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "});
			Add('9', {" ### ", "#   #", "#   #", " ####", "    #", "    #", " ### "});
			Add('!', {"  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "     ", "  #  "});
			Add('?', {" ### ", "#   #", "    #", "   # ", "  #  ", "     ", "  #  "});
			Add('.', {"     ", "     ", "     ", "     ", "     ", "     ", "  #  "});
			Add(',', {"     ", "     ", "     ", "     ", "     ", "  #  ", " #   "});
			Add('\'', {"  #  ", "  #  ", "     ", "     ", "     ", "     ", "     "});
			Add('-', {"     ", "     ", "     ", "#####", "     ", "     ", "     "});
			Add(':', {"     ", "  #  ", "     ", "     ", "     ", "  #  ", "     "});
			Add('>', {"#    ", " #   ", "  #  ", "   # ", "  #  ", " #   ", "#    "});
			Add('<', {"    #", "   # ", "  #  ", " #   ", "  #  ", "   # ", "    #"});
			Add('/', {"    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    "});
			Add('(', {"   # ", "  #  ", " #   ", " #   ", " #   ", "  #  ", "   # "});
			Add(')', {" #   ", "  #  ", "   # ", "   # ", "   # ", "  #  ", " #   "});
			Add('%', {"##  #", "##  #", "   # ", "  #  ", " #   ", "#  ##", "#  ##"});
			Add('+', {"     ", "  #  ", "  #  ", "#####", "  #  ", "  #  ", "     "});
			Add('=', {"     ", "     ", "#####", "     ", "#####", "     ", "     "});
			Add('[', {" ### ", " #   ", " #   ", " #   ", " #   ", " #   ", " ### "});
			Add(']', {" ### ", "   # ", "   # ", "   # ", "   # ", "   # ", " ### "});
			Add('"', {" # # ", " # # ", "     ", "     ", "     ", "     ", "     "});
			Add('*', {"     ", "# # #", " ### ", "#####", " ### ", "# # #", "     "});
			Add('&', {" ##  ", "#  # ", " ##  ", " #   ", "# # #", "#  # ", " ## #"});
			Add('#', {" # # ", " # # ", "#####", " # # ", "#####", " # # ", " # # "});
			Add(' ', {"     ", "     ", "     ", "     ", "     ", "     ", "     "});
		}
	};

	const FGlyphTable& Glyphs()
	{
		static FGlyphTable T;
		return T;
	}

	void DrawRaw(AHUD* Hud, const FString& Text, float X, float Y, float Px, const FLinearColor& Color, const TArray<FLinearColor>* RowColors)
	{
		const FGlyphTable& G = Glyphs();
		float CX = X;
		for (TCHAR Ch : Text.ToUpper())
		{
			const TArray<uint8>* Rows = G.Rows.Find(Ch);
			if (!Rows)
			{
				Rows = G.Rows.Find('?');
			}
			for (int32 R = 0; R < 7; ++R)
			{
				const FLinearColor& C = (RowColors && RowColors->IsValidIndex(R)) ? (*RowColors)[R] : Color;
				const uint8 Bits = (*Rows)[R];
				// merge runs of lit blocks into one rectangle
				int32 Col = 0;
				while (Col < 5)
				{
					if (Bits & (1 << (4 - Col)))
					{
						int32 Len = 1;
						while (Col + Len < 5 && (Bits & (1 << (4 - (Col + Len)))))
						{
							++Len;
						}
						Hud->DrawRect(C, CX + Col * Px, Y + R * Px, Len * Px, Px);
						Col += Len;
					}
					else
					{
						++Col;
					}
				}
			}
			CX += 6.f * Px;
		}
	}
}

float FVPixel::TextWidth(const FString& Text, float Px)
{
	return Text.IsEmpty() ? 0.f : (Text.Len() * 6.f - 1.f) * Px;
}

void FVPixel::DrawText(AHUD* Hud, const FString& Text, float X, float Y, float Px, const FLinearColor& Color,
	const TArray<FLinearColor>* RowColors, float ShadowOffset, const FLinearColor& Shadow)
{
	if (!Hud)
	{
		return;
	}
	X = FMath::RoundToFloat(X);
	Y = FMath::RoundToFloat(Y);
	if (ShadowOffset > 0.f)
	{
		DrawRaw(Hud, Text, X + ShadowOffset, Y + ShadowOffset, Px, Shadow, nullptr);
	}
	DrawRaw(Hud, Text, X, Y, Px, Color, RowColors);
}

void FVPixel::DrawTextCentred(AHUD* Hud, const FString& Text, float CentreX, float Y, float Px, const FLinearColor& Color,
	const TArray<FLinearColor>* RowColors, float ShadowOffset, const FLinearColor& Shadow)
{
	DrawText(Hud, Text, CentreX - TextWidth(Text, Px) * 0.5f, Y, Px, Color, RowColors, ShadowOffset, Shadow);
}

TArray<FString> FVPixel::Wrap(const FString& Text, float Px, float MaxWidth)
{
	TArray<FString> Words, Lines;
	Text.ParseIntoArray(Words, TEXT(" "));
	FString Line;
	for (const FString& W : Words)
	{
		const FString Try = Line.IsEmpty() ? W : Line + TEXT(" ") + W;
		if (TextWidth(Try, Px) > MaxWidth && !Line.IsEmpty())
		{
			Lines.Add(Line);
			Line = W;
		}
		else
		{
			Line = Try;
		}
	}
	if (!Line.IsEmpty())
	{
		Lines.Add(Line);
	}
	return Lines;
}

void FVPixel::DrawTortoise(AHUD* Hud, float X, float Y, float Px, int32 Frame, bool bFaceLeft)
{
	if (!Hud)
	{
		return;
	}
	// D shell rim, G shell, Y shell plates, S skin, K eye
	static const char* Body[5] = {
		".....DDDDD......",
		"...DDGYGYGDD....",
		"..DGYGYGYGYGD.SS",
		".DGYGYGYGYGYGDSK",
		".DDDDDDDDDDDDDSS",
	};
	static const char* Legs[2][2] = {
		{"..SS..SS..SS.S..", "..S...S...S....."},
		{"...SS..SS..SS...", "....S...S...S..."},
	};
	auto ColorOf = [](char C, FLinearColor& Out)
	{
		switch (C)
		{
		case 'D': Out = FLinearColor(0.05f, 0.25f, 0.06f); return true;
		case 'G': Out = FLinearColor(0.20f, 0.62f, 0.18f); return true;
		case 'Y': Out = FLinearColor(0.95f, 0.80f, 0.22f); return true;
		case 'S': Out = FLinearColor(0.62f, 0.68f, 0.32f); return true;
		case 'K': Out = FLinearColor(0.02f, 0.02f, 0.02f); return true;
		default: return false;
		}
	};
	auto Row = [&](const char* L, int32 R)
	{
		for (int32 c = 0; c < 16; ++c)
		{
			FLinearColor Col;
			if (ColorOf(L[c], Col))
			{
				const int32 DrawC = bFaceLeft ? 15 - c : c;
				Hud->DrawRect(Col, X + DrawC * Px, Y + R * Px, Px, Px);
			}
		}
	};
	for (int32 R = 0; R < 5; ++R)
	{
		Row(Body[R], R);
	}
	Row(Legs[Frame & 1][0], 5);
	Row(Legs[Frame & 1][1], 6);
}

void FVPixel::DrawBox(AHUD* Hud, float X, float Y, float W, float H, float Px, const FLinearColor& Fill, const FLinearColor& Border)
{
	if (!Hud)
	{
		return;
	}
	X = FMath::RoundToFloat(X);
	Y = FMath::RoundToFloat(Y);
	Hud->DrawRect(Fill, X + Px, Y + Px, W - 2.f * Px, H - 2.f * Px);
	// chunky border with notched corners
	Hud->DrawRect(Border, X + Px, Y, W - 2.f * Px, Px);
	Hud->DrawRect(Border, X + Px, Y + H - Px, W - 2.f * Px, Px);
	Hud->DrawRect(Border, X, Y + Px, Px, H - 2.f * Px);
	Hud->DrawRect(Border, X + W - Px, Y + Px, Px, H - 2.f * Px);
}

void FVPixel::DrawScanlines(AHUD* Hud, float Width, float Height, float Spacing, float Alpha)
{
	if (!Hud || Spacing < 2.f)
	{
		return;
	}
	const FLinearColor C(0.f, 0.f, 0.f, Alpha);
	const float Thick = FMath::Max(1.f, FMath::FloorToFloat(Spacing * 0.4f));
	for (float Y = 0.f; Y < Height; Y += Spacing)
	{
		Hud->DrawRect(C, 0.f, Y, Width, Thick);
	}
}
