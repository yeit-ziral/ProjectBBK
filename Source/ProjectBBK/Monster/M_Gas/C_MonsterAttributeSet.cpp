// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectExtension.h"
#include "C_MonsterASC.h"
#include "AbilitySystemGlobals.h"
#include "Net/UnrealNetwork.h"
#include "../C_BaseMonster.h"
#include "../Manager/C_GroggyComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	// 방향 기반 판정(쉴드 몬스터 가드각 등)에 쓸 "월드에 실제로 존재하는" 공격자를 뽑는다.
	//
	// 플레이어 ASC는 PlayerState 소유이므로 ASC::MakeEffectContext()가 만드는 컨텍스트의
	// Instigator는 **PlayerState**다. PlayerState는 월드에 배치되지 않아 GetActorLocation()이
	// 항상 (0,0,0) — 이걸 그대로 방향 판정에 쓰면 "월드 원점에서 맞았다"가 되어
	// 몬스터가 원점 쪽을 보고 있을 때만 우연히 막히는 무작위 동작이 된다.
	// EffectCauser(= 아바타 캐릭터 또는 투사체)가 실제 타격 위치에 해당한다.
	AActor* ResolveDamageSourceActor(const FGameplayEffectContextHandle& Context)
	{
		AActor* causer = Context.GetEffectCauser();
		if (IsValid(causer) && !causer->IsA<APlayerState>() && !causer->IsA<AController>())
			return causer;

		AActor* instigator = Context.GetInstigator();

		if (const APlayerState* playerState = Cast<APlayerState>(instigator))
		{
			if (APawn* pawn = playerState->GetPawn())
				return pawn;
		}
		if (const AController* controller = Cast<AController>(instigator))
		{
			if (APawn* pawn = controller->GetPawn())
				return pawn;
		}

		return instigator;
	}
}

