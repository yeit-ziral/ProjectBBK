// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BasePlayerCharactor.h"
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

DEFINE_LOG_CATEGORY(LogBasePlayerCharacter);

// Sets default values
AC_BasePlayerCharactor::AC_BasePlayerCharactor(const class FObjectInitializer& ObjectInitalizer)// replace this if i want to make movement system with GAS and Legacy style "AC_BasePlayerCharactor::AC_BasePlayerCharactor(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	:Super(ObjectInitalizer)
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

	camera = CreateDefaultSubobject<UCameraComponent>("camera");
	camera->SetupAttachment(springArm, USpringArmComponent::SocketName);
	camera->bUsePawnControlRotation = false;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);

	bAlwaysRelevant = true;

	deadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
	effectRemoveOnDeathTag = FGameplayTag::RequestGameplayTag(FName("State.RemoveOnDeath"));

	AIControllerClass = AC_PlayerAIController::StaticClass();
}

void AC_BasePlayerCharactor::SetupMappingContext()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
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

	if (abilitySystemComponent.IsValid())
	{
		// 초기 Mana 설정
		if (const UC_ChracterAttributeSetBase* Attributes =
			abilitySystemComponent->GetSet<UC_ChracterAttributeSetBase>())
		{
			abilitySystemComponent->SetNumericAttributeBase(
				Attributes->GetmanaAttribute(), 0.0f);
			abilitySystemComponent->SetNumericAttributeBase(
				Attributes->GetmaxManaAttribute(), 100.0f);
		}

		// Mana Regen GE 적용
		if (GE_ManaRegen)
		{
			FGameplayEffectContextHandle EffectContext =
				abilitySystemComponent->MakeEffectContext();

			FGameplayEffectSpecHandle SpecHandle =
				abilitySystemComponent->MakeOutgoingSpec(
					GE_ManaRegen,
					1.0f,
					EffectContext
				);

			abilitySystemComponent->ApplyGameplayEffectSpecToSelf(
				*SpecHandle.Data
			);

			UE_LOG(LogTemp, Log, TEXT("[Player] Mana Regen started"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Player] GE_ManaRegen not set!"));
		}
	}
}

// Called every frame
void AC_BasePlayerCharactor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//UpdateStamina();
}

// Called to bind functionality to input
void AC_BasePlayerCharactor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) 
	{
		// Moving
		EnhancedInputComponent->BindAction(moveAction, ETriggerEvent::Triggered, this, &AC_BasePlayerCharactor::MyMove);

		// Looking
		EnhancedInputComponent->BindAction(lookAction, ETriggerEvent::Triggered, this, &AC_BasePlayerCharactor::MyLook);

		//// Sprinting
		//EnhancedInputComponent->BindAction(sprintAction, ETriggerEvent::Triggered, this, &AC_BasePlayerCharactor::StartSprint);
		//EnhancedInputComponent->BindAction(sprintAction, ETriggerEvent::Completed, this, &AC_BasePlayerCharactor::EndSprint);

		////Attacking
		//EnhancedInputComponent->BindAction(attackAction, ETriggerEvent::Started, this, &AC_BasePlayerCharactor::OnAttack);
	}
	else
	{
		UE_LOG(LogBasePlayerCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AC_BasePlayerCharactor::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 로스터 교체 시 비소유 상태로 스폰된 캐릭터는 BeginPlay에서
	// MappingContext가 등록되지 않으므로 여기서 다시 시도
	SetupMappingContext();

	AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();

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

	// 다음 소유 시 ASC 입력 바인딩이 다시 실행되도록 리셋
	bASCInputBound = false;
}

void AC_BasePlayerCharactor::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();

	if (PS)
	{
		InitializeStartingValues(PS);

		BindASCInput();
	}
}

void AC_BasePlayerCharactor::BindASCInput()
{
	if (bASCInputBound || !abilitySystemComponent.IsValid() /* && IsValid(InputComponent)*/) // IsValid(InputComponent) this is for legacy input
		return;

	/*
	abilitySystemComponenet->BindAbilityActivationToInputComponent(InputComponent, FGameplayAbilityInputBind(FString("ConfirmTarget"), FString("CancelTarget"), FString("ProjectBBKAbilityID"), static_cast<int32>(ProjectBBKAbilityID::Confirm), static_cast<int32>(ProjectBBKAbilityID::Cancel)));
	bASCInputBound = true;
	*/

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent))
		{
			// Ability Input Mapping (예: GA_Sprint, GA_Attack 등)
			const FGameplayAbilityInputBinds Binds(
				TEXT("Confirm"), 
				TEXT("Cancel"), 
				FTopLevelAssetPath(TEXT("/Script/ProjectBBK"), TEXT("ProjectBBKAbilityID")),
				static_cast<int32>(ProjectBBKAbilityID::Confirm),
				static_cast<int32>(ProjectBBKAbilityID::Cancel)
			);

			abilitySystemComponent->BindAbilityActivationToInputComponent(EIC, Binds);
			bASCInputBound = true;
		}
	}
}

