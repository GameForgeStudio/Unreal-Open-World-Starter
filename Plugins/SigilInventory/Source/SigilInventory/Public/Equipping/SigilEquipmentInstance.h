// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "SigilEquipmentInterface.h"
#include "SigilEquipmentStructLibrary.h"
#include "SigilEquipmentInstance.generated.h"

class USigilEquipmentSystemComponent;
class AActor;
class APawn;
struct FFrame;
struct FGEquipmentActorToSpawn;

/**
 * Delegate triggered when the active state of the equipment instance changes.
 * 当装备实例的激活状态改变时触发的委托。
 * @param bNewState The new active state of the equipment. 装备的新激活状态。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSigilActiveStateChangedSignature, bool, bNewState);

/**
 * An equipment instance is a UObject tasked with managing the internal logic and runtime states of equipment.
 * 装备实例是一个UObject，负责管理装备的内部逻辑和运行时状态。
 * @attention This is the default implementation of EquipmentInterface. You can use other types of classes as equipment.
 * @注意 这是EquipmentInterface的默认实现，你可以使用其他类作为装备实例。
 */
UCLASS(BlueprintType, Blueprintable)
class SIGILINVENTORY_API USigilEquipmentInstance : public UObject, public ISigilEquipmentInterface
{
	GENERATED_BODY()

public:
	/**
	 * Constructor for the equipment instance.
	 * 装备实例的构造函数。
	 * @param ObjectInitializer The object initializer. 对象初始化器。
	 */
	USigilEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	/**
	 * Checks if the equipment instance supports networking.
	 * 检查装备实例是否支持网络。
	 * @return True if networking is supported, false otherwise. 如果支持网络则返回true，否则返回false。
	 */
	virtual bool IsSupportedForNetworking() const override;

	/**
	 * Gets the world this equipment instance belongs to.
	 * 获取装备实例所属的世界。
	 * @return The world, or nullptr if not set. 世界，如果未设置则返回nullptr。
	 */
	virtual UWorld* GetWorld() const override final;

	/**
	 * Gets the properties that should be replicated for this object.
	 * 获取需要为此对象复制的属性。
	 * @param OutLifetimeProps Array to store the replicated properties. 存储复制属性的数组。
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of UObject interface

	//~ISigilEquipmentInterface interface
	/**
	 * Sets the owning pawn for this equipment instance.
	 * 设置此装备实例的所属Pawn。
	 * @param NewPawn The new owning pawn. 新的所属Pawn。
	 */
	virtual void ReceiveOwningPawn_Implementation(APawn* NewPawn) override;

	/**
	 * Gets the owning pawn of this equipment instance.
	 * 获取此装备实例的所属Pawn。
	 * @return The owning pawn, or nullptr if not set. 所属Pawn，如果未设置则返回nullptr。
	 */
	virtual APawn* GetOwningPawn_Implementation() const override;

	/**
	 * Sets the source item for this equipment instance.
	 * 设置此装备实例的源道具。
	 * @param NewItem The new source item. 新的源道具。
	 */
	virtual void ReceiveSourceItem_Implementation(USigilItemInstance* NewItem) override;

	/**
	 * Gets the source item associated with this equipment instance.
	 * 获取与此装备实例关联的源道具。
	 * @return The source item, or nullptr if not set. 源道具，如果未设置则返回nullptr。
	 */
	virtual USigilItemInstance* GetSourceItem_Implementation() const override;

	/**
	 * Called when the equipment instance begins play.
	 * 装备实例开始运行时调用。
	 */
	virtual void OnEquipmentBeginPlay_Implementation() override;

	/**
	 * Called when the equipment instance ends play.
	 * 装备实例结束运行时调用。
	 */
	virtual void OnEquipmentEndPlay_Implementation() override;

	/**
	 * Checks if the equipment instance is currently active.
	 * 检查装备实例当前是否处于激活状态。
	 * @return True if the equipment is active, false otherwise. 如果装备处于激活状态则返回true，否则返回false。
	 */
	virtual bool IsEquipmentActive_Implementation() const override;

