// Copyright (c) 2026 Likeon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"
#include "SigilCurrencyEntry.generated.h"

class USigilCurrencyDefinition;

/**
 * Represents a currency and its associated amount.
 * 表示一种货币及其数量。
 */
USTRUCT(BlueprintType)
struct SIGILINVENTORY_API FSigilCurrencyEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/**
	 * Default constructor for the currency entry.
	 * 货币条目的默认构造函数。
	 */
	FSigilCurrencyEntry();

	/**
	 * Constructor for the currency entry with specified definition and amount.
	 * 使用指定货币定义和数量构造货币条目。
	 * @param InDefinition The currency definition. 货币定义。
	 * @param InAmount The amount of the currency. 货币数量。
	 */
	FSigilCurrencyEntry(const TObjectPtr<const USigilCurrencyDefinition>& InDefinition, float InAmount);

	/**
	 * Constructor for the currency entry with specified amount and definition (alternative order).
	 * 使用指定数量和货币定义构造货币条目（参数顺序相反）。
	 * @param InAmount The amount of the currency. 货币数量。
	 * @param InDefinition The currency definition. 货币定义。
	 */
	FSigilCurrencyEntry(float InAmount, const TObjectPtr<const USigilCurrencyDefinition>& InDefinition);

	/**
	 * Referenced currency definition.
	 * 引用的货币定义。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS")
	TObjectPtr<const USigilCurrencyDefinition> Definition;

	/**
	 * The amount of the currency.
	 * 货币的数量。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GIS")
	float Amount;

	/**
	 * The previous amount of the currency (not replicated).
	 * 货币的前一数量（不复制）。
	 */
	UPROPERTY(NotReplicated)
	float PrevAmount = 0;

	/**
	 * Checks if this currency entry is equal to another.
	 * 检查此货币条目是否与另一个相等。
	 * @param Other The other currency entry to compare with. 要比较的另一个货币条目。
	 * @return True if the entries are equal, false otherwise. 如果条目相等则返回true，否则返回false。
	 */
	bool Equals(const FSigilCurrencyEntry& Other) const;

	/**
	 * Converts the currency entry to a string representation.
	 * 将货币条目转换为字符串表示。
	 * @return The string representation of the currency entry. 货币条目的字符串表示。
	 */
	FString ToString() const;

	/**
	 * Equality operator to compare two currency entries.
	 * 比较两个货币条目的相等性运算符。
	 * @param Rhs The right-hand side currency entry to compare with. 要比较的右侧货币条目。
	 * @return True if the entries are equal, false otherwise. 如果条目相等则返回true，否则返回false。
	 */
	bool operator==(const FSigilCurrencyEntry& Rhs) const;

	/**
	 * Inequality operator to compare two currency entries.
	 * 比较两个货币条目的不等性运算符。
	 * @param Rhs The right-hand side currency entry to compare with. 要比较的右侧货币条目。
	 * @return True if the entries are not equal, false otherwise. 如果条目不相等则返回true，否则返回false。
	 */
	bool operator!=(const FSigilCurrencyEntry& Rhs) const;
};
