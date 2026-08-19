#include "OWSMoverSaveRestoreComponent.h"

#include "ClassFilter.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "LevelFilter.h"
#include "MoverComponent.h"
#include "MoverSimulationTypes.h"
#include "SaveManager.h"
#include "SaveSlot.h"

DEFINE_LOG_CATEGORY_STATIC(LogOWSMovement, Log, All);

namespace
{
	bool ContainsLoadedClass(
		const TSet<TSoftClassPtr<UObject>>& Classes,
		const UClass* Candidate)
	{
		for (const TSoftClassPtr<UObject>& Class : Classes)
		{
			if (Class.Get() == Candidate)
			{
				return true;
			}
		}

		return false;
	}

	bool IsClassAllowedBySerializedFilter(
		const FSEClassFilter& Filter,
		const UClass* Candidate)
	{
		if (!Candidate || Filter.AllowedClasses.IsEmpty())
		{
			return false;
		}

		// This is the per-class equivalent of Save Extension 1.5.6's bake:
		// the nearest explicit rule in the inheritance chain wins, with an
		// explicit allow winning if the same class appears in both sets.
		for (const UClass* Current = Candidate; Current; Current = Current->GetSuperClass())
		{
			if (ContainsLoadedClass(Filter.AllowedClasses, Current))
			{
				return true;
			}
			if (ContainsLoadedClass(Filter.IgnoredClasses, Current))
			{
				return false;
			}
		}

		return false;
	}
}

UOWSMoverSaveRestoreComponent::UOWSMoverSaveRestoreComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOWSMoverSaveRestoreComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!BindToSaveExtension())
	{
		UE_LOG(LogOWSMovement, Warning,
			TEXT("%s could not bind to Save Extension. Call BindToSaveExtension after the SaveManager is available."),
			*GetNameSafe(this));
	}
}

void UOWSMoverSaveRestoreComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromSaveExtension();
	Super::EndPlay(EndPlayReason);
}

bool UOWSMoverSaveRestoreComponent::BindToSaveExtension()
{
	if (USaveManager* SaveManager = FindSaveManager())
	{
		SaveManager->OnGameLoaded.AddUniqueDynamic(
			this, &UOWSMoverSaveRestoreComponent::HandleSaveExtensionGameLoaded);
		return true;
	}

	return false;
}

void UOWSMoverSaveRestoreComponent::UnbindFromSaveExtension()
{
	if (USaveManager* SaveManager = FindSaveManager())
	{
		SaveManager->OnGameLoaded.RemoveDynamic(
			this, &UOWSMoverSaveRestoreComponent::HandleSaveExtensionGameLoaded);
	}
}

bool UOWSMoverSaveRestoreComponent::CanCaptureGroundedCheckpoint(
	EOWSGroundedMovementResult& OutResult) const
{
	OutResult = ValidateCaptureState();
	return OutResult == EOWSGroundedMovementResult::Success;
}

EOWSGroundedMovementResult UOWSMoverSaveRestoreComponent::CaptureGroundedCheckpoint()
{
	const EOWSGroundedMovementResult ValidationResult = ValidateCaptureState();
	if (ValidationResult != EOWSGroundedMovementResult::Success)
	{
		return ValidationResult;
	}

	GroundedCheckpoint.WorldTransform = GetOwner()->GetActorTransform();
	// FTeleportEffect restores only world location and rotation. Normalize the
	// stored scale so this checkpoint never implies that scale is reconstructed.
	GroundedCheckpoint.WorldTransform.SetScale3D(FVector::OneVector);
	GroundedCheckpoint.bIsValid = true;
	return EOWSGroundedMovementResult::Success;
}

EOWSGroundedMovementResult UOWSMoverSaveRestoreComponent::RequestGroundedSave(FName SlotName)
{
	if (SlotName.IsNone())
	{
		return EOWSGroundedMovementResult::InvalidSlotName;
	}

	USaveManager* SaveManager = FindSaveManager();
	if (!SaveManager)
	{
		return EOWSGroundedMovementResult::SaveManagerUnavailable;
	}

	const EOWSGroundedMovementResult SlotResult =
		ValidateSlotPersistsComponent(SaveManager->GetActiveSlot());
	if (SlotResult != EOWSGroundedMovementResult::Success)
	{
		return SlotResult;
	}

	const EOWSGroundedMovementResult CaptureResult = CaptureGroundedCheckpoint();
	if (CaptureResult != EOWSGroundedMovementResult::Success)
	{
		return CaptureResult;
	}

	return SaveManager->SaveSlot(SlotName, true, false)
		? EOWSGroundedMovementResult::Success
		: EOWSGroundedMovementResult::RequestRejected;
}

