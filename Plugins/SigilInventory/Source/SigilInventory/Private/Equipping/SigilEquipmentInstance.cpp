// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "SigilEquipmentInstance.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#endif // UE_WITH_IRIS
#include "SigilEquipmentSystemComponent.h"
#include "Items/SigilItemInstance.h"
#include "SigilItemFragment_Equippable.h"
#include "SigilInventoryLogChannels.h"
#include "Pickups/SigilWorldItemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilEquipmentInstance)

class FLifetimeProperty;
class UClass;
class USceneComponent;

USigilEquipmentInstance::USigilEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsActive = false;
}

bool USigilEquipmentInstance::IsSupportedForNetworking() const
{
	return true;
}

UWorld* USigilEquipmentInstance::GetWorld() const
{
	if (OwningPawn)
	{
		return OwningPawn->GetWorld();
	}
	return nullptr;
}

void USigilEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// DOREPLIFETIME(ThisClass, SourceItem);

	//fix: https://forums.unrealengine.com/t/subobject-replication-for-blueprint-child-class/106205/4
	UBlueprintGeneratedClass* bpClass = Cast<UBlueprintGeneratedClass>(this->GetClass());
	if (bpClass != nullptr)
	{
		bpClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
	DOREPLIFETIME(ThisClass, EquipmentActors);
}

void USigilEquipmentInstance::ReceiveOwningPawn_Implementation(APawn* NewPawn)
{
	OwningPawn = NewPawn;
}

APawn* USigilEquipmentInstance::GetOwningPawn_Implementation() const
{
	return OwningPawn;
}

void USigilEquipmentInstance::ReceiveSourceItem_Implementation(USigilItemInstance* NewItem)
{
	SourceItem = NewItem;
}

USigilItemInstance* USigilEquipmentInstance::GetSourceItem_Implementation() const
{
	return SourceItem;
}

void USigilEquipmentInstance::OnEquipmentBeginPlay_Implementation()
{
	if (OwningPawn->HasAuthority())
	{
		SpawnAndSetupEquipmentActors(SourceItem->FindFragmentByClass<USigilItemFragment_Equippable>()->ActorsToSpawn);
	}
}

void USigilEquipmentInstance::OnEquipmentEndPlay_Implementation()
{
	DestroyEquipmentActors();
}

#if UE_WITH_IRIS
void USigilEquipmentInstance::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	using namespace UE::Net;

	// Build descriptors and allocate PropertyReplicationFragments for this object
	FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}


#endif // UE_WITH_IRIS

bool USigilEquipmentInstance::IsEquipmentActive_Implementation() const
{
	return bIsActive;
}

void USigilEquipmentInstance::OnActiveStateChanged_Implementation(bool NewActiveState)
{
	bIsActive = NewActiveState;
	SetupActiveStateForEquipmentActors(EquipmentActors);
	OnActiveStateChangedEvent.Broadcast(NewActiveState);
}

APawn* USigilEquipmentInstance::GetTypedOwningPawn(TSubclassOf<APawn> PawnType) const
{
	APawn* Result = nullptr;
	if (UClass* ActualPawnType = PawnType)
	{
		if (APawn* Pawn = Execute_GetOwningPawn(this))
		{
			if (Pawn->IsA(ActualPawnType))
			{
				Result = Pawn;
			}
		}
	}
	return Result;
}

bool USigilEquipmentInstance::CanActivate_Implementation() const
{
	return true;
}

AActor* USigilEquipmentInstance::GetTypedEquipmentActor(TSubclassOf<AActor> DesiredClass) const
{
	if (UClass* RealClass = DesiredClass)
	{
		for (const TObjectPtr<AActor>& SpawnedActor : EquipmentActors)
		{
			if (SpawnedActor && SpawnedActor->GetClass()->IsChildOf(RealClass))
			{
				return SpawnedActor;
			}
		}
	}
	return nullptr;
}

void USigilEquipmentInstance::SpawnAndSetupEquipmentActors(const TArray<FSigilEquipmentActorToSpawn>& ActorsToSpawn)
{
	if (OwningPawn == nullptr || !OwningPawn->HasAuthority())
	{
		return;
	}

	USceneComponent* AttachParent = GetAttachParentForSpawnedActors(OwningPawn);

	for (int32 i = 0; i < ActorsToSpawn.Num(); ++i)
	{
		const FSigilEquipmentActorToSpawn& SpawnInfo = ActorsToSpawn[i];
		if (SpawnInfo.ActorToSpawn.IsNull())
		{
			continue;
		}
		AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(SpawnInfo.ActorToSpawn.LoadSynchronous(), FTransform::Identity, OwningPawn);
		if (NewActor == nullptr)
		{
			SIGIL_INVENTORY_LOG(Warning, "%s Failed to spawn actor of class: %s at index: %d ", *GetName(), *SpawnInfo.ActorToSpawn->GetName(), i);
			continue;
		}
		BeforeSpawningActor(NewActor);
		NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		if (SpawnInfo.bShouldAttach)
		{
			NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
			NewActor->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);
		}
		EquipmentActors.Add(NewActor);
	}
	SetupEquipmentActors(EquipmentActors);
}

