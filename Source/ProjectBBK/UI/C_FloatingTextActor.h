// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_FloatingTextActor.generated.h"

class UWidgetComponent;

UCLASS()
class PROJECTBBK_API AC_FloatingTextActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AC_FloatingTextActor();

	// 스폰 직후 호출, 텍스트 버전에서만 사용
	UFUNCTION(BlueprintCallable, Category = "FloatingText")
	void Initialize(const FText& InText, const FLinearColor& InColor);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "FloatingText")
	TObjectPtr<UWidgetComponent> widgetComp;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText|Motion")
	float lifeTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText|Motion")
	float initialSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText|Motion")
	float gravity = 1250.f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText|Motion")
	float fadeStartRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText|Motion")
	float horizontalJitterSpeed = 30.f;

private:	
	FVector startLocation = FVector::ZeroVector;
	FVector horizontalVelocity = FVector::ZeroVector;
	float elapsed = 0.f;
};
