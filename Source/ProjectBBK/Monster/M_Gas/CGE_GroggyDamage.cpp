// Fill out your copyright notice in the Description page of Project Settings.

#include "CGE_GroggyDamage.h"
#include "C_MonsterAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

UCGE_GroggyDamage::UCGE_GroggyDamage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = UC_MonsterAttributeSet::GetReceivedGroggyAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;

    FSetByCallerFloat SBC;
    SBC.DataTag = FGameplayTag::RequestGameplayTag(FName("Data.Groggy"));

    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);
    Modifiers.Add(Mod);
}