void USigilEquipmentInstance::DestroyEquipmentActors()
{
	if (OwningPawn == nullptr || !OwningPawn->HasAuthority())
	{
		return;
	}
	for (AActor* Actor : EquipmentActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		if (Actor->GetClass()->ImplementsInterface(USigilEquipmentInterface::StaticClass()))
		{
			if (Execute_IsEquipmentActive(Actor))
			{
				Execute_OnActiveStateChanged(Actor, false);
			}
			Execute_OnEquipmentEndPlay(Actor);
			// may be destroyed during unequip.
			if (IsValid(Actor))
			{
				Execute_ReceiveOwningPawn(Actor, nullptr);
				Execute_ReceiveSourceItem(Actor, nullptr);
				Actor->Destroy();
			}
		}
		else
		{
			Actor->Destroy();
		}
	}
	EquipmentActors.Empty();
}

USceneComponent* USigilEquipmentInstance::GetAttachParentForSpawnedActors_Implementation(APawn* Pawn) const
{
	if (ACharacter* Char = Cast<ACharacter>(Pawn))
	{
		return Char->GetMesh();
	}
	if (Pawn)
	{
		return Pawn->FindComponentByClass<USkeletalMeshComponent>();
	}
	return nullptr;
}

void USigilEquipmentInstance::BeforeSpawningActor_Implementation(AActor* SpawningActor) const
{
}

void USigilEquipmentInstance::SetupEquipmentActors_Implementation(const TArray<AActor*>& InActors) const
{
	if (!OwningPawn)
	{
		return;
	}

	if (OwningPawn->HasAuthority())
	{
		for (int32 i = 0; i < InActors.Num(); i++)
		{
			USigilWorldItemComponent* WorldItem = InActors[i]->FindComponentByClass<USigilWorldItemComponent>();
			if (WorldItem != nullptr)
			{
				WorldItem->SetItemInfo(SourceItem, 1);
			}
		}
	}

	SetupInitialStateForEquipmentActors(EquipmentActors);
}

void USigilEquipmentInstance::OnRep_EquipmentActors()
{
	TryWaitEquipmentActors();
}

void USigilEquipmentInstance::TryWaitEquipmentActors()
{
	if (EquipmentActors.IsEmpty() || WaitEquipmentActorsTimer.IsValid())
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WaitEquipmentActorsTimer, FTimerDelegate::CreateUObject(this, &ThisClass::WaitEquipmentActors), 0.2f, true);
	}
}

void USigilEquipmentInstance::WaitEquipmentActors()
{
	if (SourceItem == nullptr || OwningPawn == nullptr || !WaitEquipmentActorsTimer.IsValid())
	{
		return;
	}
	if (OwningPawn->HasAuthority())
	{
		WaitEquipmentActorsTimer.Invalidate();
		return;
	}
	const USigilItemFragment_Equippable* Equippable = SourceItem->FindFragmentByClass<USigilItemFragment_Equippable>();
	if (Equippable == nullptr || Equippable->bActorBased || Equippable->ActorsToSpawn.IsEmpty())
	{
		WaitEquipmentActorsTimer.Invalidate();
		return;
	}

	if (!IsEquipmentActorsValid(Equippable->ActorsToSpawn.Num()))
	{
		return;
	}

	WaitEquipmentActorsTimer.Invalidate();
	SetupInitialStateForEquipmentActors(EquipmentActors);
	SIGIL_INVENTORY_CLOG(VeryVerbose, "Equipment Actors synced.")
}

bool USigilEquipmentInstance::IsEquipmentActorsValid(int32 Num) const
{
	if (EquipmentActors.Num() == Num)
	{
		bool bValid = true;
		for (int32 i = 0; i < EquipmentActors.Num(); i++)
		{
			if (!IsValid(EquipmentActors[i]))
			{
				bValid = false;
				break;
			}
		}
		return bValid;
	}
	return false;
}

void USigilEquipmentInstance::SetupInitialStateForEquipmentActors(const TArray<AActor*>& InActors) const
{
	for (int32 i = 0; i < InActors.Num(); i++)
	{
		AActor* Actor = InActors[i];

		if (Actor && Actor->GetClass()->ImplementsInterface(USigilEquipmentInterface::StaticClass()))
		{
			Execute_ReceiveOwningPawn(Actor, OwningPawn);
			Execute_ReceiveSourceItem(Actor, SourceItem);
			Execute_OnEquipmentBeginPlay(Actor);
			Execute_OnActiveStateChanged(Actor, bIsActive);
		}
	}
}

void USigilEquipmentInstance::SetupActiveStateForEquipmentActors(const TArray<AActor*>& InActors) const
{
	for (int32 i = 0; i < InActors.Num(); i++)
	{
		AActor* Actor = InActors[i];
		if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(USigilEquipmentInterface::StaticClass()))
		{
			Execute_OnActiveStateChanged(Actor, bIsActive);
		}
	}
}
