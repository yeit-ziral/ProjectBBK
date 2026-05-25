// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BasePlayerCharactor.h"
#include "../Skills/C_SkillManagerComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "../GAS/Abilities/C_CharacterASC.h"
#include "../GAS/Abilities/C_CharacterGA.h"
#include "../GAS/Attributes/C_ChracterAttributeSetBase.h"
#include "C_PlayerState.h"
#include "PlayerAI/C_PlayerAIController.h"
#include "PlayerAI/C_PlayerController.h"
#include "../Skills/C_SkillManagerComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Monster/C_BaseMonster.h"
#include "NiagaraFunctionLibrary.h"

DEFINE_LOG_CATEGORY(LogBasePlayerCharacter);

// Sets default values
AC_BasePlayerCharactor::AC_BasePlayerCharactor(const class FObjectInitializer &ObjectInitalizer) // replace this if i want to make movement system with GAS and Legacy style "AC_BasePlayerCharactor::AC_BasePlayerCharactor(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	: Super(ObjectInitalizer)
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	springArm = CreateDefaultSubobject<USpringArmComponent>("springArm");
	springArm->SetupAttachment(RootComponent);
	springArm->TargetArmLength = 400.0f;
	springArm->bUsePawnControlRotation = true;

	defaultArmLength = 400.f;
	defaultSpringArmRotation = FRotator::ZeroRotator;

	camera = CreateDefaultSubobject<UCameraComponent>("camera");
	camera->SetupAttachment(springArm, USpringArmComponent::SocketName);
	camera->bUsePawnControlRotation = false;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);

	bAlwaysRelevant = true;

	effectRemoveOnDeathTag = FGameplayTag::RequestGameplayTag(FName("State.RemoveOnDeath"));

	AIControllerClass = AC_PlayerAIController::StaticClass();
}

void AC_BasePlayerCharactor::SetupMappingContext()
{
	if (APlayerController *PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(playerMappingContext, 0);
		}
	}
}

// Called when the game starts or when spawned
void AC_BasePlayerCharactor::BeginPlay()
{
	Super::BeginPlay();

	SetupMappingContext();

	GetMesh()->SetReceivesDecals(false);
}

// Called every frame
void AC_BasePlayerCharactor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// UpdateStamina();

	if (bCameraBlending && springArm)
	{
		springArm->TargetArmLength = FMath::FInterpTo(
			springArm->TargetArmLength, targetArmLength, DeltaTime, 5.f);
		springArm->SetRelativeRotation(FMath::RInterpTo(
			springArm->GetRelativeRotation(), targetSpringArmRotation, DeltaTime, 5.f));

		// 탑다운→일반 복귀 시 기능 복원은 RestoreCameraAfterBlend 타이머가 담당
		// 탑다운 진입 블렌딩 완료는 여기서 처리
		if (bTopDownActive)
		{
			cameraBlendRemain -= DeltaTime;
			if (cameraBlendRemain <= 0.f)
			{
				bCameraBlending = false;
				springArm->TargetArmLength = targetArmLength;
				springArm->SetRelativeRotation(targetSpringArmRotation);
			}
		}
	}
}

void AC_BasePlayerCharactor::SetTopDownCamera(bool bTopDown, float BlendTime,
											  float TopDownArmLength, float TopDownPitch)
{
	if (!springArm)
		return;

	bTopDownActive = bTopDown;
	cameraBlendRemain = FMath::Max(BlendTime, 0.01f);
	bCameraBlending = true;

	if (bTopDown)
	{
		springArm->bUsePawnControlRotation = false;
		springArm->bInheritYaw = false;
		springArm->bInheritPitch = false;
		springArm->bInheritRoll = false;
		targetArmLength = TopDownArmLength;
		targetSpringArmRotation = FRotator(TopDownPitch, 0.f, 0.f);
	}
	else
	{
		// 블렌딩 중 카메라가 컨트롤러 방향을 향하도록 현재 Yaw를 타겟에 굽힘
		const float controllerYaw = GetController() ? GetController()->GetControlRotation().Yaw : 0.f;
		targetArmLength = defaultArmLength;
		targetSpringArmRotation = FRotator(defaultSpringArmRotation.Pitch, controllerYaw, 0.f);

		// 타이머로 확실하게 복귀 — Tick 카운트다운보다 신뢰성 높음
		GetWorldTimerManager().SetTimer(
			cameraRestoreTimerHandle,
			this, &AC_BasePlayerCharactor::RestoreCameraAfterBlend,
			FMath::Max(BlendTime, 0.01f), false);
	}
}

