// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilInventoryLogChannels.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "SigilItemInstance.h"
#include "SigilItemCollection.h"
#include "SigilItemDefinition.h"

DEFINE_LOG_CATEGORY(LogSigilInventory)


FString GetGISLogContextString(const UObject* ContextObject)
{
	ENetRole Role = ROLE_None;
	FString RoleName = TEXT("None");
	FString Name = "None";

	if (const AActor* Actor = Cast<AActor>(ContextObject))
	{
		Role = Actor->GetLocalRole();
		Name = Actor->GetName();
	}
	else if (const UActorComponent* Component = Cast<UActorComponent>(ContextObject))
	{
		if (AActor* ActorOwner = Cast<AActor>(Component->GetOuter()))
		{
			Role = ActorOwner->GetLocalRole();
			Name = ActorOwner->GetName();
		}
		else
		{
			const AActor* Owner = Component->GetOwner();
			Role = IsValid(Owner) ? Owner->GetLocalRole() : ROLE_None;
			Name = IsValid(Owner) ? Owner->GetName() : TEXT("None");
		}
	}
	else if (const USigilItemInstance* ItemInstance = Cast<USigilItemInstance>(ContextObject))
	{
		if (AActor* ActorOwner = Cast<AActor>(ItemInstance->GetOuter()))
		{
			Role = ActorOwner->GetLocalRole();
			Name = ActorOwner->GetName();
		}
		else
		{
			return FString::Printf(TEXT("(%s)'s instance(%s) "), *ItemInstance->GetDefinition()->GetName(), *ItemInstance->GetName());
		}
	}
	else if (const USigilItemCollection* Collection = Cast<USigilItemCollection>(ContextObject))
	{
		if (AActor* ActorOwner = Cast<AActor>(Collection->GetOuter()))
		{
			Role = ActorOwner->GetLocalRole();
			Name = ActorOwner->GetName();
		}
		if (Role != ROLE_None)
		{
			RoleName = (Role == ROLE_Authority) ? TEXT("Server") : TEXT("Client");
		}
		return FString::Printf(TEXT("[%s] (%s)'s %s"), *RoleName, *Name, *Collection->GetCollectionName());
	}
	else if (IsValid(ContextObject))
	{
		Name = ContextObject->GetName();
	}

	if (Role != ROLE_None)
	{
		RoleName = (Role == ROLE_Authority) ? TEXT("Server") : TEXT("Client");
	}
	return FString::Printf(TEXT("[%s] (%s)"), *RoleName, *Name);
}
