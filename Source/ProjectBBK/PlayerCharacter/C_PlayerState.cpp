// Fill out your copyright notice in the Description page of Project Settings.


#include "C_PlayerState.h"
#include "../GAS/Attributes/C_ChracterAttributeSetBase.h"
#include "../GAS/Abilities/C_CharacterASC.h"

AC_PlayerState::AC_PlayerState()
{
	abilitySystemComponent = CreateDefaultSubobject<UC_CharacterASC>(TEXT("AbilitySystemComponent"));
	abilitySystemComponent->SetIsReplicated(true);
	abilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	attributeSetBase = CreateDefaultSubobject<UC_ChracterAttributeSetBase>(TEXT("AttributeSetBase"));

	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AC_PlayerState::GetAbilitySystemComponent() const
{
	return abilitySystemComponent;
}

UC_ChracterAttributeSetBase* AC_PlayerState::GetAttributeSetBase() const
{
	return attributeSetBase;
}

bool AC_PlayerState::IsAlive() const
{
	return GetHealth() > 0.0f;
}

void AC_PlayerState::ShowAbilityConfirmCancelText(bool ShowText)
{
	// TODO ......... Implement HUD later
}

float AC_PlayerState::GetHealth() const
{
	return attributeSetBase->Gethealth();
}

float AC_PlayerState::GetMaxHealth() const
{
	return attributeSetBase->GetmaxHealth();
}

float AC_PlayerState::GetShield() const
{
	return attributeSetBase->Getshield();
}

float AC_PlayerState::GetMaxShield() const
{
	return attributeSetBase->GetmaxShield();
}

int32 AC_PlayerState::GetCharacterLevel() const
{
	return attributeSetBase->Getlevel();
}

float AC_PlayerState::GetStamina() const
{
	return attributeSetBase->Getstamina();
}

float AC_PlayerState::GetMaxStamina() const
{
	return attributeSetBase->GetmaxStamina();
}

void AC_PlayerState::BeginPlay()
{
	Super::BeginPlay();

	// to call functions from here when each kind of event happened
	if (abilitySystemComponent)
	{
		healthChangedDelegateHandle			= abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attributeSetBase->GethealthAttribute()).AddUObject(this, &AC_PlayerState::HealthChanged);
		maxHealthChangedDelegateHandle		= abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attributeSetBase->GetmaxHealthAttribute()).AddUObject(this, &AC_PlayerState::MaxHealthChanged);
		shieldChangedDelegateHandle			= abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attributeSetBase->GetshieldAttribute()).AddUObject(this, &AC_PlayerState::ShieldChanged);
		maxShieldChangedDelegateHandle		= abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attributeSetBase->GetmaxShieldAttribute()).AddUObject(this, &AC_PlayerState::MaxShieldChanged);
		characterLevelChangedDelegateHandle	= abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attributeSetBase->GetlevelAttribute()).AddUObject(this, &AC_PlayerState::CharacterLevelChanged);
		maxStaminaChangedDelegateHandle		= abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attributeSetBase->GetmaxStaminaAttribute()).AddUObject(this, &AC_PlayerState::MaxStaminaChanged);
		staminaChangedDelegateHandle		= abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attributeSetBase->GetstaminaAttribute()).AddUObject(this, &AC_PlayerState::StaminaChanged);

		abilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(FName("State.Debuff.Stun")), EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AC_PlayerState::StunTagChanged); // give stun debuff or remove
	}
}

void AC_PlayerState::HealthChanged(const FOnAttributeChangeData& Data)
{
	//UE_LOG(LogTemp, Warning, TEXT("Health Changed!"));
}

void AC_PlayerState::MaxHealthChanged(const FOnAttributeChangeData& Data)
{
	//UE_LOG(LogTemp, Warning, TEXT("MaxHealth Changed!"));
}

void AC_PlayerState::ShieldChanged(const FOnAttributeChangeData& Data)
{
	//UE_LOG(LogTemp, Warning, TEXT("Shield Changed!"));
}

void AC_PlayerState::MaxShieldChanged(const FOnAttributeChangeData& Data)
{
	//UE_LOG(LogTemp, Warning, TEXT("MaxShield Changed!"));
}

void AC_PlayerState::StaminaChanged(const FOnAttributeChangeData& Data)
{
	//UE_LOG(LogTemp, Warning, TEXT("Stamina Changed!"));
}

void AC_PlayerState::MaxStaminaChanged(const FOnAttributeChangeData& Data)
{
	//UE_LOG(LogTemp, Warning, TEXT("MaxStamina Changed!"));
}

void AC_PlayerState::CharacterLevelChanged(const FOnAttributeChangeData& Data)
{
	//UE_LOG(LogTemp, Warning, TEXT("Character Level Changed!"));
}

void AC_PlayerState::StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		FGameplayTagContainer AbilityTagsToCancel;
		AbilityTagsToCancel.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability")));

		FGameplayTagContainer AbilityTagsToIgnore;
		AbilityTagsToIgnore.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.NotCanceledByStun")));

		abilitySystemComponent->CancelAbilities(&AbilityTagsToCancel, &AbilityTagsToIgnore);
	}
}