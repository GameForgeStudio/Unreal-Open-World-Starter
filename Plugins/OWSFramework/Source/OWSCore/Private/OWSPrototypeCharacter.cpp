#include "OWSPrototypeCharacter.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/Collections/SigilItemCollection.h"
#include "Core/Collections/SigilItemSlotCollection.h"
#include "Core/Fragments/SigilItemFragment_Equippable.h"
#include "Core/Items/SigilItemDefinition.h"
#include "Core/Items/SigilItemInfo.h"
#include "Core/Items/SigilItemInstance.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Math/RotationMatrix.h"
#include "Net/UnrealNetwork.h"
#include "OWSCore.h"
#include "OWSInventoryBridgeComponent.h"
#include "OWSItemFragments.h"
#include "OWSPrototypeAttributeSet.h"
#include "OWSPrototypeGameplayAbility.h"
#include "OWSPrototypeInventoryComponents.h"
#include "SaveManager.h"
#include "SaveSlot.h"
#include "SigilInventoryTags.h"
#include "UObject/ConstructorHelpers.h"

const FName AOWSPrototypeCharacter::PrototypeSaveSlot(TEXT("OWSPrototypeCheckpoint"));

AOWSPrototypeCharacter::AOWSPrototypeCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;
	SetReplicateMovement(true);

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	// UE's stock 750,000 continuous push force is suitable for generic arcade
	// interactions but can launch an awake vehicle. Keep physical interaction so
	// lightweight lab props still react, while making mass—not a hidden character
	// assist—determine how much a person can move a car.
	GetCharacterMovement()->bEnablePhysicsInteraction = true;
	GetCharacterMovement()->bPushForceScaledToMass = false;
	GetCharacterMovement()->bScalePushForceToVelocity = true;
	GetCharacterMovement()->bPushForceUsingZOffset = true;
	GetCharacterMovement()->PushForcePointZOffsetFactor = -0.55f;
	GetCharacterMovement()->InitialPushForceFactor = 100.0f;
	GetCharacterMovement()->PushForceFactor = 7500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Full);
	AttributeSet = CreateDefaultSubobject<UOWSPrototypeAttributeSet>(TEXT("AttributeSet"));
	InventorySystem = CreateDefaultSubobject<UOWSPrototypeInventorySystemComponent>(TEXT("InventorySystem"));
	EquipmentSystem = CreateDefaultSubobject<UOWSPrototypeEquipmentSystemComponent>(TEXT("EquipmentSystem"));
	InventoryBridge = CreateDefaultSubobject<UOWSInventoryBridgeComponent>(TEXT("InventoryBridge"));

	// These exact assets are created idempotently by the editor command
	// OWS.GeneratePrototypeAssets. The first generator run also wires the
	// live CDO; subsequent editor/game starts resolve them here.
	static ConstructorHelpers::FObjectFinder<USigilItemCollectionDefinition> MainCollectionFinder(
		TEXT("/Game/OWSPrototype/Data/DA_Collection_Main.DA_Collection_Main"));
	static ConstructorHelpers::FObjectFinder<USigilItemSlotCollectionDefinition> EquippedCollectionFinder(
		TEXT("/Game/OWSPrototype/Data/DA_Collection_Equipped.DA_Collection_Equipped"));
	InventorySystem->PrototypeMainCollection = MainCollectionFinder.Object;
	InventorySystem->PrototypeEquippedCollection = EquippedCollectionFinder.Object;
	InventorySystem->PrototypeSwordDefinition = TSoftObjectPtr<USigilItemDefinition>(
		FSoftObjectPath(TEXT("/Game/OWSPrototype/Data/DA_Item_DemoSword.DA_Item_DemoSword")));
	InventorySystem->PrototypePotionDefinition = TSoftObjectPtr<USigilItemDefinition>(
		FSoftObjectPath(TEXT("/Game/OWSPrototype/Data/DA_Item_DemoPotion.DA_Item_DemoPotion")));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimationFinder(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (AnimationFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimationFinder.Class);
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultContextFinder(
		TEXT("/Game/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseContextFinder(
		TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveFinder(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookFinder(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookFinder(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpFinder(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	DefaultMappingContext = DefaultContextFinder.Object;
	MouseLookMappingContext = MouseContextFinder.Object;
	MoveAction = MoveFinder.Object;
	LookAction = LookFinder.Object;
	MouseLookAction = MouseLookFinder.Object;
	JumpAction = JumpFinder.Object;
}

UAbilitySystemComponent* AOWSPrototypeCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AOWSPrototypeCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilityActorInfo();

	if (HasAuthority())
	{
		if (USaveManager* SaveManager = USaveManager::Get(this))
		{
			TScriptInterface<ISaveExtensionInterface> Interface;
			Interface.SetObject(this);
			Interface.SetInterface(static_cast<ISaveExtensionInterface*>(this));
			SaveManager->SubscribeForEvents(Interface);
		}
		InitializePrototypeSystems();
	}
}

void AOWSPrototypeCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (USaveManager* SaveManager = USaveManager::Get(this))
		{
			TScriptInterface<ISaveExtensionInterface> Interface;
			Interface.SetObject(this);
			Interface.SetInterface(static_cast<ISaveExtensionInterface*>(this));
			SaveManager->UnsubscribeFromEvents(Interface);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AOWSPrototypeCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilityActorInfo();
}

void AOWSPrototypeCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilityActorInfo();
}

void AOWSPrototypeCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	InitializeAbilityActorInfo();
	AddInputContexts();
}

void AOWSPrototypeCharacter::InitializeAbilityActorInfo()
{
	if (AbilitySystem)
	{
		AbilitySystem->InitAbilityActorInfo(this, this);
	}
}

void AOWSPrototypeCharacter::InitializePrototypeSystems()
{
	FString ValidationError;
	if (!InventorySystem || !InventorySystem->PrepareAndValidatePrototypeConfiguration(ValidationError))
	{
		SetPrototypeStatus(FString::Printf(TEXT("Inventory setup required: %s"), *ValidationError));
		return;
	}

	InventorySystem->InitializeInventorySystem();
	if (!InventorySystem->IsInventoryInitialized())
	{
		SetPrototypeStatus(TEXT("Sigil inventory initialization failed."));
		return;
	}
	if (!InventorySystem->SeedMissingPrototypeItems(ValidationError))
	{
		SetPrototypeStatus(FString::Printf(TEXT("Inventory seed failed: %s"), *ValidationError));
		return;
	}
	EquipmentSystem->InitializeEquipmentSystemWithInventory(InventorySystem);
	SetPrototypeStatus(EquipmentSystem->IsEquipmentSystemInitialized()
		? TEXT("Ready: E equip, Q unequip, U use, P pulse, F5 save, F9 load.")
		: TEXT("Inventory initialized, but equipment initialization failed."));
}

void AOWSPrototypeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction) Enhanced->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
		if (LookAction) Enhanced->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
		if (MouseLookAction) Enhanced->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
		if (JumpAction)
		{
			Enhanced->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJump);
			Enhanced->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJump);
		}
	}

	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ThisClass::RequestEquipDemoItem);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ThisClass::RequestUnequipDemoItem);
	PlayerInputComponent->BindKey(EKeys::U, IE_Pressed, this, &ThisClass::RequestUseDemoItem);
	PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &ThisClass::RequestActivateEquipmentAbility);
	PlayerInputComponent->BindKey(EKeys::F5, IE_Pressed, this, &ThisClass::RequestSaveGroundedCheckpoint);
	PlayerInputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ThisClass::RequestLoadGroundedCheckpoint);
	PlayerInputComponent->BindKey(
		EKeys::PageUp, IE_Pressed, this, &ThisClass::IncreasePrototypeScalar);
	PlayerInputComponent->BindKey(
		EKeys::PageDown, IE_Pressed, this, &ThisClass::DecreasePrototypeScalar);
}

