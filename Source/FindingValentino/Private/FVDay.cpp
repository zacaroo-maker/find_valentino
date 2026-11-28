#include "FVDay.h"

#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "FVAudio.h"
#include "FVDog.h"
#include "FVDogPark.h"
#include "FVForagePlant.h"
#include "FVCarryComponent.h"
#include "FVDecor.h"
#include "InputCoreTypes.h"
#include "FVPickupItem.h"
#include "FVGameModes.h"
#include "FVPlantBed.h"
#include "FVPond.h"
#include "FVSaveGame.h"
#include "FVSleepSpot.h"
#include "FVValentino.h"
#include "FVValentinoAIController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	FString WithArticle(EFVFoodType F)
	{
		return FVFood::WithArticle(F);
	}
}

void UFVDaySubsystem::OnForaged(EFVFoodType Food)
{
	const uint32 Bit = 1u << (uint32)Food;
	if (bRunning && !bSummary && !(ForagedMask & Bit))
	{
		ForagedMask |= Bit;
		AddScore(25, FString::Printf(TEXT("FOUND %s"), *FVFood::Upper(Food)), TEXT("FORAGING"));
	}
}

// ---------------------------------------------------------------- intro and tips

namespace
{
	const TCHAR* HintMessage(EFVHint H)
	{
		switch (H)
		{
		case EFVHint::Move:     return TEXT("WASD TO WALK, MOUSE TO LOOK AROUND. V SWITCHES THE CAMERA.");
		case EFVHint::Search:   return TEXT("VALENTINO LOVES HIDING UNDER BUSHES, POTS AND BENCHES. LOOK LOW!");
		case EFVHint::PickUp:   return TEXT("E PICKS HIM UP. HE'S A BIT SHY AT FIRST - GIVE HIM A MOMENT.");
		case EFVHint::Sparkle:  return TEXT("SPARKLES MEAN FOOD IS READY TO PICK. HE LOVES FLOWERS AND GREENS.");
		case EFVHint::Goals:    return TEXT("NEW JOBS POP UP IN YOUR TO DO LIST. EACH ONE EARNS POINTS.");
		case EFVHint::Craving:  return TEXT("HIS THOUGHT BUBBLE SHOWS WHAT HE'S DREAMING OF. BRING HIM THAT!");
		case EFVHint::Ball:     return TEXT("LEFT CLICK TO THROW THE BALL. DIEGO WILL FETCH IT!");
		case EFVHint::Can:      return TEXT("HOLD LEFT CLICK TO WATER. REFILL THE CAN AT THE POND.");
		case EFVHint::Pet:      return TEXT("PRESS F TO PET DIEGO. PATS GIVE HIM ENERGY.");
		case EFVHint::Bedtime:  return TEXT("CARRY VALENTINO TO HIS LITTLE HOUSE TO TUCK HIM IN.");
		case EFVHint::Decorate: return TEXT("PAUSE (ESC) > DECORATE TO SPEND YOUR GARDEN POINTS ON HIS YARD.");
		default: break;
		}
		return TEXT("");
	}
}

void UFVDaySubsystem::ShowHint(EFVHint Hint)
{
	const int32 Bit = 1 << (int32)Hint;
	if ((HintsSeen & Bit) || HintQueue.Contains(Hint))
	{
		return;
	}
	HintQueue.Add(Hint);
}

