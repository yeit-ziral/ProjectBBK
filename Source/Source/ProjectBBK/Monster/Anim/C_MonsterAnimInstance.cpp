// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterAnimInstance.h"
#include "GameFramework/Pawn.h"

UC_MonsterAnimInstance::UC_MonsterAnimInstance()
{
	Speed = 0.f;
	Dir = 0.f;
	OwnerPawn = nullptr;
}

void UC_MonsterAnimInstance::NativeBeginPlay()
{
	Super::NativeInitializeAnimation();

	OwnerPawn = TryGetPawnOwner();
}

void UC_MonsterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!OwnerPawn)
    {
        OwnerPawn = TryGetPawnOwner();
        if (!OwnerPawn)
        {
            return;
        }
    }

    // 1) 월드 속도 가져오기
    FVector Velocity = OwnerPawn->GetVelocity();
    Velocity.Z = 0.f; // 점프 같은 거 없으면 Z는 무시

    // 2) 로컬 속도로 변환 (앞/뒤, 좌/우 분리)
    const FTransform& ActorTransform = OwnerPawn->GetActorTransform();
    const FVector LocalVelocity = ActorTransform.InverseTransformVectorNoScale(Velocity);

    // 3) 변수에 저장
    Speed = LocalVelocity.X;  // 앞/뒤
    Dir = LocalVelocity.Y;  // 좌/우
}