EOWSGroundedMovementResult UOWSMoverSaveRestoreComponent::RequestGroundedLoad(FName SlotName)
{
	if (SlotName.IsNone())
	{
		return EOWSGroundedMovementResult::InvalidSlotName;
	}

	const EOWSGroundedMovementResult EnvironmentResult = ValidateRestoreEnvironment();
	if (EnvironmentResult != EOWSGroundedMovementResult::Success)
	{
		return EnvironmentResult;
	}

	USaveManager* SaveManager = FindSaveManager();
	if (!SaveManager)
	{
		return EOWSGroundedMovementResult::SaveManagerUnavailable;
	}

	BindToSaveExtension();

	// A load that does not deserialize this component must fail closed instead
	// of accidentally reusing a checkpoint left in memory by another slot.
	const FOWSGroundedMovementCheckpoint PreviousCheckpoint = GroundedCheckpoint;
	GroundedCheckpoint.Reset();

	if (!SaveManager->LoadSlot(SlotName))
	{
		GroundedCheckpoint = PreviousCheckpoint;
		return EOWSGroundedMovementResult::RequestRejected;
	}

	return EOWSGroundedMovementResult::Success;
}

EOWSGroundedMovementResult UOWSMoverSaveRestoreComponent::RestoreGroundedCheckpoint()
{
	const EOWSGroundedMovementResult ValidationResult = ValidateRestoreState();
	if (ValidationResult != EOWSGroundedMovementResult::Success)
	{
		return ValidationResult;
	}

	UMoverComponent* MoverComponent = FindMoverComponent();
	check(MoverComponent);

	TSharedPtr<FTeleportEffect> TeleportEffect = MakeShared<FTeleportEffect>();
	TeleportEffect->TargetLocation = GroundedCheckpoint.WorldTransform.GetLocation();
	TeleportEffect->bUseActorRotation = false;
	TeleportEffect->TargetRotation = GroundedCheckpoint.WorldTransform.Rotator();
	MoverComponent->QueueInstantMovementEffect(TeleportEffect);

	TSharedPtr<FApplyVelocityEffect> StopInWalkingEffect = MakeShared<FApplyVelocityEffect>();
	StopInWalkingEffect->VelocityToApply = FVector::ZeroVector;
	StopInWalkingEffect->bAdditiveVelocity = false;
	StopInWalkingEffect->ForceMovementMode = DefaultModeNames::Walking;
	MoverComponent->QueueInstantMovementEffect(StopInWalkingEffect);

	return EOWSGroundedMovementResult::Success;
}

void UOWSMoverSaveRestoreComponent::HandleSaveExtensionGameLoaded(USaveSlot* LoadedSlot)
{
	const FName LoadedSlotName = LoadedSlot ? LoadedSlot->Name : NAME_None;

	LastAutomaticRestoreResult = ValidateSlotPersistsComponent(LoadedSlot);
	if (LastAutomaticRestoreResult == EOWSGroundedMovementResult::Success)
	{
		LastAutomaticRestoreResult = RestoreGroundedCheckpoint();
	}

	if (LastAutomaticRestoreResult != EOWSGroundedMovementResult::Success)
	{
		UE_LOG(LogOWSMovement, Warning,
			TEXT("Grounded Mover restore for slot '%s' was rejected with result %d."),
			*LoadedSlotName.ToString(), static_cast<int32>(LastAutomaticRestoreResult));
	}

	OnGroundedRestoreEvaluated.Broadcast(LoadedSlotName, LastAutomaticRestoreResult);
}

UMoverComponent* UOWSMoverSaveRestoreComponent::FindMoverComponent() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UMoverComponent>() : nullptr;
}

USaveManager* UOWSMoverSaveRestoreComponent::FindSaveManager() const
{
	return USaveManager::Get(this);
}

EOWSGroundedMovementResult UOWSMoverSaveRestoreComponent::ValidateCaptureState() const
{
	using namespace UE::OWSMovement;

	const AActor* Owner = GetOwner();
	UMoverComponent* MoverComponent = FindMoverComponent();

	bool bHasLayeredMoves = false;
	bool bHasMovementModifiers = false;
	if (MoverComponent)
	{
		HasTransientMoverState(*MoverComponent, bHasLayeredMoves, bHasMovementModifiers);
	}

	FGroundedCaptureValidationInput Input;
	Input.bHasOwner = Owner != nullptr;
	Input.bHasMover = MoverComponent != nullptr;
	Input.bUsesAsyncBackend = MoverComponent && MoverComponent->IsBackendAsync();
	Input.bHasWalkingMode = MoverComponent &&
		MoverComponent->FindMovementModeByName(DefaultModeNames::Walking) != nullptr;
	Input.bIsWalking = MoverComponent &&
		MoverComponent->GetMovementModeName() == DefaultModeNames::Walking;
	Input.bNextModeIsWalking = MoverComponent &&
		MoverComponent->GetNextMovementModeName() == DefaultModeNames::Walking;
	Input.bHasLayeredMoves = bHasLayeredMoves;
	Input.bHasMovementModifiers = bHasMovementModifiers;
	Input.bTransformIsValid = Owner && IsCheckpointTransformValid(Owner->GetActorTransform());

	return ValidateGroundedCapture(Input);
}