void AC_BasePlayerCharactor::RestoreCameraAfterBlend()
{
	if (!springArm || bTopDownActive)
		return;

	springArm->bInheritYaw = true;
	springArm->bInheritPitch = true;
	springArm->bInheritRoll = true;
	springArm->TargetArmLength = targetArmLength;
	springArm->SetRelativeRotation(targetSpringArmRotation);
	springArm->bUsePawnControlRotation = true;
	bCameraBlending = false;
}

void AC_BasePlayerCharactor::OnAbilityInputPressed(const FInputActionInstance &Instance)
{
	if (!abilitySystemComponent.IsValid())
		return;

	const FGameplayTag *Tag = abilityTagMap.Find(Instance.GetSourceAction());
	if (!Tag || !Tag->IsValid())
		return;
	abilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(*Tag));
}

void AC_BasePlayerCharactor::OnAbilityInputReleased(const FInputActionInstance &Instance)
{
	if (!abilitySystemComponent.IsValid())
		return;

	const FGameplayTag *Tag = releaseEventTagMap.Find(Instance.GetSourceAction());
	if (!Tag || !Tag->IsValid())
		return;
	FGameplayEventData EventData;
	abilitySystemComponent->HandleGameplayEvent(*Tag, &EventData);
}

// Called to bind functionality to input
void AC_BasePlayerCharactor::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(moveAction, ETriggerEvent::Triggered, this, &AC_BasePlayerCharactor::MyMove);

		// Looking
		EnhancedInputComponent->BindAction(lookAction, ETriggerEvent::Triggered, this, &AC_BasePlayerCharactor::MyLook);

		//// Sprinting
		// EnhancedInputComponent->BindAction(sprintAction, ETriggerEvent::Triggered, this, &AC_BasePlayerCharactor::StartSprint);
		// EnhancedInputComponent->BindAction(sprintAction, ETriggerEvent::Completed, this, &AC_BasePlayerCharactor::EndSprint);

		////Attacking
		// EnhancedInputComponent->BindAction(attackAction, ETriggerEvent::Started, this, &AC_BasePlayerCharactor::OnAttack);

		// Ability input → GAS 연결
		abilityTagMap.Empty();
		releaseEventTagMap.Empty();
		for (const FAbilityInputBinding &Binding : abilityInputBindings)
		{
			if (!Binding.inputAction)
				continue;
			abilityTagMap.Add(Binding.inputAction, Binding.abilityTag);
			if (Binding.releaseEventTag.IsValid())
				releaseEventTagMap.Add(Binding.inputAction, Binding.releaseEventTag);
			EnhancedInputComponent->BindAction(Binding.inputAction, ETriggerEvent::Started, this, &AC_BasePlayerCharactor::OnAbilityInputPressed);
			EnhancedInputComponent->BindAction(Binding.inputAction, ETriggerEvent::Completed, this, &AC_BasePlayerCharactor::OnAbilityInputReleased);
		}
	}
	else
	{
		UE_LOG(LogBasePlayerCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AC_BasePlayerCharactor::PossessedBy(AController *NewController)
{
	Super::PossessedBy(NewController);

	// 로스터 교체 시 비소유 상태로 스폰된 캐릭터는 BeginPlay에서
	// MappingContext가 등록되지 않으므로 여기서 다시 시도
	SetupMappingContext();

	AC_PlayerState *PS = GetPlayerState<AC_PlayerState>();

	if (PS)
	{
		InitializeStartingValues(PS);

		AddStartupEffects();

		AddCharacterAbilities();
	}
}

void AC_BasePlayerCharactor::UnPossessed()
{
	Super::UnPossessed();
}

void AC_BasePlayerCharactor::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AC_PlayerState *PS = GetPlayerState<AC_PlayerState>();

	if (PS)
	{
		InitializeStartingValues(PS);
	}
}

void AC_BasePlayerCharactor::InitializeStartingValues(AC_PlayerState *PS)
{
	check(PS); // this is for debugging

	abilitySystemComponent = Cast<UC_CharacterASC>(PS->GetAbilitySystemComponent());

	PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);

	// 초기화가 끝났음을 블루프린트에 알림
	if (HasAuthority())
	{
		OnASCInitialized();
	}

	attributeSetBase = PS->GetAttributeSetBase();

	InitializeAttributes();

	RestoreCharacterState();

	if (abilitySystemComponent.IsValid())
	{
		abilitySystemComponent->SetNumericAttributeBase(
			UC_ChracterAttributeSetBase::GetdamageAttribute(),
			baseDamage);
	}

	if (!bCharacterInitiailized)
	{
		bCharacterInitiailized = true;

		if (abilitySystemComponent.IsValid() && attributeSetBase.IsValid())
		{
			// Health 변경 감지
			abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
									  attributeSetBase->GethealthAttribute())
				.AddUObject(this, &AC_BasePlayerCharactor::OnHealthChanged);

			// Mana 변경 감지 (다시!)
			abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
									  attributeSetBase->GetmanaAttribute())
				.AddUObject(this, &AC_BasePlayerCharactor::OnManaChangedInternal);

			// Shield 변경 감지
			abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
									  attributeSetBase->GetshieldAttribute())
				.AddUObject(this, &AC_BasePlayerCharactor::OnShieldChanged);

			abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
									  attributeSetBase->GetmoveSpeedAttribute())
				.AddUObject(this, &AC_BasePlayerCharactor::OnMoveSpeedChanged);

			abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
									  attributeSetBase->GetstaminaAttribute())
				.AddUObject(this, &AC_BasePlayerCharactor::OnStaminaChanged);

			abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
									  attributeSetBase->GetlevelAttribute())
				.AddUObject(this, &AC_BasePlayerCharactor::OnLevelChanged);

			if (attributeSetBase.IsValid())
			{
				float InitialMoveSpeed = attributeSetBase->GetmoveSpeed();
				if (UCharacterMovementComponent *MovementComp = GetCharacterMovement())
				{
					MovementComp->MaxWalkSpeed = InitialMoveSpeed;

					UE_LOG(LogBasePlayerCharacter, Log, TEXT("Initial MoveSpeed Set: %.2f"), InitialMoveSpeed);
				}

				UE_LOG(LogBasePlayerCharacter, Log, TEXT("Attribute change delegates registered"));
			}

			FGameplayTag CanSprintTag = FGameplayTag::RequestGameplayTag(FName("State.CanSprint"));
			abilitySystemComponent->AddLooseGameplayTag(CanSprintTag);
		}
	}
}