UC_MonsterAttributeSet::UC_MonsterAttributeSet()
{
	ManaChargeRate = 0.2f;
	MinManaCharge = 1.0f;
	MaxManaCharge = 20.0f;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> ChargeManaFinder(
		TEXT("/Game/PlayerCharacter/Blueprint/GAS/Effects/GE_ChargeMana.GE_ChargeMana_C"));

	if (ChargeManaFinder.Succeeded())
	{
		GE_ChargeMana = ChargeManaFinder.Class;
		UE_LOG(LogTemp, Log, TEXT("[MonsterAttributeSet] GE_ChargeMana loaded successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[MonsterAttributeSet] Failed to load GE_ChargeMana"));
		UE_LOG(LogTemp, Error, TEXT("Check path: /Game/Skills/GE_ChargeMana.GE_ChargeMana_C"));
	}
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

void UC_MonsterAttributeSet::ChargeAttackerMana(const FGameplayEffectModCallbackData& Data, float ActualDamage)
{
	// 1. Instigator (공격자) 가져오기
	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = EffectContext.GetInstigator();

	if (!Instigator)
		return;

	// 2. 공격자 ASC 가져오기
	UAbilitySystemComponent* InstigatorASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator);

	if (!InstigatorASC)
		return;

	// 3. 궁극기 사용 여부 체크
	FGameplayTag UsingUltimateTag =
		FGameplayTag::RequestGameplayTag(FName("State.UsingUltimate"), false);

	if (UsingUltimateTag.IsValid() &&
		InstigatorASC->HasMatchingGameplayTag(UsingUltimateTag))
	{
		return; // 궁극기 사용 중에는 마나 충전 안 함
	}

	// 4. Mana 충전량 계산
	float ManaCharge = ActualDamage * ManaChargeRate;
	ManaCharge = FMath::Clamp(ManaCharge, MinManaCharge, MaxManaCharge);

	// 5. GE 확인
	if (!GE_ChargeMana)
		return;

	// 6. GE Spec 생성
	FGameplayEffectContextHandle ManaEffectContext =
		InstigatorASC->MakeEffectContext();
	ManaEffectContext.AddInstigator(Instigator, Instigator);

	FGameplayEffectSpecHandle SpecHandle =
		InstigatorASC->MakeOutgoingSpec(
			GE_ChargeMana,
			1.0f,
			ManaEffectContext
		);

	if (!SpecHandle.IsValid())
		return;

	// 7. SetByCaller로 수치 설정
	FGameplayTag ManaChargeTag =
		FGameplayTag::RequestGameplayTag(FName("Data.ManaCharge"), false);

	if (!ManaChargeTag.IsValid())
		return;

	SpecHandle.Data->SetSetByCallerMagnitude(ManaChargeTag, ManaCharge);

	// 8. GE 적용
	InstigatorASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
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

void UC_MonsterAttributeSet::CheckAndHandleDeath(float NewHP)
{
    if (NewHP <= 0.f)
    {
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

void UC_MonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
 //////////////데미지 처리////////////////////////////////////////////////////////////////////
    const bool bInvincible = Data.Target.HasMatchingGameplayTag(
        FGameplayTag::RequestGameplayTag(FName("State.Invincible")));

    if (Data.EvaluatedData.Attribute == GetReceivedDamageAttribute())
    {
        if (bInvincible) { SetReceivedDamage(0.0f); return; }

        float RawDamage = GetReceivedDamage();

        // 몬스터별 데미지 가공 (예: AC_ShieldMonster의 전면 방어각 감소) — 방어력 감산 이전
        if (AC_BaseMonster* monster = Cast<AC_BaseMonster>(GetOwningActor()))
            RawDamage = monster->ModifyIncomingDamage(RawDamage, ResolveDamageSourceActor(Data.EffectSpec.GetEffectContext()), false);

        const float Mitigated = FMath::Max(0.0f, RawDamage - Getdefense()); // 방어 반영

        const float NewHP = FMath::Clamp(GetcurHP() - Mitigated, 0.f, GetmaxHP());
        SetcurHP(NewHP);
        SetReceivedDamage(0.0f);

		if (Mitigated > 0.0f)
		{
			ChargeAttackerMana(Data, Mitigated);

			if (AC_BaseMonster* monster = Cast<AC_BaseMonster>(GetOwningActor()))
			{
				monster->StartHitFlash();
				monster->TakeHitReaction();
			}
		}

        CheckAndHandleDeath(NewHP);
        return;
    }

    // True Damage (DoT/상태이상) — 방어력 무시
    if (Data.EvaluatedData.Attribute == GetReceivedTrueDamageAttribute())
    {
        if (bInvincible) { SetReceivedTrueDamage(0.0f); return; }

        float TrueDamage = GetReceivedTrueDamage();

        if (AC_BaseMonster* monster = Cast<AC_BaseMonster>(GetOwningActor()))
            TrueDamage = monster->ModifyIncomingDamage(TrueDamage, ResolveDamageSourceActor(Data.EffectSpec.GetEffectContext()), true);

        const float NewHP = FMath::Clamp(GetcurHP() - TrueDamage, 0.f, GetmaxHP());
        SetcurHP(NewHP);
        SetReceivedTrueDamage(0.0f);

        if (TrueDamage > 0.0f)
        {
            ChargeAttackerMana(Data, TrueDamage);
        }

        CheckAndHandleDeath(NewHP);
        return;
    }
    // Groggy 누적 처리
    if (Data.EvaluatedData.Attribute == GetReceivedGroggyAttribute())
    {
        const float GroggyAmount = GetReceivedGroggy();
        SetReceivedGroggy(0.0f);

        if (GroggyAmount > 0.0f)
        {
            if (AC_BaseMonster* monster = Cast<AC_BaseMonster>(GetOwningActor()))
            {
                if (UC_GroggyComponent* groggyComp = monster->GetGroggyComponent())
                {
                    groggyComp->AddGroggy(GroggyAmount);
                }
            }
        }
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
        CheckAndHandleDeath(NewHP);
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

void UC_MonsterAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetmoveSpeedAttribute())
    {
        if (AActor* Owner = GetOwningActor())
        {
            if (ACharacter* Char = Cast<ACharacter>(Owner))
            {
                Char->GetCharacterMovement()->MaxWalkSpeed = NewValue;
            }
        }
    }
}
