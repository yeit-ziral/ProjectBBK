// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_ConsumableAction.generated.h"

class UAbilitySystemComponent;

/**
 * GE 즉시 적용만으로 표현할 수 없는 소비 아이템 동작(AOE 대상 판정, 액터 스폰 등)의 실행 단위.
 * FConsumableItemData.actionClass로 DataTable에서 지정, UC_InventoryComponent::UseItem()이
 * NewObject로 생성한 뒤 Execute()를 호출한다.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTBBK_API UC_ConsumableAction : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Consumable")
	void Execute(UAbilitySystemComponent* ASC, AActor* AvatarActor);
	virtual void Execute_Implementation(UAbilitySystemComponent* ASC, AActor* AvatarActor);
};