UAbilitySystemComponent *AC_BasePlayerCharactor::GetAbilitySystemComponent() const
{
	return abilitySystemComponent.Get();
}

bool AC_BasePlayerCharactor::IsAlive() const
{
	return !bIsDead;
}

void AC_BasePlayerCharactor::RemoveCharacterAbilities()
{
	// UC_CharacterASC* ASC = abilitySystemComponent.Get(); if abilitySystemComponent cause errors because it's a TWeakObjectPtr, we need to call .Get() to get the actual pointer

	if (GetLocalRole() != ROLE_Authority || !abilitySystemComponent.IsValid() || !abilitySystemComponent->characterAbilitiesGiven)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;

	for (const FGameplayAbilitySpec &Spec : abilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.SourceObject == this && characterAbilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	for (int32 i = 0; i < AbilitiesToRemove.Num(); i++)
	{
		abilitySystemComponent->ClearAbility(AbilitiesToRemove[i]);
	}

	abilitySystemComponent->characterAbilitiesGiven = false;
}

void AC_BasePlayerCharactor::Die()
{
	cachedDeathController = Cast<AC_PlayerController>(GetController());

	bIsDead = true;

	RemoveCharacterAbilities();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->Velocity = FVector(0);

	onCharacterDied.Broadcast(this);

	if (abilitySystemComponent.IsValid())
	{
		abilitySystemComponent->CancelAbilities();

		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(effectRemoveOnDeathTag);
		int32 NumEffectsRemoved = abilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);
	}

	if (deathMontage)
	{
		UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->OnMontageEnded.AddDynamic(this, &AC_BasePlayerCharactor::OnDeathMontageEnded);
			PlayAnimMontage(deathMontage);
		}
		else
		{
			FinishDying();
		}
	}
	else
	{
		FinishDying();
	}
}

