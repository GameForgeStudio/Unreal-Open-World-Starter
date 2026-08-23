#include "OWSInteractionTargetComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"

UOWSInteractionTargetComponent::UOWSInteractionTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
	bHiddenInGame = true;
}

FText UOWSInteractionTargetComponent::GetInteractionDisplayName() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}
	const AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor
		? OwnerActor->GetActorNameOrLabel()
		: TEXT("Interaction");
	return InteractionId.IsNone()
		? FText::FromString(OwnerName)
		: FText::FromString(FString::Printf(
			TEXT("%s - %s"), *OwnerName, *InteractionId.ToString()));
}

UObject* UOWSInteractionTargetComponent::FindBehaviorHandler() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}
	if (OwnerActor->GetClass()->ImplementsInterface(UOWSInteractionTargetHandler::StaticClass()))
	{
		return OwnerActor;
	}
	TArray<UActorComponent*> Components;
	OwnerActor->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component != this &&
			Component->GetClass()->ImplementsInterface(UOWSInteractionTargetHandler::StaticClass()))
		{
			return Component;
		}
	}
	return nullptr;
}

bool UOWSInteractionTargetComponent::CanActivate(
	AController* Activator,
	FText& OutFailureReason) const
{
	OutFailureReason = FText::GetEmpty();
	if (!bInteractionEnabled)
	{
		OutFailureReason = NSLOCTEXT(
			"OWSInteraction", "Disabled", "This interaction is unavailable.");
		return false;
	}
	if (UObject* Handler = FindBehaviorHandler())
	{
		return IOWSInteractionTargetHandler::Execute_CanActivateOWSTarget(
			Handler, const_cast<UOWSInteractionTargetComponent*>(this), Activator,
			OutFailureReason);
	}
	return true;
}

bool UOWSInteractionTargetComponent::TryActivate(
	AController* Activator,
	FText& OutFailureReason)
{
	if (!CanActivate(Activator, OutFailureReason))
	{
		return false;
	}
	if (UObject* Handler = FindBehaviorHandler())
	{
		if (!IOWSInteractionTargetHandler::Execute_ActivateOWSTarget(
			Handler, this, Activator, OutFailureReason))
		{
			return false;
		}
	}
	OnActivated.Broadcast(Activator, this);
	return true;
}
