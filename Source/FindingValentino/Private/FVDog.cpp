#include "FVDog.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "FVDogAIController.h"
#include "FVValentino.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AFVDog::AFVDog()
{
	PrimaryActorTick.bCanEverTick = true;

	// small dog: ~43 cm tall, ~68 cm long incl. tail
	GetCapsuleComponent()->InitCapsuleSize(20.f, 22.f);
	GetMesh()->SetRelativeLocation(FVector(0, 0, -22.f));
	GetMesh()->SetRelativeRotation(FRotator(0, 270.f, 0)); // model faces +Y, actor forward is +X
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0, 300.f, 0);
	Move->MaxWalkSpeed = 110.f;
	Move->MaxAcceleration = 400.f;
	Move->BrakingDecelerationWalking = 500.f;
	Move->bUseRVOAvoidance = false;
	// use the "Human" navmesh (radius 35) so he keeps out of the narrow tortoise-only gaps
	Move->NavAgentProps.AgentRadius = 34.f;
	Move->NavAgentProps.AgentHeight = 60.f;

	AIControllerClass = AFVDogAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AFVDog::BeginPlay()
{
	Super::BeginPlay();
	PlayAnim(IdleAnim, true);
}

void AFVDog::PlayAnim(UAnimSequence* Anim, bool bLoop, float Rate)
{
	if (!Anim)
	{
		return;
	}
	if (Anim != CurrentAnim)
	{
		GetMesh()->PlayAnimation(Anim, bLoop);
		CurrentAnim = Anim;
	}
	GetMesh()->SetPlayRate(Rate);
}

float AFVDog::PlayOnce(UAnimSequence* Anim)
{
	PoseLoop = nullptr;
	if (!Anim)
	{
		return 0.f;
	}
	CurrentAnim = nullptr; // force restart
	PlayAnim(Anim, false);
	OneShotTimeLeft = Anim->GetPlayLength();
	return OneShotTimeLeft;
}

void AFVDog::SetPoseLoop(UAnimSequence* Anim)
{
	PoseLoop = Anim;
}

bool AFVDog::Pet()
{
	if (PetCooldown > 0.f)
	{
		return false;
	}
	PetCooldown = 0.5f;
	Energy = FMath::Clamp(Energy + PetEnergy, 0.f, 1.f);
	HappyTimeLeft = 4.f;
	ThoughtTimeLeft = 0.f; // react right away
	return true;
}

void AFVDog::PickUp(AActor* Holder, FVector Offset)
{
	if (bHeld || !Holder)
	{
		return;
	}
	bHeld = true;
	if (AController* C = GetController())
	{
		C->StopMovement();
	}
	PoseLoop = nullptr;
	OneShotTimeLeft = 0.f;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachToActor(Holder, FAttachmentTransformRules::KeepWorldTransform);
	SetActorRelativeLocation(Offset);
	SetActorRelativeRotation(FRotator(0.f, 90.f, 0.f)); // sideways in the arms
	ThoughtTimeLeft = 0.f;
}