void AC_BasePlayerCharactor::FinishDying()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();

	if (cachedDeathController.IsValid())
		cachedDeathController->HandleCharacterDeath(this);
}

void AC_BasePlayerCharactor::OnDeathMontageEnded(UAnimMontage *Montage, bool bInterrupted)
{
	if (Montage != deathMontage)
		return;

	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AC_BasePlayerCharactor::OnDeathMontageEnded);

	FinishDying();
}

float AC_BasePlayerCharactor::GetCharacterLevel() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->Getlevel(); // In C_ChracterAttributeSetBase, we defined Gethealth() to return the health attribute value. if you confused, check "ATTRIBUTE_ACCESSORS" in C_ChracterAttributeSetBase.h
	}

	return 0.0f;
}

float AC_BasePlayerCharactor::GetExp() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->Getexperience(); // In C_ChracterAttributeSetBase, we defined Gethealth() to return the health attribute value. if you confused, check "ATTRIBUTE_ACCESSORS" in C_ChracterAttributeSetBase.h
	}
	return 0.0f;
}

float AC_BasePlayerCharactor::GetMaxExp() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->GetmaxExperience(); // In C_ChracterAttributeSetBase, we defined Gethealth() to return the health attribute value. if you confused, check "ATTRIBUTE_ACCESSORS" in C_ChracterAttributeSetBase.h
	}
	return 0.0f;
}

float AC_BasePlayerCharactor::GetHealth() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->Gethealth(); // In C_ChracterAttributeSetBase, we defined Gethealth() to return the health attribute value. if you confused, check "ATTRIBUTE_ACCESSORS" in C_ChracterAttributeSetBase.h
	}

	return 0.0f;
}

float AC_BasePlayerCharactor::GetMaxHealth() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->GetmaxHealth();
	}

	return 0.0f;
}

float AC_BasePlayerCharactor::GetShield() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->Getshield();
	}

	return 0.0f;
}

float AC_BasePlayerCharactor::GetMaxShield() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->GetmaxShield();
	}

	return 0.0f;
}

float AC_BasePlayerCharactor::GetMana() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->Getmana();
	}
	return 0.0f;
}

float AC_BasePlayerCharactor::GetMaxMana() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->GetmaxMana();
	}

	return 0.0f;
}

float AC_BasePlayerCharactor::GetStamina() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->Getstamina();
	}

	return 0.0f;
}

float AC_BasePlayerCharactor::GetMaxStamina() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->GetmaxStamina();
	}

	return 0.0f;
}

