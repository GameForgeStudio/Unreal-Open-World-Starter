#include "OWSControllerHotbarComponent.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "OWSHotbarWidget.h"
#include "TimerManager.h"

UOWSControllerHotbarComponent::UOWSControllerHotbarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOWSControllerHotbarComponent::BeginPlay()
{
	Super::BeginPlay();
	InstallInputBindings();
	RefreshPresentation();
	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwner()))
	{
		PlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	ApplyOwsInputContext();
}

void UOWSControllerHotbarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwner()))
	{
		PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	RemoveInputBindings();
	if (HotbarWidget)
	{
		HotbarWidget->RemoveFromParent();
		HotbarWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UOWSControllerHotbarComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &ThisClass::ApplyOwsInputContext));
	}
}

void UOWSControllerHotbarComponent::ApplyOwsInputContext()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;
	if (!InputSubsystem) return;

	UInputMappingContext* GaspalsContext = LoadObject<UInputMappingContext>(
		nullptr, TEXT("/GASPALS/Input/IMC_Sandbox.IMC_Sandbox"));
	UInputMappingContext* OwsContext = LoadObject<UInputMappingContext>(
		nullptr, TEXT("/Game/OWS/Input/IMC_OWSCharacter.IMC_OWSCharacter"));
	if (!OwsContext) return;

	// GASPALS is retained as the locomotion/traversal implementation. OWS owns
	// the complete input context, including the unchanged keyboard bindings.
	if (GaspalsContext) InputSubsystem->RemoveMappingContext(GaspalsContext);
	InputSubsystem->RemoveMappingContext(OwsContext);
	InputSubsystem->AddMappingContext(OwsContext, 100);
}

EOWSHotbarLayer UOWSControllerHotbarComponent::GetActiveLayer() const
{
	return ActiveLayer;
}

void UOWSControllerHotbarComponent::SetCancelContextActive(const bool bActive)
{
	bCancelContextActive = bActive;
	UpdateRoutedInputConsumption();
}

void UOWSControllerHotbarComponent::InstallInputBindings()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController() || HotbarInputComponent)
	{
		return;
	}

	HotbarInputComponent = NewObject<UInputComponent>(PlayerController, TEXT("OWSHotbarInput"));
	// This controller-level router must see modifier and roster keys before the
	// possessed pawn's legacy demo bindings.
	HotbarInputComponent->Priority = 1000;
	HotbarInputComponent->RegisterComponent();
	HotbarInputComponent->BindKey(EKeys::Gamepad_LeftShoulder, IE_Pressed, this, &ThisClass::OnLeftShoulderPressed).bConsumeInput = true;
	HotbarInputComponent->BindKey(EKeys::Gamepad_LeftShoulder, IE_Released, this, &ThisClass::OnLeftShoulderReleased).bConsumeInput = true;
	HotbarInputComponent->BindKey(EKeys::Gamepad_RightShoulder, IE_Pressed, this, &ThisClass::OnRightShoulderPressed).bConsumeInput = true;
	HotbarInputComponent->BindKey(EKeys::Gamepad_RightShoulder, IE_Released, this, &ThisClass::OnRightShoulderReleased).bConsumeInput = true;
	HotbarInputComponent->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &ThisClass::OnDPadUpPressed).bConsumeInput = true;
	HotbarInputComponent->BindKey(EKeys::Gamepad_DPad_Right, IE_Pressed, this, &ThisClass::OnDPadRightPressed).bConsumeInput = true;
	HotbarInputComponent->BindKey(EKeys::Gamepad_DPad_Down, IE_Pressed, this, &ThisClass::OnDPadDownPressed).bConsumeInput = true;
	HotbarInputComponent->BindKey(EKeys::Gamepad_DPad_Left, IE_Pressed, this, &ThisClass::OnDPadLeftPressed).bConsumeInput = true;
	HotbarInputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &ThisClass::OnFaceTopPressed);
	HotbarInputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &ThisClass::OnFaceRightPressed);
	HotbarInputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &ThisClass::OnFaceBottomPressed);
	HotbarInputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &ThisClass::OnFaceLeftPressed);
	HotbarInputComponent->BindKey(EKeys::Gamepad_RightTrigger, IE_Pressed, this, &ThisClass::OnRightTriggerPressed).bConsumeInput = true;
	HotbarInputComponent->BindKey(EKeys::Gamepad_RightTrigger, IE_Released, this, &ThisClass::OnRightTriggerReleased).bConsumeInput = true;
	PlayerController->PushInputComponent(HotbarInputComponent);
	UpdateRoutedInputConsumption();
}

void UOWSControllerHotbarComponent::RemoveInputBindings()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwner()))
	{
		if (HotbarInputComponent)
		{
			PlayerController->PopInputComponent(HotbarInputComponent);
		}
	}
	HotbarInputComponent = nullptr;
}

void UOWSControllerHotbarComponent::RefreshLayer()
{
	if (!bLeftShoulderHeld && !bRightShoulderHeld)
	{
		FirstHeldShoulder = EOWSHotbarLayer::None;
		ActiveLayer = EOWSHotbarLayer::None;
	}
	else if (bLeftShoulderHeld && bRightShoulderHeld)
	{
		ActiveLayer = FirstHeldShoulder == EOWSHotbarLayer::LeftShoulder
			? EOWSHotbarLayer::LeftThenRight
			: EOWSHotbarLayer::RightThenLeft;
	}
	else
	{
		ActiveLayer = bLeftShoulderHeld ? EOWSHotbarLayer::LeftShoulder : EOWSHotbarLayer::RightShoulder;
		FirstHeldShoulder = ActiveLayer;
	}
	UpdateRoutedInputConsumption();
	RefreshPresentation();
	OnHotbarLayerChanged.Broadcast();
}

