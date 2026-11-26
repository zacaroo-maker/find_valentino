#include "FVMenuHUD.h"

#include "Camera/CameraActor.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "FVFamilyMemberData.h"
#include "FVGameInstance.h"
#include "FVPixelArt.h"
#include "FVAudio.h"
#include "FVSaveGame.h"
#include "Components/AudioComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	// 80s arcade palette
	const FLinearColor Ink(0.03f, 0.02f, 0.12f, 0.88f);     // button fill
	const FLinearColor Cyan(0.25f, 0.85f, 1.f);
	const FLinearColor Magenta(0.95f, 0.12f, 0.55f, 0.96f);
	const FLinearColor Yellow(1.f, 0.88f, 0.15f);
	const FLinearColor White(1.f, 1.f, 1.f);
	const FLinearColor Red(1.f, 0.2f, 0.2f);
	const FLinearColor Grey(0.6f, 0.6f, 0.7f);
	const FLinearColor ShadowPurple(0.22f, 0.03f, 0.35f);

	const TArray<FLinearColor>& SunsetRows()
	{
		static const TArray<FLinearColor> Rows = {
			FLinearColor(1.f, 0.95f, 0.35f), FLinearColor(1.f, 0.82f, 0.2f), FLinearColor(1.f, 0.62f, 0.15f),
			FLinearColor(1.f, 0.45f, 0.2f), FLinearColor(1.f, 0.3f, 0.35f), FLinearColor(0.95f, 0.2f, 0.5f),
			FLinearColor(0.85f, 0.15f, 0.65f)};
		return Rows;
	}

	void DrawHeart(AHUD* Hud, float X, float Y, float Px, const FLinearColor& C)
	{
		static const char* Rows[6] = {".##.##.", "#######", "#######", ".#####.", "..###..", "...#..."};
		for (int32 r = 0; r < 6; ++r)
		{
			for (int32 c = 0; c < 7; ++c)
			{
				if (Rows[r][c] == '#')
				{
					Hud->DrawRect(C, X + c * Px, Y + r * Px, Px, Px);
				}
			}
		}
	}

	IConsoleVariable* CVar(const TCHAR* Name)
	{
		return IConsoleManager::Get().FindConsoleVariable(Name);
	}
}

// ---------------------------------------------------------------- setup

void AFVMenuHUD::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
		// HUD hit boxes get their clicks through the game's input, so it can't be "UI only"
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
	if (Family.Num() == 0)
	{
		for (const TCHAR* Id : {TEXT("Dad"), TEXT("Mom"), TEXT("Grandpa"), TEXT("Grandma"), TEXT("OlderKid"), TEXT("YoungerKid"), TEXT("Friend")})
		{
			const FString Path = FString::Printf(TEXT("/Game/Characters/Family/Data/DA_%s.DA_%s"), Id, Id);
			if (UFVFamilyMemberData* M = LoadObject<UFVFamilyMemberData>(nullptr, *Path))
			{
				Family.Add(M);
			}
		}
	}
	// keep the previous choice when coming back from the garden, and skip the title screen then
	if (const UFVGameInstance* GI = Cast<UFVGameInstance>(GetGameInstance()))
	{
		const int32 I = Family.IndexOfByKey(GI->SelectedMember);
		Selected = I == INDEX_NONE ? 0 : I;
		if (GI->SelectedMember)
		{
			Screen = EFVMenuScreen::Main;
		}
	}
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MenuCamera")))
		{
			MenuCamera = *It;
			CameraBase = MenuCamera->GetActorRotation();
			break;
		}
	}
	ApplyPixelLook(bPixelLook);
	StartMusic();
	if (const UFVSaveGame* Save = UFVSaveGame::Load())
	{
		SavedHighScore = Save->HighScore;
		SavedLastScore = Save->LastScore;
	}
}

void AFVMenuHUD::StartMusic()
{
	if (!Music)
	{
		Music = FVAudio::Play2D(this, EFVSound::MenuMusic, 0.45f);
	}
}

void AFVMenuHUD::SetCursor(int32 NewCursor)
{
	if (NewCursor != Cursor)
	{
		Cursor = NewCursor;
		FVAudio::Play2D(this, EFVSound::UIMove, 0.35f);
	}
}

