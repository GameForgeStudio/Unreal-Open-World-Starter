#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
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
		FString PreferredVehicleClass = TEXT("DriftCar");
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
					if (Candidate &&
						(PreferredVehicleClass.IsEmpty() ||
							It->GetClass()->GetName().Contains(PreferredVehicleClass)))
					{
						FoundVehicle = *It;
						FoundStockInteraction = Candidate;
						break;
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
			TArray<FName> DoorIds;
			StockInteraction->GetDoorIds(DoorIds);
			if (DoorIds.IsEmpty())
			{
				return false;
			}
			FTransform DoorTransform;
			if (!StockInteraction->GetDoorWorldTransform(DoorIds[0], DoorTransform))
			{
				return false;
			}
			const float CapsuleHalfHeight = Character->GetCapsuleComponent()
				? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
				: 90.0f;
			FVector DoorOutward = DoorTransform.GetRotation().GetForwardVector();
			DoorOutward.Z = 0.0f;
			DoorOutward.Normalize();
			FVector Location = DoorTransform.GetLocation() + DoorOutward * 160.0f;
			FCollisionQueryParams GroundParams(
				SCENE_QUERY_STAT(OWSTestDriverDoorGround), false, Character.Get());
			GroundParams.AddIgnoredActor(Vehicle.Get());
			FHitResult GroundHit;
			const FVector GroundTraceStart(
				Location.X, Location.Y, Vehicle->GetActorLocation().Z + 500.0f);
			const FVector GroundTraceEnd(
				Location.X, Location.Y, Vehicle->GetActorLocation().Z - 500.0f);
			if (World->LineTraceSingleByChannel(
					GroundHit, GroundTraceStart, GroundTraceEnd, ECC_Pawn, GroundParams) &&
				GroundHit.ImpactNormal.Z >= 0.7f)
			{
				Location.Z = GroundHit.ImpactPoint.Z + CapsuleHalfHeight + 2.0f;
			}
			else
			{
				Location.Z = DoorTransform.GetLocation().Z + CapsuleHalfHeight - 55.0f;
			}
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
		bPassed &= Test.TestTrue(TEXT("Character movement remains active after exit"),
			Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsActive());
		bPassed &= Test.TestTrue(TEXT("Capsule collision is fully restored after exit"),
			Character->GetCapsuleComponent() &&
			Character->GetCapsuleComponent()->GetCollisionEnabled() ==
				ECollisionEnabled::QueryAndPhysics);
		bPassed &= Test.TestTrue(TEXT("The character mesh no longer simulates physics after exit"),
			Character->GetMesh() && !Character->GetMesh()->IsSimulatingPhysics());
		bPassed &= Test.TestTrue(TEXT("The character animation instance remains valid after exit"),
			Character->GetMesh() && Character->GetMesh()->GetAnimInstance() != nullptr);
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
				if (!bEntryPrepared)
				{
					Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
					Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
					if (!Actors.PlaceCharacterAtDriverDoor())
					{
						Test.AddError(TEXT("Could not place the character at the selected vehicle door."));
						return true;
					}
					// Allow the physics scene to receive the teleport before the entry
					// overlap query runs on the next latent-command update.
					bEntryPrepared = true;
					PhaseStartedAt = FPlatformTime::Seconds();
					return false;
				}
				// Controller rotation can update while the physics scene receives the
				// teleport, so refresh the exact door-facing transform before entry.
				if (!Actors.PlaceCharacterAtDriverDoor())
				{
					Test.AddError(TEXT("Could not refresh the character at the selected vehicle door."));
					return true;
				}
				const bool bEntered = Actors.VehicleInteraction->DebugEnterNearestVehicle();
				if (!Test.TestTrue(TEXT("Debug entry succeeds"), bEntered))
				{
					return true;
				}
				if (!AssertEntered(Test, Actors))
				{
					return true;
				}
				bEntryPrepared = false;
				bEntering = false;
			}
			else
			{
				// Establish the stopped-exit precondition at the moment of exit. The
				// physics vehicle can drift during the settle interval after entry,
				// which otherwise sends this test through the moving bailout path.
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
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
		bool bEntryPrepared = false;
	};

	class FRepresentativeStoppedExitCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FRepresentativeStoppedExitCommand(FAutomationTestBase& InTest)
			: Test(InTest), StartedAt(FPlatformTime::Seconds())
		{
			Actors.PreferredVehicleClass = RepresentativeVehicleClasses[VehicleIndex];
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
						Test.AddError(FString::Printf(
							TEXT("Timed out resolving representative vehicle class %s."),
							*Actors.PreferredVehicleClass));
						return true;
					}
					return false;
				}
				if (!RestoreCanonicalVehicleTransform())
				{
					Test.AddError(FString::Printf(
						TEXT("Could not restore canonical transform for representative vehicle %s."),
						*Actors.PreferredVehicleClass));
					return true;
				}
				if (!ValidateDoorConfiguration())
				{
					return true;
				}
				PhaseStartedAt = Now;
				return false;
			}
			if (Now - PhaseStartedAt < 0.25)
			{
				return false;
			}

			if (!bEntered)
			{
				if (!bEntryPrepared)
				{
					Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
					Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
					if (!Actors.PlaceCharacterAtDriverDoor())
					{
						Test.AddError(TEXT("Could not place the character for representative entry."));
						return true;
					}
					bEntryPrepared = true;
					PhaseStartedAt = Now;
					return false;
				}
				if (!Actors.PlaceCharacterAtDriverDoor())
				{
					Test.AddError(TEXT("Could not refresh the representative entry transform."));
					return true;
				}
				FName FacedDoor = NAME_None;
				float DoorDistance = 0.0f;
				if (!Test.TestTrue(TEXT("Representative entry faces a door on the selected vehicle"),
						Actors.StockInteraction->FindFacedDoor(
							Actors.Character->GetActorLocation(),
							Actors.Character->GetActorForwardVector(),
							500.0f, FacedDoor, DoorDistance)))
				{
					return true;
				}
				FName ControlSeat = NAME_None;
				EnteredDoorId = FacedDoor;
				if (!Test.TestTrue(TEXT("Representative entry has an available control seat"),
						Actors.StockInteraction->SelectAvailableSeat(
							FacedDoor, true, Actors.Character.Get(), ControlSeat)) ||
					!Test.TestTrue(TEXT("Representative entry selects a control seat"),
						Actors.StockInteraction->IsControlSeat(ControlSeat)) ||
					!Test.TestTrue(TEXT("Representative vehicle entry succeeds"),
						Actors.VehicleInteraction->DebugEnterNearestVehicle()) ||
					!AssertEntered(Test, Actors))
				{
					return true;
				}
				bEntryPrepared = false;
				bEntered = true;
				PhaseStartedAt = Now;
				return false;
			}

			Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			FTransform ExpectedExitTransform;
			if (!Test.TestTrue(TEXT("Representative entry door has an exit transform"),
					Actors.StockInteraction->GetDoorExitWorldTransform(
						EnteredDoorId, ExpectedExitTransform)))
			{
				return true;
			}
			if (!Test.TestTrue(TEXT("Representative stopped exit succeeds"),
					Actors.VehicleInteraction->DebugExitVehicle()) ||
				!AssertStoppedExit(Test, Actors))
			{
				return true;
			}
			const FTransform ActualExitTransform = Actors.Character->GetActorTransform();
			const FVector ExpectedHorizontal = FVector(
				ExpectedExitTransform.GetLocation().X,
				ExpectedExitTransform.GetLocation().Y,
				0.0f);
			const FVector ActualHorizontal = FVector(
				ActualExitTransform.GetLocation().X,
				ActualExitTransform.GetLocation().Y,
				0.0f);
			if (!Test.TestTrue(*FString::Printf(
					TEXT("%s stopped exit uses configured door position (expected %s, actual %s)"),
					*Actors.PreferredVehicleClass,
					*ExpectedHorizontal.ToCompactString(),
					*ActualHorizontal.ToCompactString()),
					FVector::DistSquared(ExpectedHorizontal, ActualHorizontal) <= 0.01f) ||
				!Test.TestTrue(TEXT("Stopped exit uses the configured door rotation"),
					ActualExitTransform.GetRotation().Equals(
						ExpectedExitTransform.GetRotation(), 0.001f)))
			{
				return true;
			}
			const float CapsuleRadius = Actors.Character->GetCapsuleComponent()
				? Actors.Character->GetCapsuleComponent()->GetScaledCapsuleRadius()
				: 0.0f;
			if (!Test.TestTrue(TEXT("Stopped exit stands fully outside the vehicle body"),
					Actors.VehicleBody->Bounds.GetBox().ComputeSquaredDistanceToPoint(
						Actors.Character->GetActorLocation()) > FMath::Square(CapsuleRadius)))
			{
				return true;
			}

			++VehicleIndex;
			if (VehicleIndex >= UE_ARRAY_COUNT(RepresentativeVehicleClasses))
			{
				return true;
			}
			Actors = FTestActors();
			Actors.PreferredVehicleClass = RepresentativeVehicleClasses[VehicleIndex];
			bEntered = false;
			StartedAt = Now;
			PhaseStartedAt = Now;
			return false;
		}

	private:
		bool ValidateDoorConfiguration()
		{
			TArray<FName> DoorIds;
			Actors.StockInteraction->GetDoorIds(DoorIds);
			if (!Test.TestEqual(
					*FString::Printf(TEXT("%s has two authored doors"),
						*Actors.PreferredVehicleClass),
					DoorIds.Num(), 2))
			{
				return false;
			}

			FTransform ExitTransforms[2];
			for (int32 DoorIndex = 0; DoorIndex < DoorIds.Num(); ++DoorIndex)
			{
				if (!Test.TestTrue(
						*FString::Printf(TEXT("%s door %s has an authored exit transform"),
							*Actors.PreferredVehicleClass, *DoorIds[DoorIndex].ToString()),
						Actors.StockInteraction->GetDoorExitWorldTransform(
							DoorIds[DoorIndex], ExitTransforms[DoorIndex])))
				{
					return false;
				}
			}
			if (!Test.TestTrue(
					*FString::Printf(TEXT("%s door exits are distinct"),
						*Actors.PreferredVehicleClass),
					!ExitTransforms[0].GetLocation().Equals(
						ExitTransforms[1].GetLocation(), 1.0f)))
			{
				return false;
			}

			if (Actors.PreferredVehicleClass == TEXT("Bus"))
			{
				for (const FTransform& ExitTransform : ExitTransforms)
				{
					if (!Test.TestTrue(TEXT("Each bus exit is on the curb side"),
							FVector::DotProduct(
								ExitTransform.GetLocation() - Actors.Vehicle->GetActorLocation(),
								Actors.Vehicle->GetActorRightVector()) > 0.0f))
					{
						return false;
					}
				}
			}
			return true;
		}

		bool RestoreCanonicalVehicleTransform() const
		{
			if (!GEngine || !Actors.Vehicle.IsValid() || !Actors.VehicleBody.IsValid())
			{
				return false;
			}
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* EditorWorld = Context.World();
				if (Context.WorldType != EWorldType::Editor || !EditorWorld)
				{
					continue;
				}
				for (TActorIterator<APawn> It(EditorWorld); It; ++It)
				{
					if (It->GetClass() != Actors.Vehicle->GetClass() ||
						It->GetActorLabel() != Actors.Vehicle->GetActorLabel())
					{
						continue;
					}
					const UOWSStockVehicleInteractionComponent* EditorInteraction =
						It->FindComponentByClass<UOWSStockVehicleInteractionComponent>();
					const UPrimitiveComponent* EditorBody = EditorInteraction
						? EditorInteraction->GetVehiclePhysicsBody()
						: nullptr;
					Actors.Vehicle->SetActorTransform(
						It->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
					if (EditorBody)
					{
						Actors.VehicleBody->SetWorldTransform(
							EditorBody->GetComponentTransform(), false, nullptr,
							ETeleportType::TeleportPhysics);
					}
					Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
					Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
					return true;
				}
			}
			return false;
		}

		static constexpr const TCHAR* RepresentativeVehicleClasses[] = {
			TEXT("DefaultVehicle"),
			TEXT("DemoChassis"),
			TEXT("HyperCar"),
			TEXT("SportsCar"),
			TEXT("MuscleCar"),
			TEXT("Sedan_C"),
			TEXT("SUV"),
			TEXT("SedanEV"),
			TEXT("GT3"),
			TEXT("DriftCar"),
			TEXT("TCR"),
			TEXT("Rally"),
			TEXT("Bus"),
			TEXT("RC_Car")
		};

		FAutomationTestBase& Test;
		FTestActors Actors;
		double StartedAt;
		double PhaseStartedAt = 0.0;
		FName EnteredDoorId = NAME_None;
		int32 VehicleIndex = 0;
		bool bEntryPrepared = false;
		bool bEntered = false;
	};

	class FConstrainedStoppedExitCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FConstrainedStoppedExitCommand(FAutomationTestBase& InTest)
			: Test(InTest), StartedAt(FPlatformTime::Seconds())
		{
		}

		virtual ~FConstrainedStoppedExitCommand() override
		{
			DestroyBlockers();
			if (Actors.IsValid() && Actors.Controller->GetPawn() == Actors.Vehicle.Get())
			{
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				Actors.VehicleInteraction->DebugExitVehicle();
			}
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
						Test.AddError(TEXT("Timed out resolving actors for constrained exit testing."));
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
				if (!PrepareEntry())
				{
					return true;
				}
				break;
			case 1:
				if (!CompleteEntry())
				{
					return true;
				}
				break;
			case 2:
				if (!SpawnExitBlockers(false))
				{
					return true;
				}
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				if (!Test.TestTrue(TEXT("Driver-side obstruction uses a stopped-exit fallback"),
						Actors.VehicleInteraction->DebugExitVehicle()) ||
					!AssertStoppedExit(Test, Actors))
				{
					return true;
				}
				Test.TestTrue(TEXT("Driver-side obstruction selects the passenger side"),
					FVector::DotProduct(
						Actors.Character->GetActorLocation() - Actors.Vehicle->GetActorLocation(),
						Actors.Vehicle->GetActorRightVector()) > 0.0f);
				DestroyBlockers();
				break;
			case 3:
				if (!PrepareEntry())
				{
					return true;
				}
				break;
			case 4:
				if (!CompleteEntry() || !SpawnExitBlockers(true))
				{
					return true;
				}
				break;
			case 5:
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				if (!Test.TestFalse(TEXT("A fully blocked stopped exit is rejected"),
						Actors.VehicleInteraction->DebugExitVehicle()) ||
					!AssertEntered(Test, Actors))
				{
					return true;
				}
				DestroyBlockers();
				break;
			case 6:
				if (!Test.TestTrue(TEXT("Stopped exit succeeds after blockers are removed"),
						Actors.VehicleInteraction->DebugExitVehicle()) ||
					!AssertStoppedExit(Test, Actors))
				{
					return true;
				}
				return true;
			default:
				return true;
			}

			++Phase;
			PhaseStartedAt = Now;
			return false;
		}

	private:
		bool PrepareEntry()
		{
			Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			if (!Actors.PlaceCharacterAtDriverDoor())
			{
				Test.AddError(TEXT("Could not place the character for constrained exit testing."));
				return false;
			}
			return true;
		}

		bool CompleteEntry()
		{
			if (!Actors.PlaceCharacterAtDriverDoor())
			{
				return false;
			}
			return Test.TestTrue(TEXT("Constrained-exit setup entry succeeds"),
					Actors.VehicleInteraction->DebugEnterNearestVehicle()) &&
				AssertEntered(Test, Actors);
		}

		AActor* SpawnBlocker(const FVector& Location, const FVector& Extent) const
		{
			AActor* Blocker = Actors.World->SpawnActor<AActor>();
			if (!Blocker)
			{
				return nullptr;
			}
			UBoxComponent* Box = NewObject<UBoxComponent>(Blocker);
			if (!Box)
			{
				Blocker->Destroy();
				return nullptr;
			}
			Blocker->AddInstanceComponent(Box);
			Blocker->SetRootComponent(Box);
			Box->SetMobility(EComponentMobility::Movable);
			Box->SetBoxExtent(Extent);
			Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box->SetCollisionObjectType(ECC_WorldDynamic);
			Box->SetCollisionResponseToAllChannels(ECR_Ignore);
			Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
			Box->SetGenerateOverlapEvents(false);
			Box->RegisterComponent();
			Blocker->SetActorLocationAndRotation(
				Location, Actors.Vehicle->GetActorRotation(), false, nullptr,
				ETeleportType::TeleportPhysics);
			return Blocker;
		}

		bool SpawnExitBlockers(const bool bBlockAllCandidates)
		{
			DestroyBlockers();
			TArray<FName> DoorIds;
			Actors.StockInteraction->GetDoorIds(DoorIds);
			if (DoorIds.Num() < 2)
			{
				Test.AddError(TEXT("The constrained-exit vehicle needs at least two configured doors."));
				return false;
			}
			const FVector BlockerExtent(55.0f, 55.0f, 160.0f);
			FTransform ExitTransform;
			if (!Actors.StockInteraction->GetDoorExitWorldTransform(DoorIds[0], ExitTransform))
			{
				Test.AddError(TEXT("The preferred constrained-exit door has no exit transform."));
				return false;
			}
			Blockers.Add(SpawnBlocker(ExitTransform.GetLocation(), BlockerExtent));
			if (bBlockAllCandidates)
			{
				for (int32 DoorIndex = 1; DoorIndex < DoorIds.Num(); ++DoorIndex)
				{
					if (!Actors.StockInteraction->GetDoorExitWorldTransform(
							DoorIds[DoorIndex], ExitTransform))
					{
						Test.AddError(TEXT("A constrained-exit fallback door has no exit transform."));
						return false;
					}
					Blockers.Add(SpawnBlocker(ExitTransform.GetLocation(), BlockerExtent));
				}
			}
			for (AActor* Blocker : Blockers)
			{
				if (!Blocker)
				{
					Test.AddError(TEXT("Could not spawn a constrained-exit blocker."));
					return false;
				}
			}
			return true;
		}

		void DestroyBlockers()
		{
			for (AActor* Blocker : Blockers)
			{
				if (IsValid(Blocker))
				{
					Blocker->Destroy();
				}
			}
			Blockers.Reset();
		}

		FAutomationTestBase& Test;
		FTestActors Actors;
		TArray<TObjectPtr<AActor>> Blockers;
		double StartedAt;
		double PhaseStartedAt = 0.0;
		int32 Phase = 0;
	};

	class FMovingBailoutCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FMovingBailoutCommand(FAutomationTestBase& InTest)
			: Test(InTest), StartedAt(FPlatformTime::Seconds())
		{
		}

		virtual ~FMovingBailoutCommand() override
		{
			if (Actors.IsValid() && Actors.Controller->GetPawn() == Actors.Vehicle.Get())
			{
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				Actors.VehicleInteraction->DebugExitVehicle();
			}
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
					Actors.Vehicle->GetActorForwardVector() * BailoutSpeeds[SpeedIndex]);
				++Phase;
				break;
			case 2:
				// Re-apply the requested speed immediately before exiting. Chaos may
				// otherwise scrub enough velocity during the inter-phase wait to move
				// a boundary test into the neighboring bailout band.
				Actors.VehicleBody->SetPhysicsLinearVelocity(
					Actors.Vehicle->GetActorForwardVector() * BailoutSpeeds[SpeedIndex]);
				if (!Test.TestTrue(
						FString::Printf(TEXT("Moving bailout cycle %d succeeds at %.0f cm/s"),
							SpeedIndex + 1, BailoutSpeeds[SpeedIndex]),
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
				if (Actors.Character->GetMesh()->IsSimulatingPhysics() ||
					Actors.Controller->IsMoveInputIgnored())
				{
					const float SpeedMph = BailoutSpeeds[SpeedIndex] * 0.0223694f;
					if (Actors.Character->GetCharacterMovement() &&
						Actors.Character->GetCharacterMovement()->Velocity.Size2D() >
							BailoutSpeeds[SpeedIndex] + 100.0f)
					{
						Test.AddError(TEXT("Controlled bailout generated velocity above the inherited vehicle speed."));
						return true;
					}
					if (!bCheckedControlledAlignment && SpeedMph > 15.0f)
					{
						bCheckedControlledAlignment = true;
						Test.TestTrue(
							TEXT("Controlled roll mesh remains aligned with the camera-followed capsule"),
							FVector::Dist2D(
								Actors.Character->GetMesh()->GetComponentLocation(),
								Actors.Character->GetActorLocation()) < 100.0f);
					}
					if (!bCheckedHighSpeedRollLoop && SpeedMph > 50.0f &&
						Now - RecoveryStartedAt >= 1.75)
					{
						bCheckedHighSpeedRollLoop = true;
						Test.TestTrue(
							TEXT("High-speed controlled roll loops beyond one roll cycle"),
							Actors.Character->GetMesh()->GetAnimationMode() ==
								EAnimationMode::AnimationSingleNode);
					}
					const double RecoveryLimit = 8.0;
					if (Now - RecoveryStartedAt > RecoveryLimit)
					{
						Test.AddError(FString::Printf(
							TEXT("Bailout cycle %d did not recover within its configured limit."),
							SpeedIndex + 1));
						return true;
					}
					return false;
				}
				if (!AssertRecovered())
				{
					return true;
				}
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Actors.VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				Actors.PlaceCharacterAtDriverDoor();
				if (!Test.TestTrue(TEXT("Re-entry succeeds after bailout recovery"),
						Actors.VehicleInteraction->DebugEnterNearestVehicle()) ||
					!AssertEntered(Test, Actors))
				{
					return true;
				}
				++Phase;
				break;
			case 4:
				Actors.VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				if (!Test.TestTrue(TEXT("Cleanup stopped exit succeeds"),
						Actors.VehicleInteraction->DebugExitVehicle()) ||
					!AssertStoppedExit(Test, Actors))
				{
					return true;
				}
				++SpeedIndex;
				bCheckedControlledAlignment = false;
				bCheckedHighSpeedRollLoop = false;
				if (SpeedIndex >= UE_ARRAY_COUNT(BailoutSpeeds))
				{
					return true;
				}
				Phase = 0;
				break;
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
			const float SpeedMph = BailoutSpeeds[SpeedIndex] * 0.0223694f;
			bPassed &= Test.TestTrue(TEXT("The same character is possessed after bailout"),
				Actors.Controller->GetPawn() == Character);
			bPassed &= Test.TestFalse(TEXT("The character is visible after bailout"),
				Character->IsHidden());
			bPassed &= Test.TestFalse(TEXT("Moving bailouts do not simulate ragdoll physics"),
				Mesh && Mesh->IsSimulatingPhysics());
			bPassed &= Test.TestFalse(TEXT("Controlled bailouts retain capsule collision"),
				Character->GetCapsuleComponent() &&
				Character->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			if (SpeedMph > 15.0f)
			{
				bPassed &= Test.TestTrue(TEXT("Controlled roll temporarily ignores movement input"),
					Actors.Controller->IsMoveInputIgnored());
				bPassed &= Test.TestTrue(TEXT("Controlled roll takes root-locked full-body animation control"),
					Mesh && Mesh->GetAnimationMode() == EAnimationMode::AnimationSingleNode);
			}
			else
			{
				bPassed &= Test.TestFalse(TEXT("Lower-speed bailout leaves movement input enabled"),
					Actors.Controller->IsMoveInputIgnored());
				bPassed &= Test.TestFalse(TEXT("Lower-speed bailout does not force a full-body animation"),
					Mesh && Mesh->GetAnimationMode() == EAnimationMode::AnimationSingleNode);
			}
			bPassed &= Test.TestFalse(TEXT("Controlled bailout animation cannot inject root motion"),
				Character->IsPlayingRootMotion());
			bPassed &= Test.TestTrue(TEXT("The control seat is released during bailout"),
				Actors.StockInteraction->GetSeatForOccupant(Character) == NAME_None);
			bPassed &= Test.TestFalse(TEXT("The vehicle input context is removed during bailout"),
				Actors.HasVehicleContext());
			bPassed &= Test.TestTrue(TEXT("The camera targets the bailed-out character"),
				Actors.Controller->GetViewTarget() == Character);
			bPassed &= Test.TestFalse(TEXT("The bailout transform remains finite"),
				Character->GetActorLocation().ContainsNaN());
			const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
			bPassed &= Test.TestFalse(TEXT("The bailout capsule does not overlap the vehicle body"),
				Capsule && Actors.VehicleBody->OverlapComponent(
					Character->GetActorLocation(), Character->GetActorQuat(),
					FCollisionShape::MakeCapsule(
						Capsule->GetScaledCapsuleRadius(),
						Capsule->GetScaledCapsuleHalfHeight())));
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
			bPassed &= Test.TestFalse(TEXT("Movement input is restored after recovery"),
				Actors.Controller->IsMoveInputIgnored());
			bPassed &= Test.TestTrue(TEXT("The recovered transform remains finite"),
				!Character->GetActorLocation().ContainsNaN());
			return bPassed;
		}

		FAutomationTestBase& Test;
		inline static constexpr float BailoutSpeeds[] = {
			600.0f, 894.08f, 1200.0f, 3000.0f, 1200.0f, 600.0f
		};
		FTestActors Actors;
		double StartedAt;
		double PhaseStartedAt = 0.0;
		double RecoveryStartedAt = 0.0;
		int32 Phase = 0;
		int32 SpeedIndex = 0;
		bool bCheckedControlledAlignment = false;
		bool bCheckedHighSpeedRollLoop = false;
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
	FOWSRepresentativeStoppedExitTest,
	"OWS.CharacterVehicle.RepresentativeStoppedExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOWSRepresentativeStoppedExitTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(OWSCharacterVehicleTests::MapPath))
	{
		AddError(TEXT("Failed to open the canonical OWS map for representative stopped exits."));
		return false;
	}
	AddCommand(new OWSCharacterVehicleTests::FRepresentativeStoppedExitCommand(*this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOWSConstrainedStoppedExitTest,
	"OWS.CharacterVehicle.ConstrainedStoppedExitPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOWSConstrainedStoppedExitTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(OWSCharacterVehicleTests::MapPath))
	{
		AddError(TEXT("Failed to open the canonical OWS map for constrained stopped exits."));
		return false;
	}
	AddCommand(new OWSCharacterVehicleTests::FConstrainedStoppedExitCommand(*this));
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
