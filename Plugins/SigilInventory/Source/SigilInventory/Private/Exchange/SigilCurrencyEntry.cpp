// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "SigilCurrencyEntry.h"
#include "SigilCurrencyDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilCurrencyEntry)
FSigilCurrencyEntry::FSigilCurrencyEntry()
{
	Definition = nullptr;
	Amount = 0;
}

FSigilCurrencyEntry::FSigilCurrencyEntry(const TObjectPtr<const USigilCurrencyDefinition>& InDefinition, float InAmount)
{
	Definition = InDefinition;
	Amount = InAmount;
}

FSigilCurrencyEntry::FSigilCurrencyEntry(float InAmount, const TObjectPtr<const USigilCurrencyDefinition>& InDefinition)
{
	Definition = InDefinition;
	Amount = InAmount;
}

bool FSigilCurrencyEntry::Equals(const FSigilCurrencyEntry& Other) const
{
	return Amount == Other.Amount && Definition == Other.Definition;
}

FString FSigilCurrencyEntry::ToString() const
{
	return FString::Format(TEXT("{0} {1}"), {Definition ? Definition->GetName() : TEXT("None"), Amount});
}

bool FSigilCurrencyEntry::operator==(const FSigilCurrencyEntry& Rhs) const
{
	return Equals(Rhs);
}

bool FSigilCurrencyEntry::operator!=(const FSigilCurrencyEntry& Rhs) const
{
	return !Equals(Rhs);
}