void AFVMenuHUD::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Music)
	{
		Music->Stop();
		Music = nullptr;
	}
	ApplyPixelLook(false);
	Super::EndPlay(Reason);
}

void AFVMenuHUD::ApplyPixelLook(bool bOn)
{
	IConsoleVariable* SP = CVar(TEXT("r.ScreenPercentage"));
	IConsoleVariable* AA = CVar(TEXT("r.AntiAliasingMethod"));
	IConsoleVariable* UQ = CVar(TEXT("r.Upscale.Quality"));
	if (!SP || !AA || !UQ)
	{
		return;
	}
	if (bOn && !bPixelApplied)
	{
		SavedScreenPercentage = SP->GetFloat();
		SavedAA = AA->GetInt();
		SavedUpscale = UQ->GetInt();
		// render small, then blow it up with nearest-neighbour: big chunky pixels
		AA->Set(0, ECVF_SetByConsole);
		UQ->Set(0, ECVF_SetByConsole);
		SP->Set(18.f, ECVF_SetByConsole);
		bPixelApplied = true;
	}
	else if (!bOn && bPixelApplied)
	{
		SP->Set(SavedScreenPercentage, ECVF_SetByConsole);
		AA->Set(SavedAA, ECVF_SetByConsole);
		UQ->Set(SavedUpscale, ECVF_SetByConsole);
		bPixelApplied = false;
	}
}

// ---------------------------------------------------------------- navigation

int32 AFVMenuHUD::ItemCount() const
{
	switch (Screen)
	{
	case EFVMenuScreen::Title: return 1;
	case EFVMenuScreen::Main: return 4;
	case EFVMenuScreen::Who: return Family.Num() + 2; // members, START!, BACK
	case EFVMenuScreen::Settings: return 5;
	}
	return 1;
}

void AFVMenuHUD::GoTo(EFVMenuScreen S, int32 StartCursor)
{
	Screen = S;
	Cursor = StartCursor;
	ScreenTime = 0.f;
}

void AFVMenuHUD::Back()
{
	if (Screen != EFVMenuScreen::Title)
	{
		FVAudio::Play2D(this, EFVSound::UIBack, 0.5f);
	}
	switch (Screen)
	{
	case EFVMenuScreen::Main: GoTo(EFVMenuScreen::Title); break;
	case EFVMenuScreen::Who: GoTo(EFVMenuScreen::Main, 1); break;
	case EFVMenuScreen::Settings: GoTo(EFVMenuScreen::Main, 2); break;
	default: break;
	}
}

void AFVMenuHUD::StartGame()
{
	UFVGameInstance* GI = Cast<UFVGameInstance>(GetGameInstance());
	if (GI && Family.IsValidIndex(Selected))
	{
		GI->SelectedMember = Family[Selected];
	}
	if (GI)
	{
		GI->DayNumber = 1; // a new game starts on day one
	}
	ApplyPixelLook(false);
	FVAudio::Play2D(this, EFVSound::UIStart, 0.6f, 1.f, true); // the fanfare carries on into the garden
	UGameplayStatics::OpenLevel(this, GI ? GI->GameLevel : FName(TEXT("L_Backyard")));
}

