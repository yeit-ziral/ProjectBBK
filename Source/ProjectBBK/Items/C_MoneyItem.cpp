#include "C_MoneyItem.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../PlayerCharacter/PlayerAI/C_PlayerController.h"
#include "../Inventory/C_InventoryComponent.h"

void AC_MoneyItem::BeginPlay()
{
	cachedItemName = FText::Format(FText::FromString(TEXT("{0} gold")), moneyAmount);
	Super::BeginPlay();
	ApplyWorldMesh(nullptr);
}

void AC_MoneyItem::InitMoney(int32 InAmount)
{
	moneyAmount = InAmount;
	cachedItemName = FText::Format(FText::FromString(TEXT("{0} gold")), moneyAmount);
}

void AC_MoneyItem::OnInteract(AC_BasePlayerCharactor* Player)
{
	if (!Player) return;

	AC_PlayerController* PC = Cast<AC_PlayerController>(Player->GetController());
	if (!PC) return;

	UC_InventoryComponent* Inv = PC->GetInventory();
	if (!Inv) return;

	Inv->AddMoney(moneyAmount);
	Destroy();
}
