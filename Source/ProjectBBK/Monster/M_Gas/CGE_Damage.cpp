// Fill out your copyright notice in the Description page of Project Settings.


#include "CGE_Damage.h"
#include "C_MonsterAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

UCGE_Damage::UCGE_Damage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = UC_MonsterAttributeSet::GetReceivedDamageAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;

    FSetByCallerFloat SBC;
    SBC.DataTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));

    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);
    Modifiers.Add(Mod);
}
