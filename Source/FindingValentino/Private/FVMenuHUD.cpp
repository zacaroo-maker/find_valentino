#include "FVMenuHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "FVFamilyMemberData.h"
#include "FVGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

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
	// the seven family members, in the order of the design document
	if (Family.Num() == 0)
	{
		for (const TCHAR* Id : { TEXT("Dad"), TEXT("Mom"), TEXT("Grandpa"), TEXT("Grandma"), TEXT("OlderKid"), TEXT("YoungerKid"), TEXT("Friend") })
		{
			const FString Path = FString::Printf(TEXT("/Game/Characters/Family/Data/DA_%s.DA_%s"), Id, Id);
			if (UFVFamilyMemberData* M = LoadObject<UFVFamilyMemberData>(nullptr, *Path))
			{
				Family.Add(M);
			}
		}
	}
	// keep the previous choice highlighted when coming back from the garden
	if (const UFVGameInstance* GI = Cast<UFVGameInstance>(GetGameInstance()))
	{
		const int32 I = Family.IndexOfByKey(GI->SelectedMember);
		Selected = I == INDEX_NONE ? 0 : I;
	}
}

void AFVMenuHUD::Button(const FString& Label, FName Id, float X, float Y, float W, float H, bool bSelected, float Scale)
{
	const bool bHover = Hover == Id;
	const FLinearColor Fill = bSelected ? FLinearColor(0.95f, 0.75f, 0.35f, 0.92f)
	                        : bHover    ? FLinearColor(1.f, 1.f, 1.f, 0.35f)
	                                    : FLinearColor(0.f, 0.f, 0.f, 0.45f);
	DrawRect(Fill, X, Y, W, H);
	UFont* Font = GEngine->GetMediumFont();
	float TW = 0.f, TH = 0.f;
	GetTextSize(Label, TW, TH, Font, Scale);
	DrawText(Label, bSelected ? FLinearColor(0.1f, 0.07f, 0.03f) : FLinearColor::White, X + (W - TW) * 0.5f, Y + (H - TH) * 0.5f, Font, Scale);
	AddHitBox(FVector2D(X, Y), FVector2D(W, H), Id, true);
}

void AFVMenuHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	UFont* Font = GEngine->GetLargeFont();
	UFont* Medium = GEngine->GetMediumFont();
	const float CW = Canvas->ClipX, CH = Canvas->ClipY;

	// title
	const FString Title = TEXT("Finding Valentino");
	float TW = 0.f, TH = 0.f;
	GetTextSize(Title, TW, TH, Font, 2.2f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f), 0.f, CH * 0.06f - 12.f, CW, TH + 24.f);
	DrawText(Title, FLinearColor(1.f, 0.92f, 0.8f), (CW - TW) * 0.5f, CH * 0.06f, Font, 2.2f);

	// character list (left)
	const float ListX = CW * 0.08f, ListY = CH * 0.24f, BW = CW * 0.22f, BH = CH * 0.065f, Gap = CH * 0.012f;
	DrawText(TEXT("Who's looking for Valentino today?"), FLinearColor::White, ListX, ListY - 46.f, Medium, 1.4f);
	for (int32 i = 0; i < Family.Num(); ++i)
	{
		const UFVFamilyMemberData* M = Family[i];
		Button(M ? M->DisplayName.ToString() : TEXT("?"), FName(*FString::Printf(TEXT("Member_%d"), i)),
			ListX, ListY + i * (BH + Gap), BW, BH, i == Selected);
	}

	// description card (right)
	if (Family.IsValidIndex(Selected) && Family[Selected])
	{
		const UFVFamilyMemberData* M = Family[Selected];
		const float CX = CW * 0.40f, CY = ListY, CWd = CW * 0.48f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), CX, CY, CWd, CH * 0.3f);
		DrawText(M->DisplayName.ToString(), FLinearColor(1.f, 0.85f, 0.6f), CX + 24.f, CY + 20.f, Font, 1.6f);
		// simple word wrap for the description
		const FString Desc = M->Description.ToString();
		TArray<FString> Words;
		Desc.ParseIntoArray(Words, TEXT(" "));
		FString Line;
		float LY = CY + 90.f;
		for (const FString& Wd : Words)
		{
			const FString Try = Line.IsEmpty() ? Wd : Line + TEXT(" ") + Wd;
			float LW = 0.f, LH = 0.f;
			GetTextSize(Try, LW, LH, Medium, 1.25f);
			if (LW > CWd - 48.f && !Line.IsEmpty())
			{
				DrawText(Line, FLinearColor::White, CX + 24.f, LY, Medium, 1.25f);
				LY += LH + 6.f;
				Line = Wd;
			}
			else
			{
				Line = Try;
			}
		}
		if (!Line.IsEmpty())
		{
			DrawText(Line, FLinearColor::White, CX + 24.f, LY, Medium, 1.25f);
		}
		Button(TEXT("Start"), TEXT("Start"), CX, CY + CH * 0.34f, CWd * 0.5f - 8.f, BH * 1.2f, false, 1.6f);
		Button(TEXT("Quit"), TEXT("Quit"), CX + CWd * 0.5f + 8.f, CY + CH * 0.34f, CWd * 0.5f - 8.f, BH * 1.2f, false, 1.6f);
	}
}

void AFVMenuHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	const FString S = BoxName.ToString();
	if (S.StartsWith(TEXT("Member_")))
	{
		Selected = FCString::Atoi(*S.RightChop(7));
		return;
	}
	if (BoxName == TEXT("Start"))
	{
		UFVGameInstance* GI = Cast<UFVGameInstance>(GetGameInstance());
		if (GI && Family.IsValidIndex(Selected))
		{
			GI->SelectedMember = Family[Selected];
		}
		UGameplayStatics::OpenLevel(this, GI ? GI->GameLevel : FName(TEXT("L_Backyard")));
	}
	else if (BoxName == TEXT("Quit"))
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayerController(), EQuitPreference::Quit, false);
	}
}

void AFVMenuHUD::NotifyHitBoxBeginCursorOver(FName BoxName)
{
	Hover = BoxName;
}

void AFVMenuHUD::NotifyHitBoxEndCursorOver(FName BoxName)
{
	if (Hover == BoxName)
	{
		Hover = NAME_None;
	}
}