void AFVDog::PutDown(FVector DropLocation, float Yaw)
{
	if (!bHeld)
	{
		return;
	}
	bHeld = false;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocationAndRotation(DropLocation, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	ThoughtTimeLeft = 0.f;
}

static FText PickThought(std::initializer_list<const TCHAR*> Options)
{
	const int32 N = (int32)Options.size();
	const int32 I = FMath::RandHelper(N);
	return FText::FromString(*(Options.begin() + I));
}

void AFVDog::Say(const FText& Text, float Seconds)
{
	Thought = Text;
	SayTimeLeft = Seconds;
}

void AFVDog::Bark()
{
	if (BarkCooldown > 0.f)
	{
		return;
	}
	BarkCooldown = 1.2f;
	const float Size = GetActorScale3D().Z;
	Say(Size < 0.8f ? PickThought({TEXT("YIP YIP!"), TEXT("YIP!"), TEXT("YAP YAP YAP!")})
	  : Size > 1.5f ? PickThought({TEXT("WOOF!!"), TEXT("BORK!"), TEXT("WOOF WOOF!")})
	                : PickThought({TEXT("WOOF!"), TEXT("ARF ARF!"), TEXT("WOOF WOOF!")}), 1.6f);
	if (!bHeld && GetCharacterMovement()->IsMovingOnGround())
	{
		LaunchCharacter(FVector(0.f, 0.f, 150.f), false, false); // a little bounce with each bark
	}
	if (BarkSound)
	{
		// big dogs bark deeper
		UGameplayStatics::PlaySoundAtLocation(this, BarkSound, GetActorLocation(), 1.f, FMath::Clamp(1.2f / FMath::Max(Size, 0.3f), 0.6f, 1.8f));
	}
}

void AFVDog::UpdateThought(float DeltaSeconds)
{
	if (SayTimeLeft > 0.f)
	{
		SayTimeLeft -= DeltaSeconds;
		return;
	}
	ThoughtTimeLeft -= DeltaSeconds;
	if (State != ThoughtState)
	{
		ThoughtTimeLeft = 0.f;
		ThoughtState = State;
	}
	if (ThoughtTimeLeft > 0.f)
	{
		return;
	}
	ThoughtTimeLeft = FMath::FRandRange(5.f, 9.f);

	// is Valentino close by?
	bool bNearValentino = false;
	for (TActorIterator<AFVValentino> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < 250.f)
		{
			bNearValentino = true;
		}
	}

	if (IsHappy())
	{
		Thought = bHeld ? PickThought({ TEXT("Best. Cuddle. Ever."), TEXT("More pets please!"), TEXT("I love you <3") })
		                : PickThought({ TEXT("I love pets!"), TEXT("More pets please!"), TEXT("Happy tail! <3"), TEXT("Right behind the ears!") });
		ThoughtTimeLeft = 2.5f;
		return;
	}
	if (bHeld)
	{
		Thought = PickThought({ TEXT("Wheee! I'm flying!"), TEXT("Put me down... or don't."), TEXT("I can see the whole garden!") });
		return;
	}
	if (bNearValentino && FMath::FRand() < 0.6f)
	{
		Thought = PickThought({ TEXT("Hello, shell friend!"), TEXT("Valentino! Wanna play?"), TEXT("Why is he so slow?") });
		return;
	}
	if (bInPark)
	{
		switch (State)
		{
		case EFVDogState::Play:
			Thought = PickThought({TEXT("Can't catch me!"), TEXT("Run run run!"), TEXT("ZOOMIES!!"), TEXT("Wheee!"), TEXT("Best day ever!")});
			return;
		case EFVDogState::Lying:
			Thought = PickThought({TEXT("Phew... need a rest."), TEXT("Played out..."), TEXT("Five more minutes.")});
			return;
		case EFVDogState::Sniff:
			Thought = PickThought({TEXT("So many smells!"), TEXT("Who's been here?"), TEXT("New friends!")});
			return;
		default:
			break;
		}
	}
	if (Energy < 0.25f && State != EFVDogState::Lying)
	{
		Thought = PickThought({ TEXT("So sleepy..."), TEXT("I need a sunny nap."), TEXT("Paws... so... tired...") });
		return;
	}
	switch (State)
	{
	case EFVDogState::GoingToSun:
		Thought = PickThought({ TEXT("Where's the sunny spot?"), TEXT("Too shady here..."), TEXT("Sun, here I come!") });
		break;
	case EFVDogState::LyingDown:
		Thought = PickThought({ TEXT("Ahh, sunshine!"), TEXT("Perfect spot.") });
		break;
	case EFVDogState::Lying:
		Thought = PickThought({ TEXT("Mmm... warm sun..."), TEXT("Zzz..."), TEXT("Best spot in the garden."), TEXT("Dreaming of treats...") });
		break;
	case EFVDogState::GettingUp:
		Thought = PickThought({ TEXT("Stretchhh!"), TEXT("Nap over. Adventure time!") });
		break;
	case EFVDogState::Sniff:
		Thought = PickThought({ TEXT("Sniff sniff..."), TEXT("Something smells interesting!"), TEXT("Was Valentino here?"), TEXT("A worm? A WORM?") });
		break;
	case EFVDogState::Wander:
	default:
		Thought = Energy > 0.8f ? PickThought({ TEXT("Zoomies time!"), TEXT("Play with me?") })
		                        : PickThought({ TEXT("Patrolling the garden."), TEXT("La la la..."), TEXT("Any squirrels today?") });
		break;
	}
}

void AFVDog::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PetCooldown -= DeltaSeconds;
	HappyTimeLeft -= DeltaSeconds;
	BarkCooldown -= DeltaSeconds;
	const float Size = FMath::Max(GetActorScale3D().Z, 0.3f);

	// energy: walking and being awake cost energy, sunbathing recharges
	const float Speed = GetVelocity().Size2D();
	if (State == EFVDogState::Lying)
	{
		Energy += SunRechargePerSecond * DeltaSeconds;
	}
	else if (!bHeld)
	{
		Energy -= AwakeDrainPerSecond * DeltaSeconds + (Speed * DeltaSeconds / 100.f) * DrainPerMetre;
	}
	Energy = FMath::Clamp(Energy, 0.f, 1.f);
	// tired dogs plod, playing dogs run, bigger dogs take longer strides
	GetCharacterMovement()->MaxWalkSpeed = (60.f + 70.f * Energy) * PlaySpeed * FMath::Sqrt(Size);
	GetCharacterMovement()->MaxAcceleration = PlaySpeed > 1.f ? 900.f : 400.f;

	UpdateThought(DeltaSeconds);

	if (bHeld)
	{
		PlayAnim(IdleAnim, true, IsHappy() ? 2.5f : 1.2f); // tail wagging in the arms
		return;
	}
	if (OneShotTimeLeft > 0.f)
	{
		OneShotTimeLeft -= DeltaSeconds;
		return;
	}
	if (PoseLoop)
	{
		PlayAnim(PoseLoop, true, (IsHappy() && PoseLoop == LyingAnim) ? 2.f : 1.f);
		return;
	}
	if (Speed > 10.f)
	{
		PlayAnim(WalkAnim, true, FMath::Clamp(Speed / FMath::Max(WalkAnimSpeed * Size, 1.f), 0.4f, 3.5f));
	}
	else
	{
		PlayAnim(IdleAnim, true, IsHappy() ? 2.5f : 1.f); // happy = fast tail wag
	}
}
