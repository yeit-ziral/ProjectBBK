#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BaseItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class AC_BasePlayerCharactor;

UCLASS(Abstract)
class PROJECTBBK_API AC_BaseItem : public AActor
{
	GENERATED_BODY()

public:
	AC_BaseItem();

	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void InitItem(FName InItemID);

	virtual void OnInteract(AC_BasePlayerCharactor* Player);

	// 런타임에 플레이어와 이미 겹친 상태로 스폰된 경우(예: TreasureChest 드랍) BeginOverlap이
	// 델리게이트 바인딩 이전에 지나가 버려 상호작용 UI/입력 등록이 안 될 수 있다.
	// 스폰 직후(데이터 초기화 이후) 호출해 현재 오버랩 상태를 다시 반영한다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void RefreshOverlapState();

protected:
	virtual void BeginPlay() override;

	void ApplyWorldMesh(UStaticMesh* Mesh);

	UPROPERTY(VisibleAnywhere, Category = "Item")
	USphereComponent* collisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	UStaticMeshComponent* itemMesh;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	UWidgetComponent* interactionWidgetComp;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Mesh")
	UStaticMesh* defaultMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Item|UI")
	TSubclassOf<UUserWidget> interactionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName itemID;

	FText cachedItemName;

private:
	void NotifyPlayerInRange(AC_BasePlayerCharactor* Player);

	UFUNCTION()
	void OnItemBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnItemEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
};
