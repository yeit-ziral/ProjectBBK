// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BaseMonster.h"
#include "Manager/C_AttackManagerComponent.h"
#include "Manager/C_MonsterDataComponent.h"
#include "Manager/C_GroggyComponent.h"
#include "UI/C_MonsterHPDisplayComponent.h"
#include "AI/C_MonsterAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"

AC_BaseMonster::AC_BaseMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	monsterASC = CreateDefaultSubobject<UC_MonsterASC>(TEXT("MonsterASC"));
	monsterASC->SetIsReplicated(true);
	monsterASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	monsterAttributeSet = CreateDefaultSubobject<UC_MonsterAttributeSet>(TEXT("MonsterAttributes"));
	monsterASC->AddAttributeSetSubobject(monsterAttributeSet);

	attackManager      = CreateDefaultSubobject<UC_AttackManagerComponent>  (TEXT("AttackManager"));
	dataComponent      = CreateDefaultSubobject<UC_MonsterDataComponent>    (TEXT("DataComponent"));
	groggyComponent    = CreateDefaultSubobject<UC_GroggyComponent>         (TEXT("GroggyComponent"));
	hpDisplayComponent = CreateDefaultSubobject<UC_MonsterHPDisplayComponent>(TEXT("HPDisplayComponent"));

	AIControllerClass = AC_MonsterAIController::StaticClass();
	AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement    = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate                  = FRotator(0.f, 720.f, 0.f);

	HpWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpWidget"));
	HpWidgetComponent->SetupAttachment(GetRootComponent());
	HpWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	HpWidgetComponent->SetDrawSize(FVector2D(500.0f, 110.0f));
	HpWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	HpWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HpWidgetComponent->SetTwoSided(true);
}

void AC_BaseMonster::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 위젯 클래스 설정 — BeginPlay 전에 위젯 객체가 생성되어야 함
	if (hpDisplayComponent)
		hpDisplayComponent->InitializeWidgetClass(HpWidgetComponent);
}

void AC_BaseMonster::BeginPlay()
{
	Super::BeginPlay();

	// GAS 초기화
	if (monsterASC)
	{
		monsterASC->InitAbilityActorInfo(this, this);

		for (const TSubclassOf<UGameplayAbility>& abilityClass : gamePlayAbilities)
		{
			if (!abilityClass) continue;
			monsterASC->GiveAbility(FGameplayAbilitySpec(abilityClass, 1, 0));
		}

		monsterASC->OnMonsterDeath.AddLambda([this](UC_MonsterASC* ASC)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s died (OnMonsterDeath)"), *GetName());
		});
	}

	// DataTable에서 스탯 로드 (서버/단독 실행에서만)
	if (HasAuthority() && dataComponent)
		dataComponent->Initialize(this);

	// curGroggy는 권한 여부와 관계없이 0으로 초기화
	if (monsterAttributeSet)
		monsterAttributeSet->SetcurGroggy(0.0f);

	ApplyMonsterTypeTag();

	// UI 초기화 (DataComponent 이후 — 스탯이 이미 세팅된 상태)
	if (hpDisplayComponent)
		hpDisplayComponent->Initialize(this, HpWidgetComponent);

	// 그로기 컴포넌트 초기화
	if (groggyComponent)
		groggyComponent->Initialize(this, hpDisplayComponent);

	// 어택 매니저 초기화
	if (attackManager)
		attackManager->Initialize(this);
}

void AC_BaseMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (hpDisplayComponent)
		hpDisplayComponent->UpdateWidgetRotation();

	if (groggyComponent)
		groggyComponent->TickGroggy(DeltaTime);
}

void AC_BaseMonster::ApplyMonsterTypeTag()
{
	if (!monsterASC || !monsterTypeTag.IsValid()) return;
	monsterASC->AddLooseGameplayTag(monsterTypeTag);
}

int32 AC_BaseMonster::GetMonsterID() const
{
	return dataComponent ? dataComponent->GetMonsterId() : 0;
}

FName AC_BaseMonster::GetRowName() const
{
	return dataComponent ? dataComponent->GetRowName() : NAME_None;
}

void AC_BaseMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
