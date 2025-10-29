// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BasePlayerCharactor.h"
#include "../ProjectBBK.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "AbilitySystemComponent.h"
#include "../GAS/GGGameplayAbility.h"
#include "../GAS/AttributeSets/GGHealthSet.h"

DEFINE_LOG_CATEGORY(LogBasePlayerCharacter);

// Sets default values
AC_BasePlayerCharactor::AC_BasePlayerCharactor()
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

	abilitySystemComp = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComp"));

	healthSet = CreateDefaultSubobject<UGGHealthSet>(TEXT("HealthSet"));
}

// Called when the game starts or when spawned
void AC_BasePlayerCharactor::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(playerMappingContext, 0);
		}
	}

	healthSet->OnDamageTaken.AddUObject(this, &AC_BasePlayerCharactor::OnDamageTakenChanged);
	
	abilitySystemComp->GetGameplayAttributeValueChangeDelegate(healthSet->GetHealthAttribute()).AddUObject(this, &AC_BasePlayerCharactor::OnHealthAttributeChanged);
	abilitySystemComp->GetGameplayAttributeValueChangeDelegate(healthSet->GetHealthAttribute()).AddUObject(this, &AC_BasePlayerCharactor::OnShieldAttributeChanged);
}

// Called every frame
void AC_BasePlayerCharactor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateStamina();
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

		// Sprinting
		EnhancedInputComponent->BindAction(sprintAction, ETriggerEvent::Triggered, this, &AC_BasePlayerCharactor::StartSprint);
		EnhancedInputComponent->BindAction(sprintAction, ETriggerEvent::Completed, this, &AC_BasePlayerCharactor::EndSprint);

		// Attack Ability
		EnhancedInputComponent->BindAction(attackAction, ETriggerEvent::Started, this, &AC_BasePlayerCharactor::AttackAbility);
	}
	else
	{
		UE_LOG(LogBasePlayerCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AC_BasePlayerCharactor::SendAbilityLocalInput(const FInputActionValue& Value, int32 InputID)
{
	if(!abilitySystemComp)
		return;

	if(Value.Get<bool>())
	{
		abilitySystemComp->AbilityLocalInputPressed(InputID);
	}
	else
	{
		abilitySystemComp->AbilityLocalInputReleased(InputID);
	}
}

void AC_BasePlayerCharactor::AttackAbility(const FInputActionValue& Value)
{
	SendAbilityLocalInput(Value, static_cast<int32>(EAbilityInputID::Attack));
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

void AC_BasePlayerCharactor::StartSprint()
{
	if (bHasStamina)
	{
		GetCharacterMovement()->MaxWalkSpeed = sprintSpeed;

		if (GetVelocity().Size() >= 0.5)
		{
			bIsSprinting = true;
		}
		else
		{
			bIsSprinting = false;
		}
	}
}

void AC_BasePlayerCharactor::EndSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	bIsSprinting = false;
}

void AC_BasePlayerCharactor::UpdateStamina()
{
	if (bIsSprinting)
	{
		curStamina -= staminaDrainTime;
		curRefillDelayTime = delayBeforeRefill;
	}

	if (!bIsSprinting && curStamina < max_Stamina)
	{
		curRefillDelayTime--;

		if(curRefillDelayTime <= 0)
		{
			curStamina += staminaRefillTime;
		}
	}

	if (curStamina <= 0)
	{
		bHasStamina = false;
		EndSprint();
	}
	else
	{
		bHasStamina = true;
	}
}

UAbilitySystemComponent* AC_BasePlayerCharactor::GetAbilitySystemComponent() const
{
	return abilitySystemComp;
}

void AC_BasePlayerCharactor::InitializeAbilities()
{
	// Give Abilities, Server only
	if (!HasAuthority() || !abilitySystemComp)
		return;

	for (TSubclassOf<UGGGameplayAbility>& Ability : defaultAbilities)
	{
		FGameplayAbilitySpecHandle SpecHandle = abilitySystemComp->GiveAbility(
			FGameplayAbilitySpec(Ability, 1, static_cast<int32>(Ability.GetDefaultObject()->GetAbilityInputID()), this));
	}
}

void AC_BasePlayerCharactor::InitializeEffects()
{
	if(!abilitySystemComp)
		return;

	FGameplayEffectContextHandle EffectContext = abilitySystemComp->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect>& Effect : defaultEffects)
	{
		FGameplayEffectSpecHandle SpecHandle = abilitySystemComp->MakeOutgoingSpec(Effect, 1, EffectContext);
		if(SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle GEHandle = abilitySystemComp->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void AC_BasePlayerCharactor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if(!abilitySystemComp)
		return;

	abilitySystemComp->InitAbilityActorInfo(this, this);

	InitializeEffects();
	InitializeAbilities();
}

void AC_BasePlayerCharactor::OnDamageTakenChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayTagContainer& GameplayTagContainer, float Damage)
{
	OnDamageTaken(DamageInstigator, DamageCauser, GameplayTagContainer, Damage);
}

void AC_BasePlayerCharactor::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	OnHealthChanged(Data.OldValue, Data.NewValue);
}

void AC_BasePlayerCharactor::OnShieldAttributeChanged(const FOnAttributeChangeData& Data)
{
	OnShieldChanged(Data.OldValue, Data.NewValue);
}