void AFVMenuHUD::Activate(int32 Index)
{
	// a confirming blip (START and BACK have their own sounds)
	const bool bIsStart = (Screen == EFVMenuScreen::Main && Index == 0) || (Screen == EFVMenuScreen::Who && Index == Family.Num());
	const bool bIsBack = (Screen == EFVMenuScreen::Who && Index == Family.Num() + 1) || (Screen == EFVMenuScreen::Settings && Index == 4);
	if (!bIsStart && !bIsBack)
	{
		FVAudio::Play2D(this, EFVSound::UISelect, 0.5f);
	}
	switch (Screen)
	{
	case EFVMenuScreen::Title:
		GoTo(EFVMenuScreen::Main);
		break;
	case EFVMenuScreen::Main:
		if (Index == 0) StartGame();
		else if (Index == 1) GoTo(EFVMenuScreen::Who, Selected);
		else if (Index == 2) GoTo(EFVMenuScreen::Settings);
		else UKismetSystemLibrary::QuitGame(this, GetOwningPlayerController(), EQuitPreference::Quit, false);
		break;
	case EFVMenuScreen::Who:
		if (Index < Family.Num())
		{
			Selected = Index;
			Cursor = Family.Num(); // jump to START!
		}
		else if (Index == Family.Num()) StartGame();
		else Back();
		break;
	case EFVMenuScreen::Settings:
		if (Index == 0)
		{
			if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
			{
				S->SetFullscreenMode(S->GetFullscreenMode() == EWindowMode::Windowed ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
				S->ApplySettings(false);
			}
		}
		else if (Index == 1)
		{
			bPixelLook = !bPixelLook;
			ApplyPixelLook(bPixelLook);
		}
		else if (Index == 2 || Index == 3)
		{
			if (UFVGameInstance* GI = Cast<UFVGameInstance>(GetGameInstance()))
			{
				if (Index == 2)
				{
					GI->bSoundOn = !GI->bSoundOn;
				}
				else
				{
					GI->bMusicOn = !GI->bMusicOn;
					if (!GI->bMusicOn && Music)
					{
						Music->FadeOut(0.4f, 0.f);
						Music = nullptr;
					}
					else if (GI->bMusicOn)
					{
						StartMusic();
					}
				}
			}
		}
		else Back();
		break;
	}
}

void AFVMenuHUD::HandleKeys()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	auto Pressed = [PC](std::initializer_list<FKey> Keys)
	{
		for (const FKey& K : Keys)
		{
			if (PC->WasInputKeyJustPressed(K))
			{
				return true;
			}
		}
		return false;
	};
	const int32 N = ItemCount();
	if (Pressed({EKeys::Up, EKeys::W, EKeys::Gamepad_DPad_Up}))
	{
		SetCursor((Cursor - 1 + N) % N);
	}
	if (Pressed({EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down}))
	{
		SetCursor((Cursor + 1) % N);
	}
	if (Pressed({EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom}) && ScreenTime > 0.15f)
	{
		Activate(Cursor);
	}
	else if (Pressed({EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right}))
	{
		Back();
	}
}

void AFVMenuHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	const FString S = BoxName.ToString();
	if (S.StartsWith(TEXT("Item_")) && ScreenTime > 0.15f)
	{
		Activate(FCString::Atoi(*S.RightChop(5)));
	}
}

void AFVMenuHUD::NotifyHitBoxBeginCursorOver(FName BoxName)
{
	const FString S = BoxName.ToString();
	if (S.StartsWith(TEXT("Item_")) && Screen != EFVMenuScreen::Title)
	{
		SetCursor(FCString::Atoi(*S.RightChop(5)));
	}
}

// ---------------------------------------------------------------- drawing

float AFVMenuHUD::MenuButton(const FString& Label, int32 Index, float CentreX, float Y, float MinWidth, float Px)
{
	const bool bSel = (Cursor == Index);
	const float W = FMath::Max(MinWidth, FVPixel::TextWidth(Label, Px) + 8.f * Px);
	const float H = 11.f * Px;
	const float Bounce = bSel ? -FMath::Abs(FMath::Sin(Time * 9.f)) * Px : 0.f;
	const float X = CentreX - W * 0.5f;
	FVPixel::DrawBox(this, X, Y + Bounce, W, H, Px, bSel ? Magenta : Ink, bSel ? Yellow : Cyan);
	FVPixel::DrawTextCentred(this, Label, CentreX, Y + Bounce + 2.f * Px, Px, bSel ? Yellow : White, nullptr, Px * 0.5f, ShadowPurple);
	if (bSel)
	{
		// Valentino points at the chosen option
		const float TP = Px * 0.75f;
		FVPixel::DrawTortoise(this, X - 19.f * TP, Y + Bounce + (H - 7.f * TP) * 0.5f, TP, FMath::FloorToInt(Time * 6.f));
	}
	AddHitBox(FVector2D(X, Y), FVector2D(W, H), FName(*FString::Printf(TEXT("Item_%d"), Index)), true);
	return H;
}

