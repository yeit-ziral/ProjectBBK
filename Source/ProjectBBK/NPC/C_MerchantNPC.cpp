// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MerchantNPC.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "../Items/C_InteractionWidget.h"

// 애님 블루프린트(ABP_Merchant)의 슬롯 이름과 일치해야 함
static const FName MerchantAnimSlot(TEXT("DefaultSlot"));

AC_MerchantNPC::AC_MerchantNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	// 캡슐 = 시선 트레이스(Visibility 채널) 대상. 정확한 조준 판정 담당.
	collisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	SetRootComponent(collisionCapsule);
	collisionCapsule->InitCapsuleSize(45.f, 90.f);
	collisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	collisionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	collisionCapsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	npcMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NpcMesh"));
	npcMesh->SetupAttachment(collisionCapsule);
	npcMesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	npcMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	npcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	interactionWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
	interactionWidgetComp->SetupAttachment(collisionCapsule);
	// 모델 머리 바로 위 (메시가 캡슐 기준 -90에 부착, 모델 높이 ~100 → 머리 끝 Z≈+10)
	interactionWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	interactionWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	interactionWidgetComp->SetDrawAtDesiredSize(true);
	interactionWidgetComp->SetVisibility(false);

	interactionPrompt = FText::FromString(TEXT("[G] 대화"));
}

void AC_MerchantNPC::BeginPlay()
{
	Super::BeginPlay();

	if (interactionWidgetClass)
		interactionWidgetComp->SetWidgetClass(interactionWidgetClass);

	// 외곽선은 포커스 전엔 꺼둠. 스텐실 값만 미리 지정.
	npcMesh->SetCustomDepthStencilValue(highlightStencilValue);
	npcMesh->SetRenderCustomDepth(false);

	// 평소 idle/idle2 순환 시작
	PlayRandomIdle();
}

void AC_MerchantNPC::PlayRandomIdle()
{
	if (bTalking)
		return;

	// idle 2종이 다 있으면 번갈아(랜덤), 하나만 있으면 그것만.
	UAnimSequenceBase* pick = nullptr;
	if (idleAnim && idle2Anim)
		pick = FMath::RandBool() ? idleAnim : idle2Anim;
	else
		pick = idleAnim ? idleAnim : idle2Anim;

	if (!pick)
		return;

	UAnimInstance* anim = npcMesh->GetAnimInstance();
	if (!anim)
		return;

	// 슬롯에 다이나믹 몽타주로 1회 재생 (블렌드 인/아웃 0.4초 → 다음 idle과 크로스페이드).
	anim->PlaySlotAnimationAsDynamicMontage(pick, MerchantAnimSlot, 0.4f, 0.4f, 1.0f, 1);

	// 클립이 끝나기 0.4초 전에 다음 idle을 시작해 겹치며 부드럽게 전환.
	const float len = pick->GetPlayLength();
	GetWorldTimerManager().SetTimer(idleTimerHandle, this, &AC_MerchantNPC::PlayRandomIdle,
		FMath::Max(0.2f, len - 0.4f), false);
}

void AC_MerchantNPC::PlayTalkAnim()
{
	bTalking = true;
	GetWorldTimerManager().ClearTimer(idleTimerHandle);

	if (UAnimInstance* anim = npcMesh->GetAnimInstance())
	{
		if (talkAnim)
			// 대화 동안 talk 반복 (LoopCount 크게), 0.3초 블렌드.
			anim->PlaySlotAnimationAsDynamicMontage(talkAnim, MerchantAnimSlot, 0.3f, 0.3f, 1.0f, 9999);
	}
}

void AC_MerchantNPC::StopTalkAnim()
{
	bTalking = false;
	PlayRandomIdle(); // idle 순환 복귀
}

void AC_MerchantNPC::SetFocused(bool bFocused)
{
	npcMesh->SetRenderCustomDepth(bFocused);

	if (bFocused)
	{
		if (UC_InteractionWidget* Widget = Cast<UC_InteractionWidget>(interactionWidgetComp->GetWidget()))
			Widget->SetInteractionText(interactionPrompt);
	}

	interactionWidgetComp->SetVisibility(bFocused);
}

const FMerchantStockRow* AC_MerchantNPC::FindStockRow(FName itemID) const
{
	if (!stockTable || itemID.IsNone())
		return nullptr;

	// Row Name = itemID 규칙으로 우선 조회
	if (const FMerchantStockRow* Row = stockTable->FindRow<FMerchantStockRow>(itemID, TEXT("FindStockRow"), false))
		return Row;

	// Row Name과 itemID 필드가 다를 수 있으므로 전체 스캔 폴백
	for (const TPair<FName, uint8*>& Pair : stockTable->GetRowMap())
	{
		const FMerchantStockRow* Row = reinterpret_cast<const FMerchantStockRow*>(Pair.Value);
		if (Row && Row->itemID == itemID)
			return Row;
	}
	return nullptr;
}

TArray<FMerchantStockEntry> AC_MerchantNPC::GetStock() const
{
	TArray<FMerchantStockEntry> Result;
	if (!stockTable)
		return Result;

	for (const TPair<FName, uint8*>& Pair : stockTable->GetRowMap())
	{
		const FMerchantStockRow* Row = reinterpret_cast<const FMerchantStockRow*>(Pair.Value);
		if (!Row)
			continue;

		// itemID 필드가 비어 있으면 Row Name을 itemID로 사용
		const FName ItemID = Row->itemID.IsNone() ? Pair.Key : Row->itemID;
		if (ItemID.IsNone())
			continue;

		Result.Emplace(ItemID, Row->buyPrice, Row->stock);
	}
	return Result;
}

int32 AC_MerchantNPC::GetBuyPrice(FName itemID) const
{
	// [가격 훅] 추후 아이템 가치 필드가 생기면 여기서 우선 조회.
	if (const FMerchantStockRow* Row = FindStockRow(itemID))
		return Row->buyPrice;
	return 0;
}

int32 AC_MerchantNPC::GetSellPrice(FName itemID) const
{
	// [가격 훅] 추후 아이템 가치 * sellPriceRatio로 교체.
	if (const FMerchantStockRow* Row = FindStockRow(itemID))
		return FMath::Max(0, FMath::RoundToInt(Row->buyPrice * sellPriceRatio));
	return fallbackSellPrice;
}
