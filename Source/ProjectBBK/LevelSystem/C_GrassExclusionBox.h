// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_GrassExclusionBox.generated.h"

class UBoxComponent;

// 박스 영역 안에 랜드스케이프 자동 풀(LandscapeGrassType)이 생성되지 않게 한다.
// 엔진의 ALandscapeProxy::AddExclusionBox(C++ 전용)를 액터로 감싼 것 — 레벨에 배치만 하면 에디터·PIE·게임 모두 적용.
// 제외 영역은 박스의 월드 AABB(회전하면 AABB가 커지므로 회전 없이 배치 권장). 손으로 칠한 폴리지(InstancedFoliageActor)에는 영향 없음.
UCLASS()
class PROJECTBBK_API AC_GrassExclusionBox : public AActor
{
	GENERATED_BODY()

public:
	AC_GrassExclusionBox();

protected:
	virtual void PostRegisterAllComponents() override;
	virtual void PostUnregisterAllComponents() override;

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(VisibleAnywhere, Category = "Grass")
	UBoxComponent* exclusionBox;

private:
	void RegisterExclusion();
	void UnregisterExclusion();

	bool bRegistered = false;
};
