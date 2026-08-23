#pragma once

#include "Components/ActorComponent.h"
#include "OWSControllerHotbarComponent.generated.h"

class APlayerController;
class APawn;
class UInputComponent;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOWSHotbarLayerChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOWSCancelRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOWSCancelReleased);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOWSPrimaryRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOWSPrimaryReleased);

/** The eight physical controls exposed by one cross-hotbar layer. */
UENUM(BlueprintType)
enum class EOWSHotbarInput : uint8
{
	DPadUp,
	DPadRight,
	DPadDown,
	DPadLeft,
	FaceTop,
	FaceRight,
	FaceBottom,
	FaceLeft
};

/** The four ordered shoulder-modifier layers. */
UENUM(BlueprintType)
enum class EOWSHotbarLayer : uint8
{
	None,
	LeftShoulder,
	RightShoulder,
	LeftThenRight,
	RightThenLeft
};

UENUM(BlueprintType)
enum class EOWSHotbarPresentation : uint8
{
	Hidden,
	DeveloperOnly,
	PlayerFacing
};

UENUM(BlueprintType)
enum class EOWSHotbarCellShape : uint8
{
	Square,
	Circle
};

/** A generic address into the 32-slot cross hotbar. No inventory assumption is made. */
USTRUCT(BlueprintType)
struct OWSCORE_API FOWSHotbarSlotAddress
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Hotbar")
	EOWSHotbarLayer Layer = EOWSHotbarLayer::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Hotbar")
	EOWSHotbarInput Input = EOWSHotbarInput::DPadUp;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOWSHotbarSlotPressed, FOWSHotbarSlotAddress, Slot);

/**
 * Persistent OWS controller component. It owns modifier state and routes the
 * 32 generic hotbar slots, while normal movement actions remain on the pawn.
 */
UCLASS(ClassGroup=(OWS), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="OWS Cross Hotbar Controller"))
class OWSCORE_API UOWSControllerHotbarComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UOWSControllerHotbarComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Fired for every L1/R1-layered D-pad or face-button press. */
	UPROPERTY(BlueprintAssignable, Category="OWS|Hotbar")
	FOnOWSHotbarLayerChanged OnHotbarLayerChanged;

	UPROPERTY(BlueprintAssignable, Category="OWS|Hotbar")
	FOnOWSHotbarSlotPressed OnHotbarSlotPressed;

	/** UI visibility is a presentation choice; the input routing always remains available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Hotbar|Presentation")
	EOWSHotbarPresentation Presentation = EOWSHotbarPresentation::DeveloperOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Hotbar|Presentation")
	EOWSHotbarCellShape CellShape = EOWSHotbarCellShape::Square;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Hotbar|Presentation", meta=(ClampMin="12.0"))
	float CellSize = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Hotbar|Presentation")
	FVector2D ScreenPosition = FVector2D(0.5f, 0.78f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Roster")
	bool bEnableRosterCycling = true;

	/** Placed pawns with this tag form the level roster. They are sorted by name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OWS|Roster")
	FName RosterActorTag = TEXT("OWS.TestCharacter");

	UFUNCTION(BlueprintPure, Category="OWS|Hotbar")
	EOWSHotbarLayer GetActiveLayer() const;

	/** Menu/UI code can set this while it has a cancelable state. */
	UFUNCTION(BlueprintCallable, Category="OWS|Input")
	void SetCancelContextActive(bool bActive);

	UFUNCTION(BlueprintPure, Category="OWS|Input")
	bool IsCancelContextActive() const { return bCancelContextActive; }

	UPROPERTY(BlueprintAssignable, Category="OWS|Input")
	FOnOWSCancelRequested OnCancelRequested;

	UPROPERTY(BlueprintAssignable, Category="OWS|Input")
	FOnOWSCancelReleased OnCancelReleased;

	/** Generic R2 primary-hand/unarmed action seam. Gameplay assigns its behavior. */
	UPROPERTY(BlueprintAssignable, Category="OWS|Input")
	FOnOWSPrimaryRequested OnPrimaryRequested;

	UPROPERTY(BlueprintAssignable, Category="OWS|Input")
	FOnOWSPrimaryReleased OnPrimaryReleased;

private:
	void InstallInputBindings();
	void RemoveInputBindings();
	void RefreshLayer();
	void RefreshPresentation();
	void UpdateRoutedInputConsumption();
	void ApplyOwsInputContext();

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);
	void Route(EOWSHotbarInput Input);
	void CycleRoster(int32 Direction);

	void OnLeftShoulderPressed();
	void OnLeftShoulderReleased();
	void OnRightShoulderPressed();
	void OnRightShoulderReleased();
	void OnDPadUpPressed();
	void OnDPadRightPressed();
	void OnDPadDownPressed();
	void OnDPadLeftPressed();
	void OnFaceTopPressed();
	void OnFaceRightPressed();
	void OnFaceRightReleased();
	void OnFaceBottomPressed();
	void OnFaceLeftPressed();
	void OnKeyboardCancelPressed();
	void OnKeyboardCancelReleased();
	void OnRightTriggerPressed();
	void OnRightTriggerReleased();

	TObjectPtr<UInputComponent> HotbarInputComponent;
	TObjectPtr<UUserWidget> HotbarWidget;
	bool bLeftShoulderHeld = false;
	bool bRightShoulderHeld = false;
	bool bCancelContextActive = false;
	bool bCancelPressedInContext = false;
	EOWSHotbarLayer FirstHeldShoulder = EOWSHotbarLayer::None;
	EOWSHotbarLayer ActiveLayer = EOWSHotbarLayer::None;
};
