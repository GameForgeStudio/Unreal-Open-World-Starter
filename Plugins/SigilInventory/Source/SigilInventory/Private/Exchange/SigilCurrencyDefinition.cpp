// Copyright (c) 2026 Likeon. All Rights Reserved.


#include "Exchange/SigilCurrencyDefinition.h"

#include "SigilInventorySubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SigilCurrencyDefinition)

FSigilCurrencyExchangeRate::FSigilCurrencyExchangeRate(const USigilCurrencyDefinition* InCurrency, float InExchangeRate)
{
	Currency = InCurrency;
	ExchangeRate = InExchangeRate;
}


bool USigilCurrencyDefinition::TryGetExchangeRateTo(const USigilCurrencyDefinition* OtherCurrency, double& ExchangeRate) const
{
	if (OtherCurrency == nullptr)
	{
		ExchangeRate = -1;
		return false;
	}

	//same currency
	if (OtherCurrency == this)
	{
		ExchangeRate = 1;
		return true;
	}

	const FSigilCurrencyExchangeRate RootExchangeRate = GetRootExchangeRate();
	const FSigilCurrencyExchangeRate OtherRootExchangeRate = OtherCurrency->GetRootExchangeRate();
	if (RootExchangeRate.Currency != OtherRootExchangeRate.Currency)
	{
		ExchangeRate = -1;
		return false;
	}

	ExchangeRate = RootExchangeRate.ExchangeRate / OtherRootExchangeRate.ExchangeRate;
	return true;
}

FSigilCurrencyExchangeRate USigilCurrencyDefinition::GetRootExchangeRate(double AdditionalExchangeRate) const
{
	if (ParentCurrency)
	{
		return ParentCurrency->GetRootExchangeRate(AdditionalExchangeRate * ExchangeRateToParent);
	}
	return FSigilCurrencyExchangeRate(this, AdditionalExchangeRate);
}
