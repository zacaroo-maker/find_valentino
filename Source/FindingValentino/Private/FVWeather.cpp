#include "FVWeather.h"

#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "FVAudio.h"
#include "FVValentino.h"
#include "FVDay.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"

void UFVWeatherSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	auto P = [](const TCHAR* Name, float Temp, float Sun, float BirdVol, float BirdRate, float Wind, float Weight, bool bCloudy, float Cloud)
	{
		FFVWeatherPreset W;
		W.Name = Name; W.Temperature = Temp; W.Sun = Sun; W.BirdVolume = BirdVol; W.BirdRate = BirdRate;
		W.WindVolume = Wind; W.Weight = Weight; W.bCloudy = bCloudy; W.Cloud = Cloud;
		return W;
	};
	Presets = {
		P(TEXT("COOL SPRING MORNING"), 15.f, 0.75f, 0.32f, 0.82f, 0.14f, 1.f, false, 0.3f),
		P(TEXT("MILD DAY"), 21.f, 0.9f, 0.4f, 0.88f, 0.15f, 1.5f, false, 0.2f),
		P(TEXT("WARM SPRING DAY"), 25.f, 1.f, 0.6f, 1.f, 0.1f, 2.f, false, 0.1f),    // lively birdsong
		P(TEXT("HOT SUMMER DAY"), 31.f, 1.15f, 0.25f, 0.78f, 0.06f, 1.f, false, 0.f), // birds hide from the heat
		P(TEXT("CLOUDY AND BREEZY"), 17.f, 0.3f, 0.18f, 0.74f, 0.45f, 1.f, true, 1.f),
	};

	// a post-process volume of our own for the weather's colour grading (sits on top of the level's)
	LookVolume = InWorld.SpawnActor<APostProcessVolume>();
	if (LookVolume)
	{
		LookVolume->bUnbound = true;
		LookVolume->Priority = 10.f;
		LookVolume->BlendWeight = 1.f;
	}
	// the sky's clouds: a dynamic copy of the cloud material we can thicken on grey days
	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		if (UVolumetricCloudComponent* C = It->FindComponentByClass<UVolumetricCloudComponent>())
		{
			if (UMaterialInterface* M = C->GetMaterial())
			{
				CloudMID = UMaterialInstanceDynamic::Create(M, this);
				CloudMID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Cloud_GlobalCoverage")), CloudCoverageBase);
				CloudMID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Cloud_GlobalDensity")), CloudDensityBase);
				C->SetMaterial(CloudMID);
			}
		}
		if (UExponentialHeightFogComponent* F = It->FindComponentByClass<UExponentialHeightFogComponent>())
		{
			Fog = F;
			FogBase = F->FogDensity;
		}
	}
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

void UFVWeatherSubsystem::ApplyLook(const FFVWeatherPreset& W)
{
	const UFVDaySubsystem* Day = GetWorld()->GetSubsystem<UFVDaySubsystem>();
	const float Golden = (Day && Day->IsRunning()) ? Day->GetGolden() : 0.f; // 0 midday .. 1 low sun
	const float C = FMath::Clamp(W.Cloud, 0.f, 1.f);

	// clouds roll in: more coverage, a bit denser and greyer
	if (CloudMID)
	{
		CloudMID->SetScalarParameterValue(TEXT("Cloud_GlobalCoverage"), FMath::Lerp(CloudCoverageBase, 0.5f, C));
		CloudMID->SetScalarParameterValue(TEXT("Cloud_GlobalDensity"), CloudDensityBase * (1.f + 0.8f * C));
		const float Alb = FMath::Lerp(0.98f, 0.72f, C);
		CloudMID->SetVectorParameterValue(TEXT("Cloud_AlbedoColor"), FLinearColor(Alb, Alb, Alb * 1.02f, 0.5f));
	}
	// a little haze on grey days and in the golden hours
	if (Fog)
	{
		Fog->SetFogDensity(FogBase * (1.f + 2.2f * C + 0.8f * Golden));
	}
	// colour grading: grey days are flatter, cooler and a touch darker (auto exposure would otherwise
	// brighten them right back up); low sun gives everything a warm golden tint
	if (LookVolume)
	{
		FPostProcessSettings& S = LookVolume->Settings;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = 1.f - 0.7f * C - 0.2f * Golden;
		S.bOverride_ColorSaturation = true;
		const float Sat = 1.f - 0.28f * C + 0.08f * Golden;
		S.ColorSaturation = FVector4(Sat, Sat, Sat, 1.f);
		S.bOverride_ColorContrast = true;
		const float Con = 1.f - 0.1f * C;
		S.ColorContrast = FVector4(Con, Con, Con, 1.f);
		S.bOverride_ColorGain = true;
		const float G = Golden * (1.f - 0.6f * C);
		S.ColorGain = FVector4(1.f + 0.05f * G - 0.03f * C, 1.f - 0.01f * G - 0.01f * C, 1.f - 0.1f * G + 0.04f * C, 1.f);
	}
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
	W.Cloud = FMath::Lerp(A.Cloud, B.Cloud, T);
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
	// the sun fades a little as it sinks towards the horizon (the day's clock sets its angle and colour)
	float Elevation = 45.f;
	if (const UFVDaySubsystem* Day = GetWorld()->GetSubsystem<UFVDaySubsystem>())
	{
		if (Day->IsRunning())
		{
			Elevation = Day->GetSunElevation();
		}
	}
	if (SunLight && SunBase > 0.f)
	{
		const float Low = 0.55f + 0.45f * FMath::SmoothStep(3.f, 22.f, Elevation);
		SunLight->GetLightComponent()->SetIntensity(SunBase * W.Sun * Low);
	}
	ApplyLook(W);
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
