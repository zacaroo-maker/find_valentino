#include "FVSleepSpot.h"

#include "Components/BillboardComponent.h"
#include "FVSaveGame.h"
#include "Kismet/GameplayStatics.h"

AFVSleepSpot::AFVSleepSpot()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(Root);
		Sprite->SetRelativeLocation(FVector(0, 0, 40));
	}
#endif
}

// ---- save game (kept here to avoid another tiny file) ----

UFVSaveGame* UFVSaveGame::Load()
{
	if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		if (UFVSaveGame* S = Cast<UFVSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0)))
		{
			return S;
		}
	}
	return Cast<UFVSaveGame>(UGameplayStatics::CreateSaveGameObject(UFVSaveGame::StaticClass()));
}

void UFVSaveGame::Save()
{
	UGameplayStatics::SaveGameToSlot(this, Slot, 0);
}
