// Fill out your copyright notice in the Description page of Project Settings.


#include "ANC_MeleeSpecialAttack.h"
#include "../C_BaseMonster.h"
#include "../Manager/C_AttackManagerComponent.h"

void UANC_MeleeSpecialAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    AC_BaseMonster* OwnerMonster = Cast<AC_BaseMonster>(MeshComp->GetOwner());
    if (!OwnerMonster) return;

    if (UC_AttackManagerComponent* Manager = OwnerMonster->GetAttackManager())
    {
        Manager->DoSlam();
    }
}