	/**
	 * Called when the active state of the equipment changes.
	 * 装备激活状态改变时调用。
	 * @param NewActiveState The new active state. 新的激活状态。
	 */
	virtual void OnActiveStateChanged_Implementation(bool NewActiveState) override;
	//~End of ISigilEquipmentInterface interface

	/**
	 * Gets the owning pawn cast to a specific type.
	 * 获取转换为特定类型的所属Pawn。
	 * @param PawnType The desired pawn class. 期望的Pawn类。
	 * @return The owning pawn cast to the specified type, or nullptr if not valid. 转换为指定类型的所属Pawn，如果无效则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|EquipmentInstance", meta=(DeterminesOutputType=PawnType))
	APawn* GetTypedOwningPawn(TSubclassOf<APawn> PawnType) const;

	/**
	 * Determines if the equipment can be activated. Override in Blueprint for custom logic.
	 * 判断装备是否可以激活，可在蓝图中重写以实现自定义逻辑。
	 * @return True if the equipment can be activated, false otherwise. 如果装备可以激活则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintNativeEvent, Category="GIS|EquipmentInstance")
	bool CanActivate() const;

	/**
	 * Gets all actors spawned by this equipment instance.
	 * 获取由此装备实例生成的所有Actor。
	 * @return Array of spawned actors. 生成的Actor数组。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|EquipmentInstance")
	TArray<AActor*> GetEquipmentActors() const { return EquipmentActors; }

	/**
	 * Gets the first spawned actor matching the desired class (including subclasses).
	 * 获取第一个匹配指定类型（包括子类）的由装备实例生成的Actor。
	 * @param DesiredClass The desired actor class. 期望的Actor类。
	 * @return The matching actor, or nullptr if not found. 匹配的Actor，如果未找到则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GIS|EquipmentInstance", meta=(DeterminesOutputType="DesiredClass", DynamicOutputParam="ReturnValue"))
	AActor* GetTypedEquipmentActor(TSubclassOf<AActor> DesiredClass) const;

	/**
	 * Event triggered when the active state of the equipment changes.
	 * 装备激活状态改变时触发的事件。
	 */
	UPROPERTY(BlueprintAssignable, Category="EquipmentInstance")
	FSigilActiveStateChangedSignature OnActiveStateChangedEvent;

protected:
#if UE_WITH_IRIS
	/**
	 * Registers replication fragments for networking (Iris-specific).
	 * 为网络注册复制片段（特定于Iris）。
	 * @param Context The fragment registration context. 片段注册上下文。
	 * @param RegistrationFlags The registration flags. 注册标志。
	 */
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS

	/**
	 * Gets the scene component to which spawned actors will attach.
	 * 获取生成Actor将附加到的场景组件。
	 * @param Pawn The pawn owning this equipment instance. 拥有此装备实例的Pawn。
	 * @return The scene component to attach to, or nullptr if not applicable. 要附加到的场景组件，如果不适用则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category="GIS|EquipmentInstance")
	USceneComponent* GetAttachParentForSpawnedActors(APawn* Pawn) const;

#pragma region Equipment Actors
	/**
	 * Spawns and sets up actors associated with this equipment instance.
	 * 生成并设置与此装备实例关联的Actor。
	 * @param ActorsToSpawn The actors to spawn. 要生成的Actor。
	 */
	virtual void SpawnAndSetupEquipmentActors(const TArray<FSigilEquipmentActorToSpawn>& ActorsToSpawn);

	/**
	 * Destroys all actors associated with this equipment instance.
	 * 销毁与此装备实例关联的所有Actor。
	 */
	virtual void DestroyEquipmentActors();

