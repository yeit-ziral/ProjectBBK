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
#include "TimerManager.h"

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
	HpWidgetComponent->SetupAttachment(GetRootComponent());
	HpWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	HpWidgetComponent->SetDrawSize(FVector2D(500.0f, 110.0f));
	HpWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	HpWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HpWidgetComponent->SetTwoSided(true);
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

	if (monsterAttributeSet)
	{
		monsterAttributeSet->SetcurGroggy(0.0f);
	}

	ApplyMonsterTypeTag();
	InitializeMonsterHpWidget();
	BindAttributeDelegates();
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
	
	PrintMonsterTags();
}

// Called every frame
void AC_BaseMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHpWidgetRotation();

	/////////////////////
	const float CurrentHp = GetcurHP();
	const float MaxHp = GetmaxHP();

	const float CurrentGroggy = GetcurGroggy();
	const float MaxGroggy = GetmaxGroggy();

	const FString DebugString = FString::Printf(
		TEXT("[%s] HP : %.1f / %.1f | Groggy : %.1f / %.1f"),
		*GetName(),
		CurrentHp,
		MaxHp,
		CurrentGroggy,
		MaxGroggy
	);

	GEngine->AddOnScreenDebugMessage(
		reinterpret_cast<uint64>(this),
		0.0f,
		FColor::Red,
		DebugString
	);

	AddGroggy(1.0f);
	/////////////////////
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

void AC_BaseMonster::PrintMonsterTags()
{
	if (!monsterASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("monsterASC is NULL"));
		return;
	}

	FGameplayTagContainer OwnedTags;
	monsterASC->GetOwnedGameplayTags(OwnedTags);

	UE_LOG(LogTemp, Warning, TEXT("===== Monster Owned Tags Start ====="));

	if (OwnedTags.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Owned Tags"));
	}

	for (const FGameplayTag& Tag : OwnedTags)
	{
		UE_LOG(LogTemp, Warning, TEXT("Owned Tag : %s"), *Tag.ToString());
	}

	UE_LOG(LogTemp, Warning, TEXT("===== Monster Owned Tags End ====="));
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

	MonsterHpWidget->SetMaxGroggy(GetmaxGroggy());
	MonsterHpWidget->SetCurrentGroggy(GetcurGroggy());

	MonsterHpWidget->SetMonsterLevel(1);
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

void AC_BaseMonster::UpdateHpWidgetRotation()
{
	if (!HpWidgetComponent)
		return;

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
		return;

	FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();

	FRotator WidgetRotation = FRotator(0.0f, CameraRotation.Yaw + 180.0f, 0.0f);
	HpWidgetComponent->SetWorldRotation(WidgetRotation);
}

TSubclassOf<UUserWidget> AC_BaseMonster::GetHpWidgetClass() const
{
	return hpWidgetClass;
}

void AC_BaseMonster::BindAttributeDelegates()
{
	if (!monsterASC)
		return;

	monsterASC->GetGameplayAttributeValueChangeDelegate
    (UC_MonsterAttributeSet::GetcurHPAttribute()).AddUObject(this, &AC_BaseMonster::OnHpChanged);

	monsterASC->GetGameplayAttributeValueChangeDelegate
	(UC_MonsterAttributeSet::GetmaxHPAttribute()).AddUObject(this, &AC_BaseMonster::OnMaxHpChanged);

	monsterASC->GetGameplayAttributeValueChangeDelegate
	(UC_MonsterAttributeSet::GetcurGroggyAttribute()).AddUObject(this, &AC_BaseMonster::OnGroggyChanged);

	monsterASC->GetGameplayAttributeValueChangeDelegate
	(UC_MonsterAttributeSet::GetmaxGroggyAttribute()).AddUObject(this, &AC_BaseMonster::OnMaxGroggyChanged);
}

void AC_BaseMonster::OnHpChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!MonsterHpWidget)
		return;

	MonsterHpWidget->SetCurrentHp(ChangeData.NewValue);

	UE_LOG(LogTemp, Warning, TEXT("[%s] curHP Changed: %f"), *GetName(), ChangeData.NewValue);
}

void AC_BaseMonster::OnMaxHpChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!MonsterHpWidget)
		return;

	MonsterHpWidget->SetMaxHp(ChangeData.NewValue);
	MonsterHpWidget->SetCurrentHp(GetcurHP());

	UE_LOG(LogTemp, Warning, TEXT("[%s] maxHP Changed: %f"), *GetName(), ChangeData.NewValue);
}

void AC_BaseMonster::OnGroggyChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!MonsterHpWidget)
		return;

	MonsterHpWidget->SetCurrentGroggy(ChangeData.NewValue);

	UE_LOG(LogTemp, Warning, TEXT("[%s] curGroggy Changed: %f"), *GetName(), ChangeData.NewValue);
}

void AC_BaseMonster::OnMaxGroggyChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!MonsterHpWidget)
		return;

	MonsterHpWidget->SetMaxGroggy(ChangeData.NewValue);
	MonsterHpWidget->SetCurrentGroggy(GetcurGroggy());

	UE_LOG(LogTemp, Warning, TEXT("[%s] maxGroggy Changed: %f"), *GetName(), ChangeData.NewValue);
}

void AC_BaseMonster::AddGroggy(float GroggyAmount)
{
	if (!monsterAttributeSet)
		return;

	const float CurrentGroggyValue = GetcurGroggy();
	const float MaxGroggyValue = GetmaxGroggy();

	const float NewGroggyValue = FMath::Clamp(
		CurrentGroggyValue + GroggyAmount,
		0.0f,
		MaxGroggyValue
	);

	monsterAttributeSet->SetcurGroggy(NewGroggyValue);

	if (MonsterHpWidget)
	{
		MonsterHpWidget->SetCurrentGroggy(NewGroggyValue);
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] Groggy : %.1f / %.1f"),
		*GetName(),
		NewGroggyValue,
		MaxGroggyValue
	);

	if (NewGroggyValue >= MaxGroggyValue)
	{
		if (!GetWorldTimerManager().IsTimerActive(groggyResetTimerHandle))
		{
			GetWorldTimerManager().SetTimer(
				groggyResetTimerHandle,
				this,
				&AC_BaseMonster::ResetGroggy,
				5.0f,
				false
			);

			UE_LOG(LogTemp, Warning, TEXT("[%s] Groggy max reached. Reset in 5 seconds."), *GetName());
		}
	}
}

void AC_BaseMonster::ResetGroggy()
{
	if (!monsterAttributeSet)
		return;

	monsterAttributeSet->SetcurGroggy(0.0f);

	if (MonsterHpWidget)
	{
		MonsterHpWidget->SetCurrentGroggy(monsterAttributeSet->GetcurGroggy());
	}

}




// Called to bind functionality to input
void AC_BaseMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

