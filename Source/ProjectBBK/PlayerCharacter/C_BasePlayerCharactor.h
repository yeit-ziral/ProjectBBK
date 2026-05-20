// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Logging/LogMacros.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "Delegates/DelegateCombinations.h"
#include "../ProjectBBK.h"
#include "C_PlayerState.h"
#include "C_BasePlayerCharactor.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
struct FInputActionInstance;

DECLARE_LOG_CATEGORY_EXTERN(LogBasePlayerCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AC_BasePlayerCharactor *, Character);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManaChangedDelegate, float, Percent);

USTRUCT(BlueprintType)
struct FAbilityInputBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag abilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag releaseEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction *inputAction = nullptr;
};

USTRUCT()
struct FCharacterSavedState
{
	GENERATED_BODY()

	float health = -1.f;
	float stamina = -1.f;
	float shield = 0.f;
	float mana = 0.f;
};

struct FSavedGEState
{
	FGameplayEffectSpec Spec;	   // Set by Caller 포함한 전체 스펙
	float RemainingDuration = 0.f; // 남은 지속 시간
};

UCLASS(Blueprintable, config = Game)
class PROJECTBBK_API AC_BasePlayerCharactor : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AC_BasePlayerCharactor(const class FObjectInitializer &ObjectInitalizer); // replace this if i want to make movement system with GAS "AC_BasePlayerCharactor(const class FObjectInitializer& ObjectInitializer)"

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

	virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

	UPROPERTY(BlueprintAssignable, Category = "ProjectBBK|Character")
	FCharacterDiedDelegate onCharacterDied;

	UPROPERTY(BlueprintAssignable, Category = "ProjectBBK|Character|UI")
	FOnManaChangedDelegate OnManaChanged;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character")
	virtual bool IsAlive() const;

	virtual void RemoveCharacterAbilities();

	virtual void Die();

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character")
	virtual void FinishDying();

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS")
	void OnASCInitialized();

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetCharacterLevel() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetExp() const;
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetMaxExp() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetHealth() const;
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetShield() const;
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetMaxShield() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetMana() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetMaxMana() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetStamina() const;
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Camera")
	void SetTopDownCamera(bool bTopDown, float BlendTime = 0.75f,
						  float TopDownArmLength = 1600.f, float TopDownPitch = -85.f);

	void SaveCharacterState();	  // 교체 직전에 캐릭터 상태 저장
	void RestoreCharacterState(); // 교체 후 저장된 캐릭터 상태 복원 (Health, Stamina, Shield, Mana 등)

	void SaveActiveEffects(UAbilitySystemComponent *ASC);	 // 캐릭터 교체 시 활성화 된 GE 저장
	void RestoreActiveEffects(UAbilitySystemComponent *ASC); // 캐릭터 교체 후 저장된 GE 복원

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController *NewController) override;

	virtual void UnPossessed() override;

	virtual void OnRep_PlayerState() override;

	// MappingContext 등록 (BeginPlay + PossessedBy 양쪽에서 호출)
	void SetupMappingContext();

	void InitializeStartingValues(AC_PlayerState *PS);

	// Movement
	void MyMove(const FInputActionValue &Value);
	void MyLook(const FInputActionValue &Value);

	//// Sprint
	// void StartSprint();
	// void EndSprint();

	////Stamina
	// void UpdateStamina();

	// Attack
	void OnAttack(const FInputActionValue &Value);

	virtual void AddCharacterAbilities();

	virtual void InitializeAttributes();

	virtual void AddStartupEffects();

	virtual void SetHealth(float NewHealth);

	virtual void SetShield(float NewShield);

	virtual void SetStamina(float NewStamina);

	virtual void SetMana(float NewMana);

	virtual void OnHealthChanged(const FOnAttributeChangeData &Data);
	virtual void OnManaChangedInternal(const FOnAttributeChangeData &Data);
	virtual void OnShieldChanged(const FOnAttributeChangeData &Data);
	virtual void OnMoveSpeedChanged(const FOnAttributeChangeData &Data);
	virtual void OnStaminaChanged(const FOnAttributeChangeData &Data);

protected:
	// 카메라 블렌딩
	bool bCameraBlending = false;
	bool bTopDownActive = false;
	float cameraBlendRemain = 0.f;
	float targetArmLength = 400.f;
	FRotator targetSpringArmRotation = FRotator::ZeroRotator;
	float defaultArmLength = 400.f;
	FRotator defaultSpringArmRotation = FRotator::ZeroRotator;
	FTimerHandle cameraRestoreTimerHandle;
	void RestoreCameraAfterBlend();
	UPROPERTY(EditAnywhere, Category = "Movement")
	float staminaDrainTime;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float staminaRefillTime;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float delayBeforeRefill;

	float curRefillDelayTime;

	FVector movementInput;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float walkSpeed = 600.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float sprintSpeed = 900.0f;

	//// Moved to Sprinting GAS
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	// float curStamina = 100.0f;
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	// float max_Stamina = 100.0f;
	// bool bHasStamina = true;
	// bool bIsSprinting = false;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent *springArm;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent *camera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext *playerMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction *moveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction *lookAction;

	// 멀티캐릭터 (Week 1에서 구현 예정)
	int32 ActiveCharacterIndex = 0;

	TWeakObjectPtr<class UC_CharacterASC> abilitySystemComponent;

	TWeakObjectPtr<class UC_ChracterAttributeSetBase> attributeSetBase;

	FGameplayTag deadTag;
	FGameplayTag effectRemoveOnDeathTag;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ProjectBBK|Character")
	FText characterName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ProjectBBK|Animation")
	UAnimMontage *deathMontage;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ProjectBBK|Abilities")
	TArray<TSubclassOf<class UC_CharacterGA>> characterAbilities; // Abilities to give to character when possessed

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectBBK|Abilities")
	TArray<FAbilityInputBinding> abilityInputBindings;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ProjectBBK|Abilities")
	TSubclassOf<class UGameplayEffect> defaultAttributes; // Initialize default values of character's attributes

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ProjectBBK|Abilities")
	TArray<TSubclassOf<class UGameplayEffect>> startupEffects; // any other gameplay effects to apply like glowing, etc.

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mana")
	TSubclassOf<UGameplayEffect> GE_ManaRegen;

	// 캐릭터 교체 시 활성화 된 GE 저장
	TArray<FSavedGEState> savedActiveEffects;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectBBK|Attribute")
	float baseDamage = 200.f;

private:
	TMap<const UInputAction *, FGameplayTag> abilityTagMap;
	TMap<const UInputAction *, FGameplayTag> releaseEventTagMap;

	void OnAbilityInputPressed(const FInputActionInstance &Instance);
	void OnAbilityInputReleased(const FInputActionInstance &Instance);

	bool bCharacterInitiailized = false;

	FCharacterSavedState savedState;
};
