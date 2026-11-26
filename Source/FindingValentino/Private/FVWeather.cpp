#include "FVWeather.h"

#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "FVAudio.h"
#include "FVValentino.h"

void UFVWeatherSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	auto P = [](const TCHAR* Name, float Temp, float Sun, float BirdVol, float BirdRate, float Wind, float Weight, bool bCloudy)
	{
		FFVWeatherPreset W;
		W.Name = Name; W.Temperature = Temp; W.Sun = Sun; W.BirdVolume = BirdVol; W.BirdRate = BirdRate;
		W.WindVolume = Wind; W.Weight = Weight; W.bCloudy = bCloudy;
		return W;
	};
	Presets = {
		P(TEXT("COOL SPRING MORNING"), 15.f, 0.75f, 0.32f, 0.82f, 0.14f, 1.f, false),
		P(TEXT("MILD DAY"), 21.f, 0.9f, 0.4f, 0.88f, 0.15f, 1.5f, false),
		P(TEXT("WARM SPRING DAY"), 25.f, 1.f, 0.6f, 1.f, 0.1f, 2.f, false),   // lively birdsong
		P(TEXT("HOT SUMMER DAY"), 31.f, 1.15f, 0.25f, 0.78f, 0.06f, 1.f, false), // birds hide from the heat
		P(TEXT("CLOUDY AND BREEZY"), 17.f, 0.45f, 0.18f, 0.74f, 0.45f, 1.f, true),
	};
	// start on a pleasant day more often than not
	From = To = FMath::RandBool() ? 2 : FMath::RandHelper(Presets.Num());
	Blend = 1.f;
	NextChange = FMath::FRandRange(240.f, 420.f);

	for (TActorIterator<ADirectionalLight> It(&InWorld); It; ++It)
	{
		SunLight = *It;
		SunBase = It->GetLightComponent()->Intensity;
		break;
	}
	const FFVWeatherPreset W = Blended();
	Birds = FVAudio::Play2D(&InWorld, EFVSound::Birds, W.BirdVolume, W.BirdRate);
	Wind = FVAudio::Play2D(&InWorld, EFVSound::Wind, W.WindVolume);
	if (Birds) Birds->bAutoDestroy = false;
	if (Wind) Wind->bAutoDestroy = false;
	Temperature = W.Temperature;
}

FString UFVWeatherSubsystem::GetWeatherName() const
{
	return Presets.IsValidIndex(To) ? Presets[To].Name : FString();
}

bool UFVWeatherSubsystem::IsCloudy() const
{
	return Presets.IsValidIndex(To) && Presets[To].bCloudy;
}

void UFVWeatherSubsystem::SetWeather(int32 Index, float BlendSeconds)
{
	if (!Presets.IsValidIndex(Index))
	{
		return;
	}
	// start the new blend from wherever we are now
	From = Blend >= 0.5f ? To : From;
	To = Index;
	Blend = 0.f;
	BlendTime = FMath::Max(BlendSeconds, 0.1f);
	NextChange = FMath::FRandRange(240.f, 420.f);
}

void UFVWeatherSubsystem::PickNext()
{
	float Total = 0.f;
	for (int32 i = 0; i < Presets.Num(); ++i)
	{
		Total += (i == To) ? 0.f : Presets[i].Weight;
	}
	float R = FMath::FRandRange(0.f, Total);
	for (int32 i = 0; i < Presets.Num(); ++i)
	{
		if (i == To)
		{
			continue;
		}
		R -= Presets[i].Weight;
		if (R <= 0.f)
		{
			SetWeather(i, 45.f); // weather changes slowly
			return;
		}
	}
}

FFVWeatherPreset UFVWeatherSubsystem::Blended() const
{
	if (!Presets.IsValidIndex(From) || !Presets.IsValidIndex(To))
	{
		return FFVWeatherPreset();
	}
	const FFVWeatherPreset& A = Presets[From];
	const FFVWeatherPreset& B = Presets[To];
	const float T = FMath::SmoothStep(0.f, 1.f, Blend);
	FFVWeatherPreset W = B;
	W.Temperature = FMath::Lerp(A.Temperature, B.Temperature, T);
	W.Sun = FMath::Lerp(A.Sun, B.Sun, T);
	W.BirdVolume = FMath::Lerp(A.BirdVolume, B.BirdVolume, T);
	W.BirdRate = FMath::Lerp(A.BirdRate, B.BirdRate, T);
	W.WindVolume = FMath::Lerp(A.WindVolume, B.WindVolume, T);
	return W;
}

void UFVWeatherSubsystem::Tick(float DeltaTime)
{
	if (Presets.Num() == 0)
	{
		return;
	}
	if (Blend < 1.f)
	{
		Blend = FMath::Min(1.f, Blend + DeltaTime / BlendTime);
	}
	NextChange -= DeltaTime;
	if (NextChange <= 0.f)
	{
		PickNext();
	}
	const FFVWeatherPreset W = Blended();
	Temperature = W.Temperature;

	// Valentino feels the temperature (slow when cool, heads for the pond when hot)
	for (TActorIterator<AFVValentino> It(GetWorld()); It; ++It)
	{
		It->Temperature = Temperature;
	}
	if (SunLight && SunBase > 0.f)
	{
		SunLight->GetLightComponent()->SetIntensity(SunBase * W.Sun);
	}
	const float Menu = bMenuMode ? 0.45f : 1.f;
	if (Birds)
	{
		Birds->SetVolumeMultiplier(W.BirdVolume * Menu);
		Birds->SetPitchMultiplier(W.BirdRate);
	}
	if (Wind)
	{
		Wind->SetVolumeMultiplier(W.WindVolume * Menu);
	}
}
