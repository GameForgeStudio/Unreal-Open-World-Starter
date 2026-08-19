// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SigilDropperComponent.h"
#include "SigilCurrencyDropper.generated.h"

class USigilCurrencySystemComponent;

/**
 * Component for handling currency drop logic.
 * 处理货币掉落逻辑的组件。
 */
UCLASS(ClassGroup=(GIS), meta=(BlueprintSpawnableComponent))
class SIGILINVENTORY_API USigilCurrencyDropper : public USigilDropperComponent
{
	GENERATED_BODY()

public:
	/**
	 * Initializes the currency dropper component at the start of play.
	 * 在游戏开始时初始化货币掉落组件。
	 */
	virtual void BeginPlay() override;

	/**
	 * Executes the currency drop logic.
	 * 执行货币掉落逻辑。
	 */
	virtual void Drop() override;

protected:
	/**
	 * Reference to the currency system component.
	 * 货币系统组件的引用。
	 */
	UPROPERTY()
	USigilCurrencySystemComponent* MyCurrency;

#if WITH_EDITOR
	/**
	 * Validates the component's data in the editor.
	 * 在编辑器中验证组件的数据。
	 * @param Context The data validation context. 数据验证上下文。
	 * @return The result of the data validation. 数据验证的结果。
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};