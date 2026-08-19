// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Pickups/SigilPickupComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilPickupComponent)

// Sets default values for this component's properties
USigilPickupComponent::USigilPickupComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool USigilPickupComponent::Pickup(USigilInventorySystemComponent* Picker)
{
	return true;
}

void USigilPickupComponent::NotifyPickupSuccess()
{
	OnPickupSuccess.Broadcast();
}

void USigilPickupComponent::NotifyPickupFailed()
{
	OnPickupFail.Broadcast();
}
