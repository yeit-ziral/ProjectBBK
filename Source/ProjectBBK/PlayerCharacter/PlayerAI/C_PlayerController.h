// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "C_PlayerController.generated.h"

class AC_BasePlayerCharactor;
class UInputAction;

UCLASS()
class PROJECTBBK_API AC_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 캐릭터 교체 (0-based index). Blueprint에서도 호출 가능.
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|CharacterRoster")
	void SwitchToCharacter(int32 NextIndex);

	// Tab키: 다음 캐릭터로 순환
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|CharacterRoster")
	void SwitchToNextCharacter();

	UFUNCTION(BlueprintPure, Category = "ProjectBBK|CharacterRoster")
	int32 GetCurrentCharacterIndex() const { return currentCharacterIndex; }

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

protected:
	// BP_PlayerController에서 교체할 캐릭터 클래스를 순서대로 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|CharacterRoster")
	TArray<TSubclassOf<AC_BasePlayerCharactor>> characterRosterClasses;

	// 런타임에 스폰된 캐릭터 인스턴스 목록
	UPROPERTY(BlueprintReadOnly, Category = "ProjectBBK|CharacterRoster")
	TArray<AC_BasePlayerCharactor*> characterRoster;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectBBK|CharacterRoster")
	int32 currentCharacterIndex = 0;

	// 숫자 키 1, 2로 직접 교체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_SwitchChar0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_SwitchChar1;

	// Tab키로 순환 교체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_SwitchCharNext;

private:
	bool bIsSwitching = false;
};
