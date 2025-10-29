// Fill out your copyright notice in the Description page of Project Settings.


#include "GGBaseSet.h"

void UGGBaseSet::ClampAttributeOnChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
}

void UGGBaseSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttributeOnChange(Attribute, NewValue);
}

void UGGBaseSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	ClampAttributeOnChange(Attribute, NewValue);
}
