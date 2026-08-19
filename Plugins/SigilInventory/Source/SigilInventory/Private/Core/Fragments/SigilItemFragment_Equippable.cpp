// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilItemFragment_Equippable.h"

#include "SigilEquipmentInstance.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilItemFragment_Equippable)


USigilItemFragment_Equippable::USigilItemFragment_Equippable()
{
	InstanceType = USigilEquipmentInstance::StaticClass();
}

#if WITH_EDITOR
void USigilItemFragment_Equippable::PreSave(FObjectPreSaveContext SaveContext)
{
	if (InstanceType.IsNull())
	{
		bActorBased = false;
	}
	else
	{
		UClass* InstanceClass = InstanceType.LoadSynchronous();
		bActorBased = InstanceClass->IsChildOf(AActor::StaticClass());
	}
	Super::PreSave(SaveContext);
}
#endif
