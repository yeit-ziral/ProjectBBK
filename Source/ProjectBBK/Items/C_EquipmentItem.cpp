#include "C_EquipmentItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"

void AC_EquipmentItem::InitItem(FName InItemID)
{
	Super::InitItem(InItemID);

	if (!equipmentDataTable) return;

	FEquipmentItemData* Data = equipmentDataTable->FindRow<FEquipmentItemData>(InItemID, TEXT("EquipmentItem"));
	if (!Data) return;

	cachedData = *Data;
	bDataLoaded = true;
	cachedItemName = cachedData.itemName;
	ApplyWorldMesh(cachedData.worldMesh);
}

void AC_EquipmentItem::OnInteract(AC_BasePlayerCharactor* Player)
{
	if (!bDataLoaded || !Player) return;

	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (!ASC || !GE_EquipBonus) return;

	// [임시] 인벤토리 미구현 — 즉시 장착 효과 적용 후 Destroy
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GE_EquipBonus, 1.f, ASC->MakeEffectContext());
	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Equip.MaxHealth")), cachedData.bonusMaxHealth);
	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Equip.MaxStamina")), cachedData.bonusMaxStamina);
	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Equip.MoveSpeed")), cachedData.bonusMoveSpeed);
	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Equip.Defense")), cachedData.bonusDefense);
	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Equip.Damage")), cachedData.bonusDamage);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	Destroy();
}
