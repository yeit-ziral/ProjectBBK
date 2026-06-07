// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "C_BBKGameUserSettings.generated.h"

class USoundMix;
class USoundClass;

UCLASS(config=GameUserSettings)
class PROJECTBBK_API UC_BBKGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static UC_BBKGameUserSettings* Get();

	void ApplyVolumeSettings(UObject* WorldContext, USoundMix* SoundMix,
		USoundClass* MasterClass, USoundClass* BGMClass, USoundClass* SFXClass);

	float GetMasterVolume() const { return MasterVolume; }
	float GetBGMVolume()    const { return BGMVolume; }
	float GetSFXVolume()    const { return SFXVolume; }

	void SetMasterVolume(float Value) { MasterVolume = FMath::Clamp(Value, 0.f, 1.f); }
	void SetBGMVolume(float Value)    { BGMVolume    = FMath::Clamp(Value, 0.f, 1.f); }
	void SetSFXVolume(float Value)    { SFXVolume    = FMath::Clamp(Value, 0.f, 1.f); }

private:
	UPROPERTY(Config)
	float MasterVolume = 1.0f;

	UPROPERTY(Config)
	float BGMVolume = 1.0f;

	UPROPERTY(Config)
	float SFXVolume = 1.0f;
};