void AC_BasePlayerCharactor::MyMove(const FInputActionValue &Value)
{
	if (Controller != nullptr)
	{
		FVector2D MovementVector = Value.Get<FVector2D>();

		FVector ForwardDirection;
		FVector RightDirection;

		if (bTopDownActive)
		{
			// 탑다운 시점: W=월드+X(위), S=월드-X(아래), D=월드+Y(오른쪽), A=월드-Y(왼쪽)
			ForwardDirection = FVector::ForwardVector;
			RightDirection = FVector::RightVector;
		}
		else
		{
			const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
			ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		}

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AC_BasePlayerCharactor::MyLook(const FInputActionValue &Value)
{
	if (bTopDownActive)
		return;

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

//// move to GAS
// void AC_BasePlayerCharactor::StartSprint()
//{
//	if (bHasStamina)
//	{
//		GetCharacterMovement()->MaxWalkSpeed = sprintSpeed;
//
//		if (GetVelocity().Size() >= 0.5)
//		{
//			bIsSprinting = true;
//		}
//		else
//		{
//			bIsSprinting = false;
//		}
//	}
// }
//
// void AC_BasePlayerCharactor::EndSprint()
//{
//	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
//	bIsSprinting = false;
// }

void AC_BasePlayerCharactor::OnAttack(const FInputActionValue &Value)
{
	if (!abilitySystemComponent.IsValid())
		return;
}

//// moved to GAS
// void AC_BasePlayerCharactor::UpdateStamina()
//{
//	if (bIsSprinting)
//	{
//		curStamina -= staminaDrainTime;
//		curRefillDelayTime = delayBeforeRefill;
//	}
//
//	if (!bIsSprinting && curStamina < max_Stamina)
//	{
//		curRefillDelayTime--;
//
//		if(curRefillDelayTime <= 0)
//		{
//			curStamina += staminaRefillTime;
//		}
//	}
//
//	if (curStamina <= 0)
//	{
//		bHasStamina = false;
//		EndSprint();
//	}
//	else
//	{
//		bHasStamina = true;
//	}
// }

void AC_BasePlayerCharactor::AddCharacterAbilities()
{
	if (GetLocalRole() != ROLE_Authority || !abilitySystemComponent.IsValid() || abilitySystemComponent->characterAbilitiesGiven)
	{
		return;
	}

	for (TSubclassOf<UC_CharacterGA> &StartupAbility : characterAbilities)
	{
		abilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbility, 1, INDEX_NONE, this));
	}

	// for checking test gameplay ability
	abilitySystemComponent->characterAbilitiesGiven = true;

	if (UC_SkillManagerComponent *SkillManager = FindComponentByClass<UC_SkillManagerComponent>())
	{
		SkillManager->InitializeDefaultSkill();
	}
}

void AC_BasePlayerCharactor::InitializeAttributes()
{
	if (!abilitySystemComponent.IsValid() || !defaultAttributes)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Missing DefaultAttributes for %s. Please fill in the character's blueprint"), *FString(__FUNCTION__), *GetName());
		return;
	}

	// 이미 초기화 된 상태라면 공유 진행도 보존
	bool bPreserveProgression = abilitySystemComponent->startupEffectsApplied && attributeSetBase.IsValid();
	float savedLevel = 0.f, savedExp = 0.f, savedMaxExp = 0.f;

	if (bPreserveProgression)
	{
		savedLevel = attributeSetBase->Getlevel();
		savedExp = attributeSetBase->Getexperience();
		savedMaxExp = attributeSetBase->GetmaxExperience();
	}

	FGameplayEffectContextHandle EffectContext = abilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle NewHandle = abilitySystemComponent->MakeOutgoingSpec(defaultAttributes, GetCharacterLevel(), EffectContext);

	if (NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = abilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), abilitySystemComponent.Get());
	}

	// GE가 리셋한 level/exp을 보존된 값으로 복원 + 레벨업 스탯 보너스 재적용
	if (bPreserveProgression)
	{
		attributeSetBase->Setlevel(savedLevel);
		attributeSetBase->Setexperience(savedExp);
		attributeSetBase->SetmaxExperience(savedMaxExp);

		// 레벨 1이 기본이므로 (레벨 - 1) 만큼 보너스 재적용
		float bonusLevel = savedLevel - 1;

		if (savedLevel > 0.f)
		{
			attributeSetBase->SetmaxHealth(attributeSetBase->GetmaxHealth() + bonusLevel * 50.f);
			attributeSetBase->SetmaxStamina(attributeSetBase->GetmaxStamina() + bonusLevel * 20.f);
			attributeSetBase->Setdamage(attributeSetBase->Getdamage() + bonusLevel * 30.f);
		}
	}
}

