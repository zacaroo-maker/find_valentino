#include "FVGameModes.h"

#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FVFamilyCharacter.h"
#include "FVFamilyMemberData.h"
#include "FVGameInstance.h"
#include "FVHUD.h"
#include "FVAudio.h"
#include "FVWeather.h"
#include "Engine/World.h"
#include "FVMenuHUD.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpectatorPawn.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------- garden game mode

AFVGameMode::AFVGameMode()
{
	HUDClass = AFVHUD::StaticClass();
	PlayerControllerClass = AFVPlayerController::StaticClass();
}

UFVFamilyMemberData* AFVGameMode::GetSelectedMember() const
{
	const UFVGameInstance* GI = Cast<UFVGameInstance>(GetGameInstance());
	return GI ? GI->SelectedMember.Get() : nullptr;
}

void AFVGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// Menu mode: asked for explicitly, or a packaged game starting with nobody chosen yet.
	// (Play-in-editor without "?Menu" goes straight into the garden for quick testing.)
	const bool bAsked = UGameplayStatics::HasOption(Options, TEXT("Menu"));
	const bool bFreshStart = !GetSelectedMember() && !GetWorld()->IsPlayInEditor();
	bMenuMode = bAsked || bFreshStart;
	if (bMenuMode)
	{
		HUDClass = AFVMenuHUD::StaticClass();
		PlayerControllerClass = APlayerController::StaticClass();
	}
}

void AFVGameMode::BeginPlay()
{
	Super::BeginPlay();
	// garden ambience (birds, wind) follows the weather; quieter under the menu music
	if (UFVWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UFVWeatherSubsystem>())
	{
		Weather->SetMenuMode(bMenuMode);
	}
	if (!bMenuMode)
	{
		return;
	}
	// look at the garden through the camera tagged "MenuCamera"
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MenuCamera")) && PC)
		{
			PC->SetViewTarget(*It);
			break;
		}
	}
}

UClass* AFVGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (bMenuMode)
	{
		return ASpectatorPawn::StaticClass();
	}
	// use the member's own character (e.g. their MetaHuman) when one exists, otherwise the level's default
	if (const UFVFamilyMemberData* M = GetSelectedMember())
	{
		if (UClass* C = M->CharacterClass.LoadSynchronous())
		{
			return C;
		}
		// no character of their own, but a MetaHuman: the standard family character wears it
		if (!M->MetaHumanClass.IsNull())
		{
			return AFVFamilyCharacter::StaticClass();
		}
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AFVGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);
	UFVFamilyMemberData* M = GetSelectedMember();
	if (!M || bMenuMode)
	{
		return;
	}
	if (AFVFamilyCharacter* FC = Cast<AFVFamilyCharacter>(PlayerPawn))
	{
		FC->ApplyMemberData(M);
	}
	if (ACharacter* C = Cast<ACharacter>(PlayerPawn))
	{
		C->GetCharacterMovement()->MaxWalkSpeed = M->WalkSpeed;
	}
}

// ---------------------------------------------------------------- main menu game mode

AFVMenuGameMode::AFVMenuGameMode()
{
	HUDClass = AFVMenuHUD::StaticClass();
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
}

void AFVMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
	// look through the camera tagged "MenuCamera" (a nice view of the garden)
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MenuCamera")) && PC)
		{
			PC->SetViewTarget(*It);
			break;
		}
	}
}

// ---------------------------------------------------------------- player controller (pause menu)

void AFVPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (MenuMappingContext)
		{
			Sub->AddMappingContext(MenuMappingContext, 10);
		}
	}
	FInputModeGameOnly Mode;
	SetInputMode(Mode);
	bShowMouseCursor = false;
}

void AFVPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!PauseAction)
	{
		PauseAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Core/Input/IA_Pause.IA_Pause"));
	}
	if (!MenuMappingContext)
	{
		MenuMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Core/Input/IMC_Menu.IMC_Menu"));
	}
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PauseAction)
		{
			EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &AFVPlayerController::TogglePauseMenu);
		}
	}
}

void AFVPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	// footsteps for whoever is being played, timed to their feet touching the ground
	Footsteps.Update(Cast<ACharacter>(GetPawn()), DeltaTime);
}

void AFVPlayerController::Weather(int32 Index)
{
	if (UFVWeatherSubsystem* W = GetWorld()->GetSubsystem<UFVWeatherSubsystem>())
	{
		W->SetWeather(Index, 3.f);
	}
}

void AFVPlayerController::TogglePauseMenu()
{
	bMenuOpen = !bMenuOpen;
	FVAudio::Play2D(this, EFVSound::UIPause, 0.6f, bMenuOpen ? 1.f : 0.85f);
	SetPause(bMenuOpen);
	bShowMouseCursor = bMenuOpen;
	bEnableClickEvents = bMenuOpen;
	bEnableMouseOverEvents = bMenuOpen;
	if (bMenuOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
	}
	else
	{
		FInputModeGameOnly Mode;
		SetInputMode(Mode);
	}
}

void AFVPlayerController::GoToMainMenu()
{
	SetPause(false);
	const UFVGameInstance* GI = Cast<UFVGameInstance>(GetGameInstance());
	UGameplayStatics::OpenLevel(this, GI ? GI->GameLevel : FName(TEXT("L_Backyard")), true, TEXT("Menu"));
}