void UOWSControllerHotbarComponent::RefreshPresentation()
{
	if (Presentation == EOWSHotbarPresentation::Hidden)
	{
		return;
	}
#if UE_BUILD_SHIPPING
	if (Presentation == EOWSHotbarPresentation::DeveloperOnly)
	{
		return;
	}
#endif
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController()) return;
	if (!HotbarWidget)
	{
		UOWSHotbarWidget* Widget = CreateWidget<UOWSHotbarWidget>(PlayerController, UOWSHotbarWidget::StaticClass());
		HotbarWidget = Widget;
		Widget->Configure(CellShape, CellSize, ScreenPosition);
		Widget->AddToViewport(100);
	}
	static_cast<UOWSHotbarWidget*>(HotbarWidget.Get())->SetLayer(ActiveLayer);
}

void UOWSControllerHotbarComponent::UpdateRoutedInputConsumption()
{
	if (!HotbarInputComponent) return;
	const bool bConsumeFaces = ActiveLayer != EOWSHotbarLayer::None;
	for (FInputKeyBinding& Binding : HotbarInputComponent->KeyBindings)
	{
		const FKey Key = Binding.Chord.Key;
		if (Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Gamepad_FaceButton_Right ||
			Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_FaceButton_Left)
		{
			Binding.bConsumeInput = bConsumeFaces || (Key == EKeys::Gamepad_FaceButton_Right && bCancelContextActive);
		}
	}
}

void UOWSControllerHotbarComponent::Route(const EOWSHotbarInput Input)
{
	if (ActiveLayer != EOWSHotbarLayer::None)
	{
		FOWSHotbarSlotAddress Slot;
		Slot.Layer = ActiveLayer;
		Slot.Input = Input;
		OnHotbarSlotPressed.Broadcast(Slot);
		if (UOWSHotbarWidget* Widget = Cast<UOWSHotbarWidget>(HotbarWidget))
		{
			Widget->SetPressedSlot(Input);
		}
	}
}

void UOWSControllerHotbarComponent::CycleRoster(const int32 Direction)
{
	if (!bEnableRosterCycling) return;
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController) return;
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(this, RosterActorTag, Actors);
	TArray<APawn*> Pawns;
	for (AActor* Actor : Actors)
	{
		if (APawn* Pawn = Cast<APawn>(Actor)) Pawns.Add(Pawn);
	}
	Pawns.Sort([](const APawn& A, const APawn& B) { return A.GetName() < B.GetName(); });
	if (Pawns.Num() < 2) return;
	int32 CurrentIndex = Pawns.IndexOfByKey(PlayerController->GetPawn());
	if (CurrentIndex == INDEX_NONE) CurrentIndex = 0;
	const int32 NextIndex = (CurrentIndex + Direction + Pawns.Num()) % Pawns.Num();
	PlayerController->Possess(Pawns[NextIndex]);
}

void UOWSControllerHotbarComponent::OnLeftShoulderPressed()
{
	if (!bLeftShoulderHeld && !bRightShoulderHeld) FirstHeldShoulder = EOWSHotbarLayer::LeftShoulder;
	bLeftShoulderHeld = true;
	RefreshLayer();
}
void UOWSControllerHotbarComponent::OnLeftShoulderReleased() { bLeftShoulderHeld = false; RefreshLayer(); }
void UOWSControllerHotbarComponent::OnRightShoulderPressed()
{
	if (!bLeftShoulderHeld && !bRightShoulderHeld) FirstHeldShoulder = EOWSHotbarLayer::RightShoulder;
	bRightShoulderHeld = true;
	RefreshLayer();
}
void UOWSControllerHotbarComponent::OnRightShoulderReleased() { bRightShoulderHeld = false; RefreshLayer(); }
void UOWSControllerHotbarComponent::OnDPadUpPressed() { Route(EOWSHotbarInput::DPadUp); }
void UOWSControllerHotbarComponent::OnDPadRightPressed()
{
	if (ActiveLayer == EOWSHotbarLayer::None) CycleRoster(1); else Route(EOWSHotbarInput::DPadRight);
}
void UOWSControllerHotbarComponent::OnDPadDownPressed() { Route(EOWSHotbarInput::DPadDown); }
void UOWSControllerHotbarComponent::OnDPadLeftPressed()
{
	if (ActiveLayer == EOWSHotbarLayer::None) CycleRoster(-1); else Route(EOWSHotbarInput::DPadLeft);
}
void UOWSControllerHotbarComponent::OnFaceTopPressed() { Route(EOWSHotbarInput::FaceTop); }
void UOWSControllerHotbarComponent::OnFaceRightPressed()
{
	if (ActiveLayer != EOWSHotbarLayer::None) Route(EOWSHotbarInput::FaceRight);
	else if (bCancelContextActive) OnCancelRequested.Broadcast();
}
void UOWSControllerHotbarComponent::OnFaceBottomPressed() { Route(EOWSHotbarInput::FaceBottom); }
void UOWSControllerHotbarComponent::OnFaceLeftPressed() { Route(EOWSHotbarInput::FaceLeft); }
void UOWSControllerHotbarComponent::OnRightTriggerPressed() { OnPrimaryRequested.Broadcast(); }
void UOWSControllerHotbarComponent::OnRightTriggerReleased() { OnPrimaryReleased.Broadcast(); }
