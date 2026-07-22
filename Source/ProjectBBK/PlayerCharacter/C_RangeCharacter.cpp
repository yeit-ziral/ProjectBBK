// Fill out your copyright notice in the Description page of Project Settings.

#include "C_RangeCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Monster/C_BaseMonster.h"

AC_RangeCharacter::AC_RangeCharacter(const class FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AC_RangeCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AC_RangeCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	RegisterAmmoDelegate();
}

void AC_RangeCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	RegisterAmmoDelegate();
}

void AC_RangeCharacter::RegisterAmmoDelegate()
{
	if (bAmmoDelegateRegistered)
		return;

	AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();

	if (!PS)
		return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	UC_ChracterAttributeSetBase* AttrSet = PS->GetAttributeSetBase();
	if (!ASC || !AttrSet)
		return;

	ASC->GetGameplayAttributeValueChangeDelegate(AttrSet->GetammoAttribute()).AddUObject(this, &AC_RangeCharacter::OnAmmoChangedInternal);

	bAmmoDelegateRegistered = true;

	OnAmmoChanged.Broadcast(AttrSet->Getammo(), AttrSet->GetmaxAmmo());
}

void AC_RangeCharacter::OnAmmoChangedInternal(const FOnAttributeChangeData& Data)
{
	OnAmmoChanged.Broadcast(Data.NewValue, GetMaxAmmo());

	if (Data.NewValue <= 0.0f)
	{
		TryAutoReload();
	}
}

void AC_RangeCharacter::TryAutoReload()
{
	AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();
	if (!PS)
		return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
		return;

	// 이미 재장전 중이면 재장전 시도하지 않음
	if(ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Reloading"))))
		return;

	// 태그로 GA_Reload 활성화 시도
	FGameplayTagContainer ReloadTag;
	ReloadTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Reload")));
	ASC->TryActivateAbilitiesByTag(ReloadTag);
}

void AC_RangeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AC_RangeCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

AC_BaseMonster *AC_RangeCharacter::GetHighestPriorityTarget() const
{
	TArray<AActor *> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor *> ActorsToIgnore;
	ActorsToIgnore.Add(const_cast<AC_RangeCharacter *>(this));

	UKismetSystemLibrary::SphereOverlapActors(
		this,
		GetActorLocation(),
		detectionRange,
		ObjectTypes,
		AC_BaseMonster::StaticClass(),
		ActorsToIgnore,
		OverlappedActors);

	AC_BaseMonster *BestTarget = nullptr;
	float BestDistSq = FLT_MAX;

	for (AActor *Actor : OverlappedActors)
	{
		AC_BaseMonster *Monster = Cast<AC_BaseMonster>(Actor);
		if (!Monster || Monster->GetcurHP() <= 0)
			continue;

		float DistSq = FVector::DistSquared(GetActorLocation(), Monster->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Monster;
		}
	}

	return BestTarget;
}

void AC_RangeCharacter::FaceTarget(AActor *Target)
{
	if (!Target)
		return;

	FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
		SetActorRotation(Direction.Rotation());
}

float AC_RangeCharacter::GetAmmo() const
{
	const AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();
	return(PS && PS->GetAttributeSetBase()) ? PS->GetAttributeSetBase()->Getammo() : 0.f;
}

float AC_RangeCharacter::GetMaxAmmo() const
{
	const AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();
	return(PS && PS->GetAttributeSetBase()) ? PS->GetAttributeSetBase()->GetmaxAmmo() : 0.f;
}

float AC_RangeCharacter::GetReloadTime() const
{
	const AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();
	return(PS && PS->GetAttributeSetBase()) ? PS->GetAttributeSetBase()->GetreloadTime() : 0.f;
}

void AC_RangeCharacter::RefillAmmo()
{
	if (!HasAuthority())
		return;

	AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();
	if(!PS || !PS->GetAttributeSetBase())
		return;

	UC_ChracterAttributeSetBase* AttrSet = PS->GetAttributeSetBase();
	AttrSet->Setammo(AttrSet->GetmaxAmmo());
}

void AC_RangeCharacter::NotifyReloadStarted()
{
	OnReloadStarted.Broadcast(GetReloadTime());
}

void AC_RangeCharacter::NotifyReloadFinished()
{
	OnReloadFinished.Broadcast();
}