void AFVMenuHUD::DrawLogo(float CentreY, float Px)
{
	const float Bob = FMath::RoundToFloat(FMath::Sin(Time * 2.2f) * P * 0.6f);
	const float Shadow = FMath::Max(2.f, FMath::RoundToFloat(Px * 0.45f));
	FVPixel::DrawTextCentred(this, TEXT("FINDING"), CW * 0.5f, CentreY - 8.f * Px + Bob, Px, White, &SunsetRows(), Shadow, ShadowPurple);
	FVPixel::DrawTextCentred(this, TEXT("VALENTINO!"), CW * 0.5f, CentreY + 1.f * Px + Bob, Px, White, &SunsetRows(), Shadow, ShadowPurple);
}

void AFVMenuHUD::DrawTopBar()
{
	const float S = FMath::Max(2.f, FMath::RoundToFloat(P * 0.6f));
	const float Y = 3.f * S;
	FVPixel::DrawText(this, TEXT("1UP"), CW * 0.05f, Y, S, Red);
	FVPixel::DrawText(this, FString::Printf(TEXT("%06d"), SavedLastScore), CW * 0.05f, Y + 9.f * S, S, White);
	FVPixel::DrawTextCentred(this, TEXT("HI-SCORE"), CW * 0.5f, Y, S, Red);
	FVPixel::DrawTextCentred(this, FString::Printf(TEXT("%06d"), SavedHighScore), CW * 0.5f, Y + 9.f * S, S, White);
	const FString Credit = TEXT("CREDIT 01");
	FVPixel::DrawText(this, Credit, CW * 0.95f - FVPixel::TextWidth(Credit, S), Y, S, Cyan);
}

void AFVMenuHUD::DrawTitle()
{
	const float L = FMath::RoundToFloat(P * 3.f);
	DrawLogo(CH * 0.36f, L);

	const float M = P * 1.2f;
	if (FMath::Fmod(Time, 1.f) < 0.62f)
	{
		FVPixel::DrawTextCentred(this, TEXT("PRESS START"), CW * 0.5f, CH * 0.62f, M, Yellow, nullptr, M * 0.5f, ShadowPurple);
	}
	const float S = FMath::Max(2.f, FMath::RoundToFloat(P * 0.6f));
	FVPixel::DrawTextCentred(this, TEXT("ENTER OR CLICK"), CW * 0.5f, CH * 0.62f + 12.f * M * 0.85f, S, Grey);

	// Valentino plodding across the bottom of the screen
	const float TP = P * 1.1f;
	const float Span = CW + 40.f * TP;
	const float X = FMath::Fmod(Time * 22.f * TP * 0.25f, Span) - 20.f * TP;
	FVPixel::DrawTortoise(this, X, CH * 0.84f, TP, FMath::FloorToInt(Time * 4.f));

	FVPixel::DrawTextCentred(this, TEXT("(C) 2026  A FAMILY GARDEN GAME"), CW * 0.5f, CH * 0.94f, S, Grey);
	// anywhere on screen counts as "start"
	AddHitBox(FVector2D(0.f, 0.f), FVector2D(CW, CH), TEXT("Item_0"), true);
}

void AFVMenuHUD::DrawMain()
{
	DrawLogo(CH * 0.2f, FMath::RoundToFloat(P * 1.8f));
	const float M = P;
	const float MinW = FVPixel::TextWidth(TEXT("WHO'S LOOKING?"), M) + 10.f * M;
	float Y = CH * 0.38f;
	const TCHAR* Items[4] = {TEXT("PLAY"), TEXT("WHO'S LOOKING?"), TEXT("SETTINGS"), TEXT("QUIT")};
	for (int32 i = 0; i < 4; ++i)
	{
		Y += MenuButton(Items[i], i, CW * 0.5f, Y, MinW, M) + 3.f * M;
	}
	const float S = FMath::Max(2.f, FMath::RoundToFloat(P * 0.6f));
	if (Family.IsValidIndex(Selected) && Family[Selected])
	{
		const FString As = FString::Printf(TEXT("PLAYING AS: %s"), *Family[Selected]->DisplayName.ToString());
		FVPixel::DrawTextCentred(this, As, CW * 0.5f, Y + 2.f * M, S * 1.3f, Yellow, nullptr, S * 0.5f, ShadowPurple);
	}
	FVPixel::DrawTextCentred(this, TEXT("ARROWS OR MOUSE TO MOVE - ENTER OR CLICK TO PICK"), CW * 0.5f, CH * 0.93f, S, Grey);
}

