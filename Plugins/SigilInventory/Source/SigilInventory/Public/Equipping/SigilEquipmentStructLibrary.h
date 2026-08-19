// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SigilEquipmentStructLibrary.generated.h"

USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilEquipmentActorToSpawn
{
	GENERATED_BODY()

	FSigilEquipmentActorToSpawn()
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category=Equipment)
	TSoftClassPtr<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category=Equipment)
	bool bShouldAttach{true};

	UPROPERTY(EditAnywhere, Category=Equipment, meta=(EditCondition="bShouldAttach", EditConditionHides))
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category=Equipment, meta=(EditCondition="bShouldAttach", EditConditionHides))
	FTransform AttachTransform;
};
