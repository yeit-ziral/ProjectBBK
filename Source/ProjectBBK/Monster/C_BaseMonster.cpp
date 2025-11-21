// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BaseMonster.h"
#include "Manager/C_AttackManagerComponent.h"
#include "Data/MonsterData.h"                 
#include "AI/C_MonsterAIController.h"  
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AC_BaseMonster::AC_BaseMonster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MonsterASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("MonsterASC"));
	MonsterASC->SetIsReplicated(true);
	MonsterASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	MonsterAttributeSet = CreateDefaultSubobject<UC_MonsterAttributeSet>(TEXT("MonsterAttributes"));
	MonsterASC->AddAttributeSetSubobject(MonsterAttributeSet);



	attackManager = CreateDefaultSubobject<UC_AttackManagerComponent>(TEXT("AttackManager"));


	AIControllerClass = AC_MonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// Called when the game starts or when spawned
void AC_BaseMonster::BeginPlay()
{
	Super::BeginPlay();


	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Bear C++ BeginPlay!"));

	MonsterASC->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		if (!monsterTable.Get())
			monsterTable.LoadSynchronous();

		InitializeAttributesFromDataTable();
	}
	//////////////////////
	if (GEngine)
	{
		FString Msg = FString::Printf(TEXT("BeginPlay: MonsterId=%d, MaxHP=%.1f, ATK=%.1f"),
			monsterId,
			MonsterAttributeSet ? MonsterAttributeSet->GetmaxHP() : -1.0f,
			MonsterAttributeSet ? MonsterAttributeSet->Getattack() : -1.0f);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Msg);
	}
	/////////////////////////
	if (attackManager)
		attackManager->Initialize(this);
	
}

void AC_BaseMonster::InitializeAttributesFromDataTable()
{
	if (!MonsterAttributeSet)
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

	MonsterAttributeSet->InitmaxHP          (Data->MaxHP);
	MonsterAttributeSet->InitmaxGroggy      (Data->MaxGroggy);
	MonsterAttributeSet->Initattack         (Data->Attack);
	MonsterAttributeSet->Initdefense        (Data->Defense);
	MonsterAttributeSet->InitattackRange    (Data->AttackRange);
	MonsterAttributeSet->InitmoveSpeed      (Data->MoveSpeed);
	MonsterAttributeSet->InitnormalCooldown (Data->NormalCooldown);
	MonsterAttributeSet->InitspecialCooldown(Data->SpecialCooldown);

	MonsterAttributeSet->SetcurHP(Data->MaxHP);    
	MonsterAttributeSet->SetcurGroggy(Data->MaxGroggy);

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = MonsterAttributeSet->GetmoveSpeed();
	}

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

