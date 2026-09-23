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

	// 스폰 직후 아이템 메시 밑면이 지면에 닿도록 Z를 맞춘다.
	// 액터 원점(= CollisionSphere 중심)과 메시 바닥의 차이를 보정하므로
	// 피벗이 메시 가운데인 에셋(코인 등)도 파묻히지 않는다.
	void SnapToGround();

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

	// 스폰 시 지면에 붙일지 — 공중에 띄워둘 아이템은 인스턴스별로 끄면 된다
	UPROPERTY(EditAnywhere, Category = "Item|Placement")
	bool bSnapToGroundOnSpawn = true;

	// 지면에 붙인 뒤 띄울 여유(cm) — 메시가 바닥과 z-fighting 나는 것 방지
	UPROPERTY(EditAnywhere, Category = "Item|Placement", meta = (ClampMin = "0.0"))
	float groundSnapOffset = 2.f;

	// 지면 탐색을 시작할 높이(cm). 너무 크게 잡으면 실내에서 천장을 먼저 맞는다.
	UPROPERTY(EditAnywhere, Category = "Item|Placement", meta = (ClampMin = "0.0"))
	float groundSnapUpMargin = 50.f;

	// 아래로 지면을 찾을 최대 거리(cm)
	UPROPERTY(EditAnywhere, Category = "Item|Placement", meta = (ClampMin = "1.0"))
	float groundSnapTraceDistance = 1000.f;

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