void AOWSPrototypeCharacter::AddInputContexts()
{
	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;
	if (!Subsystem) return;
	if (DefaultMappingContext)
	{
		Subsystem->RemoveMappingContext(DefaultMappingContext);
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	if (MouseLookMappingContext)
	{
		Subsystem->RemoveMappingContext(MouseLookMappingContext);
		Subsystem->AddMappingContext(MouseLookMappingContext, 1);
	}
}

void AOWSPrototypeCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Input.X);
}

void AOWSPrototypeCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void AOWSPrototypeCharacter::StartJump() { Jump(); }
void AOWSPrototypeCharacter::StopJump() { StopJumping(); }

USigilItemInstance* AOWSPrototypeCharacter::FindFirstItem(const bool bEquipped, const bool bUseItem) const
{
	if (!InventorySystem) return nullptr;
	TArray<FSigilItemInfo> Items;
	InventorySystem->GetAllItemInfosInCollection(
		bEquipped ? SigilCollectionTags::Equipped : SigilCollectionTags::Main, Items);
	for (const FSigilItemInfo& Info : Items)
	{
		if (!IsValid(Info.Item)) continue;
		if (bUseItem ? Info.Item->FindFragmentByClass<UOWSItemFragment_UseEffect>() != nullptr
			: Info.Item->FindFragmentByClass<USigilItemFragment_Equippable>() != nullptr)
		{
			return Info.Item;
		}
	}
	return nullptr;
}

