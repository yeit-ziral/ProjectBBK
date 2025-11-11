// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UC_MonsterAttributeSet::UC_MonsterAttributeSet()
{
}

#pragma region onRep functions
void UC_MonsterAttributeSet::OnRep_CurHP(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, CurHP, OldValue);
}

void UC_MonsterAttributeSet::OnRep_MaxHP(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, MaxHP, OldValue);
}

void UC_MonsterAttributeSet::OnRep_Groggy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, CurGroggy, OldValue);
}

void UC_MonsterAttributeSet::OnRep_MaxGroggy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, MaxGroggy, OldValue);
}

void UC_MonsterAttributeSet::OnRep_Attack(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, Attack, OldValue);
}

void UC_MonsterAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, Defense, OldValue);
}

void UC_MonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, MoveSpeed, OldValue);
}

void UC_MonsterAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, AttackRange, OldValue);
}

void UC_MonsterAttributeSet::OnRep_NormalCooldown(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, NormalCooldown, OldValue);
}

void UC_MonsterAttributeSet::OnRep_SpecialCooldown(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, SpecialCooldown, OldValue);
}

#pragma endregion

void UC_MonsterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, CurHP,           COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, MaxHP,           COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, CurGroggy,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, MaxGroggy,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, Attack,          COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, Defense,         COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, MoveSpeed,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, AttackRange,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, NormalCooldown,  COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, SpecialCooldown, COND_None, REPNOTIFY_Always);
}

void UC_MonsterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    auto NonNegative = [](float& V) { V = FMath::Max(V, 0.f); };

    if (Attribute == GetMaxHPAttribute())
        NewValue = FMath::Max(NewValue, 1.f);
    else if (Attribute == GetCurHPAttribute())       
        NonNegative(NewValue);
    else if (Attribute == GetMaxGroggyAttribute())    
        NewValue = FMath::Max(NewValue, 1.f);
    else if (Attribute == GetCurGroggyAttribute())       
        NonNegative(NewValue);
    else if (Attribute == GetAttackAttribute())      
        NonNegative(NewValue);
    else if (Attribute == GetDefenseAttribute())     
        NonNegative(NewValue);
    else if (Attribute == GetMoveSpeedAttribute())    
        NonNegative(NewValue);
    else if (Attribute == GetAttackRangeAttribute())  
        NonNegative(NewValue);
    else if (Attribute == GetNormalCooldownAttribute() || Attribute == GetSpecialCooldownAttribute()) 
        NonNegative(NewValue);
}

void UC_MonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetReceivedDamageAttribute())
    {
        const float RawDamage = GetReceivedDamage();

        const float Mitigated = FMath::Max(0.0f, RawDamage - GetDefense()); // 방어력 반영

        const float NewHP = FMath::Clamp(GetCurHP() - Mitigated, 0.f, GetMaxHP());
        SetCurHP(NewHP);


        SetReceivedDamage(0.0f);
        return;
    }

    if (Data.EvaluatedData.Attribute == GetMaxHPAttribute())
    {
        SetCurHP(FMath::Clamp(GetCurHP(), 0.0f, GetMaxHP()));
    }
    else if (Data.EvaluatedData.Attribute == GetCurHPAttribute())
    {
        SetCurHP(FMath::Clamp(GetCurHP(), 0.0f, GetMaxHP()));
    }
    else if (Data.EvaluatedData.Attribute == GetMaxGroggyAttribute())
    {
        SetCurGroggy(FMath::Clamp(GetCurGroggy(), 0.0f, GetMaxGroggy()));
    }
    else if (Data.EvaluatedData.Attribute == GetCurGroggyAttribute())
    {
        SetCurGroggy(FMath::Clamp(GetCurGroggy(), 0.0f, GetMaxGroggy()));

    }
}
