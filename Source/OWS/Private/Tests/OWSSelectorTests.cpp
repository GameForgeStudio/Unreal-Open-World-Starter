#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "OWSInteractionTargetComponent.h"
#include "OWSSelectorComponent.h"
#include "OWSStockVehicleInteractionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace OWSSelectorTests
{
	constexpr TCHAR MapPath[] = TEXT("/Game/OWS/Levels/OWS_CombinedDemo");

	class FRuntimeReadoutCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FRuntimeReadoutCommand(FAutomationTestBase& InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			if (StartedAt <= 0.0)
			{
				StartedAt = FPlatformTime::Seconds();
			}
			if (GEngine)
			{
				for (const FWorldContext& Context : GEngine->GetWorldContexts())
				{
					UWorld* World = Context.World();
					if (!World || (Context.WorldType != EWorldType::PIE && Context.WorldType != EWorldType::Game))
					{
						continue;
					}
					APlayerController* Controller = World->GetFirstPlayerController();
					ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
					UOWSSelectorComponent* Selector = Character
						? Character->FindComponentByClass<UOWSSelectorComponent>()
						: nullptr;
					if (Selector && !bRangeSelectorsDisabled)
					{
						for (FOWSSelectorFunction& Function : Selector->SelectorFunctions)
						{
							for (FOWSRangeSelector& RangeSelector : Function.SelectorStack)
							{
								RangeSelector.bEnabled = false;
							}
						}
						Test.TestFalse(TEXT("Precision targeting does not require an enabled awareness detector"),
							Selector->HasValidSelectorStacks());
						bRangeSelectorsDisabled = true;
					}
					if (Selector && Selector->IsDebugReadoutMounted() &&
						(!TargetCharacter.IsValid() || !TargetMenuToken.IsValid()))
					{
						for (TActorIterator<AActor> It(World); It; ++It)
						{
							if (ACharacter* OtherCharacter = Cast<ACharacter>(*It);
								OtherCharacter && OtherCharacter != Character)
							{
								TargetCharacter = OtherCharacter;
							}
							if (It->GetClass()->GetName().StartsWith(TEXT("LevelButton")))
							{
								const float DistanceSquared = FVector::DistSquared(
									It->GetActorLocation(), Character->GetActorLocation());
								if (DistanceSquared < TargetMenuTokenDistanceSquared)
								{
									TargetMenuToken = *It;
									TargetMenuTokenDistanceSquared = DistanceSquared;
								}
							}
						}
					}
					AActor* AimTarget = bCharacterDetected ? TargetMenuToken.Get() : TargetCharacter.Get();
					if (Selector && AimTarget)
					{
						FVector RayOrigin = Character->GetActorLocation();
						if (const USkeletalMeshComponent* Mesh = Character->GetMesh();
							Mesh && Mesh->DoesSocketExist(TEXT("head")))
						{
							RayOrigin = Mesh->GetSocketLocation(TEXT("head"));
						}
						FVector TargetOrigin;
						FVector TargetExtent;
						AimTarget->GetActorBounds(false, TargetOrigin, TargetExtent, true);
						Controller->SetControlRotation((TargetOrigin - RayOrigin).Rotation());
						Selector->RefreshSelection();
					}
					if (!bCharacterDetected && Selector && Selector->IsDebugReadoutMounted() &&
						Selector->GetDetectedActor() == TargetCharacter.Get())
					{
						Test.TestTrue(TEXT("Selector readout is mounted on the local player viewport"), true);
						Test.TestNotNull(TEXT("Yellow precision ray detects another OWS character"), Selector->GetDetectedActor());
						bCharacterDetected = true;
					}
					if (bCharacterDetected && Selector &&
						Selector->GetDetectedActor() == TargetMenuToken.Get())
					{
						Test.TestNotNull(TEXT("Yellow precision ray detects a ground menu token"), Selector->GetDetectedActor());
						return true;
					}
				}
			}
			if (FPlatformTime::Seconds() - StartedAt > 20.0)
			{
				Test.AddError(TEXT("Timed out waiting for the yellow precision ray to detect both another OWS character and a ground menu token in PIE."));
				return true;
			}
			return false;
		}

	private:
		FAutomationTestBase& Test;
		double StartedAt = 0.0;
		TWeakObjectPtr<ACharacter> TargetCharacter;
		TWeakObjectPtr<AActor> TargetMenuToken;
		float TargetMenuTokenDistanceSquared = TNumericLimits<float>::Max();
		bool bCharacterDetected = false;
		bool bRangeSelectorsDisabled = false;
	};

	class FActivationRoutingCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FActivationRoutingCommand(FAutomationTestBase& InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			if (StartedAt <= 0.0)
			{
				StartedAt = FPlatformTime::Seconds();
			}
			if (!GEngine)
			{
				return HasTimedOut();
			}
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (!World || (Context.WorldType != EWorldType::PIE &&
					Context.WorldType != EWorldType::Game))
				{
					continue;
				}
				APlayerController* Controller = World->GetFirstPlayerController();
				ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
				UOWSSelectorComponent* Selector = Character
					? Character->FindComponentByClass<UOWSSelectorComponent>()
					: nullptr;
				if (!Controller || !Character || !Selector)
				{
					continue;
				}
				if (!TargetVehicle.IsValid())
				{
					for (TActorIterator<APawn> It(World); It; ++It)
					{
						UOWSStockVehicleInteractionComponent* Interaction =
							It->FindComponentByClass<UOWSStockVehicleInteractionComponent>();
						if (!Interaction)
						{
							continue;
						}
						TArray<FName> DoorIds;
						Interaction->GetDoorIds(DoorIds);
						if (!DoorIds.IsEmpty())
						{
							TargetVehicle = *It;
							TargetDoor = Interaction->GetDoorInteractionTarget(DoorIds[0]);
							break;
						}
					}
					if (!TargetVehicle.IsValid() || !TargetDoor.IsValid())
					{
						continue;
					}
					const float CapsuleHalfHeight = Character->GetCapsuleComponent()
						? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
						: 90.0f;
					const FTransform DoorTransform = TargetDoor->GetComponentTransform();
					FVector DoorOutward = DoorTransform.GetRotation().GetForwardVector();
					DoorOutward.Z = 0.0f;
					DoorOutward.Normalize();
					FVector Location = DoorTransform.GetLocation() + DoorOutward * 160.0f;
					FCollisionQueryParams Params(
						SCENE_QUERY_STAT(OWSSelectorActivationGround), false, Character);
					Params.AddIgnoredActor(TargetVehicle.Get());
					FHitResult GroundHit;
					if (World->LineTraceSingleByChannel(
						GroundHit,
						FVector(Location.X, Location.Y, TargetVehicle->GetActorLocation().Z + 500.0f),
						FVector(Location.X, Location.Y, TargetVehicle->GetActorLocation().Z - 500.0f),
						ECC_Pawn, Params) && GroundHit.ImpactNormal.Z >= 0.7f)
					{
						Location.Z = GroundHit.ImpactPoint.Z + CapsuleHalfHeight + 2.0f;
					}
					else
					{
						Location.Z = DoorTransform.GetLocation().Z + CapsuleHalfHeight - 55.0f;
					}
					const FRotator AimRotation =
						(DoorTransform.GetLocation() - Location).Rotation();
					Character->SetActorLocationAndRotation(
						Location, AimRotation, false, nullptr, ETeleportType::TeleportPhysics);
					Controller->SetControlRotation(AimRotation);
					continue;
				}
				FVector RayOrigin = Character->GetActorLocation();
				if (const USkeletalMeshComponent* Mesh = Character->GetMesh();
					Mesh && Mesh->DoesSocketExist(TEXT("head")))
				{
					RayOrigin = Mesh->GetSocketLocation(TEXT("head"));
				}
				Controller->SetControlRotation(
					(TargetDoor->GetComponentLocation() - RayOrigin).Rotation());
				Selector->RefreshSelection();
				if (!bActivationRequested &&
					Selector->GetDetectedInteractionTarget() == TargetDoor.Get())
				{
					Test.TestEqual(TEXT("Selector resolves the exact authored vehicle door"),
						Selector->GetDetectedInteractionTarget()->InteractionId,
						TargetDoor->InteractionId);
					bActivationRequested = true;
					Test.TestTrue(TEXT("Shared Activate dispatch accepts the selected door"),
						Selector->ActivateCurrentTarget());
				}
				if (bActivationRequested && Controller->GetPawn() == TargetVehicle.Get())
				{
					Test.TestTrue(TEXT("Vehicle entry consumes shared Activate routing"), true);
					return true;
				}
			}
			return HasTimedOut();
		}

	private:
		bool HasTimedOut()
		{
			if (FPlatformTime::Seconds() - StartedAt <= 20.0)
			{
				return false;
			}
			Test.AddError(TEXT(
				"Timed out resolving and activating an authored vehicle-door target."));
			return true;
		}

		FAutomationTestBase& Test;
		double StartedAt = 0.0;
		TWeakObjectPtr<APawn> TargetVehicle;
		TWeakObjectPtr<UOWSInteractionTargetComponent> TargetDoor;
		bool bActivationRequested = false;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOWSSelectorDefaultStackTest,
	"OWS.Selector.DefaultStack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOWSSelectorDefaultStackTest::RunTest(const FString& Parameters)
{
	const UOWSSelectorComponent* Component = NewObject<UOWSSelectorComponent>();
	TestNotNull(TEXT("Selector component can be created"), Component);
	if (!Component)
	{
		return false;
	}

	TestTrue(TEXT("Default selector stack is valid"), Component->HasValidSelectorStacks());
	TestEqual(TEXT("Independent precision ray uses the 40 metre default"),
		Component->PrecisionRayLength, 4000.0f);
	TestTrue(TEXT("Independent precision ray detects world dynamic objects"),
		Component->PrecisionRayObjectTypes.Contains(ECC_WorldDynamic));
	TestEqual(TEXT("One default selector function"), Component->SelectorFunctions.Num(), 1);
	if (Component->SelectorFunctions.Num() != 1)
	{
		return false;
	}

	const FOWSSelectorFunction& Function = Component->SelectorFunctions[0];
	TestEqual(TEXT("Default function is Activate"), Function.Name, FName(TEXT("Activate")));
	TestTrue(TEXT("F is a default activation binding"),
		Function.ActivationKeys.Contains(EKeys::F));
	TestTrue(TEXT("Square/X is a default activation binding"),
		Function.ActivationKeys.Contains(EKeys::Gamepad_FaceButton_Left));
	TestEqual(TEXT("Default stack contains orb and two cones"), Function.SelectorStack.Num(), 3);
	if (Function.SelectorStack.Num() == 3)
	{
		TestEqual(TEXT("First selector is reach orb"), Function.SelectorStack[0].Shape, EOWSRangeSelectorShape::Sphere);
		TestFalse(TEXT("Reach orb does not require facing"), Function.SelectorStack[0].bRequiresFacing);
		TestEqual(TEXT("Second selector is a cone"), Function.SelectorStack[1].Shape, EOWSRangeSelectorShape::Cone);
		TestEqual(TEXT("Third selector is a cone"), Function.SelectorStack[2].Shape, EOWSRangeSelectorShape::Cone);
		TestEqual(TEXT("Short cone uses the 15 metre default"), Function.SelectorStack[1].Length, 1500.0f);
		TestEqual(TEXT("Short cone uses a 90 degree full angle"), Function.SelectorStack[1].HalfAngleDegrees, 45.0f);
		TestEqual(TEXT("Long cone uses the 40 metre default"), Function.SelectorStack[2].Length, 4000.0f);
		TestEqual(TEXT("Long cone uses a 50 degree full angle"), Function.SelectorStack[2].HalfAngleDegrees, 25.0f);
		TestTrue(TEXT("Short cone is wider than long cone"),
			Function.SelectorStack[1].HalfAngleDegrees > Function.SelectorStack[2].HalfAngleDegrees);
		TestTrue(TEXT("Long cone reaches farther than short cone"),
			Function.SelectorStack[2].Length > Function.SelectorStack[1].Length);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOWSSelectorBlueprintAttachmentTest,
	"OWS.Selector.SharedCharacterAttachment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOWSSelectorBlueprintAttachmentTest::RunTest(const FString& Parameters)
{
	const UBlueprint* Blueprint = LoadObject<UBlueprint>(
		nullptr, TEXT("/Game/OWS/Characters/Core/CBP_OWSCharacter_Base.CBP_OWSCharacter_Base"));
	TestNotNull(TEXT("Shared OWS character Blueprint loads"), Blueprint);
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return false;
	}

	const UOWSSelectorComponent* Selector = nullptr;
	if (Blueprint->SimpleConstructionScript)
	{
		for (const USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (const UOWSSelectorComponent* Candidate = Node
				? Cast<UOWSSelectorComponent>(Node->ComponentTemplate)
				: nullptr)
			{
				Selector = Candidate;
				break;
			}
		}
	}
	TestNotNull(TEXT("Shared OWS character owns the selector component"), Selector);
	if (!Selector || !Selector->HasValidSelectorStacks() ||
		Selector->SelectorFunctions.Num() != 1 ||
		Selector->SelectorFunctions[0].SelectorStack.Num() != 3)
	{
		return false;
	}
	const TArray<FOWSRangeSelector>& Stack = Selector->SelectorFunctions[0].SelectorStack;
	TestTrue(TEXT("Shared character routes F through Activate"),
		Selector->SelectorFunctions[0].ActivationKeys.Contains(EKeys::F));
	TestTrue(TEXT("Shared character routes Square/X through Activate"),
		Selector->SelectorFunctions[0].ActivationKeys.Contains(EKeys::Gamepad_FaceButton_Left));
	TestEqual(TEXT("Shared character short cone is 15 metres"), Stack[1].Length, 1500.0f);
	TestEqual(TEXT("Shared character short cone full angle is 90 degrees"), Stack[1].HalfAngleDegrees, 45.0f);
	TestEqual(TEXT("Shared character long cone is 40 metres"), Stack[2].Length, 4000.0f);
	TestEqual(TEXT("Shared character long cone full angle is 50 degrees"), Stack[2].HalfAngleDegrees, 25.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOWSSelectorRuntimeReadoutTest,
	"OWS.Selector.RuntimeReadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOWSSelectorRuntimeReadoutTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(OWSSelectorTests::MapPath))
	{
		AddError(TEXT("Failed to open the canonical OWS map for the selector readout test."));
		return false;
	}
	AddCommand(new OWSSelectorTests::FRuntimeReadoutCommand(*this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOWSSelectorActivationRoutingTest,
	"OWS.Selector.VehicleActivationRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOWSSelectorActivationRoutingTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(OWSSelectorTests::MapPath))
	{
		AddError(TEXT("Failed to open the canonical OWS map for activation routing."));
		return false;
	}
	AddCommand(new OWSSelectorTests::FActivationRoutingCommand(*this));
	return true;
}

#endif
