#include "C_ConsumableItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"

void AC_ConsumableItem::InitItem(FName InItemID)
{
	Super::InitItem(InItemID);

	if (!consumableDataTable) return;

	FConsumableItemData* Data = consumableDataTable->FindRow<FConsumableItemData>(InItemID, TEXT("ConsumableItem"));
	if (!Data) return;

	cachedData = *Data;
	bDataLoaded = true;
	cachedItemName = cachedData.itemName;
	ApplyWorldMesh(cachedData.worldMesh);
}

void AC_ConsumableItem::OnInteract(AC_BasePlayerCharactor* Player)
{
	if (!bDataLoaded || !Player) return;

	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (!ASC || !cachedData.consumeEffect) return;

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(cachedData.consumeEffect, 1.f, ASC->MakeEffectContext());
	if (cachedData.magnitudeTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(cachedData.magnitudeTag, cachedData.magnitude);
	}
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	Destroy();
}
