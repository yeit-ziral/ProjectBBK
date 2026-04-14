// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "C_MonsterDataComponent.generated.h"

class AC_BaseMonster;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTBBK_API UC_MonsterDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UC_MonsterDataComponent();

	// HasAuthority() 확인 후 BeginPlay에서 호출
	void Initialize(AC_BaseMonster* InOwner);

	FName GetRowName() const { return rowName; }
	int32 GetMonsterId() const { return monsterId; }

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (AllowPrivateAccess = "true"))
	FName rowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UDataTable> monsterTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	int32 monsterId = 0;

	UPROPERTY()
	AC_BaseMonster* ownerMonster = nullptr;
};