void AOWSPrototypeCharacter::RequestEquipDemoItem()
{
	if (InventoryBridge) InventoryBridge->RequestEquipItem(FindFirstItem(false, false));
}

void AOWSPrototypeCharacter::RequestUnequipDemoItem()
{
	if (InventoryBridge) InventoryBridge->RequestUnequipItem(FindFirstItem(true, false));
}

void AOWSPrototypeCharacter::RequestUseDemoItem()
{
	if (InventoryBridge) InventoryBridge->RequestUseItem(FindFirstItem(false, true));
}

void AOWSPrototypeCharacter::RequestActivateEquipmentAbility()
{
	if (AbilitySystem) AbilitySystem->TryActivateAbilityByClass(UOWSPrototypeGameplayAbility::StaticClass());
}

void AOWSPrototypeCharacter::RequestSaveGroundedCheckpoint()
{
	if (HasAuthority()) SaveGroundedCheckpointAuthority(); else ServerSaveGroundedCheckpoint();
}

void AOWSPrototypeCharacter::RequestLoadGroundedCheckpoint()
{
	if (HasAuthority()) LoadGroundedCheckpointAuthority(); else ServerLoadGroundedCheckpoint();
}

void AOWSPrototypeCharacter::RequestAdjustPrototypeScalar(const float Delta)
{
	if (HasAuthority()) AdjustPrototypeScalarAuthority(Delta); else ServerAdjustPrototypeScalar(Delta);
}

void AOWSPrototypeCharacter::IncreasePrototypeScalar()
{
	RequestAdjustPrototypeScalar(1.0f);
}

void AOWSPrototypeCharacter::DecreasePrototypeScalar()
{
	RequestAdjustPrototypeScalar(-1.0f);
}

bool AOWSPrototypeCharacter::ServerSaveGroundedCheckpoint_Validate() { return true; }
void AOWSPrototypeCharacter::ServerSaveGroundedCheckpoint_Implementation() { SaveGroundedCheckpointAuthority(); }
bool AOWSPrototypeCharacter::ServerLoadGroundedCheckpoint_Validate() { return true; }
void AOWSPrototypeCharacter::ServerLoadGroundedCheckpoint_Implementation() { LoadGroundedCheckpointAuthority(); }
bool AOWSPrototypeCharacter::ServerAdjustPrototypeScalar_Validate(const float Delta)
{
	return FMath::IsFinite(Delta) && FMath::Abs(Delta) <= 10.0f;
}
void AOWSPrototypeCharacter::ServerAdjustPrototypeScalar_Implementation(const float Delta)
{
	AdjustPrototypeScalarAuthority(Delta);
}

void AOWSPrototypeCharacter::SaveGroundedCheckpointAuthority()
{
	if (!GetCharacterMovement()->IsMovingOnGround())
	{
		SetPrototypeStatus(TEXT("Checkpoint rejected: the character must be grounded."));
		return;
	}
	USaveManager* Manager = USaveManager::Get(this);
	if (!Manager || Manager->IsSavingOrLoading())
	{
		SetPrototypeStatus(TEXT("Checkpoint rejected: Save Extension is unavailable or busy."));
		return;
	}
	GroundedCheckpoint = GetActorTransform();
	bHasGroundedCheckpoint = true;
	SetPrototypeStatus(Manager->SaveSlot(PrototypeSaveSlot, true, false)
		? TEXT("Saving grounded checkpoint...")
		: TEXT("Save Extension rejected the save request."));
}