void UFVDaySubsystem::UpdateIntroAndHints(float DeltaTime)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (bIntro)
	{
		IntroAge += DeltaTime;
		bool bKey = false;
		if (PC && IntroAge > 1.2f)
		{
			for (const FKey& K : { EKeys::E, EKeys::SpaceBar, EKeys::Enter, EKeys::LeftMouseButton, EKeys::W, EKeys::A, EKeys::S, EKeys::D })
			{
				bKey |= PC->IsInputKeyDown(K);
			}
		}
		if (bKey || IntroAge > 16.f)
		{
			bIntro = false;
			FVAudio::Play2D(this, EFVSound::UISelect, 0.4f);
			ShowHint(EFVHint::Move);
		}
		return; // tips wait until the card is gone
	}

	// things worth a tip the first time they happen
	if (const APawn* P = PC ? PC->GetPawn() : nullptr)
	{
		if (const UFVCarryComponent* C = P->FindComponentByClass<UFVCarryComponent>())
		{
			if (const AFVPickupItem* I = C->GetHeldItem())
			{
				if (I->Kind == EFVItemKind::Ball) { ShowHint(EFVHint::Ball); }
				if (I->Kind == EFVItemKind::WateringCan) { ShowHint(EFVHint::Can); }
			}
			if (!C->GetPetPrompt().IsEmpty()) { ShowHint(EFVHint::Pet); }
		}
	}
	if (HasGoal(EFVGoal::FindValentino))
	{
		SearchTime += DeltaTime;
		if (SearchTime > 35.f) { ShowHint(EFVHint::Search); }
	}

	// one tip at a time, with a little pause between them
	if (!HintText.IsEmpty())
	{
		HintAge += DeltaTime;
		if (HintAge > 8.f)
		{
			HintText.Empty();
			HintGap = 1.5f;
		}
		return;
	}
	HintGap -= DeltaTime;
	if (HintGap <= 0.f && HintQueue.Num() > 0)
	{
		const EFVHint H = HintQueue[0];
		HintQueue.RemoveAt(0);
		HintText = HintMessage(H);
		HintAge = 0.f;
		UE_LOG(LogTemp, Log, TEXT("FVHint: %s"), *HintText);
		HintsSeen |= 1 << (int32)H;
		if (UFVSaveGame* Save = UFVSaveGame::Load())
		{
			Save->HintsSeen = HintsSeen;
			Save->Save();
		}
		FVAudio::Play2D(this, EFVSound::UIMove, 0.35f);
	}
}

void UFVDaySubsystem::OnFetch()
{
	if (!bRunning || bSummary)
	{
		return;
	}
	++Fetches;
	// outside the fetch goal: a little reward for each throw he brings back (capped)
	if (!HasGoal(EFVGoal::Fetch) && FetchPoints < 100)
	{
		FetchPoints += 20;
		AddScore(20, TEXT("GOOD FETCH!"), TEXT("DIEGO"));
	}
}

void UFVDaySubsystem::SetCraving(bool bOn, EFVFoodType Food)
{
	if (AFVValentino* V = GetValentino())
	{
		V->bHasCraving = bOn;
		V->Craving = Food;
	}
}

// ---------------------------------------------------------------- lookups

