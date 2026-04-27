// Fill out your copyright notice in the Description page of Project Settings.

#include "ANC_DeathVFX.h"
#include "../C_BaseMonster.h"

void UANC_DeathVFX::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	AC_BaseMonster* Monster = Cast<AC_BaseMonster>(MeshComp->GetOwner());
	if (!Monster) return;

	Monster->OnDeathMontageVFXPoint();
}
