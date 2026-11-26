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
	// garden ambience: birdsong and a light breeze (quieter under the menu music)
	FVAudio::Play2D(this, EFVSound::Birds, bMenuMode ? 0.25f : 0.55f);
	FVAudio::Play2D(this, EFVSound::Wind, bMenuMode ? 0.08f : 0.18f);
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
	// soft footsteps on the grass, one per stride
	const ACharacter* C = Cast<ACharacter>(GetPawn());
	if (!C || !C->GetCharacterMovement())
	{
		return;
	}
	const float Speed = C->GetVelocity().Size2D();
	if (C->GetCharacterMovement()->IsMovingOnGround() && Speed > 30.f)
	{
		StepDistance += Speed * DeltaTime;
		const float Stride = FMath::Lerp(70.f, 110.f, FMath::Clamp((Speed - 150.f) / 350.f, 0.f, 1.f));
		if (StepDistance >= Stride)
		{
			StepDistance = 0.f;
			const FVector Foot = C->GetActorLocation() - FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			FVAudio::PlayAt(this, EFVSound::Step, Foot, 0.35f, FMath::FRandRange(0.9f, 1.1f));
		}
	}
	else
	{
		StepDistance = 0.f;
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
