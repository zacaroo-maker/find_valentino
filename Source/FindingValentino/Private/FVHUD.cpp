#include "FVHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "FVCarryComponent.h"
#include "FVDog.h"
#include "FVFamilyMemberData.h"
#include "FVGameInstance.h"
#include "FVGameModes.h"
#include "FVPixelArt.h"
#include "FVAudio.h"
#include "FVValentino.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	// same 80s arcade palette as the start screen
	const FLinearColor Ink(0.03f, 0.02f, 0.12f, 0.85f);
	const FLinearColor Cyan(0.25f, 0.85f, 1.f);
	const FLinearColor Magenta(0.95f, 0.12f, 0.55f, 0.96f);
	const FLinearColor Yellow(1.f, 0.88f, 0.15f);
	const FLinearColor White(1.f, 1.f, 1.f);
	const FLinearColor Red(1.f, 0.2f, 0.2f);
	const FLinearColor Empty(0.18f, 0.14f, 0.26f, 0.9f);
	const FLinearColor ShadowPurple(0.22f, 0.03f, 0.35f);

	const TArray<FLinearColor>& SunsetRows()
	{
		static const TArray<FLinearColor> Rows = {
			FLinearColor(1.f, 0.95f, 0.35f), FLinearColor(1.f, 0.82f, 0.2f), FLinearColor(1.f, 0.62f, 0.15f),
			FLinearColor(1.f, 0.45f, 0.2f), FLinearColor(1.f, 0.3f, 0.35f), FLinearColor(0.95f, 0.2f, 0.5f),
			FLinearColor(0.85f, 0.15f, 0.65f)};
		return Rows;
	}
}

float AFVHUD::Px() const
{
	return Canvas ? FMath::Max(2.f, FMath::RoundToFloat(Canvas->ClipY / 180.f)) : 6.f;
}

void AFVHUD::DrawPauseMenu()
{
	const float CW = Canvas->ClipX, CH = Canvas->ClipY;
	const float P = Px();
	const float T = GetWorld()->GetRealTimeSeconds();
	DrawRect(FLinearColor(0.02f, 0.f, 0.08f, 0.6f), 0.f, 0.f, CW, CH);
	const float H2 = FMath::RoundToFloat(P * 2.f);
	FVPixel::DrawTextCentred(this, TEXT("PAUSED"), CW * 0.5f, CH * 0.2f, H2, White, &SunsetRows(), FMath::RoundToFloat(H2 * 0.45f), ShadowPurple);

	const TPair<const TCHAR*, FName> Buttons[] = {{TEXT("RESUME"), TEXT("Resume")}, {TEXT("MAIN MENU"), TEXT("MainMenu")}, {TEXT("QUIT"), TEXT("Quit")}};
	const float W = FVPixel::TextWidth(TEXT("MAIN MENU"), P) + 14.f * P;
	const float H = 11.f * P;
	float Y = CH * 0.4f;
	for (const auto& B : Buttons)
	{
		const bool bSel = Hover == B.Value;
		const float Bounce = bSel ? -FMath::Abs(FMath::Sin(T * 9.f)) * P : 0.f;
		const float X = (CW - W) * 0.5f;
		FVPixel::DrawBox(this, X, Y + Bounce, W, H, P, bSel ? Magenta : Ink, bSel ? Yellow : Cyan);
		FVPixel::DrawTextCentred(this, B.Key, CW * 0.5f, Y + Bounce + 2.f * P, P, bSel ? Yellow : White, nullptr, P * 0.5f, ShadowPurple);
		if (bSel)
		{
			const float TP = P * 0.75f;
			FVPixel::DrawTortoise(this, X - 19.f * TP, Y + Bounce + (H - 7.f * TP) * 0.5f, TP, FMath::FloorToInt(T * 6.f));
		}
		AddHitBox(FVector2D(X, Y), FVector2D(W, H), B.Value, true);
		Y += H + 3.f * P;
	}
	FVPixel::DrawScanlines(this, CW, CH, FMath::Max(3.f, FMath::RoundToFloat(P * 0.5f)), 0.14f);
}

void AFVHUD::NotifyHitBoxBeginCursorOver(FName BoxName)
{
	if (BoxName != Hover)
	{
		FVAudio::Play2D(this, EFVSound::UIMove, 0.35f);
	}
	Hover = BoxName;
}

void AFVHUD::NotifyHitBoxClick(FName BoxName)
{
	FVAudio::Play2D(this, EFVSound::UISelect, 0.5f);
	Super::NotifyHitBoxClick(BoxName);
	AFVPlayerController* PC = Cast<AFVPlayerController>(GetOwningPlayerController());
	if (BoxName == TEXT("Resume") && PC)
	{
		PC->TogglePauseMenu();
	}
	else if (BoxName == TEXT("MainMenu") && PC)
	{
		PC->GoToMainMenu();
	}
	else if (BoxName == TEXT("Quit"))
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}

