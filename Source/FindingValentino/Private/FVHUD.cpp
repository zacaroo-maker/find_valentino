#include "FVHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "FVCarryComponent.h"
#include "FVDog.h"
#include "FVValentino.h"
#include "FVFamilyMemberData.h"
#include "FVGameInstance.h"
#include "FVGameModes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

void AFVHUD::DrawPauseMenu()
{
	UFont* Font = GEngine->GetLargeFont();
	UFont* Medium = GEngine->GetMediumFont();
	const float CW = Canvas->ClipX, CH = Canvas->ClipY;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f, 0.f, CW, CH);
	const FString Title = TEXT("Paused");
	float TW = 0.f, TH = 0.f;
	GetTextSize(Title, TW, TH, Font, 2.f);
	DrawText(Title, FLinearColor(1.f, 0.92f, 0.8f), (CW - TW) * 0.5f, CH * 0.25f, Font, 2.f);
	const float BW = CW * 0.24f, BH = CH * 0.075f, X = (CW - BW) * 0.5f;
	const TPair<const TCHAR*, FName> Buttons[] = { { TEXT("Resume"), TEXT("Resume") }, { TEXT("Main menu"), TEXT("MainMenu") }, { TEXT("Quit"), TEXT("Quit") } };
	for (int32 i = 0; i < 3; ++i)
	{
		const float Y = CH * 0.38f + i * (BH + 16.f);
		DrawRect(Hover == Buttons[i].Value ? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y, BW, BH);
		float W = 0.f, H = 0.f;
		GetTextSize(Buttons[i].Key, W, H, Medium, 1.5f);
		DrawText(Buttons[i].Key, FLinearColor::White, X + (BW - W) * 0.5f, Y + (BH - H) * 0.5f, Medium, 1.5f);
		AddHitBox(FVector2D(X, Y), FVector2D(BW, BH), Buttons[i].Value, true);
	}
}

void AFVHUD::NotifyHitBoxClick(FName BoxName)
{
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
	const APawn* Pawn = GetOwningPawn();
	const UFVCarryComponent* Carry = Pawn ? Pawn->FindComponentByClass<UFVCarryComponent>() : nullptr;
	UFont* Font = GEngine->GetMediumFont();

	auto DrawCentered = [&](const FText& Text, float YFrac, float Scale, const FLinearColor& Color)
	{
		if (Text.IsEmpty())
		{
			return;
		}
		const FString S = Text.ToString();
		float W = 0.f, H = 0.f;
		GetTextSize(S, W, H, Font, Scale);
		const float X = (Canvas->ClipX - W) * 0.5f;
		const float Y = Canvas->ClipY * YFrac;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), X - 16.f, Y - 8.f, W + 32.f, H + 16.f);
		DrawText(S, Color, X, Y, Font, Scale);
	};

	// ---- prompts & messages ----
	if (Carry)
	{
		DrawCentered(Carry->GetPrompt(), PromptY, PromptScale, FLinearColor(1.f, 0.97f, 0.9f));
		DrawCentered(Carry->GetPetPrompt(), PromptY + 0.05f, PromptScale * 0.85f, FLinearColor(1.f, 0.85f, 0.9f));
		DrawCentered(Carry->GetUsePrompt(), PromptY + 0.10f, PromptScale * 0.85f, FLinearColor(0.8f, 0.92f, 1.f));
		DrawCentered(Carry->GetToast(), 0.12f, PromptScale, FLinearColor(1.f, 0.9f, 0.55f));
	}

	// ---- energy meters for both animals (top-left) ----
	const float X = 30.f, BarW = 200.f, BarH = 14.f;
	float Y = 30.f;
	auto Bar = [&](const FString& Label, float Value, const FLinearColor& Col)
	{
		DrawText(Label, FLinearColor::White, X + 8.f, Y - 2.f, Font, 0.9f);
		DrawRect(FLinearColor(0.15f, 0.15f, 0.15f, 0.8f), X + 100.f, Y, BarW, BarH);
		DrawRect(Col, X + 100.f, Y, BarW * FMath::Clamp(Value, 0.f, 1.f), BarH);
		Y += 24.f;
	};
	auto Panel = [&](const FString& Name, const FLinearColor& NameCol, int32 Rows)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.4f), X - 10.f, Y - 8.f, BarW + 130.f, 34.f + Rows * 24.f);
		DrawText(Name, NameCol, X, Y, Font, 1.1f);
		Y += 30.f;
	};
	for (TActorIterator<AFVValentino> It(GetWorld()); It; ++It)
	{
		Panel(TEXT("Valentino"), FLinearColor(1.f, 0.85f, 0.9f), 2);
		Bar(TEXT("Energy"), It->Energy, FLinearColor(0.35f, 0.8f, 0.3f));
		Bar(TEXT("Thirst"), It->Thirst, FLinearColor(0.3f, 0.6f, 1.f));
		Y += 14.f;
		break;
	}
	for (TActorIterator<AFVDog> It(GetWorld()); It; ++It)
	{
		Panel(It->DogName.ToString(), FLinearColor(1.f, 0.75f, 0.65f), 1);
		Bar(TEXT("Energy"), It->Energy, FLinearColor(1.f, 0.7f, 0.2f));
		Y += 14.f;
	}

	// keep thought bubbles clear of the meters panel
	const float PanelRight = X + BarW + 130.f;
	const float PanelBottom = Y;

	// ---- the dog's thought bubble ----
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	for (TActorIterator<AFVDog> It(GetWorld()); It; ++It)
	{
		const FText Thought = It->GetThought();
		const FVector Head = It->GetActorLocation() + FVector(0, 0, 55.f);
		const float Dist = FVector::Dist(CamLoc, Head);
		if (Thought.IsEmpty() || Dist > 2500.f || FVector::DotProduct(CamRot.Vector(), (Head - CamLoc).GetSafeNormal()) < 0.2f)
		{
			continue;
		}
		const FVector Screen = Project(Head, true);
		const float Scale = FMath::Clamp(900.f / FMath::Max(Dist, 1.f), 0.75f, 1.3f);
		const FString S = Thought.ToString();
		float W = 0.f, H = 0.f;
		GetTextSize(S, W, H, Font, Scale);
		const float Pad = 12.f;
		const float BX = Screen.X - W * 0.5f - Pad;
		float BY = Screen.Y - H - Pad * 2.f - 40.f * Scale;
		if (BX < PanelRight && BY < PanelBottom)
		{
			BY = PanelBottom + 6.f; // would cover the energy meters: drop it just below them
		}
		const FLinearColor Fill(1.f, 1.f, 1.f, 0.88f);
		// cloud: main box + rounded-looking corners, then three shrinking "thought dots" down to his head
		DrawRect(Fill, BX, BY, W + Pad * 2.f, H + Pad * 2.f);
		DrawRect(Fill, BX + 6.f, BY - 5.f, W + Pad * 2.f - 12.f, 5.f);
		DrawRect(Fill, BX + 6.f, BY + H + Pad * 2.f, W + Pad * 2.f - 12.f, 5.f);
		const float DotX = Screen.X;
		float DotY = BY + H + Pad * 2.f + 10.f;
		for (int32 i = 0; i < 3; ++i)
		{
			const float R = (9.f - i * 2.5f) * Scale;
			DrawRect(Fill, DotX - R * 0.5f - i * 6.f, DotY, R, R);
			DotY += R + 5.f;
		}
		DrawText(S, FLinearColor(0.12f, 0.12f, 0.15f), BX + Pad, BY + Pad, Font, Scale);
	}
}
