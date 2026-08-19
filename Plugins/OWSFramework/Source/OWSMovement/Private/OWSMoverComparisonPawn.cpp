#include "OWSMoverComparisonPawn.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "Engine/Engine.h"
#include "Engine/CollisionProfile.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Math/RotationMatrix.h"
#include "MoverDataModelTypes.h"
#include "Net/UnrealNetwork.h"
#include "OWSMoverComparisonSaveSlot.h"
#include "OWSMoverSaveRestoreComponent.h"
#include "SaveManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogOWSMoverComparison, Log, All);

const FName AOWSMoverComparisonPawn::ComparisonSaveSlot(
	TEXT("OWSMoverComparisonCheckpoint"));

AOWSMoverComparisonPawn::AOWSMoverComparisonPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Save Extension must not directly write this actor's transform. The saved
	// bridge component queues its restore through Mover instead.
	Tags.AddUnique(FName(TEXT("!SaveTransform")));

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(42.0f, 96.0f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->SetCanEverAffectNavigation(true);
	RootComponent = CapsuleComponent;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CapsuleComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CapsuleComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("MoverComponent"));
	MoverComponent->SetUpdatedComponent(CapsuleComponent);
	SaveRestoreComponent = CreateDefaultSubobject<UOWSMoverSaveRestoreComponent>(TEXT("MoverSaveRestore"));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MeshFinder.Succeeded())
	{
		MeshComponent->SetSkeletalMeshAsset(MeshFinder.Object);
	}
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimationFinder(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (AnimationFinder.Succeeded())
	{
		MeshComponent->SetAnimInstanceClass(AnimationFinder.Class);
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultContextFinder(
		TEXT("/Game/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseContextFinder(
		TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveFinder(
		TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookFinder(
		TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookFinder(
		TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpFinder(
		TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	DefaultMappingContext = DefaultContextFinder.Object;
	MouseLookMappingContext = MouseContextFinder.Object;
	MoveAction = MoveFinder.Object;
	LookAction = LookFinder.Object;
	MouseLookAction = MouseLookFinder.Object;
	JumpAction = JumpFinder.Object;
}

void AOWSMoverComparisonPawn::BeginPlay()
{
	Super::BeginPlay();

	SaveRestoreComponent->OnGroundedRestoreEvaluated.AddUniqueDynamic(
		this,
		&ThisClass::HandleAutomaticRestoreEvaluated);

	if (HasAuthority())
	{
		if (USaveManager* SaveManager = USaveManager::Get(this))
		{
			// Comparison mode owns this in-memory slot. Forcing the class here
			// avoids changing global Save Extension settings used by the main pawn.
			SaveManager->EnsureActiveSlot(UOWSMoverComparisonSaveSlot::StaticClass(), true);
		}
	}

	if (IsLocallyControlled())
	{
		UE_LOG(LogOWSMoverComparison, Display,
			TEXT("Mover comparison active: move/look/jump, F5 grounded save, F9 grounded load."));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				8.0f,
				FColor::Cyan,
				TEXT("MOVER COMPARISON - F5 save grounded checkpoint, F9 load"));
		}
	}
}

void AOWSMoverComparisonPawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	AddInputContexts();
}

void AOWSMoverComparisonPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(
				MoveAction, ETriggerEvent::Triggered, this, &ThisClass::OnMoveTriggered);
			EnhancedInput->BindAction(
				MoveAction, ETriggerEvent::Completed, this, &ThisClass::OnMoveCompleted);
		}
		if (LookAction)
		{
			EnhancedInput->BindAction(
				LookAction, ETriggerEvent::Triggered, this, &ThisClass::OnLookTriggered);
		}
		if (MouseLookAction)
		{
			EnhancedInput->BindAction(
				MouseLookAction, ETriggerEvent::Triggered, this, &ThisClass::OnLookTriggered);
		}
		if (JumpAction)
		{
			EnhancedInput->BindAction(
				JumpAction, ETriggerEvent::Started, this, &ThisClass::OnJumpStarted);
			EnhancedInput->BindAction(
				JumpAction, ETriggerEvent::Completed, this, &ThisClass::OnJumpCompleted);
		}
	}

	PlayerInputComponent->BindKey(
		EKeys::F5, IE_Pressed, this, &ThisClass::RequestSaveComparisonCheckpoint);
	PlayerInputComponent->BindKey(
		EKeys::F9, IE_Pressed, this, &ThisClass::RequestLoadComparisonCheckpoint);
}

void AOWSMoverComparisonPawn::AddInputContexts()
{
	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;
	if (!InputSubsystem)
	{
		return;
	}

	if (DefaultMappingContext)
	{
		InputSubsystem->RemoveMappingContext(DefaultMappingContext);
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	if (MouseLookMappingContext)
	{
		InputSubsystem->RemoveMappingContext(MouseLookMappingContext);
		InputSubsystem->AddMappingContext(MouseLookMappingContext, 1);
	}
}

void AOWSMoverComparisonPawn::OnMoveTriggered(const FInputActionValue& Value)
{
	CachedMoveInput = Value.Get<FVector2D>().GetClampedToMaxSize(1.0f);
}

void AOWSMoverComparisonPawn::OnMoveCompleted(const FInputActionValue& Value)
{
	CachedMoveInput = FVector2D::ZeroVector;
}

void AOWSMoverComparisonPawn::OnLookTriggered(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AOWSMoverComparisonPawn::OnJumpStarted(const FInputActionValue& Value)
{
	bJumpJustPressed = !bJumpPressed;
	bJumpPressed = true;
}

void AOWSMoverComparisonPawn::OnJumpCompleted(const FInputActionValue& Value)
{
	bJumpPressed = false;
	bJumpJustPressed = false;
}

void AOWSMoverComparisonPawn::ProduceInput_Implementation(
	const int32 SimTimeMs,
	FMoverInputCmdContext& InputCmdResult)
{
	FCharacterDefaultInputs& Inputs =
		InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	Inputs.ControlRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();

	const FRotator YawRotation(0.0f, Inputs.ControlRotation.Yaw, 0.0f);
	const FVector WorldIntent =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) * CachedMoveInput.Y +
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) * CachedMoveInput.X;
	Inputs.SetMoveInput(EMoveInputType::DirectionalIntent, WorldIntent.GetClampedToMaxSize(1.0f));
	Inputs.OrientationIntent = WorldIntent.GetSafeNormal();
	Inputs.SuggestedMovementMode = NAME_None;
	Inputs.bUsingMovementBase = false;
	Inputs.MovementBase = nullptr;
	Inputs.MovementBaseBoneName = NAME_None;
	Inputs.bIsJumpPressed = bJumpPressed;
	Inputs.bIsJumpJustPressed = bJumpJustPressed;

	bJumpJustPressed = false;
}

void AOWSMoverComparisonPawn::RequestSaveComparisonCheckpoint()
{
	if (HasAuthority())
	{
		SaveComparisonCheckpointAuthority();
	}
	else
	{
		ServerSaveComparisonCheckpoint();
	}
}

void AOWSMoverComparisonPawn::RequestLoadComparisonCheckpoint()
{
	if (HasAuthority())
	{
		LoadComparisonCheckpointAuthority();
	}
	else
	{
		ServerLoadComparisonCheckpoint();
	}
}

bool AOWSMoverComparisonPawn::ServerSaveComparisonCheckpoint_Validate()
{
	return true;
}

void AOWSMoverComparisonPawn::ServerSaveComparisonCheckpoint_Implementation()
{
	SaveComparisonCheckpointAuthority();
}

bool AOWSMoverComparisonPawn::ServerLoadComparisonCheckpoint_Validate()
{
	return true;
}

void AOWSMoverComparisonPawn::ServerLoadComparisonCheckpoint_Implementation()
{
	LoadComparisonCheckpointAuthority();
}

void AOWSMoverComparisonPawn::SaveComparisonCheckpointAuthority()
{
	SetCheckpointResult(
		SaveRestoreComponent->RequestGroundedSave(ComparisonSaveSlot),
		TEXT("Grounded save"));
}

void AOWSMoverComparisonPawn::LoadComparisonCheckpointAuthority()
{
	SetCheckpointResult(
		SaveRestoreComponent->RequestGroundedLoad(ComparisonSaveSlot),
		TEXT("Grounded load request"));
}

void AOWSMoverComparisonPawn::HandleAutomaticRestoreEvaluated(
	const FName LoadedSlotName,
	const EOWSGroundedMovementResult Result)
{
	if (HasAuthority())
	{
		SetCheckpointResult(Result, TEXT("Automatic Mover restore"));
	}
}

void AOWSMoverComparisonPawn::SetCheckpointResult(
	const EOWSGroundedMovementResult NewResult,
	const TCHAR* Operation)
{
	LastCheckpointResult = NewResult;
	DisplayCheckpointResult(Operation);
	ForceNetUpdate();
}

void AOWSMoverComparisonPawn::OnRep_LastCheckpointResult()
{
	DisplayCheckpointResult(TEXT("Checkpoint"));
}

void AOWSMoverComparisonPawn::DisplayCheckpointResult(const TCHAR* Operation) const
{
	const FText ResultText = UEnum::GetDisplayValueAsText(LastCheckpointResult);
	const FString Message = FString::Printf(
		TEXT("%s: %s"),
		Operation,
		*ResultText.ToString());
	UE_LOG(LogOWSMoverComparison, Display, TEXT("%s"), *Message);
	if (IsLocallyControlled() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			740079,
			5.0f,
			LastCheckpointResult == EOWSGroundedMovementResult::Success
				? FColor::Green
				: FColor::Yellow,
			Message);
	}
}

void AOWSMoverComparisonPawn::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, LastCheckpointResult);
}