void AC_BasePlayerCharactor::InitializeStartingValues(AC_PlayerState* PS)
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

	abilitySystemComponent->SetTagMapCount(deadTag, 0);

	InitializeAttributes();

	SetHealth(GetMaxHealth());
	SetShield(GetMaxShield());

	if (abilitySystemComponent.IsValid() && attributeSetBase.IsValid())
	{
		// Health 변경 감지
		abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			attributeSetBase->GethealthAttribute()
		).AddUObject(this, &AC_BasePlayerCharactor::OnHealthChanged);

		// Mana 변경 감지 (다시!)
		abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			attributeSetBase->GetmanaAttribute()
		).AddUObject(this, &AC_BasePlayerCharactor::OnManaChangedInternal);

		// Shield 변경 감지
		abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			attributeSetBase->GetshieldAttribute()
		).AddUObject(this, &AC_BasePlayerCharactor::OnShieldChanged);

		abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			attributeSetBase->GetmoveSpeedAttribute()
		).AddUObject(this, &AC_BasePlayerCharactor::OnMoveSpeedChanged);

		abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			attributeSetBase->GetstaminaAttribute()
		).AddUObject(this, &AC_BasePlayerCharactor::OnStaminaChanged);

		if (attributeSetBase.IsValid())
		{
			float InitialMoveSpeed = attributeSetBase->GetmoveSpeed();
			if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
			{
				MovementComp->MaxWalkSpeed = InitialMoveSpeed;

				UE_LOG(LogBasePlayerCharacter, Log, TEXT("Initial MoveSpeed Set: %.2f"), InitialMoveSpeed);
			}

			UE_LOG(LogBasePlayerCharacter, Log, TEXT("Attribute change delegates registered"));
		}

		FGameplayTag CanSprintTag = FGameplayTag::RequestGameplayTag(FName("State.CanSprint"));
		abilitySystemComponent->AddLooseGameplayTag(CanSprintTag);
	}

	// Input binding (client)
	BindASCInput();
}

UAbilitySystemComponent* AC_BasePlayerCharactor::GetAbilitySystemComponent() const
{
	return abilitySystemComponent.Get();
}

bool AC_BasePlayerCharactor::IsAlive() const
{
	return GetHealth() > 0.0f;
}

int32 AC_BasePlayerCharactor::GetAbilityLevel(ProjectBBKAbilityID AbilityID) const
{
	return 1; //will create sort of system that helps I keep track of all my abilites
}

void AC_BasePlayerCharactor::RemoveCharacterAbilities()
{
	//UC_CharacterASC* ASC = abilitySystemComponent.Get(); if abilitySystemComponent cause errors because it's a TWeakObjectPtr, we need to call .Get() to get the actual pointer

	if(GetLocalRole() != ROLE_Authority || !abilitySystemComponent.IsValid() || abilitySystemComponent->characterAbilitiesGiven)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;

	for(const FGameplayAbilitySpec& Spec : abilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.SourceObject == this && characterAbilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	for(int32 i = 0; i < AbilitiesToRemove.Num(); i++)
	{
		abilitySystemComponent->ClearAbility(AbilitiesToRemove[i]);
	}

	abilitySystemComponent->characterAbilitiesGiven = false;
}

void AC_BasePlayerCharactor::Die()
{
	RemoveCharacterAbilities();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->Velocity = FVector(0);

	onCharacterDied.Broadcast(this);

	if(abilitySystemComponent.IsValid())
	{
		abilitySystemComponent->CancelAbilities();
		
		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(effectRemoveOnDeathTag);
		int32 NumEffectsRemoved = abilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);
		abilitySystemComponent->AddLooseGameplayTag(deadTag);
	}

	if (deathMontage)
	{
		PlayAnimMontage(deathMontage);
	}
	else
	{
		FinishDying();
	}
}

void AC_BasePlayerCharactor::FinishDying()
{
	Destroy();
}

