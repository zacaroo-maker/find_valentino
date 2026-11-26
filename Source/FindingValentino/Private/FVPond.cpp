#include "FVPond.h"

#include "Components/BillboardComponent.h"

AFVPond::AFVPond()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	DrinkPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DrinkPoint"));
	DrinkPoint->SetupAttachment(Root);
#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(Root);
		Sprite->SetRelativeLocation(FVector(0, 0, 60));
	}
#endif
}