AFVValentino* UFVDaySubsystem::GetValentino() const
{
	for (TActorIterator<AFVValentino> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

AFVValentinoAIController* UFVDaySubsystem::GetValentinoAI() const
{
	const AFVValentino* V = GetValentino();
	return V ? Cast<AFVValentinoAIController>(V->GetController()) : nullptr;
}

AFVDog* UFVDaySubsystem::GetDiego() const
{
	for (TActorIterator<AFVDog> It(GetWorld()); It; ++It)
	{
		if (It->bShowMeter)
		{
			return *It; // the family dog (park dogs don't show a meter)
		}
	}
	return nullptr;
}

FString UFVDaySubsystem::GetClockText() const
{
	const int32 H = FMath::FloorToInt(Hour);
	const int32 M = FMath::FloorToInt((Hour - H) * 60.f / 10.f) * 10; // ten-minute steps, like a digital watch
	return FString::Printf(TEXT("%02d:%02d"), H, M);
}

// ---------------------------------------------------------------- start / score

void UFVDaySubsystem::StartDay(int32 Number)
{
	Day = FMath::Max(1, Number);
	Hour = StartHour;
	Score = 0;
	PetPoints = 0;
	bRunning = true;
	bSummary = bInBed = bNewHighScore = bBedtimeAnnounced = false;
	EndDelay = -1.f;
	Goals.Reset();
	Popups.Reset();
	ByCategory.Reset();
	CategoryOrder.Reset();
	FindTimer = 0.f;
	SinceFound = 0.f;
	NextGoalIn = 25.f;
	NextCravingIn = FMath::FRandRange(30.f, 50.f);
	ForagedMask = 0;
	Fetches = 0;
	FetchPoints = 0;
	SetCraving(false);
	if (const UFVSaveGame* Save = UFVSaveGame::Load())
	{
		HighScore = Save->HighScore;
		HintsSeen = Save->HintsSeen;
	}
	// the story card on the first day of a game
	bIntro = Day == 1;
	IntroAge = 0.f;
	HintQueue.Reset();
	HintText.Empty();
	HintGap = 2.f;
	SearchTime = 0.f;
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		Sun = *It;
		SunBase = It->GetActorRotation();
		SunColorBase = It->GetLightComponent()->GetLightColor();
		break;
	}
	if (const AFVDog* D = GetDiego())
	{
		LastPetCount = D->TimesPetted;
	}
	// Valentino has wandered off overnight and is hiding somewhere new
	if (AFVValentinoAIController* AI = GetValentinoAI())
	{
		AI->HideAtRandomSpot();
	}
	AddGoal(EFVGoal::FindValentino, TEXT("FIND VALENTINO"));
	Announce(FString::Printf(TEXT("DAY %d"), Day), 3.5f);
	UpdateSun();
}

void UFVDaySubsystem::AddScore(int32 Points, const FString& Why, const FString& Category, bool bBig)
{
	Score += Points;
	if (UFVDecorSubsystem* Decor = GetWorld()->GetSubsystem<UFVDecorSubsystem>())
	{
		Decor->AddSavings(Points); // everything earned can be spent on his yard
		if (Decor->GetSavings() >= 200)
		{
			ShowHint(EFVHint::Decorate);
		}
	}
	if (!ByCategory.Contains(Category))
	{
		CategoryOrder.Add(Category);
	}
	ByCategory.FindOrAdd(Category) += Points;
	FFVPopup P;
	P.Text = FString::Printf(TEXT("+%d %s"), Points, *Why);
	P.bBig = bBig;
	Popups.Add(P);
	FVAudio::Play2D(this, bBig ? EFVSound::UIStart : EFVSound::UISelect, bBig ? 0.45f : 0.35f);
}

void UFVDaySubsystem::AddPopup(const FString& Text, bool bBig)
{
	FFVPopup P;
	P.Text = Text;
	P.bBig = bBig;
	Popups.Add(P);
}

void UFVDaySubsystem::Announce(const FString& Text, float Seconds)
{
	Banner = Text;
	BannerTime = Seconds;
}

bool UFVDaySubsystem::HasGoal(EFVGoal Type) const
{
	return Goals.ContainsByPredicate([Type](const FFVGoal& G) { return G.Type == Type; });
}

void UFVDaySubsystem::AddGoal(EFVGoal Type, const FString& Text, AActor* Target, int32 Baseline, float Needed)
{
	FFVGoal G;
	G.Type = Type;
	G.Text = Text;
	G.Target = Target;
	G.Baseline = Baseline;
	G.Needed = Needed;
	Goals.Add(G);
	if (Type == EFVGoal::Craving) { ShowHint(EFVHint::Craving); }
	else if (Type == EFVGoal::Bedtime) { ShowHint(EFVHint::Bedtime); }
	else if (Type != EFVGoal::FindValentino && Type != EFVGoal::FindAgain) { ShowHint(EFVHint::Goals); }
	FVAudio::Play2D(this, EFVSound::UIPause, 0.35f);
}

// ---------------------------------------------------------------- the sun

void UFVDaySubsystem::UpdateSun()
{
	if (!Sun)
	{
		return;
	}
	const float T = FMath::Clamp((Hour - StartHour) / (EndHour - StartHour), 0.f, 1.f);
	// low and golden in the morning, high at midday, low and orange in the evening
	const float Elevation = 4.f + 54.f * FMath::Sin(PI * T);
	const float Yaw = SunBase.Yaw + (T - 0.5f) * 150.f;
	Sun->SetActorRotation(FRotator(-Elevation, Yaw, 0.f));
	// golden hour: the light starts warming well before sunset (and is golden first thing in the morning)
	// golden hour: the evening light starts warming from mid-afternoon; mornings are a softer gold
	float Warm = FMath::Pow(FMath::Clamp(1.f - (Elevation - 4.f) / 44.f, 0.f, 1.f), 1.15f);
	if (T < 0.5f)
	{
		Warm *= 0.55f;
	}
	Golden = Warm;
	SunElevation = Elevation;
	Sun->GetLightComponent()->SetLightColor(FMath::Lerp(SunColorBase, FLinearColor(1.f, 0.58f, 0.32f), Warm * 0.8f));
}

// ---------------------------------------------------------------- goals

void UFVDaySubsystem::MaybeAddGoal()
{
	const int32 Active = Goals.FilterByPredicate([](const FFVGoal& G)
	{
		return G.Type != EFVGoal::FindValentino && G.Type != EFVGoal::FindAgain && G.Type != EFVGoal::Bedtime && G.Type != EFVGoal::Craving;
	}).Num();
	if (Active >= 2 || Hour >= BedtimeHour - 0.25f)
	{
		return;
	}
	AFVValentino* V = GetValentino();
	AFVDog* D = GetDiego();
	TArray<TFunction<void()>> Options;

	if (V && !HasGoal(EFVGoal::Feed) && V->Energy < 0.75f)
	{
		Options.Add([this, V]() { AddGoal(EFVGoal::Feed, TEXT("VALENTINO IS HUNGRY"), V, V->TimesEaten); });
	}
	if (!HasGoal(EFVGoal::WaterBed))
	{
		TArray<AFVPlantBed*> Dry;
		for (TActorIterator<AFVPlantBed> It(GetWorld()); It; ++It)
		{
			if (It->GetWetness() < 0.5f)
			{
				Dry.Add(*It);
			}
		}
		if (Dry.Num() > 0)
		{
			AFVPlantBed* B = Dry[FMath::RandHelper(Dry.Num())];
			Options.Add([this, B]() { AddGoal(EFVGoal::WaterBed, FString::Printf(TEXT("WATER THE %s"), *B->DisplayName.ToString().ToUpper()), B); });
		}
	}
	const UFVDecorSubsystem* DecorSub = GetWorld()->GetSubsystem<UFVDecorSubsystem>();
	const bool bParkOpen = DecorSub && DecorSub->IsParkUnlocked();
	if (D && bParkOpen && !HasGoal(EFVGoal::DiegoPark) && D->Energy > 0.3f && !D->bInPark)
	{
		Options.Add([this, D]() { AddGoal(EFVGoal::DiegoPark, TEXT("TAKE DIEGO TO THE DOG PARK"), D, 0, 15.f); });
	}
	if (D && !HasGoal(EFVGoal::Fetch) && D->Energy > 0.35f && !D->bInPark)
	{
		bool bBall = false;
		for (TActorIterator<AFVPickupItem> It(GetWorld()); It && !bBall; ++It)
		{
			bBall = It->Kind == EFVItemKind::Ball && !AFVDogPark::FindAt(this, It->GetActorLocation());
		}
		if (bBall)
		{
			Options.Add([this, D]() { AddGoal(EFVGoal::Fetch, TEXT("PLAY FETCH WITH DIEGO"), D, Fetches, 3.f); });
		}
	}
	if (D && !HasGoal(EFVGoal::PetDiego))
	{
		Options.Add([this, D]() { AddGoal(EFVGoal::PetDiego, TEXT("DIEGO WANTS CUDDLES"), D, D->TimesPetted, 5.f); });
	}
	if (V && !HasGoal(EFVGoal::PondCool) && V->Temperature >= 28.f)
	{
		// on hot days this one comes up first
		Options.Insert([this, V]() { AddGoal(EFVGoal::PondCool, TEXT("IT'S HOT! VALENTINO NEEDS THE POND"), V); }, 0);
	}
	if (Options.Num() > 0)
	{
		const int32 Pick = (V && V->Temperature >= 28.f && !HasGoal(EFVGoal::PondCool)) ? 0 : FMath::RandHelper(Options.Num());
		Options[Pick]();
	}
}

void UFVDaySubsystem::UpdateGoals(float DeltaTime)
{
	AFVValentino* V = GetValentino();
	AFVValentinoAIController* AI = GetValentinoAI();
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);

	for (int32 i = Goals.Num() - 1; i >= 0; --i)
	{
		FFVGoal& G = Goals[i];
		G.Age += DeltaTime;
		bool bDone = false;
		switch (G.Type)
		{
		case EFVGoal::FindValentino:
		case EFVGoal::FindAgain:
			if (V && Player)
			{
				// spotted him: close by and in plain view, or already picked up
				const float Dist = FVector::Dist(V->GetActorLocation(), Player->GetActorLocation());
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(FVFind), false, Player);
				Params.AddIgnoredActor(V);
				const FVector Eye = Player->GetActorLocation() + FVector(0, 0, 60.f);
				const bool bClear = !GetWorld()->LineTraceSingleByChannel(Hit, Eye, V->GetActorLocation(), ECC_Visibility, Params);
				if (V->IsHeld() || (Dist < 260.f && bClear) || Dist < 120.f)
				{
					bDone = true;
					const bool bMorning = G.Type == EFVGoal::FindValentino;
					const int32 Bonus = FMath::Max(0, (bMorning ? 400 : 150) - FMath::RoundToInt(G.Age * (bMorning ? 2.f : 1.5f)));
					AddScore((bMorning ? 300 : 100) + Bonus, TEXT("FOUND VALENTINO!"), TEXT("FINDING VALENTINO"), true);
					SinceFound = 0.f;
					if (AI)
					{
						AI->StopHiding();
					}
					ShowHint(EFVHint::PickUp);
					ShowHint(EFVHint::Sparkle);
				}
			}
			break;

		case EFVGoal::Feed:
			if (V && V->TimesEaten > G.Baseline)
			{
				bDone = true;
				AddScore(150, FString::Printf(TEXT("FED %s"), *WithArticle(V->LastFood)), TEXT("FEEDING"));
				if (FVFood::IsTreat(V->LastFood))
				{
					AddScore(50, TEXT("A TASTY TREAT!"), TEXT("FEEDING"));
				}
			}
			break;

		case EFVGoal::Craving:
			if (V && V->TimesEaten > G.Baseline)
			{
				if (V->LastFood == G.Food)
				{
					bDone = true;
					AddScore(250, TEXT("JUST WHAT HE WANTED!"), TEXT("FEEDING"), true);
				}
				else
				{
					G.Baseline = V->TimesEaten; // nice, but not what he was dreaming of - keep wishing
				}
			}
			break;

		case EFVGoal::WaterBed:
			if (const AFVPlantBed* B = Cast<AFVPlantBed>(G.Target.Get()))
			{
				if (B->GetWetness() >= 0.99f)
				{
					bDone = true;
					AddScore(120, TEXT("WATERED"), TEXT("GARDENING"));
				}
			}
			else
			{
				Goals.RemoveAt(i);
				continue;
			}
			break;

		case EFVGoal::DiegoPark:
			if (const AFVDog* D = Cast<AFVDog>(G.Target.Get()))
			{
				if (D->bInPark)
				{
					G.Progress += DeltaTime;
				}
				if (G.Progress >= G.Needed)
				{
					bDone = true;
					AddScore(200, TEXT("DIEGO PLAYED AT THE PARK"), TEXT("DIEGO"));
				}
			}
			break;

		case EFVGoal::PetDiego:
			if (const AFVDog* D = Cast<AFVDog>(G.Target.Get()))
			{
				G.Progress = float(D->TimesPetted - G.Baseline);
				if (G.Progress >= G.Needed)
				{
					bDone = true;
					AddScore(100, TEXT("HAPPY DIEGO"), TEXT("DIEGO"));
				}
			}
			break;

		case EFVGoal::Fetch:
			G.Progress = float(Fetches - G.Baseline);
			if (G.Progress >= G.Needed)
			{
				bDone = true;
				AddScore(150, TEXT("FETCH CHAMPION!"), TEXT("DIEGO"));
			}
			break;

		case EFVGoal::PondCool:
			if (V)
			{
				bool bAtPond = V->State == EFVValentinoState::AtPond;
				for (TActorIterator<AFVPond> It(GetWorld()); It && !bAtPond; ++It)
				{
					bAtPond = !V->IsHeld() && FVector::Dist2D(It->GetActorLocation(), V->GetActorLocation()) < 260.f;
				}
				if (bAtPond)
				{
					bDone = true;
					AddScore(150, TEXT("COOLED OFF"), TEXT("CARE"));
				}
			}
			break;

		case EFVGoal::Bedtime:
			if (V && !V->IsHeld())
			{
				for (TActorIterator<AFVSleepSpot> It(GetWorld()); It; ++It)
				{
					if (It->Contains(V->GetActorLocation()))
					{
						bDone = true;
						bInBed = true;
						const int32 Rested = FMath::RoundToInt(V->Energy * 200.f);
						AddScore(300, TEXT("TUCKED IN FOR THE NIGHT"), TEXT("BEDTIME"), true);
						if (Rested > 0)
						{
							AddScore(Rested, TEXT("WELL-FED BONUS"), TEXT("BEDTIME"));
						}
						if (AI)
						{
							AI->SetAsleep(true);
						}
						Announce(TEXT("GOOD NIGHT, VALENTINO"), 3.5f);
						EndDelay = 4.5f;
						break;
					}
				}
			}
			break;
		}
		// daytime goals that nobody got to quietly go away after a while (no penalty)
		const bool bExpires = G.Type != EFVGoal::FindValentino && G.Type != EFVGoal::Bedtime && G.Type != EFVGoal::FindAgain;
		if (bDone || (bExpires && G.Age > 300.f))
		{
			Goals.RemoveAt(i);
		}
	}

	// little rewards that aren't goals: every few pats of Diego
	if (const AFVDog* D = GetDiego())
	{
		if (D->TimesPetted - LastPetCount >= 3 && PetPoints < 100 && !HasGoal(EFVGoal::PetDiego))
		{
			LastPetCount = D->TimesPetted;
			PetPoints += 20;
			AddScore(20, TEXT("GOOD BOY"), TEXT("DIEGO"));
		}
		else if (HasGoal(EFVGoal::PetDiego))
		{
			LastPetCount = D->TimesPetted;
		}
	}

	// Valentino's thought bubble follows the craving goal (gone when it's done, expires or bedtime clears it)
	{
		const FFVGoal* C = Goals.FindByPredicate([](const FFVGoal& G) { return G.Type == EFVGoal::Craving; });
		SetCraving(C != nullptr, C ? C->Food : EFVFoodType::Romaine);
	}

	// he slipped off to hide again during the day
	SinceFound += DeltaTime;
	if (V && !HasGoal(EFVGoal::FindValentino) && !HasGoal(EFVGoal::FindAgain) && !HasGoal(EFVGoal::Bedtime) && SinceFound > 60.f
		&& V->State == EFVValentinoState::Hiding && Hour < BedtimeHour)
	{
		AddGoal(EFVGoal::FindAgain, TEXT("VALENTINO IS HIDING AGAIN"));
	}
}

