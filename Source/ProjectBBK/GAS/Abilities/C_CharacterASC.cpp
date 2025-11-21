// Fill out your copyright notice in the Description page of Project Settings.


#include "C_CharacterASC.h"

void UC_CharacterASC::ReceiveDamage(UC_CharacterASC* SourceASC, float UnmitigatedDamage, float MitigatedDamage)
{
	receivedDamage.Broadcast(SourceASC, UnmitigatedDamage, MitigatedDamage);
}
