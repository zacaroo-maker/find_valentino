#include "FVDecor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "FVAudio.h"
#include "FVForagePlant.h"
#include "FVHidingSpot.h"
#include "FVPickupItem.h"
#include "FVSaveGame.h"
#include "GameFramework/Pawn.h"

namespace
{
	UStaticMesh* LoadMesh(const TCHAR* Path)
	{
		return Path ? LoadObject<UStaticMesh>(nullptr, Path) : nullptr;
	}
	FString PlantMesh(EFVFoodType F, bool bProduce)
	{
		const TCHAR* N = TEXT("Dandelion");
		switch (F)
		{
		case EFVFoodType::Romaine:    N = TEXT("Romaine"); break;
		case EFVFoodType::Strawberry: N = TEXT("Strawberry"); break;
		case EFVFoodType::Tomato:     N = TEXT("Tomato"); break;
		case EFVFoodType::Chicory:    N = TEXT("Chicory"); break;
		case EFVFoodType::Hibiscus:   N = TEXT("Hibiscus"); break;
		default: break;
		}
		const FString Name = FString::Printf(TEXT("SM_FV_Plant_%s%s"), N, bProduce ? TEXT("_Produce") : TEXT(""));
		return FString::Printf(TEXT("/Game/Environment/Items/Forage/%s.%s"), *Name, *Name);
	}
}

const TArray<FFVDecorDef>& UFVDecorSubsystem::Catalog()
{
	static TArray<FFVDecorDef> Items;
	if (Items.Num() == 0)
	{
		auto Prop = [](const TCHAR* Id, const TCHAR* Name, const TCHAR* Blurb, int32 Price, const TCHAR* Mesh, float Scale, FName Tag, bool bHide)
		{
			FFVDecorDef D; D.Id = Id; D.Name = Name; D.Blurb = Blurb; D.Price = Price; D.Mesh = Mesh; D.Scale = Scale; D.Tag = Tag; D.bHide = bHide;
			return D;
		};
		auto Plant = [](const TCHAR* Id, const TCHAR* Name, const TCHAR* Blurb, int32 Price, EFVFoodType Food)
		{
			FFVDecorDef D; D.Id = Id; D.Name = Name; D.Blurb = Blurb; D.Price = Price; D.bPlant = true; D.Food = Food;
			return D;
		};
		Items = {
			Prop(TEXT("Rock"), TEXT("MOSSY ROCK"), TEXT("SOMETHING TO CLIMB ON"), 200,
				TEXT("/Game/Environment/PolyHaven/rock_moss_set_02/rock_moss_set_02_v3.rock_moss_set_02_v3"), 1.f, NAME_None, false),
			Prop(TEXT("BaskStone"), TEXT("BASKING STONE"), TEXT("WARM IN THE SUN - HE'LL NAP ON IT"), 400,
				TEXT("/Game/Environment/GardenProps/SM_FV_SteppingStone.SM_FV_SteppingStone"), 1.4f, TEXT("FV_BaskStone"), false),
			Prop(TEXT("WaterDish"), TEXT("WATER DISH"), TEXT("A DRINK WITHOUT THE LONG WALK"), 500,
				TEXT("/Game/Environment/Items/Decor/SM_FV_WaterDish.SM_FV_WaterDish"), 1.f, TEXT("FV_WaterDish"), false),
			Prop(TEXT("HideLog"), TEXT("HOLLOW LOG"), TEXT("A COSY PLACE TO HIDE"), 600,
				TEXT("/Game/Environment/GardenProps/SM_FV_LogHollow.SM_FV_LogHollow"), 1.f, NAME_None, true),
			Plant(TEXT("Dandelions"), TEXT("DANDELION PATCH"), TEXT("YELLOW FLOWERS HE LOVES"), 300, EFVFoodType::Dandelion),
			Plant(TEXT("Chicory"), TEXT("CHICORY"), TEXT("BLUE FLOWERS AND TASTY LEAVES"), 400, EFVFoodType::Chicory),
			Plant(TEXT("Strawberry"), TEXT("STRAWBERRY PLANT"), TEXT("A SWEET TREAT NOW AND THEN"), 500, EFVFoodType::Strawberry),
			Plant(TEXT("Tomato"), TEXT("TOMATO PLANT"), TEXT("ANOTHER SWEET TREAT"), 600, EFVFoodType::Tomato),
			Plant(TEXT("Hibiscus"), TEXT("HIBISCUS BUSH"), TEXT("BIG PINK FLOWERS TO MUNCH"), 800, EFVFoodType::Hibiscus),
		};
	}
	return Items;
}

void UFVDecorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UFVSaveGame* Save = UFVSaveGame::Load();
	if (!Save)
	{
		return;
	}
	Savings = Save->Savings;
	// put back everything that was bought on earlier days
	for (int32 i = 0; i < Save->Decor.Num(); ++i)
	{
		const FFVDecorSave& S = Save->Decor[i];
		if (const FFVDecorDef* Def = Catalog().FindByPredicate([&](const FFVDecorDef& D) { return D.Id == S.Type; }))
		{
			SpawnDecor(*Def, S.Transform, i);
		}
	}
}

AActor* UFVDecorSubsystem::SpawnDecor(const FFVDecorDef& Def, const FTransform& Where, int32 SaveIndex)
{
	UWorld* World = GetWorld();
	if (Def.bPlant)
	{
		AFVForagePlant* P = World->SpawnActorDeferred<AFVForagePlant>(AFVForagePlant::StaticClass(), Where);
		if (!P)
		{
			return nullptr;
		}
		P->FoodType = Def.Food;
		P->RegrowSeconds = Def.Food == EFVFoodType::Dandelion ? 90.f : 140.f;
		P->PlantMesh->SetStaticMesh(LoadMesh(*PlantMesh(Def.Food, false)));
		P->ProduceMesh->SetStaticMesh(LoadMesh(*PlantMesh(Def.Food, true)));
		P->FinishSpawning(Where);
		return P;
	}
	FTransform T = Where;
	T.SetScale3D(FVector(Def.Scale));
	AFVPickupItem* I = World->SpawnActorDeferred<AFVPickupItem>(AFVPickupItem::StaticClass(), T);
	if (!I)
	{
		return nullptr;
	}
	I->Kind = EFVItemKind::Prop;
	I->DisplayName = FText::FromString(FString(Def.Name).ToLower());
	I->Mesh->SetStaticMesh(LoadMesh(Def.Mesh));
	I->DecorIndex = SaveIndex;
	if (!Def.Tag.IsNone())
	{
		I->Tags.Add(Def.Tag);
	}
	I->FinishSpawning(T);
	if (Def.bHide)
	{
		// a hiding spot inside the log that travels with it
		AFVHidingSpot* Spot = World->SpawnActor<AFVHidingSpot>(AFVHidingSpot::StaticClass(), FTransform(Where.GetRotation(), Where.GetLocation() + FVector(0, 0, 3.f)));
		if (Spot)
		{
			Spot->Attractiveness = 1.5f;
			Spot->AttachToActor(I, FAttachmentTransformRules::KeepWorldTransform);
		}
	}
	return I;
}

void UFVDecorSubsystem::AddSavings(int32 Points)
{
	Savings += Points;
	if (UFVSaveGame* Save = UFVSaveGame::Load())
	{
		Save->Savings = Savings;
		Save->Save();
	}
}

FString UFVDecorSubsystem::Buy(int32 CatalogIndex, APawn* Who)
{
	if (!Catalog().IsValidIndex(CatalogIndex) || !Who)
	{
		return FString();
	}
	const FFVDecorDef& Def = Catalog()[CatalogIndex];
	if (Savings < Def.Price)
	{
		return FString();
	}
	// in front of you, on the ground
	const FVector Fwd = Who->GetActorForwardVector().GetSafeNormal2D();
	const FVector Near = Who->GetActorLocation() + Fwd * (Def.bPlant ? 90.f : 130.f);
	FVector Ground = Near - FVector(0, 0, 90.f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FVDecorGround), true, Who);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Near + FVector(0, 0, 100.f), Near - FVector(0, 0, 400.f), ECC_Visibility, Params))
	{
		Ground = Hit.ImpactPoint;
	}
	const FTransform Where(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Ground);

	UFVSaveGame* Save = UFVSaveGame::Load();
	if (!Save)
	{
		return FString();
	}
	Savings -= Def.Price;
	Save->Savings = Savings;
	FFVDecorSave Entry;
	Entry.Type = Def.Id;
	Entry.Transform = Where;
	const int32 Index = Save->Decor.Add(Entry);
	Save->Save();
	SpawnDecor(Def, Where, Index);
	FVAudio::Play2D(this, EFVSound::Sprout, 0.6f);
	return Def.bPlant ? FString::Printf(TEXT("Planted a %s!"), *FString(Def.Name).ToLower())
	                  : FString::Printf(TEXT("A new %s! Carry it anywhere with E."), *FString(Def.Name).ToLower());
}

void UFVDecorSubsystem::OnMoved(AFVPickupItem* Item)
{
	if (!Item || Item->DecorIndex < 0)
	{
		return;
	}
	if (UFVSaveGame* Save = UFVSaveGame::Load())
	{
		if (Save->Decor.IsValidIndex(Item->DecorIndex))
		{
			FTransform T = Item->GetActorTransform();
			T.SetScale3D(FVector::OneVector); // the catalogue sets the scale
			Save->Decor[Item->DecorIndex].Transform = T;
			Save->Save();
		}
	}
}
