// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MerchantData.h"
#include "C_MerchantNPC.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UWidgetComponent;
class UDataTable;

/**
 * 아이템을 파는 상인 NPC.
 * - 시선(카메라 전방 LineTrace)이 캡슐에 닿으면 AC_PlayerController가 SetFocused(true)를 호출 →
 *   스켈레탈 메시에 Custom Depth 외곽선 ON + 상호작용 위젯("[G] 대화") 표시.
 * - G키 상호작용 → AC_PlayerController가 대화창을 열고 이 NPC를 전달.
 *
 * 가격은 DT_MerchantStock(stockTable)에 둔다. 아이템 데이터 파일은 건드리지 않음.
 */
UCLASS()
class PROJECTBBK_API AC_MerchantNPC : public AActor
{
	GENERATED_BODY()

public:
	AC_MerchantNPC();

	// 시선 포커스 상태 토글 — 외곽선 + 상호작용 위젯 표시.
	void SetFocused(bool bFocused);

	// 대화 시작 시 talk 모션 재생 (idle 순환 중단). 대화창을 열 때 PlayerController가 호출.
	void PlayTalkAnim();

	// 대화 종료 시 idle 순환 복귀. 대화/상점을 닫을 때 PlayerController가 호출.
	void StopTalkAnim();

	// 파는 물건 전체를 런타임용 배열로 반환 (stockTable에서 Row Name = itemID로 수집).
	UFUNCTION(BlueprintCallable, Category = "Merchant")
	TArray<FMerchantStockEntry> GetStock() const;

	// 구매가 — 플레이어가 사면 지불하는 금액. stockTable의 buyPrice.
	// [가격 훅] 추후 아이템에 가치 필드가 생기면 여기서 아이템 값을 우선 조회하도록 교체.
	UFUNCTION(BlueprintPure, Category = "Merchant")
	int32 GetBuyPrice(FName itemID) const;

	// 판매가 — 플레이어가 팔면 받는 금액. 재고에 있으면 buyPrice * sellPriceRatio, 없으면 fallbackSellPrice.
	// [가격 훅] 추후 아이템 가치 * sellPriceRatio로 교체.
	UFUNCTION(BlueprintPure, Category = "Merchant")
	int32 GetSellPrice(FName itemID) const;

	// 인사/소개 대사 (대화창에서 순서대로 표시)
	UFUNCTION(BlueprintPure, Category = "Merchant")
	const TArray<FText>& GetDialogueLines() const { return dialogueLines; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Merchant")
	UCapsuleComponent* collisionCapsule;

	UPROPERTY(VisibleAnywhere, Category = "Merchant")
	USkeletalMeshComponent* npcMesh;

	UPROPERTY(VisibleAnywhere, Category = "Merchant")
	UWidgetComponent* interactionWidgetComp;

	// 상호작용 안내 위젯 (WBP_Interaction 지정) — SetInteractionText로 "[G] 대화" 표시
	UPROPERTY(EditDefaultsOnly, Category = "Merchant|UI")
	TSubclassOf<UUserWidget> interactionWidgetClass;

	// 포커스 시 위젯에 표시할 안내 문구
	UPROPERTY(EditDefaultsOnly, Category = "Merchant|UI")
	FText interactionPrompt;

	// 파는 물건 목록 (Row 구조체: FMerchantStockRow). BP에서 DT_MerchantStock 지정.
	UPROPERTY(EditAnywhere, Category = "Merchant")
	UDataTable* stockTable = nullptr;

	// 인사 대사
	UPROPERTY(EditAnywhere, Category = "Merchant", meta = (MultiLine = "true"))
	TArray<FText> dialogueLines;

	// 판매가 배율 (구매가 대비) — 0.5면 산 값의 절반에 되팖
	UPROPERTY(EditAnywhere, Category = "Merchant", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float sellPriceRatio = 0.5f;

	// 재고에 없는 아이템을 팔 때 지급할 기본 판매가
	UPROPERTY(EditAnywhere, Category = "Merchant", meta = (ClampMin = "0"))
	int32 fallbackSellPrice = 5;

	// 외곽선 Custom Depth 스텐실 값 (포스트프로세스 아웃라인 머티리얼과 매칭)
	UPROPERTY(EditAnywhere, Category = "Merchant|Highlight", meta = (ClampMin = "0", ClampMax = "255"))
	int32 highlightStencilValue = 1;

	// 평소 재생할 idle 모션 2종 (번갈아 재생). BP에서 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Merchant|Anim")
	UAnimSequenceBase* idleAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Merchant|Anim")
	UAnimSequenceBase* idle2Anim = nullptr;

	// 대화 중 재생할 talk 모션. BP에서 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Merchant|Anim")
	UAnimSequenceBase* talkAnim = nullptr;

private:
	// stockTable에서 itemID로 Row를 찾음 — 없으면 nullptr
	const FMerchantStockRow* FindStockRow(FName itemID) const;

	// idle/idle2 중 하나를 재생하고, 끝나는 시점에 다시 자신을 호출하도록 타이머 설정 (번갈아 순환).
	void PlayRandomIdle();

	FTimerHandle idleTimerHandle;
	bool bTalking = false;
};