void AFVHUD::DrawPrompt(const FText& Text, float Y, const FLinearColor& TextColor)
{
	if (Text.IsEmpty())
	{
		return;
	}
	const float P = Px();
	const float S = FMath::Max(2.f, FMath::RoundToFloat(P * 0.6f));
	// "E  -  Pick up Valentino" -> a keycap [E] and the words
	const FString Full = Text.ToString().ToUpper();
	FString Key, Rest;
	if (!Full.Split(TEXT("  -  "), &Key, &Rest))
	{
		Key.Empty();
		Rest = Full;
	}
	const float KeyW = Key.IsEmpty() ? 0.f : FVPixel::TextWidth(Key, S) + 6.f * S;
	const float Gap = Key.IsEmpty() ? 0.f : 3.f * S;
	const float TW = FVPixel::TextWidth(Rest, S);
	const float BoxW = KeyW + Gap + TW + 8.f * S;
	const float BoxH = 13.f * S;
	const float X = (Canvas->ClipX - BoxW) * 0.5f;
	FVPixel::DrawBox(this, X, Y, BoxW, BoxH, S * 0.75f, Ink, Cyan);
	float CX = X + 4.f * S;
	if (!Key.IsEmpty())
	{
		FVPixel::DrawBox(this, CX, Y + 2.f * S, KeyW, BoxH - 4.f * S, S * 0.6f, FLinearColor(0.15f, 0.1f, 0.3f, 1.f), Yellow);
		FVPixel::DrawText(this, Key, CX + 3.f * S, Y + 3.f * S, S, Yellow);
		CX += KeyW + Gap;
	}
	FVPixel::DrawText(this, Rest, CX, Y + 3.f * S, S, TextColor, nullptr, S * 0.4f, ShadowPurple);
}

void AFVHUD::DrawToast(const FText& Text, float Y)
{
	if (Text.IsEmpty())
	{
		return;
	}
	const float P = Px();
	const float S = FMath::Max(2.f, FMath::RoundToFloat(P * 0.7f));
	const FString Msg = Text.ToString().ToUpper();
	const float W = FVPixel::TextWidth(Msg, S) + 10.f * S;
	const float H = 14.f * S;
	const float X = (Canvas->ClipX - W) * 0.5f;
	FVPixel::DrawBox(this, X, Y, W, H, S * 0.75f, Magenta, Yellow);
	FVPixel::DrawTextCentred(this, Msg, Canvas->ClipX * 0.5f, Y + 3.5f * S, S, Yellow, nullptr, S * 0.4f, ShadowPurple);
}

void AFVHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	if (const AFVPlayerController* FPC = Cast<AFVPlayerController>(GetOwningPlayerController()))
	{
		if (FPC->IsPauseMenuOpen())
		{
			DrawPauseMenu();
			return;
		}
	}
	const float CH = Canvas->ClipY;
	const float P = Px();
	const float S = FMath::Max(2.f, FMath::RoundToFloat(P * 0.6f));
	const float T = GetWorld()->GetRealTimeSeconds();
	const APawn* Pawn = GetOwningPawn();
	const UFVCarryComponent* Carry = Pawn ? Pawn->FindComponentByClass<UFVCarryComponent>() : nullptr;

	// ---- prompts & messages ----
	if (Carry)
	{
		const float Row = 15.f * S;
		DrawPrompt(Carry->GetPrompt(), CH * PromptY, White);
		DrawPrompt(Carry->GetPetPrompt(), CH * PromptY + Row, FLinearColor(1.f, 0.75f, 0.85f));
		DrawPrompt(Carry->GetUsePrompt(), CH * PromptY + Row * 2.f, Cyan);
		DrawToast(Carry->GetToast(), CH * 0.12f);
	}

	// ---- energy meters for both animals (top-left), arcade style ----
	const float X = 4.f * P;
	float Y = 4.f * P;
	const int32 Segments = 12;
	const float SegW = 3.f * S, SegGap = S;
	const float LabelW = FVPixel::TextWidth(TEXT("ENERGY "), S);
	const float PanelW = 4.f * S + LabelW + Segments * (SegW + SegGap) + 4.f * S;
	auto Meter = [&](const TCHAR* Label, float Value, const FLinearColor& Col, float RowY)
	{
		Value = FMath::Clamp(Value, 0.f, 1.f);
		const bool bLow = Value < 0.2f;
		FVPixel::DrawText(this, Label, X + 4.f * S, RowY, S, bLow && FMath::Fmod(T, 0.8f) < 0.4f ? Red : Cyan);
		const int32 Lit = FMath::CeilToInt(Value * Segments - 0.01f);
		for (int32 i = 0; i < Segments; ++i)
		{
			const FLinearColor C = i < Lit ? (bLow ? Red : Col) : Empty;
			DrawRect(C, X + 4.f * S + LabelW + i * (SegW + SegGap), RowY, SegW, 7.f * S);
		}
	};
	auto Panel = [&](const FString& Name, int32 Rows, TFunctionRef<void(float)> Body)
	{
		const float H = 4.f * S + 9.f * S + Rows * 10.f * S + 2.f * S;
		FVPixel::DrawBox(this, X, Y, PanelW, H, S * 0.75f, Ink, Cyan);
		FVPixel::DrawText(this, Name.ToUpper(), X + 4.f * S, Y + 3.f * S, S, Yellow, nullptr, S * 0.4f, ShadowPurple);
		Body(Y + 3.f * S + 10.f * S);
		Y += H + 2.f * S;
	};
	for (TActorIterator<AFVValentino> It(GetWorld()); It; ++It)
	{
		AFVValentino* V = *It;
		Panel(TEXT("Valentino"), 2, [&](float RY)
		{
			Meter(TEXT("ENERGY"), V->Energy, FLinearColor(0.35f, 0.95f, 0.3f), RY);
			Meter(TEXT("THIRST"), V->Thirst, FLinearColor(0.3f, 0.65f, 1.f), RY + 10.f * S);
		});
		break;
	}
	for (TActorIterator<AFVDog> It(GetWorld()); It; ++It)
	{
		AFVDog* D = *It;
		if (!D->bShowMeter)
		{
			continue;
		}
		Panel(D->DogName.ToString(), 1, [&](float RY)
		{
			Meter(TEXT("ENERGY"), D->Energy, FLinearColor(1.f, 0.65f, 0.15f), RY);
		});
	}

	// keep thought bubbles clear of the meters panel
	const float PanelRight = X + PanelW;
	const float PanelBottom = Y;

	// ---- dogs' thought bubbles ----
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	// nearest first (Diego always first), and never let two bubbles overlap
	TArray<AFVDog*> Dogs;
	for (TActorIterator<AFVDog> It(GetWorld()); It; ++It)
	{
		Dogs.Add(*It);
	}
	Dogs.Sort([&](const AFVDog& A, const AFVDog& B)
	{
		if (A.bShowMeter != B.bShowMeter) return A.bShowMeter;
		return FVector::DistSquared(A.GetActorLocation(), CamLoc) < FVector::DistSquared(B.GetActorLocation(), CamLoc);
	});
	TArray<FBox2D> Drawn;
	for (AFVDog* It : Dogs)
	{
		const FText Thought = It->GetThought();
		const FVector Head = It->GetActorLocation() + FVector(0, 0, 55.f * It->GetActorScale3D().Z);
		const float Dist = FVector::Dist(CamLoc, Head);
		if (Thought.IsEmpty() || Dist > (It->bShowMeter ? 2500.f : 1400.f) ||FVector::DotProduct(CamRot.Vector(), (Head - CamLoc).GetSafeNormal()) < 0.2f)
		{
			continue;
		}
		const FVector Screen = Project(Head, true);
		const float TS = FMath::Max(2.f, FMath::RoundToFloat(S * FMath::Clamp(900.f / FMath::Max(Dist, 1.f), 0.6f, 1.2f)));
		const FString Msg = Thought.ToString().ToUpper();
		// comic-book cloud: room for the round ends and puffy bumps around the words
		const float H = 15.f * TS;
		const float W = FVPixel::TextWidth(Msg, TS) + H * 0.9f;
		const float BX = Screen.X - W * 0.5f;
		float BY = Screen.Y - H - 20.f * TS;
		if (BX < PanelRight && BY < PanelBottom)
		{
			BY = PanelBottom + 2.f * S; // would cover the meters: drop it just below them
		}
		const FBox2D Rect(FVector2D(BX - 2.f * TS, BY - 6.f * TS), FVector2D(BX + W + 2.f * TS, BY + H + 14.f * TS));
		bool bOverlaps = false;
		for (const FBox2D& D : Drawn)
		{
			bOverlaps |= D.Intersect(Rect);
		}
		if (bOverlaps)
		{
			continue;
		}
		Drawn.Add(Rect);
		const FLinearColor Fill(1.f, 1.f, 1.f, 0.95f);
		const FLinearColor Edge(0.1f, 0.08f, 0.2f);
		const float Px = FMath::Max(1.f, FMath::RoundToFloat(TS * 0.5f));
		FVPixel::DrawThoughtCloud(this, BX, BY, W, H, Px, Fill, Edge);
		// three shrinking round "thought bubbles" trailing down to the dog
		float DY = BY + H + 3.f * TS;
		for (int32 i = 0; i < 3; ++i)
		{
			const float R = (2.4f - i * 0.7f) * TS;
			const float DX = Screen.X - i * 1.8f * TS;
			FVPixel::DrawDisc(this, DX, DY + R, R + FMath::Max(Px, 2.f), Px, Edge);
			FVPixel::DrawDisc(this, DX, DY + R, R, Px, Fill);
			DY += R * 2.f + 1.5f * TS;
		}
		FVPixel::DrawTextCentred(this, Msg, BX + W * 0.5f, BY + (H - 7.f * TS) * 0.5f, TS, Edge);
	}
}