void AOWSPrototypeCharacter::LoadGroundedCheckpointAuthority()
{
	USaveManager* Manager = USaveManager::Get(this);
	if (!Manager || Manager->IsSavingOrLoading() || !Manager->IsSlotSaved(PrototypeSaveSlot))
	{
		SetPrototypeStatus(TEXT("No loadable OWS prototype checkpoint is available."));
		return;
	}
	SetPrototypeStatus(Manager->LoadSlot(PrototypeSaveSlot)
		? TEXT("Loading grounded checkpoint...")
		: TEXT("Save Extension rejected the load request."));
}

void AOWSPrototypeCharacter::AdjustPrototypeScalarAuthority(const float Delta)
{
	PrototypeScalar = FMath::Clamp(PrototypeScalar + Delta, -1000.0f, 1000.0f);
	SetPrototypeStatus(FString::Printf(TEXT("Prototype scalar: %.1f (PageUp/PageDown)"), PrototypeScalar));
}

void AOWSPrototypeCharacter::OnSaveFinished(const FSELevelFilter& Filter, const bool bError)
{
	USaveManager* Manager = USaveManager::Get(this);
	const USaveSlot* ActiveSlot = Manager ? Manager->GetActiveSlot() : nullptr;
	if (!ActiveSlot || ActiveSlot->Name != PrototypeSaveSlot)
	{
		return;
	}

	SetPrototypeStatus(bError ? TEXT("Checkpoint save failed.") : TEXT("Grounded checkpoint saved."));
}

void AOWSPrototypeCharacter::OnLoadFinished(const FSELevelFilter& Filter, const bool bError)
{
	USaveManager* Manager = USaveManager::Get(this);
	const USaveSlot* ActiveSlot = Manager ? Manager->GetActiveSlot() : nullptr;
	if (!ActiveSlot || ActiveSlot->Name != PrototypeSaveSlot)
	{
		return;
	}

	if (bError)
	{
		SetPrototypeStatus(TEXT("Checkpoint load failed."));
		return;
	}
	if (HasAuthority() && bHasGroundedCheckpoint)
	{
		GetCharacterMovement()->StopMovementImmediately();
		SetActorTransform(GroundedCheckpoint, false, nullptr, ETeleportType::TeleportPhysics);
		ForceNetUpdate();
	}
	SetPrototypeStatus(TEXT("Grounded checkpoint and prototype scalar loaded."));
}

void AOWSPrototypeCharacter::SetPrototypeStatus(const FString& NewStatus)
{
	PrototypeStatus = NewStatus;
	UE_LOG(LogOWSCore, Log, TEXT("%s"), *PrototypeStatus);
	ForceNetUpdate();
}

void AOWSPrototypeCharacter::SetVehicleSeatedPresentation(
	const bool bSeated)
{
	if (!HasAuthority())
	{
		return;
	}

	bVehicleSeatedPresentation = bSeated;
	ApplyVehicleSeatedPresentation();
	ForceNetUpdate();
}

void AOWSPrototypeCharacter::OnRep_VehicleSeatedPresentation()
{
	ApplyVehicleSeatedPresentation();
}

void AOWSPrototypeCharacter::ApplyVehicleSeatedPresentation()
{
	// Attachment and actor hidden state have their own replication paths. This
	// closes AActor's non-replicated collision-enable gap for seated occupants.
	SetActorEnableCollision(!bVehicleSeatedPresentation);
}


void AOWSPrototypeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, GroundedCheckpoint);
	DOREPLIFETIME(ThisClass, bHasGroundedCheckpoint);
	DOREPLIFETIME(ThisClass, PrototypeScalar);
	DOREPLIFETIME(ThisClass, PrototypeStatus);
	DOREPLIFETIME(ThisClass, bVehicleSeatedPresentation);
}
