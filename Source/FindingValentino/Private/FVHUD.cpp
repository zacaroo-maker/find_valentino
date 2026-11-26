#include "FVHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "FVCarryComponent.h"
#include "GameFramework/Pawn.h"

void AFVHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	const APawn* Pawn = GetOwningPawn();
	const UFVCarryComponent* Carry = Pawn ? Pawn->FindComponentByClass<UFVCarryComponent>() : nullptr;
	if (!Carry)
	{
		return;
	}
	const FText Prompt = Carry->GetPrompt();
	if (Prompt.IsEmpty())
	{
		return;
	}
	UFont* Font = GEngine->GetMediumFont();
	const FString S = Prompt.ToString();
	float W = 0.f, H = 0.f;
	GetTextSize(S, W, H, Font, PromptScale);
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Y = Canvas->ClipY * PromptY;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), X - 16.f, Y - 8.f, W + 32.f, H + 16.f);
	DrawText(S, FLinearColor(1.f, 0.97f, 0.9f), X, Y, Font, PromptScale);
}
