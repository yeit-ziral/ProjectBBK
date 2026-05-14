// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossStormPatternGA.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "../../C_BossMonster.h"

UC_BossStormPatternGA::UC_BossStormPatternGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_BossStormPatternGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    AActor* BossActor = GetAvatarActorFromActorInfo();
    if (!BossActor)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    FVector Center = BossActor->GetActorLocation();

    cachedSpawnPoints.Empty();

    GenerateStormTriangle(Center, patternRadius, cachedSpawnPoints);


    for (const FVector& Point : cachedSpawnPoints)
    {
        UGameplayStatics::SpawnDecalAtLocation(GetWorld(), magicCircleDecal, FVector(decalSize), Point, FRotator(-90.f, 0, 0), 10.f);
    }


    UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo();
    if (asc)
        asc->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Invincible"));

    FTimerHandle StormTimer;

    GetWorld()->GetTimerManager().SetTimer(StormTimer, this, &UC_BossStormPatternGA::SpawnStorms, stormDelay, false);
    UE_LOG(LogTemp, Warning, TEXT("StormPattern: Timer set, delay=%.1f, points=%d"), stormDelay, cachedSpawnPoints.Num());
}



void UC_BossStormPatternGA::GenerateStormTriangle(const FVector& Center, float Radius, TArray<FVector>& OutPoints)
{
    OutPoints.Empty();

    UWorld* World = GetWorld();
    if (!World) return;

    float Distance = FMath::RandRange(Radius * 0.4f, Radius * 0.7f);
    float BaseAngle = FMath::RandRange(0.f, 360.f);

    TArray<FVector> TempPoints;

    for (int32 i = 0; i < 3; ++i)
    {
        float Angle = BaseAngle + i * 120.f;
        float Rad = FMath::DegreesToRadians(Angle);

        FVector FlatPoint(
            Center.X + FMath::Cos(Rad) * Distance,
            Center.Y + FMath::Sin(Rad) * Distance,
            Center.Z
        );

        TempPoints.Add(FlatPoint);
    }

    for (const FVector& P : TempPoints)
    {
        FVector Start = P + FVector(0, 0, 800.f);
        FVector End = P - FVector(0, 0, 2000.f);

        FHitResult Hit;

        bool bHit = World->LineTraceSingleByChannel(
            Hit,
            Start,
            End,
            ECC_WorldStatic
        );

        if (bHit)
        {
            OutPoints.Add(Hit.ImpactPoint);
        }
    }
}

void UC_BossStormPatternGA::SpawnStorms()
{
    UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo();
    if (asc)
        asc->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Invincible"));

    for (const FVector& Point : cachedSpawnPoints)
    {
        GetWorld()->SpawnActor<AActor>(stormActorClass, Point, FRotator::ZeroRotator);
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