// ---------------------------------------------------------------- tick / end

void UFVDaySubsystem::Tick(float DeltaTime)
{
	// popups and banners keep animating even on the summary
	for (int32 i = Popups.Num() - 1; i >= 0; --i)
	{
		Popups[i].Age += DeltaTime;
		if (Popups[i].Age > 2.6f)
		{
			Popups.RemoveAt(i);
		}
	}
	BannerTime -= DeltaTime;
	if (!bRunning || bSummary)
	{
		return;
	}
	UpdateIntroAndHints(DeltaTime);

	// the clock only starts once he's been found (the morning search isn't against the clock)
	const bool bSearching = HasGoal(EFVGoal::FindValentino);
	if (!bSearching)
	{
		Hour += DeltaTime * (EndHour - StartHour) / FMath::Max(DayLengthSeconds, 10.f);
	}
	UpdateSun();
	UpdateGoals(DeltaTime);

	if (!bSearching)
	{
		NextGoalIn -= DeltaTime;
		if (NextGoalIn <= 0.f)
		{
			NextGoalIn = FMath::FRandRange(50.f, 90.f);
			MaybeAddGoal();
		}
		// now and then Valentino dreams of something in particular
		NextCravingIn -= DeltaTime;
		AFVValentino* V = GetValentino();
		if (NextCravingIn <= 0.f && V && !HasGoal(EFVGoal::Craving) && !HasGoal(EFVGoal::FindAgain) && Hour < BedtimeHour - 0.25f)
		{
			NextCravingIn = FMath::FRandRange(80.f, 140.f);
			TArray<EFVFoodType> Growing; // only wish for things that grow in this garden
			for (TActorIterator<AFVForagePlant> It(GetWorld()); It; ++It)
			{
				Growing.AddUnique(It->FoodType);
			}
			if (Growing.Num() > 0)
			{
				const EFVFoodType Food = Growing[FMath::RandHelper(Growing.Num())];
				AddGoal(EFVGoal::Craving, FString::Printf(TEXT("VALENTINO WANTS %s"), *WithArticle(Food)), V, V->TimesEaten);
				Goals.Last().Food = Food;
				SetCraving(true, Food);
			}
		}
	}
	if (!bBedtimeAnnounced && Hour >= BedtimeHour)
	{
		bBedtimeAnnounced = true;
		Goals.RemoveAll([](const FFVGoal& G) { return G.Type != EFVGoal::Bedtime && G.Type != EFVGoal::FindValentino; });
		AddGoal(EFVGoal::Bedtime, TEXT("PUT VALENTINO TO BED"));
		Announce(TEXT("BEDTIME!"), 3.f);
	}
	if (EndDelay > 0.f)
	{
		EndDelay -= DeltaTime;
		if (EndDelay <= 0.f)
		{
			EndDay();
		}
	}
	else if (Hour >= EndHour)
	{
		EndDay();
	}
}

void UFVDaySubsystem::EndDay()
{
	if (bSummary)
	{
		return;
	}
	bSummary = true;
	SummaryLines.Reset();
	for (const FString& C : CategoryOrder)
	{
		SummaryLines.Add({C, ByCategory[C]});
	}
	if (!bInBed)
	{
		SummaryLines.Add({TEXT("VALENTINO SLEPT OUTSIDE"), 0});
	}
	UFVSaveGame* Save = UFVSaveGame::Load();
	if (Save)
	{
		bNewHighScore = Score > Save->HighScore;
		Save->HighScore = FMath::Max(Save->HighScore, Score);
		Save->LastScore = Score;
		++Save->DaysPlayed;
		Save->Save();
		HighScore = Save->HighScore;
	}
	if (AFVPlayerController* PC = Cast<AFVPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->ShowDaySummary();
	}
	FVAudio::Play2D(this, EFVSound::UIStart, 0.6f);
}
