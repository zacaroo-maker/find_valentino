#include "FVDay.h"

#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "FVAudio.h"
#include "FVDog.h"
#include "FVDogPark.h"
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
	FString FoodName(EFVFoodType F)
	{
		switch (F)
		{
		case EFVFoodType::Lettuce: return TEXT("LETTUCE");
		case EFVFoodType::Dandelion: return TEXT("DANDELION");
		case EFVFoodType::Strawberry: return TEXT("STRAWBERRY");
		case EFVFoodType::Worm: return TEXT("WORM");
		}
		return TEXT("FOOD");
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
	if (const UFVSaveGame* Save = UFVSaveGame::Load())
	{
		HighScore = Save->HighScore;
	}
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
	const float Warm = FMath::Clamp(1.f - (Elevation - 4.f) / 22.f, 0.f, 1.f);
	Sun->GetLightComponent()->SetLightColor(FMath::Lerp(SunColorBase, FLinearColor(1.f, 0.62f, 0.38f), Warm * 0.8f));
}

// ---------------------------------------------------------------- goals

void UFVDaySubsystem::MaybeAddGoal()
{
	const int32 Active = Goals.FilterByPredicate([](const FFVGoal& G)
	{
		return G.Type != EFVGoal::FindValentino && G.Type != EFVGoal::FindAgain && G.Type != EFVGoal::Bedtime;
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
	if (D && !HasGoal(EFVGoal::DiegoPark) && D->Energy > 0.3f && !D->bInPark)
	{
		Options.Add([this, D]() { AddGoal(EFVGoal::DiegoPark, TEXT("TAKE DIEGO TO THE DOG PARK"), D, 0, 15.f); });
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
				}
			}
			break;

		case EFVGoal::Feed:
			if (V && V->TimesEaten > G.Baseline)
			{
				bDone = true;
				const bool bFav = V->LastFood == EFVFoodType::Strawberry;
				AddScore(150, FString::Printf(TEXT("FED A %s"), *FoodName(V->LastFood)), TEXT("FEEDING"));
				if (bFav)
				{
					AddScore(100, TEXT("HIS FAVOURITE!"), TEXT("FEEDING"));
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
