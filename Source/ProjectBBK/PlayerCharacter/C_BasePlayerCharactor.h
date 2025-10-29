// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "InputActionValue.h"
#include "Logging/LogMacros.h"
#include "C_BasePlayerCharactor.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogBasePlayerCharacter, Log, All);

UCLASS(Blueprintable, config = Game)
class PROJECTBBK_API AC_BasePlayerCharactor : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* springArm;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* camera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* playerMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* moveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* lookAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* sprintAction;

	/** Attack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* attackAction;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	class UAbilitySystemComponent* abilitySystemComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	class UGGHealthSet* healthSet;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GAS")
	TArray < TSubclassOf<class UGGGameplayAbility>> defaultAbilities;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GAS")
	TArray < TSubclassOf<class UGameplayEffect>> defaultEffects;

public:
	// Sets default values for this character's properties
	AC_BasePlayerCharactor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Movement
	void MyMove(const FInputActionValue& Value);
	void MyLook(const FInputActionValue& Value);

	// Sprint
	void StartSprint();
	void EndSprint();

	//Stamina
	void UpdateStamina();

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void InitializeAbilities();
	virtual void InitializeEffects();

	virtual void PostInitializeComponents() override;

	virtual void OnDamageTakenChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayTagContainer& GameplayTagContainer, float Damage);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS")
	void OnDamageTaken(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayTagContainer& GameplayTagContainer, float Damage);

	virtual void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS")
	void OnHealthChanged(float OldValue, float NewValue);

	virtual void OnShieldAttributeChanged(const FOnAttributeChangeData& Data);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS")
	void OnShieldChanged(float OldValue, float NewValue);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void SendAbilityLocalInput(const FInputActionValue& Value, int32 InputID);

	void AttackAbility(const FInputActionValue& Value);

protected:

	//float Health = 100.0f;
	//const float MAX_HEALTH = 100.0f;


	UPROPERTY(EditAnywhere, Category = "Movement")
	float curStamina = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float max_Stamina = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float staminaDrainTime;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float staminaRefillTime;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float delayBeforeRefill;

	float curRefillDelayTime;
	bool bHasStamina = true;

	FVector movementInput;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float walkSpeed = 600.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float sprintSpeed = 900.0f;

	bool bIsSprinting = false;

	// ����Ī�� (Week 1���� ������)
	int32 ActiveCharacterIndex = 0;
};