EOWSGroundedMovementResult UOWSMoverSaveRestoreComponent::ValidateRestoreState() const
{
	using namespace UE::OWSMovement;

	const AActor* Owner = GetOwner();
	UMoverComponent* MoverComponent = FindMoverComponent();

	bool bHasLayeredMoves = false;
	bool bHasMovementModifiers = false;
	if (MoverComponent)
	{
		HasTransientMoverState(*MoverComponent, bHasLayeredMoves, bHasMovementModifiers);
	}

	FGroundedRestoreValidationInput Input;
	Input.bHasOwner = Owner != nullptr;
	Input.bHasMover = MoverComponent != nullptr;
	Input.bUsesAsyncBackend = MoverComponent && MoverComponent->IsBackendAsync();
	Input.bHasWalkingMode = MoverComponent &&
		MoverComponent->FindMovementModeByName(DefaultModeNames::Walking) != nullptr;
	Input.bHasPendingMovementModeChange = MoverComponent &&
		MoverComponent->GetNextMovementModeName() != MoverComponent->GetMovementModeName();
	Input.bHasLayeredMoves = bHasLayeredMoves;
	Input.bHasMovementModifiers = bHasMovementModifiers;
	Input.bHasCheckpoint = GroundedCheckpoint.bIsValid;
	Input.bCheckpointTransformIsValid =
		IsCheckpointTransformValid(GroundedCheckpoint.WorldTransform);

	return ValidateGroundedRestore(Input);
}

EOWSGroundedMovementResult UOWSMoverSaveRestoreComponent::ValidateRestoreEnvironment() const
{
	using namespace UE::OWSMovement;

	const AActor* Owner = GetOwner();
	UMoverComponent* MoverComponent = FindMoverComponent();

	bool bHasLayeredMoves = false;
	bool bHasMovementModifiers = false;
	if (MoverComponent)
	{
		HasTransientMoverState(*MoverComponent, bHasLayeredMoves, bHasMovementModifiers);
	}

	FGroundedRestoreValidationInput Input;
	Input.bHasOwner = Owner != nullptr;
	Input.bHasMover = MoverComponent != nullptr;
	Input.bUsesAsyncBackend = MoverComponent && MoverComponent->IsBackendAsync();
	Input.bHasWalkingMode = MoverComponent &&
		MoverComponent->FindMovementModeByName(DefaultModeNames::Walking) != nullptr;
	Input.bHasPendingMovementModeChange = MoverComponent &&
		MoverComponent->GetNextMovementModeName() != MoverComponent->GetMovementModeName();
	Input.bHasLayeredMoves = bHasLayeredMoves;
	Input.bHasMovementModifiers = bHasMovementModifiers;
	Input.bHasCheckpoint = true;
	Input.bCheckpointTransformIsValid = true;

	return ValidateGroundedRestore(Input);
}

EOWSGroundedMovementResult UOWSMoverSaveRestoreComponent::ValidateSlotPersistsComponent(
	USaveSlot* Slot) const
{
	if (!Slot)
	{
		return EOWSGroundedMovementResult::ComponentNotIncludedInSaveSlot;
	}

	if (!IsOwnerPersistedBySlot(*Slot))
	{
		return EOWSGroundedMovementResult::OwnerNotIncludedInSaveSlot;
	}

	return IsClassAllowedBySerializedFilter(Slot->ComponentFilter, GetClass())
		? EOWSGroundedMovementResult::Success
		: EOWSGroundedMovementResult::ComponentNotIncludedInSaveSlot;
}

bool UOWSMoverSaveRestoreComponent::IsOwnerPersistedBySlot(USaveSlot& Slot) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// Save Extension stores player-controlled pawns through GameState::PlayerArray
	// and deliberately skips them during ordinary level-actor serialization.
	if (const APawn* Pawn = Cast<APawn>(Owner); Pawn && Pawn->IsPlayerControlled())
	{
		const APlayerState* PlayerState = Pawn->GetPlayerState();
		const AGameStateBase* GameState = Pawn->GetWorld() ? Pawn->GetWorld()->GetGameState() : nullptr;
		if (!PlayerState || !GameState)
		{
			return false;
		}

		for (const TObjectPtr<APlayerState>& RegisteredPlayer : GameState->PlayerArray)
		{
			if (RegisteredPlayer.Get() == PlayerState)
			{
				return true;
			}
		}

		return false;
	}

	// Match the installed Save Extension save task, including its current use
	// of bIsLoading=true while preparing the filter for a save operation.
	FSELevelFilter LevelFilter;
	Slot.GetLevelFilter(true, LevelFilter);
	return IsClassAllowedBySerializedFilter(LevelFilter.ActorFilter, Owner->GetClass());
}

bool UOWSMoverSaveRestoreComponent::HasTransientMoverState(
	const UMoverComponent& MoverComponent,
	bool& bOutHasLayeredMoves,
	bool& bOutHasMovementModifiers) const
{
	const FMoverSyncState& SyncState = MoverComponent.GetSyncState();
	bOutHasLayeredMoves = SyncState.LayeredMoves.HasAnyMoves() ||
		SyncState.LayeredMoveInstances.HasAnyMoves();
	bOutHasMovementModifiers = SyncState.MovementModifiers.HasAnyMoves();
	return bOutHasLayeredMoves || bOutHasMovementModifiers;
}
