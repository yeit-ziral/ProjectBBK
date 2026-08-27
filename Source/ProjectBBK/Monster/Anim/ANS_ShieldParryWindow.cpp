// Fill out your copyright notice in the Description page of Project Settings.

#include "ANS_ShieldParryWindow.h"
#include "../C_ShieldMonster.h"

namespace
{
	AC_ShieldMonster* GetShieldMonster(USkeletalMeshComponent* MeshComp)
	{
		return MeshComp ? Cast<AC_ShieldMonster>(MeshComp->GetOwner()) : nullptr;
	}
}

void UANS_ShieldParryWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AC_ShieldMonster* monster = GetShieldMonster(MeshComp))
		monster->BeginAttackTelegraph(TotalDuration);
}

void UANS_ShieldParryWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AC_ShieldMonster* monster = GetShieldMonster(MeshComp))
		monster->ResolveStrike();
}
