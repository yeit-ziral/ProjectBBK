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

DECLARE_LOG_CATEGORY_EXTERN(LogBasePlayerCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AC_BasePlayerCharactor*, Character);


UCLASS(Blueprintable, config = Game)
class PROJECTBBK_API AC_BasePlayerCharactor : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()


public:
	// Sets default values for this character's properties
	AC_BasePlayerCharactor(const class FObjectInitializer& ObjectInitalizer); // replace this if i want to make movement system with GAS "AC_BasePlayerCharactor(const class FObjectInitializer& ObjectInitializer)"

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(BlueprintAssignable, Category = "ProjectBBK|Character")
	FCharacterDiedDelegate onCharacterDied;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character")
	virtual bool IsAlive() const;

	//this will get the ability level for any ability
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character")
	virtual int32 GetAbilityLevel(ProjectBBKAbilityID AbilityID) const;

	virtual void RemoveCharacterAbilities();

	virtual void Die();

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character")
	virtual void FinishDying();


	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetCharacterLevel() const;
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetHealth() const;
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetShield() const;
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Character|Attribute")
	float GetMaxShield() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	// Binding Input with GAS
	virtual void BindASCInput();

	void InitializeStartingValues(AC_PlayerState* PS);

	// Movement
	void MyMove(const FInputActionValue& Value);
	void MyLook(const FInputActionValue& Value);

	// Sprint
	void StartSprint();
	void EndSprint();

	//Stamina
	void UpdateStamina();

	virtual void AddCharacterAbilities();

	virtual void InitializeAttributes();

	virtual void AddStartupEffects();

	virtual void SetHealth(float NewHealth);

	virtual void SetShield(float NewShield);

protected:

	float Health = 100.0f;
	const float MAX_HEALTH = 100.0f;

	// Protecting from duplication
	bool bASCInputBound = false;

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

	// 스위칭용 (Week 1에는 구조만)
	int32 ActiveCharacterIndex = 0;

	TWeakObjectPtr<class UC_ChaAbilitySystemComponent> abilitySystemComponent;

	TWeakObjectPtr<class UC_ChracterAttributeSetBase> attributeSetBase;

	FGameplayTag deadTag;
	FGameplayTag effectRemoveOnDeathTag;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ProjectBBK|Character")
	FText characterName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ProjectBBK|Animation")
	UAnimMontage* deathMontage;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ProjectBBK|Abilities")
	TArray<TSubclassOf<class UC_CharacterGameplayAbility>> characterAbilities; //Abilities to give to character when possessed

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ProjectBBK|Abilities")
	TSubclassOf<class UGameplayEffect> defaultAttributes; //Initialize default values of character's attributes

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ProjectBBK|Abilities")
	TArray<TSubclassOf<class UGameplayEffect>> startupEffects; // any other gameplay effects to apply like glowing, etc.
};
