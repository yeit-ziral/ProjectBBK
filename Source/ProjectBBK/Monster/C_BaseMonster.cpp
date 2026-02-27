// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BaseMonster.h"
#include "Manager/C_AttackManagerComponent.h"
#include "Data/MonsterData.h"                 
#include "AI/C_MonsterAIController.h"  
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI/C_BossMonsterHPWidget.h"
#include "UI/C_NormalMonsterHPWidget.h"

// Sets default values
AC_BaseMonster::AC_BaseMonster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	monsterASC = CreateDefaultSubobject<UC_MonsterASC>(TEXT("MonsterASC"));
	monsterASC->SetIsReplicated(true);
	monsterASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	monsterAttributeSet = CreateDefaultSubobject<UC_MonsterAttributeSet>(TEXT("MonsterAttributes"));
	monsterASC->AddAttributeSetSubobject(monsterAttributeSet);



	attackManager = CreateDefaultSubobject<UC_AttackManagerComponent>(TEXT("AttackManager"));


	AIControllerClass = AC_MonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);

	HpWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpWidget"));
}

// Called when the game starts or when spawned
void AC_BaseMonster::BeginPlay()
{
	Super::BeginPlay();


	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Bear C++ BeginPlay!"));

	if (monsterASC)
	{
		monsterASC->InitAbilityActorInfo(this, this);
		for (const TSubclassOf<UGameplayAbility>& abilityClass : gamePlayAbilities)
		{
			if (!abilityClass) continue;

			monsterASC->GiveAbility(FGameplayAbilitySpec(abilityClass, 1, 0));
			UE_LOG(LogTemp, Error, TEXT("GiveAbility: %s"), *abilityClass->GetName());
		}

		// 죽음 이벤트 바인딩 (AI, 이펙트, 사운드 등)
		monsterASC->OnMonsterDeath.AddLambda([this](UC_MonsterASC* ASC)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s died (OnMonsterDeath)"), *GetName());
				// 여기서 애니메이션, Destroy 타이머, AI 비활성화 등 처리
			});
	}

	if (HasAuthority())
	{
		if (!monsterTable.Get())
			monsterTable.LoadSynchronous();

		InitializeAttributesFromDataTable();
	}

	ApplyMonsterTypeTag();
	InitializeMonsterHpWidget();
	//////////////////////
	if (GEngine)
	{
		FString Msg = FString::Printf(TEXT("BeginPlay: MonsterId=%d, MaxHP=%.1f, ATK=%.1f"),
			monsterId,
			monsterAttributeSet ? monsterAttributeSet->GetmaxHP() : -1.0f,
			monsterAttributeSet ? monsterAttributeSet->Getattack() : -1.0f);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Msg);
	}
	/////////////////////////
	if (attackManager)
		attackManager->Initialize(this);
	
}

void AC_BaseMonster::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InitializeHpWidgetClass();
}

void AC_BaseMonster::ApplyMonsterTypeTag()
{
	if (!monsterASC)
		return;

	if (monsterTypeTag.IsValid())
	{
		monsterASC->AddLooseGameplayTag(monsterTypeTag);
	}
}

void AC_BaseMonster::InitializeAttributesFromDataTable()
{
	if (!monsterAttributeSet)
		return;


	UDataTable* Table = monsterTable.Get();
	if (!Table)
		Table = monsterTable.LoadSynchronous();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("MonsterDataTable not loaded!"));
		return;
	}

	const FMonsterData* Data = Table->FindRow<FMonsterData>(rowName, TEXT("InitMonster"));
	if (!Data)
	{
		UE_LOG(LogTemp, Warning, TEXT("Row %s not found in MonsterDataTable"), *rowName.ToString());
		return;
	}

	
	monsterId = Data->MonsterId;

	monsterAttributeSet->InitmaxHP          (Data->MaxHP);
	monsterAttributeSet->InitmaxGroggy      (Data->MaxGroggy);
	monsterAttributeSet->Initattack         (Data->Attack);
	monsterAttributeSet->Initdefense        (Data->Defense);
	monsterAttributeSet->InitattackRange    (Data->AttackRange);
	monsterAttributeSet->InitmoveSpeed      (Data->MoveSpeed);
	monsterAttributeSet->InitnormalCooldown (Data->NormalCooldown);
	monsterAttributeSet->InitspecialCooldown(Data->SpecialCooldown);

	monsterAttributeSet->SetcurHP(Data->MaxHP);    
	monsterAttributeSet->SetcurGroggy(Data->MaxGroggy);

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = monsterAttributeSet->GetmoveSpeed();
	}
	UE_LOG(LogTemp, Error, TEXT("Monster MaxHP = %d"), GetmaxHP());
}

void AC_BaseMonster::InitializeMonsterHpWidget()
{
	if (!HpWidgetComponent)
		return;

	UUserWidget* Widget = HpWidgetComponent->GetUserWidgetObject();
	if (!Widget)
		return;

	MonsterHpWidget = Cast<UC_MonsterHPWidgetBase>(Widget);

	if (!MonsterHpWidget)
		return;

	MonsterHpWidget->SetMaxHp(GetmaxHP());
	MonsterHpWidget->SetCurrentHp(GetcurHP());
	MonsterHpWidget->SetMonsterName(FText::FromName(rowName));

}


void AC_BaseMonster::InitializeHpWidgetClass()
{
	if (!HpWidgetComponent)
		return;

	TSubclassOf<UUserWidget> WidgetClass = GetHpWidgetClass();
	if (!WidgetClass)
		return;

	HpWidgetComponent->SetWidget(nullptr);
	HpWidgetComponent->SetWidgetClass(WidgetClass);
}

TSubclassOf<UUserWidget> AC_BaseMonster::GetHpWidgetClass() const
{
	return UC_NormalMonsterHPWidget::StaticClass();
}


// Called every frame
void AC_BaseMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AC_BaseMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

