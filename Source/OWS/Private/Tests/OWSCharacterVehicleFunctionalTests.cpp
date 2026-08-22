#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "OWSStockVehicleInteractionComponent.h"
#include "OWSVehicleInteractionComponent.h"

namespace OWSCharacterVehicleTests
{
	constexpr TCHAR MapPath[] = TEXT("/Game/OWS/Levels/OWS_CombinedDemo");
	constexpr TCHAR VehicleContextPath[] =
		TEXT("/KinetiForge/Template/Input/IMC_VehicleDefault.IMC_VehicleDefault");

	UWorld* FindGameWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) &&
				Context.World())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	struct FTestActors
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<APlayerController> Controller;
		TWeakObjectPtr<ACharacter> Character;
		TWeakObjectPtr<APawn> Vehicle;
		TWeakObjectPtr<UOWSVehicleInteractionComponent> VehicleInteraction;
		TWeakObjectPtr<UOWSStockVehicleInteractionComponent> StockInteraction;
		TWeakObjectPtr<UPrimitiveComponent> VehicleBody;
		TWeakObjectPtr<UInputMappingContext> VehicleContext;

		bool Resolve()
		{
			UWorld* FoundWorld = FindGameWorld();
			APlayerController* FoundController = FoundWorld
				? FoundWorld->GetFirstPlayerController()
				: nullptr;
			ACharacter* FoundCharacter = FoundController
				? Cast<ACharacter>(FoundController->GetPawn())
				: nullptr;
			UOWSVehicleInteractionComponent* FoundVehicleInteraction = FoundController
				? FoundController->FindComponentByClass<UOWSVehicleInteractionComponent>()
				: nullptr;

			APawn* FoundVehicle = nullptr;
			UOWSStockVehicleInteractionComponent* FoundStockInteraction = nullptr;
			if (FoundWorld)
			{
				for (TActorIterator<APawn> It(FoundWorld); It; ++It)
				{
					UOWSStockVehicleInteractionComponent* Candidate =
						It->FindComponentByClass<UOWSStockVehicleInteractionComponent>();
					if (Candidate && It->GetClass()->GetName().Contains(TEXT("DriftCar")))
					{
						FoundVehicle = *It;
						FoundStockInteraction = Candidate;
						break;
					}
					if (!FoundVehicle && Candidate)
					{
						FoundVehicle = *It;
						FoundStockInteraction = Candidate;
					}
				}
			}

			UInputMappingContext* FoundVehicleContext = LoadObject<UInputMappingContext>(
				nullptr, VehicleContextPath);
			if (!FoundWorld || !FoundController || !FoundCharacter ||
				!FoundVehicleInteraction || !FoundVehicle || !FoundStockInteraction ||
				!FoundVehicleContext)
			{
				return false;
			}

			World = FoundWorld;
			Controller = FoundController;
			Character = FoundCharacter;
			Vehicle = FoundVehicle;
			VehicleInteraction = FoundVehicleInteraction;
			StockInteraction = FoundStockInteraction;
			VehicleBody = FoundStockInteraction->GetVehiclePhysicsBody();
			VehicleContext = FoundVehicleContext;
			return VehicleBody.IsValid();
		}

		bool IsValid() const
		{
			return World.IsValid() && Controller.IsValid() && Character.IsValid() &&
				Vehicle.IsValid() && VehicleInteraction.IsValid() &&
				StockInteraction.IsValid() && VehicleBody.IsValid() && VehicleContext.IsValid();
		}

		bool HasVehicleContext() const
		{
			const APlayerController* PlayerController = Controller.Get();
			const ULocalPlayer* LocalPlayer = PlayerController
				? PlayerController->GetLocalPlayer()
				: nullptr;
			const UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
				? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
					const_cast<ULocalPlayer*>(LocalPlayer))
				: nullptr;
			return InputSubsystem && InputSubsystem->HasMappingContext(VehicleContext.Get());
		}

		bool PlaceCharacterAtDriverDoor() const
		{
			if (!IsValid())
			{
				return false;
			}
			FTransform DoorTransform;
			if (!StockInteraction->GetDoorWorldTransform(TEXT("LeftDoor"), DoorTransform))
			{
				return false;
			}
			const float CapsuleHalfHeight = Character->GetCapsuleComponent()
				? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
				: 90.0f;
			FVector Location = DoorTransform.GetLocation() - Vehicle->GetActorRightVector() * 160.0f;
			Location.Z = DoorTransform.GetLocation().Z + CapsuleHalfHeight - 55.0f;
			FVector ToDoor = DoorTransform.GetLocation() - Location;
			ToDoor.Z = 0.0f;
			Character->SetActorLocationAndRotation(
				Location, ToDoor.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
			return true;
		}
	};

	bool AssertEntered(FAutomationTestBase& Test, const FTestActors& Actors)
	{
		bool bPassed = true;
		bPassed &= Test.TestTrue(TEXT("The vehicle is possessed"),
			Actors.Controller->GetPawn() == Actors.Vehicle.Get());
		bPassed &= Test.TestTrue(TEXT("The on-foot character is hidden"),
			Actors.Character->IsHidden());
		bPassed &= Test.TestFalse(TEXT("The stored character collision is disabled"),
			Actors.Character->GetActorEnableCollision());
		bPassed &= Test.TestTrue(TEXT("The stored character is attached to the vehicle"),
			Actors.Character->GetAttachParentActor() == Actors.Vehicle.Get());
		bPassed &= Test.TestTrue(TEXT("The control seat records the same character"),
			Actors.StockInteraction->GetSeatForOccupant(Actors.Character.Get()) != NAME_None);
		bPassed &= Test.TestTrue(TEXT("The vehicle input context is active"),
			Actors.HasVehicleContext());
		bPassed &= Test.TestTrue(TEXT("The camera targets the possessed vehicle"),
			Actors.Controller->GetViewTarget() == Actors.Vehicle.Get());
		return bPassed;
	}

	bool AssertStoppedExit(FAutomationTestBase& Test, const FTestActors& Actors)
	{
		bool bPassed = true;
		ACharacter* Character = Actors.Character.Get();
		bPassed &= Test.TestTrue(TEXT("The same character is possessed after exit"),
			Actors.Controller->GetPawn() == Character);
		bPassed &= Test.TestFalse(TEXT("The character is visible after exit"), Character->IsHidden());
		bPassed &= Test.TestTrue(TEXT("Character collision is restored after exit"),
			Character->GetActorEnableCollision());
		bPassed &= Test.TestTrue(TEXT("The character is detached after exit"),
			Character->GetAttachParentActor() == nullptr);
		bPassed &= Test.TestTrue(TEXT("The control seat is released after exit"),
			Actors.StockInteraction->GetSeatForOccupant(Character) == NAME_None);
		bPassed &= Test.TestFalse(TEXT("The vehicle input context is removed after exit"),
			Actors.HasVehicleContext());
		bPassed &= Test.TestTrue(TEXT("The camera targets the character after exit"),
			Actors.Controller->GetViewTarget() == Character);
		bPassed &= Test.TestTrue(TEXT("Character movement is enabled after exit"),
			Character->GetCharacterMovement() &&
			Character->GetCharacterMovement()->MovementMode != MOVE_None);
		return bPassed;
	}

	class FStoppedCyclesCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FStoppedCyclesCommand(FAutomationTestBase& InTest)
			: Test(InTest), StartedAt(FPlatformTime::Seconds())
		{
		}

		virtual bool Update() override
		{
			if (!Actors.IsValid())
			{
				if (!Actors.Resolve())
				{
					if (FPlatformTime::Seconds() - StartedAt > 20.0)
					{
						Test.AddError(TEXT("Timed out resolving the canonical OWS character and vehicle actors."));
						return true;
					}
					return false;
				}
				PhaseStartedAt = FPlatformTime::Seconds();
				return false;
			}

			if (FPlatformTime::Seconds() - PhaseStartedAt < 0.25)
			{
				return false;
			}

			if (bEntering)
			{
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				if (!Actors.PlaceCharacterAtDriverDoor())
				{
					Test.AddError(TEXT("Could not place the character at the selected vehicle door."));
					return true;
				}
				if (!Test.TestTrue(TEXT("Debug entry succeeds"),
					Actors.VehicleInteraction->DebugEnterNearestVehicle()))
				{
					return true;
				}
				if (!AssertEntered(Test, Actors))
				{
					return true;
				}
				bEntering = false;
			}
			else
			{
				if (!Test.TestTrue(TEXT("Stopped exit succeeds"),
					Actors.VehicleInteraction->DebugExitVehicle()))
				{
					return true;
				}
				if (!AssertStoppedExit(Test, Actors))
				{
					return true;
				}
				++CompletedCycles;
				if (CompletedCycles >= 5)
				{
					return true;
				}
				bEntering = true;
			}
			PhaseStartedAt = FPlatformTime::Seconds();
			return false;
		}

	private:
		FAutomationTestBase& Test;
		FTestActors Actors;
		double StartedAt;
		double PhaseStartedAt = 0.0;
		int32 CompletedCycles = 0;
		bool bEntering = true;
	};

	class FMovingBailoutCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FMovingBailoutCommand(FAutomationTestBase& InTest)
			: Test(InTest), StartedAt(FPlatformTime::Seconds())
		{
		}

		virtual bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (!Actors.IsValid())
			{
				if (!Actors.Resolve())
				{
					if (Now - StartedAt > 20.0)
					{
						Test.AddError(TEXT("Timed out resolving the canonical OWS character and vehicle actors."));
						return true;
					}
					return false;
				}
				PhaseStartedAt = Now;
				return false;
			}
			if (Now - PhaseStartedAt < 0.25)
			{
				return false;
			}

			switch (Phase)
			{
			case 0:
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				if (!Actors.PlaceCharacterAtDriverDoor() ||
					!Test.TestTrue(TEXT("Entry before bailout succeeds"),
						Actors.VehicleInteraction->DebugEnterNearestVehicle()) ||
					!AssertEntered(Test, Actors))
				{
					return true;
				}
				++Phase;
				break;
			case 1:
				Actors.VehicleBody->WakeAllRigidBodies();
				Actors.VehicleBody->SetPhysicsLinearVelocity(
					Actors.Vehicle->GetActorForwardVector() * 1000.0f);
				++Phase;
				break;
			case 2:
				if (!Test.TestTrue(TEXT("Moving bailout succeeds"),
					Actors.VehicleInteraction->DebugExitVehicle()))
				{
					return true;
				}
				if (!AssertBailoutStarted())
				{
					return true;
				}
				++Phase;
				RecoveryStartedAt = Now;
				break;
			case 3:
				if (Actors.Character->GetMesh()->IsSimulatingPhysics())
				{
					if (Now - RecoveryStartedAt > 8.0)
					{
						Test.AddError(TEXT("The bailed-out character did not recover within eight seconds."));
						return true;
					}
					return false;
				}
				if (!AssertRecovered())
				{
					return true;
				}
				Actors.PlaceCharacterAtDriverDoor();
				if (!Test.TestFalse(TEXT("Immediate re-entry after bailout is rejected"),
					Actors.VehicleInteraction->DebugEnterNearestVehicle()))
				{
					return true;
				}
				Actors.Character->SetActorLocation(
					Actors.Vehicle->GetActorLocation() + FVector(800.0f, 800.0f, 100.0f),
					false, nullptr, ETeleportType::TeleportPhysics);
				++Phase;
				break;
			case 4:
				Actors.PlaceCharacterAtDriverDoor();
				if (!Test.TestTrue(TEXT("Re-entry succeeds after leaving the release radius"),
					Actors.VehicleInteraction->DebugEnterNearestVehicle()) ||
					!AssertEntered(Test, Actors))
				{
					return true;
				}
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				if (!Test.TestTrue(TEXT("Cleanup stopped exit succeeds"),
					Actors.VehicleInteraction->DebugExitVehicle()) ||
					!AssertStoppedExit(Test, Actors))
				{
					return true;
				}
				return true;
			default:
				return true;
			}

			PhaseStartedAt = Now;
			return false;
		}

	private:
		bool AssertBailoutStarted()
		{
			bool bPassed = true;
			ACharacter* Character = Actors.Character.Get();
			USkeletalMeshComponent* Mesh = Character->GetMesh();
			bPassed &= Test.TestTrue(TEXT("The same character is possessed after bailout"),
				Actors.Controller->GetPawn() == Character);
			bPassed &= Test.TestFalse(TEXT("The character is visible after bailout"),
				Character->IsHidden());
			bPassed &= Test.TestTrue(TEXT("The ragdoll mesh simulates physics"),
				Mesh && Mesh->IsSimulatingPhysics());
			bPassed &= Test.TestTrue(TEXT("The capsule is disabled during bailout ragdoll"),
				Character->GetCapsuleComponent() &&
				Character->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			bPassed &= Test.TestTrue(TEXT("The control seat is released during bailout"),
				Actors.StockInteraction->GetSeatForOccupant(Character) == NAME_None);
			bPassed &= Test.TestFalse(TEXT("The vehicle input context is removed during bailout"),
				Actors.HasVehicleContext());
			bPassed &= Test.TestTrue(TEXT("The camera targets the bailed-out character"),
				Actors.Controller->GetViewTarget() == Character);
			bPassed &= Test.TestFalse(TEXT("The bailout transform remains finite"),
				Character->GetActorLocation().ContainsNaN());
			return bPassed;
		}

		bool AssertRecovered()
		{
			bool bPassed = true;
			ACharacter* Character = Actors.Character.Get();
			bPassed &= Test.TestFalse(TEXT("The recovered mesh no longer simulates physics"),
				Character->GetMesh()->IsSimulatingPhysics());
			bPassed &= Test.TestTrue(TEXT("Capsule collision is restored after recovery"),
				Character->GetCapsuleComponent()->GetCollisionEnabled() ==
					ECollisionEnabled::QueryAndPhysics);
			bPassed &= Test.TestTrue(TEXT("Character movement is enabled after recovery"),
				Character->GetCharacterMovement() &&
				Character->GetCharacterMovement()->MovementMode != MOVE_None);
			bPassed &= Test.TestTrue(TEXT("The recovered character remains possessed"),
				Actors.Controller->GetPawn() == Character);
			bPassed &= Test.TestTrue(TEXT("The recovered transform remains finite"),
				!Character->GetActorLocation().ContainsNaN());
			return bPassed;
		}

		FAutomationTestBase& Test;
		FTestActors Actors;
		double StartedAt;
		double PhaseStartedAt = 0.0;
		double RecoveryStartedAt = 0.0;
		int32 Phase = 0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOWSStoppedVehicleCyclesTest,
	"OWS.CharacterVehicle.StoppedExitRepeatedCycles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOWSStoppedVehicleCyclesTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(OWSCharacterVehicleTests::MapPath))
	{
		AddError(TEXT("Failed to open the canonical OWS map for the stopped-exit test."));
		return false;
	}
	AddCommand(new OWSCharacterVehicleTests::FStoppedCyclesCommand(*this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOWSMovingBailoutRecoveryTest,
	"OWS.CharacterVehicle.MovingBailoutRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOWSMovingBailoutRecoveryTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(OWSCharacterVehicleTests::MapPath))
	{
		AddError(TEXT("Failed to open the canonical OWS map for the moving-bailout test."));
		return false;
	}
	AddCommand(new OWSCharacterVehicleTests::FMovingBailoutCommand(*this));
	return true;
}

#endif
