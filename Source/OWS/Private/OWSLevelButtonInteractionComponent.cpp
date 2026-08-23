#include "OWSLevelButtonInteractionComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

const FName UOWSLevelButtonInteractionComponent::TriggerComponentName(TEXT("Trigger"));
const FName UOWSLevelButtonInteractionComponent::SimulatePressFunctionName(TEXT("SimulatePress"));

UOWSLevelButtonInteractionComponent::UOWSLevelButtonInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOWSLevelButtonInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	OwnerActor->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (Primitive && Primitive->GetFName() == TriggerComponentName)
		{
			Primitive->SetGenerateOverlapEvents(false);
			return;
		}
	}

	UE_LOG(LogTemp, Error,
		TEXT("[OWSInteraction] %s has a LevelButton adapter but no Trigger component."),
		*OwnerActor->GetActorNameOrLabel());
}

bool UOWSLevelButtonInteractionComponent::OwnsTarget(
	const UOWSInteractionTargetComponent* Target) const
{
	return Target && Target->GetOwner() == GetOwner();
}

bool UOWSLevelButtonInteractionComponent::CanActivateOWSTarget_Implementation(
	UOWSInteractionTargetComponent* Target,
	AController*,
	FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnsTarget(Target))
	{
		OutFailureReason = NSLOCTEXT(
			"OWSInteraction", "WrongLevelButtonTarget",
			"This interaction target does not belong to this button.");
		return false;
	}
	if (!OwnerActor->FindFunction(SimulatePressFunctionName))
	{
		OutFailureReason = NSLOCTEXT(
			"OWSInteraction", "MissingSimulatePress",
			"This button does not provide its SimulatePress action.");
		return false;
	}
	return true;
}

bool UOWSLevelButtonInteractionComponent::ActivateOWSTarget_Implementation(
	UOWSInteractionTargetComponent* Target,
	AController* Activator,
	FText& OutFailureReason)
{
	if (!CanActivateOWSTarget_Implementation(Target, Activator, OutFailureReason))
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	UFunction* SimulatePress = OwnerActor->FindFunction(SimulatePressFunctionName);
	OwnerActor->ProcessEvent(SimulatePress, nullptr);
	return true;
}
