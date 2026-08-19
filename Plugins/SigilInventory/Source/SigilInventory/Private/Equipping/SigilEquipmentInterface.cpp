// Copyright (c) 2026 Likeon. All Rights Reserved.

#include "SigilEquipmentInterface.h"
#include "GameFramework/Pawn.h"
#include "SigilEquipmentInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilEquipmentInterface)

void ISigilEquipmentInterface::ReceiveOwningPawn_Implementation(APawn* NewPawn)
{
}

APawn* ISigilEquipmentInterface::GetOwningPawn_Implementation() const
{
	APawn* ReturnPawn = Cast<APawn>(_getUObject()->GetOuter());
	return ReturnPawn;
}

void ISigilEquipmentInterface::ReceiveSourceItem_Implementation(USigilItemInstance* NewItem)
{
}

USigilItemInstance* ISigilEquipmentInterface::GetSourceItem_Implementation() const
{
	return nullptr;
}

void ISigilEquipmentInterface::OnEquipped_Implementation()
{
}

void ISigilEquipmentInterface::OnActiveStateChanged_Implementation(bool bNewActiveState)
{
}

bool ISigilEquipmentInterface::IsEquipmentActive_Implementation() const
{
	return false;
}

void ISigilEquipmentInterface::OnEquipmentBeginPlay_Implementation()
{
}

void ISigilEquipmentInterface::OnEquipmentEndPlay_Implementation()
{
}

void ISigilEquipmentInterface::OnUnequipped_Implementation()
{
}

// Add default functionality here for any ISigilEquipmentInterface functions that are not pure virtual.

bool ISigilEquipmentInterface::IsReplicationManaged_Implementation()
{
	return _getUObject() && _getUObject()->GetClass()->IsChildOf(USigilEquipmentInstance::StaticClass());
}
