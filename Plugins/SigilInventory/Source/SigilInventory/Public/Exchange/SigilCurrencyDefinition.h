// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "SigilCurrencyDefinition.generated.h"

class USigilCurrencyDefinition;

/**
 * Struct to represent a currency exchange rate.
 * 表示货币汇率的结构体。
 */
struct FSigilCurrencyExchangeRate
{
	/**
	 * The currency definition for the exchange rate.
	 * 汇率的货币定义。
	 */
	TObjectPtr<const USigilCurrencyDefinition> Currency;

	/**
	 * The exchange rate value.
	 * 汇率值。
	 */
	float ExchangeRate;

	/**
	 * Constructor for the currency exchange rate.
	 * 货币汇率的构造函数。
	 * @param InCurrency The currency definition. 货币定义。
	 * @param InExchangeRate The exchange rate value. 汇率值。
	 */
	FSigilCurrencyExchangeRate(const USigilCurrencyDefinition* InCurrency, float InExchangeRate);
};

/**
 * Defines properties for a currency type in the inventory system.
 * 定义库存系统中货币类型的属性。
 */
UCLASS(BlueprintType)
class SIGILINVENTORY_API USigilCurrencyDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * The display name of the currency for UI purposes.
	 * 货币的UI显示名称。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Common")
	FText DisplayName;

	/**
	 * The description of the currency for UI purposes.
	 * 货币的UI描述。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Common")
	FText Description;

	/**
	 * The icon for the currency for UI display.
	 * 货币的UI图标。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Common")
	TSoftObjectPtr<UTexture2D> Icon;

	/**
	 * The maximum amount allowed for this currency (0 for unlimited).
	 * 货币的最大数量（0表示无限制）。
	 * @details If the amount exceeds this value, it will attempt to convert to another currency.
	 * @细节 如果数量超过此值，将尝试转换为其他货币。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Common", meta=(ClampMin=0))
	float MaxAmount{0};

	/**
	 * The parent currency used to compute exchange rates.
	 * 用于计算汇率的父级货币。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exchange")
	TObjectPtr<const USigilCurrencyDefinition> ParentCurrency;

	/**
	 * The exchange rate to the parent currency (e.g., 100 cents = 1 dollar).
	 * 相对于父级货币的汇率（例如，100分=1美元）。
	 * @details If this currency is a 10-unit note and the parent is a 100-unit note, set to 10 for 1:10 exchange.
	 * @细节 如果此货币是10元钞，父级是100元钞，设置值为10表示1个100元钞可兑换10个10元钞。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exchange", meta=(ClampMin=1))
	float ExchangeRateToParent{1};

	/**
	 * The currency to convert to when this currency exceeds MaxAmount.
	 * 当货币数量超过最大值时转换到的溢出货币。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exchange")
	TObjectPtr<const USigilCurrencyDefinition> OverflowCurrency;

	/**
	 * The currency to convert fractional remainders to.
	 * 分数余数转换到的分数货币。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exchange")
	TObjectPtr<const USigilCurrencyDefinition> FractionCurrency;

	/**
	 * Attempts to get the exchange rate to another currency.
	 * 尝试获取针对其他货币的汇率。
	 * @param OtherCurrency The currency to compare with. 要比较的其他货币。
	 * @param ExchangeRate The resulting exchange rate (output). 输出的汇率。
	 * @return True if the exchange rate was found, false otherwise. 如果找到汇率则返回true，否则返回false。
	 * @details Returns the rate as "1 this currency = ExchangeRate other currency".
	 * @细节 以“1个当前货币 = 汇率个其他货币”的形式返回汇率。
	 */
	bool TryGetExchangeRateTo(const USigilCurrencyDefinition* OtherCurrency, double& ExchangeRate) const;

	/**
	 * Gets the exchange rate to the root currency.
	 * 获取相对于根货币的汇率。
	 * @param AdditionalExchangeRate An external exchange rate for recursive calculations. 用于递归计算的外部汇率。
	 * @return The exchange rate to the root currency. 相对于根货币的汇率。
	 */
	FSigilCurrencyExchangeRate GetRootExchangeRate(double AdditionalExchangeRate = 1) const;
};