void AFVMenuHUD::DrawWho()
{
	const float H2 = FMath::RoundToFloat(P * 1.8f);
	FVPixel::DrawTextCentred(this, TEXT("WHO'S LOOKING?"), CW * 0.5f, CH * 0.1f, H2, White, &SunsetRows(), FMath::RoundToFloat(H2 * 0.45f), ShadowPurple);

	// list of family members (left)
	const float M = P * 0.9f;
	float ListW = 0.f;
	for (const UFVFamilyMemberData* F : Family)
	{
		if (F) ListW = FMath::Max(ListW, FVPixel::TextWidth(F->DisplayName.ToString().ToUpper(), M));
	}
	ListW += 10.f * M;
	const float ListCX = CW * 0.08f + 20.f * M + ListW * 0.5f;
	float Y = CH * 0.25f;
	for (int32 i = 0; i < Family.Num(); ++i)
	{
		const FString Name = Family[i] ? Family[i]->DisplayName.ToString() : TEXT("?");
		const float H = MenuButton(Name, i, ListCX, Y, ListW, M);
		if (i == Selected)
		{
			// a heart marks who is picked (pointing at someone only previews them)
			const float HS = M * 0.7f;
			DrawHeart(this, ListCX + ListW * 0.5f - 10.f * HS, Y + (H - 6.f * HS) * 0.5f, HS, Red);
		}
		Y += H + 1.6f * M;
	}

	// details card (right): whoever is pointed at, otherwise whoever is picked
	const int32 Show = Cursor < Family.Num() ? Cursor : Selected;
	if (!Family.IsValidIndex(Show) || !Family[Show])
	{
		return;
	}
	const UFVFamilyMemberData* F = Family[Show];
	const float CX = ListCX + ListW * 0.5f + 8.f * M;
	const float CWd = CW * 0.92f - CX;
	const float CY = CH * 0.25f;
	const float CHt = CH * 0.44f;
	FVPixel::DrawBox(this, CX, CY, CWd, CHt, P * 0.75f, Ink, Cyan);
	const float Pad = 4.f * P;
	const float NamePx = FMath::RoundToFloat(P * 1.5f);
	FVPixel::DrawText(this, F->DisplayName.ToString(), CX + Pad, CY + Pad, NamePx, Yellow, nullptr, NamePx * 0.4f, ShadowPurple);

	const float S = FMath::Max(2.f, FMath::RoundToFloat(P * 0.6f));
	float LY = CY + Pad + 11.f * NamePx;
	// Valentino's opinion, in hearts
	FVPixel::DrawText(this, TEXT("VALENTINO:"), CX + Pad, LY, S, Cyan);
	const int32 Hearts = FMath::Clamp(FMath::RoundToInt(F->ValentinoComfort * 3.f), 1, 5);
	for (int32 h = 0; h < 5; ++h)
	{
		DrawHeart(this, CX + Pad + FVPixel::TextWidth(TEXT("VALENTINO: "), S) + h * 9.f * S, LY, S, h < Hearts ? Red : FLinearColor(0.25f, 0.2f, 0.3f));
	}
	LY += 10.f * S;
	// speed, as a bar of blocks
	FVPixel::DrawText(this, TEXT("SPEED:"), CX + Pad, LY, S, Cyan);
	const int32 Blocks = FMath::Clamp(FMath::RoundToInt(F->WalkSpeed / 40.f), 1, 10);
	for (int32 b = 0; b < 10; ++b)
	{
		DrawRect(b < Blocks ? Yellow : FLinearColor(0.25f, 0.2f, 0.3f), CX + Pad + FVPixel::TextWidth(TEXT("VALENTINO: "), S) + b * 4.f * S, LY, 3.f * S, 7.f * S);
	}
	LY += 14.f * S;
	for (const FString& Line : FVPixel::Wrap(F->Description.ToString(), S, CWd - 2.f * Pad))
	{
		FVPixel::DrawText(this, Line, CX + Pad, LY, S, White);
		LY += 10.f * S;
	}
	const bool bPicked = (Show == Selected);
	FVPixel::DrawText(this, bPicked ? TEXT("PICKED!") : TEXT("CLICK OR ENTER TO PICK"), CX + Pad, CY + CHt - Pad - 7.f * S, S,
		bPicked ? Yellow : (FMath::Fmod(Time, 1.f) < 0.6f ? Cyan : Grey));

	// START! and BACK under the card
	const float BY = CY + CHt + 3.f * P;
	const float BW = CWd * 0.5f - 2.f * P;
	MenuButton(TEXT("START!"), Family.Num(), CX + BW * 0.5f, BY, BW, P);
	MenuButton(TEXT("BACK"), Family.Num() + 1, CX + CWd - BW * 0.5f, BY, BW, P);
}

