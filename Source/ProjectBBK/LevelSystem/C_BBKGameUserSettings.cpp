// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BBKGameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"

UC_BBKGameUserSettings* UC_BBKGameUserSettings::Get()
{
	return Cast<UC_BBKGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void UC_BBKGameUserSettings::ApplyVolumeSettings(UObject* WorldContext, USoundMix* SoundMix,
	USoundClass* MasterClass, USoundClass* BGMClass, USoundClass* SFXClass)
{
	if (!WorldContext || !SoundMix) return;

	UGameplayStatics::PushSoundMixModifier(WorldContext, SoundMix);

	if (MasterClass)
		UGameplayStatics::SetSoundMixClassOverride(WorldContext, SoundMix, MasterClass, MasterVolume, 1.f, 0.f, true);
	if (BGMClass)
		UGameplayStatics::SetSoundMixClassOverride(WorldContext, SoundMix, BGMClass, BGMVolume, 1.f, 0.f, true);
	if (SFXClass)
		UGameplayStatics::SetSoundMixClassOverride(WorldContext, SoundMix, SFXClass, SFXVolume, 1.f, 0.f, true);
}
