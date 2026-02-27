// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "C_MonsterASC.h"
#include "AbilitySystemGlobals.h"
#include "Net/UnrealNetwork.h"

UC_MonsterAttributeSet::UC_MonsterAttributeSet()
{
}

#pragma region onRep functions
void UC_MonsterAttributeSet::OnRep_CurHP(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, curHP, OldValue);
}

void UC_MonsterAttributeSet::OnRep_MaxHP(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, maxHP, OldValue);
}

void UC_MonsterAttributeSet::OnRep_Groggy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, curGroggy, OldValue);
}

void UC_MonsterAttributeSet::OnRep_MaxGroggy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, maxGroggy, OldValue);
}

void UC_MonsterAttributeSet::OnRep_Attack(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, attack, OldValue);
}

void UC_MonsterAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, defense, OldValue);
}

void UC_MonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, moveSpeed, OldValue);
}

void UC_MonsterAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, attackRange, OldValue);
}

void UC_MonsterAttributeSet::OnRep_NormalCooldown(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, normalCooldown, OldValue);
}

void UC_MonsterAttributeSet::OnRep_SpecialCooldown(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UC_MonsterAttributeSet, specialCooldown, OldValue);
}

#pragma endregion

void UC_MonsterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, curHP,           COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, maxHP,           COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, curGroggy,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, maxGroggy,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, attack,          COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, defense,         COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, moveSpeed,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, attackRange,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, normalCooldown,  COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UC_MonsterAttributeSet, specialCooldown, COND_None, REPNOTIFY_Always);
}

void UC_MonsterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    auto NonNegative = [](float& V) { V = FMath::Max(V, 0.f); };

    if (Attribute == GetmaxHPAttribute())
        NewValue = FMath::Max(NewValue, 1.f);
    else if (Attribute == GetcurHPAttribute())       
        NonNegative(NewValue);
    else if (Attribute == GetmaxGroggyAttribute())    
        NewValue = FMath::Max(NewValue, 1.f);
    else if (Attribute == GetcurGroggyAttribute())       
        NonNegative(NewValue);
    else if (Attribute == GetattackAttribute())      
        NonNegative(NewValue);
    else if (Attribute == GetdefenseAttribute())     
        NonNegative(NewValue);
    else if (Attribute == GetmoveSpeedAttribute())    
        NonNegative(NewValue);
    else if (Attribute == GetattackRangeAttribute())  
        NonNegative(NewValue);
    else if (Attribute == GetnormalCooldownAttribute() || Attribute == GetspecialCooldownAttribute()) 
        NonNegative(NewValue);
}

void UC_MonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
 //////////////데미지 처리////////////////////////////////////////////////////////////////////
    if (Data.EvaluatedData.Attribute == GetReceivedDamageAttribute())
    {
        const float RawDamage = GetReceivedDamage();

        const float Mitigated = FMath::Max(0.0f, RawDamage - Getdefense()); // 방어력 반영

        const float NewHP = FMath::Clamp(GetcurHP() - Mitigated, 0.f, GetmaxHP());
        SetcurHP(NewHP);

		UE_LOG(LogTemp, Warning, TEXT("[Monster] HP: %.1f (Damage: %.1f)"),
			NewHP, Mitigated);

        SetReceivedDamage(0.0f);
        return;
    }
///////////////////////////////////////////////////////////////////////////////////////////////
    if (Data.EvaluatedData.Attribute == GetmaxHPAttribute())
    {
        SetcurHP(FMath::Clamp(GetcurHP(), 0.0f, GetmaxHP()));
    }
    else if (Data.EvaluatedData.Attribute == GetcurHPAttribute())
    {
        const float NewHP = GetcurHP();
        SetcurHP(FMath::Clamp(NewHP, 0.f, GetmaxHP()));

        if (NewHP <= 0.f)
        {
            // ASC 찾아서 HandleDeath 호출
            if (AActor* Owner = GetOwningActor())
            {
                if (UAbilitySystemComponent* ASCBase = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
                {
                    if (UC_MonsterASC* monsterASC = Cast<UC_MonsterASC>(ASCBase))
                    {
                        monsterASC->HandleDeath();
                    }
                }
            }
        }
    }
    else if (Data.EvaluatedData.Attribute == GetmaxGroggyAttribute())
    {
        SetcurGroggy(FMath::Clamp(GetcurGroggy(), 0.0f, GetmaxGroggy()));
    }
    else if (Data.EvaluatedData.Attribute == GetcurGroggyAttribute())
    {
        SetcurGroggy(FMath::Clamp(GetcurGroggy(), 0.0f, GetmaxGroggy()));

    }
}