void AC_BasePlayerCharactor::AddStartupEffects()
{
	if (GetLocalRole() != ROLE_Authority || !abilitySystemComponent.IsValid() || abilitySystemComponent->startupEffectsApplied)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = abilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect> &GameplayEffect : startupEffects)
	{
		FGameplayEffectSpecHandle NewHandle = abilitySystemComponent->MakeOutgoingSpec(GameplayEffect, GetCharacterLevel(), EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle = abilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), abilitySystemComponent.Get());
		}
	}

	if (GE_ManaRegen)
	{
		FGameplayEffectSpecHandle NewHandle = abilitySystemComponent->MakeOutgoingSpec(GE_ManaRegen, 1.0f, EffectContext);
		if (NewHandle.IsValid())
		{
			abilitySystemComponent->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
		}
	}

	abilitySystemComponent->startupEffectsApplied = true;
}

void AC_BasePlayerCharactor::SetHealth(float NewHealth)
{
	if (attributeSetBase.IsValid())
	{
		attributeSetBase->Sethealth(NewHealth); // this code is dangerous cuz it directly sets the health value, bypassing any gameplay effects or modifiers
	}
}

void AC_BasePlayerCharactor::SetShield(float NewShield)
{
	if (attributeSetBase.IsValid())
	{
		attributeSetBase->Setshield(NewShield);
	}
}

void AC_BasePlayerCharactor::SetStamina(float NewStamina)
{
	if (attributeSetBase.IsValid())
	{
		attributeSetBase->Setstamina(NewStamina);
	}
}

void AC_BasePlayerCharactor::SetMana(float NewMana)
{
	if (attributeSetBase.IsValid())
	{
		attributeSetBase->Setmana(NewMana);
	}
}

void AC_BasePlayerCharactor::OnHealthChanged(const FOnAttributeChangeData &Data)
{
	if (Data.NewValue < Data.OldValue && !bSuppressDeath)
		PlayHitFlash();

	// 사망 처리
	if (Data.NewValue <= 0.0f && Data.OldValue > 0.0f && !bIsDead && !bSuppressDeath)
	{
		if (GetController() != nullptr) // 현재 조종 중인 캐릭터만 사망 처리
		{
			Die();
		}
	}
}

void AC_BasePlayerCharactor::OnManaChangedInternal(const FOnAttributeChangeData &Data)
{
	float Mana = Data.NewValue;
	float MaxMana = GetMaxMana();
	float Percent = (MaxMana > 0.0f) ? (Mana / MaxMana) : 0.0f;

	// UI 업데이트 브로드캐스트
	OnManaChanged.Broadcast(Percent);

	// UE_LOG(LogBasePlayerCharacter, Log, TEXT("Mana Changed: %.2f / %.2f (%.1f%%)"), Mana, MaxMana, Percent * 100.0f);

	// if (Percent >= 1.0f)
	//{
	//	UE_LOG(LogBasePlayerCharacter, Warning, TEXT("=== MANA FULL! ULTIMATE READY! ==="));
	// }
}

void AC_BasePlayerCharactor::OnShieldChanged(const FOnAttributeChangeData &Data)
{
	float Shield = Data.NewValue;
	float MaxShield = GetMaxShield();
}

void AC_BasePlayerCharactor::OnMoveSpeedChanged(const FOnAttributeChangeData &Data)
{
	float NewMoveSpeed = Data.NewValue;
	float OldMoveSpeed = Data.OldValue;

	if (UCharacterMovementComponent *MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = NewMoveSpeed;
	}
}

void AC_BasePlayerCharactor::OnStaminaChanged(const FOnAttributeChangeData &Data)
{
	float Stamina = Data.NewValue;
	float MaxStamina = GetMaxStamina();
}

void AC_BasePlayerCharactor::OnLevelChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue > Data.OldValue && levelUpEffect && !bSuppressDeath)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			levelUpEffect,
			GetMesh(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true);
	}
}