float AC_BasePlayerCharactor::GetCharacterLevel() const
{
	if (attributeSetBase.IsValid())
	{
		return attributeSetBase->Getlevel(); // In C_ChracterAttributeSetBase, we defined Gethealth() to return the health attribute value. if you confused, check "ATTRIBUTE_ACCESSORS" in C_ChracterAttributeSetBase.h
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

void AC_BasePlayerCharactor::MyMove(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		//Get 2D vector (x = forward/backward, y = right/left)
		FVector2D MovementVector = Value.Get<FVector2D>();

		//Get forward and right direction
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		//adding Movement forward and right
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AC_BasePlayerCharactor::MyLook(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

//// move to GAS
//void AC_BasePlayerCharactor::StartSprint()
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
//}
//
//void AC_BasePlayerCharactor::EndSprint()
//{
//	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
//	bIsSprinting = false;
//}

void AC_BasePlayerCharactor::OnAttack(const FInputActionValue& Value)
{
	if (!abilitySystemComponent.IsValid()) return;

	//const int32 AttackInputID = static_cast<int32>(ProjectBBKAbilityID::Attack);

	//FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Input.Attack");

	//FGameplayTagContainer AbilityTags;
	//AbilityTags.AddTag(Tag);

	//abilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(Tag));
}

//// moved to GAS
//void AC_BasePlayerCharactor::UpdateStamina()
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
//}

void AC_BasePlayerCharactor::AddCharacterAbilities()
{
	if (GetLocalRole() != ROLE_Authority || !abilitySystemComponent.IsValid() || !abilitySystemComponent->characterAbilitiesGiven)
	{
		return;
	}

	for(TSubclassOf<UC_CharacterGA>& StartupAbility : characterAbilities)
	{
		abilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(
				StartupAbility,
				GetAbilityLevel(StartupAbility.GetDefaultObject()->abilityID),
				static_cast<int32>(StartupAbility.GetDefaultObject()->abilityInputID),
				this
			)
		);

	}

	//for checking test gameplay ability

	//abilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UC_TestGA::StaticClass(), 1, 0));

	abilitySystemComponent->characterAbilitiesGiven = true;
}

void AC_BasePlayerCharactor::InitializeAttributes()
{
	if(!abilitySystemComponent.IsValid())
	{
		return;
	}

	if (!defaultAttributes)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Missing DefaultAttributes for %s. Please fill in the character's blueprint"), *FString(__FUNCTION__), *GetName());
		return;
	}

	FGameplayEffectContextHandle EffectContext = abilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle NewHandle = abilitySystemComponent->MakeOutgoingSpec(defaultAttributes, GetCharacterLevel(), EffectContext);

	if(NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = abilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), abilitySystemComponent.Get());
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

	for(TSubclassOf<UGameplayEffect>& GameplayEffect : startupEffects)
	{
		FGameplayEffectSpecHandle NewHandle = abilitySystemComponent->MakeOutgoingSpec(GameplayEffect, GetCharacterLevel(), EffectContext);
		if(NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle = abilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), abilitySystemComponent.Get());
		}
	}

	abilitySystemComponent->startupEffectsApplied = true;
}

void AC_BasePlayerCharactor::SetHealth(float NewHealth)
{
	if(attributeSetBase.IsValid())
	{
		attributeSetBase->Sethealth(NewHealth); // this code is dangerous cuz it directly sets the health value, bypassing any gameplay effects or modifiers

		//ApplyModToAttribute(C_CharacterAttributeSetBase::GetHealthAttribute(), EGameplayModOp::Additive, NewHealth); 
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

void AC_BasePlayerCharactor::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	float Health = Data.NewValue;
	float MaxHealth = GetMaxHealth();

	//UE_LOG(LogBasePlayerCharacter, Log, TEXT("Health Changed: %.2f / %.2f"), Health, MaxHealth);

	// 사망 처리
	if (Health <= 0.0f && IsAlive())
	{
		Die();
	}
}

void AC_BasePlayerCharactor::OnManaChangedInternal(const FOnAttributeChangeData& Data)
{
	float Mana = Data.NewValue;
	float MaxMana = GetMaxMana();
	float Percent = (MaxMana > 0.0f) ? (Mana / MaxMana) : 0.0f;

	// UI 업데이트 브로드캐스트
	OnManaChanged.Broadcast(Percent);

	//UE_LOG(LogBasePlayerCharacter, Log, TEXT("Mana Changed: %.2f / %.2f (%.1f%%)"), Mana, MaxMana, Percent * 100.0f);

	//if (Percent >= 1.0f)
	//{
	//	UE_LOG(LogBasePlayerCharacter, Warning, TEXT("=== MANA FULL! ULTIMATE READY! ==="));
	//}
}

void AC_BasePlayerCharactor::OnShieldChanged(const FOnAttributeChangeData& Data)
{
	float Shield = Data.NewValue;
	float MaxShield = GetMaxShield();

	//UE_LOG(LogBasePlayerCharacter, Log, TEXT("Shield Changed: %.2f / %.2f"), Shield, MaxShield);
}

void AC_BasePlayerCharactor::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	float NewMoveSpeed = Data.NewValue;
	float OldMoveSpeed = Data.OldValue;

	/*UE_LOG(LogBasePlayerCharacter, Log, TEXT("MoveSpeed Changed: %.2f -> %.2f"),
		OldMoveSpeed, NewMoveSpeed);*/

	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = NewMoveSpeed;

		//UE_LOG(LogBasePlayerCharacter, Log, TEXT("MaxWalkSpeed Updated: %.2f"), NewMoveSpeed);
	}
}

void AC_BasePlayerCharactor::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	float Stamina = Data.NewValue;
	float MaxStamina = GetMaxStamina();

	//UE_LOG(LogBasePlayerCharacter, Log, TEXT("Shield Changed: %.2f / %.2f"), Stamina, MaxStamina);

	//if (Data.NewValue <= 0.f)
	//{
	//	abilitySystemComponent->CancelAbilities(
	//		nullptr,   // WithTags
	//		nullptr,   // WithoutTags
	//		SprintAbilityClass
	//	);
	//}
}
