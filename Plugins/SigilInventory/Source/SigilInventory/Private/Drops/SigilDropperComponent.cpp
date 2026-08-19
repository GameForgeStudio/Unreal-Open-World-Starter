// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Drops/SigilDropperComponent.h"
#include "Engine/World.h"
#include "SigilInventoryLogChannels.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilDropperComponent)


USigilDropperComponent::USigilDropperComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USigilDropperComponent::Drop()
{
}


AActor* USigilDropperComponent::CreatePickupActorInstance_Implementation()
{
	UWorld* World = GetWorld();
	check(World);
	if (PickupActorClass.IsNull())
	{
		SIGIL_INVENTORY_CLOG(Error, "missing PickupActorClass!");
		return nullptr;
	}
	UClass* PickupClass = PickupActorClass.LoadSynchronous();
	if (PickupClass == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "failed to load PickupActorClass!");
		return nullptr;
	}

	FVector Origin = CalcDropOrigin();
	AActor* Pickup = World->SpawnActor<AActor>(PickupClass, FTransform(Origin + CalcDropOffset()));
	if (Pickup == nullptr)
	{
		SIGIL_INVENTORY_CLOG(Error, "failed to spawn pickup actor from PickupActorClass(%s)!", *PickupClass->GetName());
		return nullptr;
	}
	return Pickup;
}

FVector USigilDropperComponent::CalcDropOrigin_Implementation() const
{
	if (IsValid(DropTransform))
	{
		return DropTransform->GetActorLocation();
	}

	FVector OriginLocation = GetOwner()->GetActorLocation();

	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		OriginLocation.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	else if (const UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(GetOwner()->GetRootComponent()))
	{
		OriginLocation.Z -= CapsuleComponent->GetScaledCapsuleHalfHeight();
	}
	return OriginLocation;
}

FVector USigilDropperComponent::CalcDropOffset_Implementation() const
{
	const float RandomX = UKismetMathLibrary::RandomFloatInRange(-DropRadius, DropRadius);
	const float RandomY = UKismetMathLibrary::RandomFloatInRange(-DropRadius, DropRadius);

	return FVector(RandomX, RandomY, 0);
}
