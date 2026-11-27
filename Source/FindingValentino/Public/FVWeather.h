// Finding Valentino - simple weather: a few kinds of day that slowly drift from one to the next.
// Drives the temperature (Valentino reacts to it), how bright the sun is, and the garden ambience (birds, wind).
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FVWeather.generated.h"

class UAudioComponent;
class ADirectionalLight;

USTRUCT(BlueprintType)
struct FFVWeatherPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Temperature = 22.f;
	/** Sun brightness relative to the level's setting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Sun = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float BirdVolume = 0.4f;
	/** Playback rate of the birdsong: higher = busier, livelier chirping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float BirdRate = 0.85f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float WindVolume = 0.15f;
	/** Rough chance weight when picking the next weather. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Weight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bCloudy = false;
	/** How overcast the sky looks (0 = clear blue, 1 = grey cloud cover). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Cloud = 0.f;
};

UCLASS()
class FINDINGVALENTINO_API UFVWeatherSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UFVWeatherSubsystem, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type T) const override { return T == EWorldType::Game || T == EWorldType::PIE; }

	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetTemperature() const { return Temperature; }
	/** Name of the weather we're in (or heading into). */
	UFUNCTION(BlueprintPure, Category = "Weather")
	FString GetWeatherName() const;
	UFUNCTION(BlueprintPure, Category = "Weather")
	bool IsCloudy() const;
	UFUNCTION(BlueprintPure, Category = "Weather")
	bool IsSunnyLooking() const { return !IsCloudy(); }

	/** Jump to a weather by index (for testing). */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeather(int32 Index, float BlendSeconds = 5.f);

	/** Quieter ambience under the main menu. */
	void SetMenuMode(bool bMenu) { bMenuMode = bMenu; }

	TArray<FFVWeatherPreset> Presets;

private:
	void PickNext();
	FFVWeatherPreset Blended() const;

	int32 From = 0;
	int32 To = 0;
	float Blend = 1.f;
	float BlendTime = 30.f;
	float NextChange = 300.f;
	float Temperature = 22.f;
	float SunBase = -1.f;
	bool bMenuMode = false;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Birds;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Wind;
	UPROPERTY(Transient)
	TObjectPtr<ADirectionalLight> SunLight;

	// ---- how the day looks: cloud cover, haze, colour grading ----
	void ApplyLook(const FFVWeatherPreset& W);
	UPROPERTY(Transient)
	TObjectPtr<class APostProcessVolume> LookVolume;
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> CloudMID;
	UPROPERTY(Transient)
	TObjectPtr<class UExponentialHeightFogComponent> Fog;
	float FogBase = 0.01f;
	float CloudCoverageBase = -0.2f;
	float CloudDensityBase = 0.008f;
};