void AC_BasePlayerCharactor::PlayHitFlash()
{
	if (!hitFlashMaterial)
		return;
	GetMesh()->SetOverlayMaterial(hitFlashMaterial);
	GetWorldTimerManager().SetTimer(
		hitFlashTimerHandle,
		this, &AC_BasePlayerCharactor::ClearHitFlash,
		0.15f, false);
}

void AC_BasePlayerCharactor::ClearHitFlash()
{
	GetMesh()->SetOverlayMaterial(nullptr);
}

void AC_BasePlayerCharactor::SaveCharacterState()
{
	if (!attributeSetBase.IsValid())
		return;

	savedState.health  = GetHealth();
	savedState.shield  = GetShield();
	savedState.stamina = GetStamina();
	savedState.mana    = GetMana();

	if (UC_SkillManagerComponent* SM = FindComponentByClass<UC_SkillManagerComponent>())
		savedState.activeSkillIndex = SM->activeSkillIndex;
}

void AC_BasePlayerCharactor::RestoreCharacterState()
{
	if (!attributeSetBase.IsValid())
		return;

	if (savedState.health <= 0.f)
	{
		SetHealth(GetMaxHealth());
		SetShield(0.f);
		SetStamina(GetMaxStamina());
		SetMana(0.f);
	}
	else
	{
		SetHealth(savedState.health);
		SetShield(savedState.shield);
		SetStamina(savedState.stamina);
		SetMana(savedState.mana);
	}
	// 스킬 인덱스는 AddCharacterAbilities 이후에 적용해야 하므로 여기서 처리하지 않음
	// → activeSkillIndex는 OnCharacterSwitched 핸들러(HUD)가 SwitchCommonSkill 호출 시 반영됨
}

void AC_BasePlayerCharactor::InjectPreSavedState(float Health, float Stamina, float Shield, float Mana)
{
	savedState.health  = Health;
	savedState.stamina = Stamina;
	savedState.shield  = Shield;
	savedState.mana    = Mana;
}

void AC_BasePlayerCharactor::SaveActiveEffects(UAbilitySystemComponent *ASC)
{
	savedActiveEffects.Empty();

	FGameplayTagContainer tagsToMatch;
	tagsToMatch.AddTag(FGameplayTag::RequestGameplayTag(FName("State")));
	tagsToMatch.AddTag(FGameplayTag::RequestGameplayTag(FName("Effect.Cooldown")));

	// State 태그를 부여하는 GE 핸들 목록
	TArray<FActiveGameplayEffectHandle> handles = ASC->GetActiveEffects(
		FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(tagsToMatch));

	float worldTime = GetWorld()->GetTimeSeconds();

	for (const FActiveGameplayEffectHandle &Handle : handles)
	{
		const FActiveGameplayEffect *activeGE = ASC->GetActiveGameplayEffect(Handle);
		if (!activeGE)
			continue;

		float remaining = activeGE->GetTimeRemaining(worldTime);
		if (remaining <= 0.f)
			continue; // 이미 만료된 효과는 저장하지 않음

		FSavedGEState saved;
		saved.Spec = activeGE->Spec;
		saved.RemainingDuration = remaining;

		savedActiveEffects.Add(saved);
	}
}

void AC_BasePlayerCharactor::RestoreActiveEffects(UAbilitySystemComponent *ASC)
{
	for (const FSavedGEState &saved : savedActiveEffects)
	{
		if (saved.RemainingDuration <= 0.f)
			continue; // 만료된 효과는 복원하지 않음

		FGameplayEffectSpec specCopy = saved.Spec;
		specCopy.Duration = saved.RemainingDuration;

		ASC->ApplyGameplayEffectSpecToSelf(specCopy);
	}

	savedActiveEffects.Empty();
}

void AC_BasePlayerCharactor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if(springArm)
	{
		springArm->bDoCollisionTest = false;
		springArm->ProbeChannel = ECC_GameTraceChannel1; // Set Cameral Collision Channel to void collision with monsters
	}
}
