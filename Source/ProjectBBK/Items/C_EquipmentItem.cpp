#include "C_EquipmentItem.h"
#include "ItemData.h"
#include "Engine/DataTable.h"

void AC_EquipmentItem::InitItem(FName InItemID)
{
	Super::InitItem(InItemID);

	if (!equipmentDataTable) return;

	FEquipmentItemData* Data = equipmentDataTable->FindRow<FEquipmentItemData>(InItemID, TEXT("EquipmentItem"));
	if (!Data) return;

	cachedItemName = Data->itemName;
	ApplyWorldMesh(Data->worldMesh);
}