void AFVMenuHUD::DrawSettings()
{
	const float H2 = FMath::RoundToFloat(P * 1.8f);
	FVPixel::DrawTextCentred(this, TEXT("SETTINGS"), CW * 0.5f, CH * 0.14f, H2, White, &SunsetRows(), FMath::RoundToFloat(H2 * 0.45f), ShadowPurple);
	const UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const bool bFull = S && S->GetFullscreenMode() != EWindowMode::Windowed;
	const float M = P;
	const UFVGameInstance* GI = Cast<UFVGameInstance>(GetGameInstance());
	const bool bSound = !GI || GI->bSoundOn;
	const bool bMusic = !GI || GI->bMusicOn;
	const FString Items[5] = {
		FString::Printf(TEXT("FULLSCREEN: %s"), bFull ? TEXT("ON") : TEXT("OFF")),
		FString::Printf(TEXT("PIXEL LOOK: %s"), bPixelLook ? TEXT("ON") : TEXT("OFF")),
		FString::Printf(TEXT("SOUND: %s"), bSound ? TEXT("ON") : TEXT("OFF")),
		FString::Printf(TEXT("MUSIC: %s"), bMusic ? TEXT("ON") : TEXT("OFF")),
		TEXT("BACK")};
	const float MinW = FVPixel::TextWidth(TEXT("FULLSCREEN: OFF"), M) + 10.f * M;
	float Y = CH * 0.32f;
	for (int32 i = 0; i < 5; ++i)
	{
		Y += MenuButton(Items[i], i, CW * 0.5f, Y, MinW, M) + 2.5f * M;
	}
}

void AFVMenuHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	const float Now = GetWorld()->GetRealTimeSeconds();
	const float Dt = LastDrawTime < 0.f ? 0.f : FMath::Min(Now - LastDrawTime, 0.1f);
	LastDrawTime = Now;
	Time += Dt;
	ScreenTime += Dt;

	CW = Canvas->ClipX;
	CH = Canvas->ClipY;
	P = FMath::Max(2.f, FMath::RoundToFloat(CH / 180.f));

	// the camera drifts slowly over the garden
	if (MenuCamera)
	{
		MenuCamera->SetActorRotation(CameraBase + FRotator(FMath::Sin(Time * 0.11f) * 1.5f, FMath::Sin(Time * 0.07f) * 9.f, 0.f));
	}

	HandleKeys();

	// darken the garden a little so the chunky text pops
	DrawRect(FLinearColor(0.02f, 0.f, 0.08f, Screen == EFVMenuScreen::Title ? 0.25f : 0.45f), 0.f, 0.f, CW, CH);
	DrawTopBar();
	switch (Screen)
	{
	case EFVMenuScreen::Title: DrawTitle(); break;
	case EFVMenuScreen::Main: DrawMain(); break;
	case EFVMenuScreen::Who: DrawWho(); break;
	case EFVMenuScreen::Settings: DrawSettings(); break;
	}
	FVPixel::DrawScanlines(this, CW, CH, FMath::Max(3.f, FMath::RoundToFloat(P * 0.5f)), 0.16f);
}
