#include "FVHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "FVCarryComponent.h"
#include "FVDog.h"
#include "FVFamilyMemberData.h"
#include "FVForagePlant.h"
#include "FVGameInstance.h"
#include "FVGameModes.h"
#include "FVPixelArt.h"
#include "FVAudio.h"
#include "FVWeather.h"
#include "FVDay.h"
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

void AFVHUD::DrawDaySummary()
{
	const UFVDaySubsystem* D = GetWorld()->GetSubsystem<UFVDaySubsystem>();
	if (!D)
	{
		return;
	}
	const float CW = Canvas->ClipX, CH = Canvas->ClipY;
	const float P = Px();
	const float S = FMath::Max(2.f, FMath::RoundToFloat(P * 0.7f));
	const float T = GetWorld()->GetRealTimeSeconds();
	DrawRect(FLinearColor(0.02f, 0.f, 0.08f, 0.7f), 0.f, 0.f, CW, CH);
	const float H2 = FMath::RoundToFloat(P * 2.f);
	FVPixel::DrawTextCentred(this, FString::Printf(TEXT("DAY %d COMPLETE"), D->GetDay()), CW * 0.5f, CH * 0.1f, H2, White, &SunsetRows(),
		FMath::RoundToFloat(H2 * 0.45f), ShadowPurple);

	// the day's points, by kind
	const auto& Lines = D->GetSummaryLines();
	const float BoxW = FMath::Min(CW * 0.6f, 70.f * 6.f * S);
	const float BoxX = (CW - BoxW) * 0.5f;
	const float BoxY = CH * 0.28f;
	const float BoxH = 6.f * S + (Lines.Num() + 2) * 11.f * S + 4.f * S;
	FVPixel::DrawBox(this, BoxX, BoxY, BoxW, BoxH, S * 0.75f, Ink, Cyan);
	float Y = BoxY + 5.f * S;
	for (const auto& L : Lines)
	{
		FVPixel::DrawText(this, L.Key, BoxX + 5.f * S, Y, S, L.Value > 0 ? White : FLinearColor(1.f, 0.55f, 0.4f));
		const FString V = FString::Printf(TEXT("%d"), L.Value);
		FVPixel::DrawText(this, V, BoxX + BoxW - 5.f * S - FVPixel::TextWidth(V, S), Y, S, Yellow);
		Y += 11.f * S;
	}
	DrawRect(Cyan, BoxX + 5.f * S, Y + 2.f * S, BoxW - 10.f * S, S * 0.5f);
	Y += 6.f * S;
	FVPixel::DrawText(this, TEXT("TOTAL"), BoxX + 5.f * S, Y, S * 1.2f, Yellow);
	const FString Tot = FString::Printf(TEXT("%06d"), D->GetScore());
	FVPixel::DrawText(this, Tot, BoxX + BoxW - 5.f * S - FVPixel::TextWidth(Tot, S * 1.2f), Y, S * 1.2f, Yellow, nullptr, S * 0.5f, ShadowPurple);

	Y = BoxY + BoxH + 4.f * S;
	if (D->IsNewHighScore())
	{
		if (FMath::Fmod(T, 0.8f) < 0.5f)
		{
			FVPixel::DrawTextCentred(this, TEXT("NEW HI-SCORE!"), CW * 0.5f, Y, S * 1.6f, Red, nullptr, S * 0.6f, ShadowPurple);
		}
	}
	else
	{
		FVPixel::DrawTextCentred(this, FString::Printf(TEXT("HI-SCORE %06d"), D->GetHighScore()), CW * 0.5f, Y, S * 1.2f, FLinearColor(0.6f, 0.6f, 0.7f));
	}

	// NEXT DAY / MAIN MENU
	const TPair<const TCHAR*, FName> Buttons[] = {{TEXT("NEXT DAY"), TEXT("NextDay")}, {TEXT("MAIN MENU"), TEXT("MainMenu")}};
	const float BW = FVPixel::TextWidth(TEXT("MAIN MENU"), P) + 12.f * P;
	const float BH = 11.f * P;
	float BX = CW * 0.5f - BW - 3.f * P;
	const float BY = Y + 14.f * S;
	for (const auto& B : Buttons)
	{
		const bool bSel = Hover == B.Value;
		const float Bounce = bSel ? -FMath::Abs(FMath::Sin(T * 9.f)) * P : 0.f;
		FVPixel::DrawBox(this, BX, BY + Bounce, BW, BH, P, bSel ? Magenta : Ink, bSel ? Yellow : Cyan);
		FVPixel::DrawTextCentred(this, B.Key, BX + BW * 0.5f, BY + Bounce + 2.f * P, P, bSel ? Yellow : White, nullptr, P * 0.5f, ShadowPurple);
		AddHitBox(FVector2D(BX, BY), FVector2D(BW, BH), B.Value, true);
		BX += BW + 6.f * P;
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
	else if (BoxName == TEXT("NextDay") && PC)
	{
		PC->NextDay();
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
		if (FPC->IsSummaryOpen())
		{
			DrawDaySummary();
			return;
		}
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

	// ---- the day: TO DO list under the meters ----
	const UFVDaySubsystem* DayDir = GetWorld()->GetSubsystem<UFVDaySubsystem>();
	float ListW = PanelW;
	if (DayDir && DayDir->IsRunning() && DayDir->GetGoals().Num() > 0)
	{
		TArray<FString> Lines;
		for (const FFVGoal& G : DayDir->GetGoals())
		{
			FString L = G.Text;
			if (G.Needed > 0.f)
			{
				L += FString::Printf(TEXT(" %d/%d"), FMath::Min(FMath::FloorToInt(G.Progress), FMath::RoundToInt(G.Needed)), FMath::RoundToInt(G.Needed));
			}
			Lines.Add(L);
			ListW = FMath::Max(ListW, 4.f * S + 9.f * S + FVPixel::TextWidth(L, S) + 4.f * S);
		}
		const float H = 4.f * S + 9.f * S + Lines.Num() * 10.f * S + 2.f * S;
		FVPixel::DrawBox(this, X, Y, ListW, H, S * 0.75f, Ink, Cyan);
		FVPixel::DrawText(this, TEXT("TO DO"), X + 4.f * S, Y + 3.f * S, S, Yellow, nullptr, S * 0.4f, ShadowPurple);
		float LY = Y + 3.f * S + 10.f * S;
		for (const FString& L : Lines)
		{
			// a little blinking arrow in front of each task
			FVPixel::DrawText(this, TEXT(">"), X + 4.f * S, LY, S, FMath::Fmod(T, 1.f) < 0.6f ? Yellow : Cyan);
			FVPixel::DrawText(this, L, X + 4.f * S + 9.f * S, LY, S, White);
			LY += 10.f * S;
		}
		Y += H + 2.f * S;
	}

	// ---- clock and score (top centre) ----
	if (DayDir && DayDir->IsRunning())
	{
		const FString Clock = FString::Printf(TEXT("DAY %d  %s"), DayDir->GetDay(), *DayDir->GetClockText());
		const FString Sc = FString::Printf(TEXT("SCORE %06d"), DayDir->GetScore());
		const float BW = FMath::Max(FVPixel::TextWidth(Clock, S), FVPixel::TextWidth(Sc, S)) + 8.f * S;
		const float BH = 4.f * S + 7.f * S + 3.f * S + 7.f * S + 3.f * S;
		const float BX = (Canvas->ClipX - BW) * 0.5f;
		FVPixel::DrawBox(this, BX, 4.f * P, BW, BH, S * 0.75f, Ink, Cyan);
		FVPixel::DrawTextCentred(this, Clock, Canvas->ClipX * 0.5f, 4.f * P + 4.f * S, S, DayDir->GetHour() >= DayDir->BedtimeHour ? FLinearColor(1.f, 0.55f, 0.2f) : Cyan);
		FVPixel::DrawTextCentred(this, Sc, Canvas->ClipX * 0.5f, 4.f * P + 14.f * S, S, Yellow, nullptr, S * 0.4f, ShadowPurple);

		// points popping up and floating away
		float PY = CH * 0.3f;
		for (const FFVPopup& Pop : DayDir->GetPopups())
		{
			const float A = FMath::Clamp(2.6f - Pop.Age, 0.f, 1.f);
			const float Size = (Pop.bBig ? 1.3f : 1.f) * S;
			const float Rise = Pop.Age * 12.f * S;
			FVPixel::DrawTextCentred(this, Pop.Text, Canvas->ClipX * 0.5f, PY - Rise, Size, FLinearColor(1.f, 0.9f, 0.2f, A), nullptr,
				Size * 0.45f, FLinearColor(0.22f, 0.03f, 0.35f, A));
			PY += 10.f * Size;
		}
		// big announcements ("DAY 2", "BEDTIME!")
		const FString Banner = DayDir->GetBanner();
		if (!Banner.IsEmpty())
		{
			const float BP = FMath::RoundToFloat(P * 2.2f);
			FVPixel::DrawTextCentred(this, Banner, Canvas->ClipX * 0.5f, CH * 0.18f, BP, White, &SunsetRows(), FMath::RoundToFloat(BP * 0.45f), ShadowPurple);
		}
	}

	// keep thought bubbles clear of the meters and list
	const float PanelRight = X + ListW;
	const float PanelBottom = Y;

	// ---- today's weather (top-right): a little sun or cloud, the name and the temperature ----
	if (const UFVWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UFVWeatherSubsystem>())
	{
		const FString Name = Weather->GetWeatherName();
		if (!Name.IsEmpty())
		{
			const FString Temp = FString::Printf(TEXT("%dC"), FMath::RoundToInt(Weather->GetTemperature()));
			const float IconW = 10.f * S;
			const float W = 4.f * S + IconW + 3.f * S + FMath::Max(FVPixel::TextWidth(Name, S), FVPixel::TextWidth(Temp, S)) + 4.f * S;
			const float H = 4.f * S + 7.f * S + 3.f * S + 7.f * S + 3.f * S;
			const float WX = Canvas->ClipX - 4.f * P - W;
			const float WY = 4.f * P;
			FVPixel::DrawBox(this, WX, WY, W, H, S * 0.75f, Ink, Cyan);
			const float IX = WX + 4.f * S + IconW * 0.5f, IY = WY + H * 0.5f;
			if (Weather->IsCloudy())
			{
				FVPixel::DrawDisc(this, IX - 2.f * S, IY + S, 3.f * S, S, FLinearColor(0.8f, 0.82f, 0.9f));
				FVPixel::DrawDisc(this, IX + 1.5f * S, IY - S, 3.5f * S, S, FLinearColor(0.9f, 0.92f, 0.97f));
				FVPixel::DrawDisc(this, IX + 3.f * S, IY + 1.5f * S, 2.5f * S, S, FLinearColor(0.8f, 0.82f, 0.9f));
			}
			else
			{
				const bool bHot = Weather->GetTemperature() >= 29.f;
				const FLinearColor SunC = bHot ? FLinearColor(1.f, 0.55f, 0.15f) : Yellow;
				// rays that slowly pulse, then the sun
				const float Ray = (FMath::Fmod(T, 1.2f) < 0.6f ? 5.f : 4.5f) * S;
				for (int32 a = 0; a < 8; ++a)
				{
					const float Ang = a * PI / 4.f;
					DrawRect(SunC, FMath::RoundToFloat(IX + FMath::Cos(Ang) * Ray - S * 0.5f), FMath::RoundToFloat(IY + FMath::Sin(Ang) * Ray - S * 0.5f), S, S);
				}
				FVPixel::DrawDisc(this, IX, IY, 3.f * S, S, SunC);
			}
			const float TX = WX + 4.f * S + IconW + 3.f * S;
			FVPixel::DrawText(this, Name, TX, WY + 4.f * S, S, Yellow, nullptr, S * 0.4f, ShadowPurple);
			FVPixel::DrawText(this, Temp, TX, WY + 4.f * S + 10.f * S, S, White);
		}
	}

	// ---- dogs' thought bubbles ----
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	// ---- ripe plants twinkle now and then, so you can spot food from across the garden ----
	{
		const float Now = GetWorld()->GetTimeSeconds();
		for (TActorIterator<AFVForagePlant> It(GetWorld()); It; ++It)
		{
			if (!It->IsRipe())
			{
				continue;
			}
			const FVector C = It->GetProduceCentre() + FVector(0, 0, 8.f);
			const float Dist = FVector::Dist(CamLoc, C);
			if (Dist > 3000.f || FVector::DotProduct(CamRot.Vector(), (C - CamLoc).GetSafeNormal()) < 0.3f)
			{
				continue;
			}
			// each plant: a 0.7 s twinkle every 2.6 s, at its own moment
			const float Cycle = FMath::Fmod(Now / 2.6f + It->SparklePhase, 1.f) * 2.6f;
			if (Cycle > 0.7f)
			{
				continue;
			}
			const float Grow = FMath::Sin(PI * Cycle / 0.7f); // 0 -> 1 -> 0
			const FVector Sc = Project(C, true);
			if (Sc.Z <= 0.f)
			{
				continue;
			}
			const float SP = FMath::Max(2.f, FMath::RoundToFloat(S * 0.5f * FMath::Clamp(1200.f / FMath::Max(Dist, 1.f), 0.6f, 1.4f)));
			const int32 Arm = FMath::RoundToInt(1.f + 3.f * Grow);
			const float X0 = FMath::RoundToFloat(Sc.X), Y0 = FMath::RoundToFloat(Sc.Y);
			const FLinearColor Star(1.f, 0.97f, 0.6f, 0.95f);
			DrawRect(FLinearColor::White, X0 - SP * 0.5f, Y0 - SP * 0.5f, SP, SP);
			for (int32 k = 1; k <= Arm; ++k)
			{
				const FLinearColor C2 = k == Arm ? Star * FLinearColor(1, 1, 1, 0.6f) : Star;
				DrawRect(C2, X0 - SP * 0.5f + k * SP, Y0 - SP * 0.5f, SP, SP);
				DrawRect(C2, X0 - SP * 0.5f - k * SP, Y0 - SP * 0.5f, SP, SP);
				DrawRect(C2, X0 - SP * 0.5f, Y0 - SP * 0.5f + k * SP, SP, SP);
				DrawRect(C2, X0 - SP * 0.5f, Y0 - SP * 0.5f - k * SP, SP, SP);
			}
			if (Arm >= 3) // little diagonal glints at the peak
			{
				for (int32 dx = -1; dx <= 1; dx += 2)
				{
					for (int32 dy = -1; dy <= 1; dy += 2)
					{
						DrawRect(Star, X0 - SP * 0.5f + dx * SP, Y0 - SP * 0.5f + dy * SP, SP, SP);
					}
				}
			}
		}
	}

	// ---- thought bubbles: Valentino's wishes and the dogs' thoughts ----
	struct FThinker
	{
		FVector Head;
		FString Msg;
		float MaxDist;
		int32 Priority; // lower draws first
	};
	TArray<FThinker> Thinkers;
	for (TActorIterator<AFVValentino> It(GetWorld()); It; ++It)
	{
		const FString Wish = It->GetThought();
		if (!Wish.IsEmpty())
		{
			Thinkers.Add({It->GetActorLocation() + FVector(0, 0, 14.f), Wish, 2500.f, 0});
		}
	}
	for (TActorIterator<AFVDog> It(GetWorld()); It; ++It)
	{
		const FText Thought = It->GetThought();
		if (!Thought.IsEmpty())
		{
			Thinkers.Add({It->GetActorLocation() + FVector(0, 0, 55.f * It->GetActorScale3D().Z), Thought.ToString().ToUpper(),
				It->bShowMeter ? 2500.f : 1400.f, It->bShowMeter ? 1 : 2});
		}
	}
	// Valentino, then Diego, then nearest first; never let two bubbles overlap
	Thinkers.Sort([&](const FThinker& A, const FThinker& B)
	{
		if (A.Priority != B.Priority) return A.Priority < B.Priority;
		return FVector::DistSquared(A.Head, CamLoc) < FVector::DistSquared(B.Head, CamLoc);
	});
	TArray<FBox2D> Drawn;
	for (const FThinker& It : Thinkers)
	{
		const FVector Head = It.Head;
		const float Dist = FVector::Dist(CamLoc, Head);
		if (Dist > It.MaxDist || FVector::DotProduct(CamRot.Vector(), (Head - CamLoc).GetSafeNormal()) < 0.2f)
		{
			continue;
		}
		const FVector Screen = Project(Head, true);
		const float TS = FMath::Max(2.f, FMath::RoundToFloat(S * FMath::Clamp(900.f / FMath::Max(Dist, 1.f), 0.6f, 1.2f)));
		const FString& Msg = It.Msg;
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
