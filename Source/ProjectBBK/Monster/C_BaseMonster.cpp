// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BaseMonster.h"
#include "Manager/C_AttackManagerComponent.h"
#include "Data/MonsterData.h"                 
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AC_BaseMonster::AC_BaseMonster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	attackManager = CreateDefaultSubobject<UC_AttackManagerComponent>(TEXT("AttackManager"));

}

// Called when the game starts or when spawned
void AC_BaseMonster::BeginPlay()
{
	Super::BeginPlay();


	if (monsterTable.IsNull())
		return;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Bear C++ BeginPlay!"));

	static const FString Context(TEXT("MonsterData"));
	UDataTable* Table = monsterTable.LoadSynchronous();
	if (!Table)
		return;

	const FMonsterData* MonsterData = Table->FindRow<FMonsterData>(rowName, Context);
	if (MonsterData)
	{
		ApplyData(*MonsterData);
	}

	if (attackManager)
		attackManager->Initialize(this);
	
}

void AC_BaseMonster::ApplyData(const FMonsterData& Data)
{
	monsterId = Data.Id;
	hp = Data.HP;
	attack = Data.Attack;
	moveSpeed = Data.MoveSpeed;
	attackRange = Data.AttackRange;
	attackCooldown = Data.AttackCoolDown;
	GetCharacterMovement()->MaxWalkSpeed = moveSpeed;
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