	/**
	 * Allows custom logic to spawn associated actors (commented out).
	 * 允许使用自定义逻辑生成关联的Actor（已注释）。
	 */
	// UFUNCTION(BlueprintImplementableEvent, BlueprintPure=false, Category="GIS|EquipmentInstance", meta=(DisplayName="SpawnEquipmentActors"))
	// bool K2_SpawnEquipmentActors(const TArray<FSigilEquipmentActorToSpawn>& ActorsToSpawn, TArray<AActor*>& OutActors) const;

	/**
	 * Called before an actor is spawned to allow additional setup.
	 * 在Actor生成前调用以允许额外设置。
	 * @param SpawningActor The actor about to be spawned. 即将生成的Actor。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|EquipmentInstance")
	void BeforeSpawningActor(AActor* SpawningActor) const;

	/**
	 * Sets up actors after they have been spawned.
	 * 在Actor生成后进行设置。
	 * @param InActors The spawned actors to configure. 已生成的Actor，需进行配置。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="GIS|EquipmentInstance")
	void SetupEquipmentActors(const TArray<AActor*>& InActors) const;

	/**
	 * Implementation of SetupEquipmentActors.
	 * SetupEquipmentActors 的实现。
	 * @param InActors The spawned actors to configure. 已生成的Actor，需进行配置。
	 */
	virtual void SetupEquipmentActors_Implementation(const TArray<AActor*>& InActors) const;

	/**
	 * Called when the equipment actors are replicated.
	 * 装备Actor复制时调用。
	 */
	UFUNCTION()
	void OnRep_EquipmentActors();

	/**
	 * Attempts to wait for equipment actors to be ready.
	 * 尝试等待装备Actor准备就绪。
	 */
	void TryWaitEquipmentActors();

	/**
	 * Waits for equipment actors to be ready.
	 * 等待装备Actor准备就绪。
	 */
	void WaitEquipmentActors();

	/**
	 * Checks if the specified number of equipment actors is valid.
	 * 检查指定数量的装备Actor是否有效。
	 * @param Num The number of actors to check. 要检查的Actor数量。
	 * @return True if the number of actors is valid, false otherwise. 如果Actor数量有效则返回true，否则返回false。
	 */
	bool IsEquipmentActorsValid(int32 Num) const;

	/**
	 * Propagates initial state to all equipment actors.
	 * 将初始状态传播到所有装备Actor。
	 * @param InActors The actors to set up. 要设置的Actor。
	 */
	virtual void SetupInitialStateForEquipmentActors(const TArray<AActor*>& InActors) const;

	/**
	 * Propagates active state to all equipment actors.
	 * 将激活状态传播到所有装备Actor。
	 * @param InActors The actors to set up. 要设置的Actor。
	 */
	virtual void SetupActiveStateForEquipmentActors(const TArray<AActor*>& InActors) const;

#pragma endregion

	/**
	 * The pawn that owns this equipment instance.
	 * 拥有此装备实例的Pawn。
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="EquipmentInstance")
	TObjectPtr<APawn> OwningPawn;

	/**
	 * The source item associated with this equipment instance.
	 * 与此装备实例关联的源道具。
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="EquipmentInstance")
	TObjectPtr<USigilItemInstance> SourceItem;

	/**
	 * Indicates whether the equipment instance is currently active.
	 * 指示装备实例当前是否处于激活状态。
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="EquipmentInstance")
	bool bIsActive;

	/**
	 * Array of actors spawned by this equipment instance.
	 * 由此装备实例生成的Actor数组。
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="EquipmentInstance", ReplicatedUsing=OnRep_EquipmentActors)
	TArray<TObjectPtr<AActor>> EquipmentActors;

	/**
	 * Timer handle for waiting on equipment actors.
	 * 等待装备Actor的定时器句柄。
	 */
	FTimerHandle WaitEquipmentActorsTimer;

	/**
	 * Counter for tracking wait attempts for equipment actors.
	 * 用于跟踪等待装备Actor尝试的计数器。
	 */
	int32 WaitCounter = 0;
};
