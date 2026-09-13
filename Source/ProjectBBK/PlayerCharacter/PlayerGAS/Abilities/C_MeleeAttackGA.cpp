// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MeleeAttackGA.h"

void UC_MeleeAttackGA::ResetCombo()
{
	comboIndex = 0;
    bIsInputBuffered = false;
    bAdvancingCombo = false;

    if (AActor* Avatar = GetAvatarActorFromActorInfo())
    {
        Avatar->GetWorldTimerManager().ClearTimer(comboInputBufferTimerHandle);
    }
}

void UC_MeleeAttackGA::BufferComboInput()
{
    bIsInputBuffered = true;

    // 스윙 아주 초반에 누른 입력이 윈도우까지 살아있으면 어색하다. 유효 시간을 걸어 오래된 입력은 스스로 사라지게 한다.
    if (AActor* Avatar = GetAvatarActorFromActorInfo())
    {
        Avatar->GetWorldTimerManager().SetTimer(
            comboInputBufferTimerHandle, this,
            &UC_MeleeAttackGA::OnComboInputBufferTimerExpired,
            inputBufferTime, false);
    }
}

bool UC_MeleeAttackGA::TryAdvanceCombo()
{
    if (!bIsInputBuffered)
        return false;

    if (!comboMontages.IsValidIndex(comboIndex + 1))
        return false;

    ++comboIndex;
	bIsInputBuffered = false;
	bAdvancingCombo = true;

    if(AActor* Avatar = GetAvatarActorFromActorInfo())
    {
        Avatar->GetWorldTimerManager().ClearTimer(comboInputBufferTimerHandle);
	}

    return true;
}

UAnimMontage* UC_MeleeAttackGA::GetCurrentComboMontage() const
{
	return comboMontages.IsValidIndex(comboIndex) ? comboMontages[comboIndex] : nullptr;
}

bool UC_MeleeAttackGA::ConsumeAdvancingFlag()
{
	const bool bWasAdvancing = bAdvancingCombo;
	bAdvancingCombo = false;
	return bWasAdvancing;
}

void UC_MeleeAttackGA::OnComboInputBufferTimerExpired()
{
    bIsInputBuffered = false;
}
