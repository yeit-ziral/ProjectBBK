#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "ANC_PlayerGameplayEvent.generated.h"

/**
 * 플레이어 몽타주용 범용 AnimNotify.
 * 지정한 eventTag를 소유 액터(캐릭터)에게 GameplayEvent로 보낸다.
 * 캐릭터가 IAbilitySystemInterface를 구현하므로, 이벤트는 PlayerState의 ASC로 전달된다.
 *
 * 예) AM_MeleeAttack의 콤보 판정 프레임에 배치 → Event.Combo.Window
 */
UCLASS(meta = (DisplayName = "Player Gameplay Event"))
class PROJECTBBK_API UANC_PlayerGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 몽타주 타임라인의 마커에 표시될 이름. 태그를 그대로 보여줘서 마커를 클릭하지 않아도 무슨 이벤트인지 알 수 있게 한다.
	virtual FString GetNotifyName_Implementation() const override;

	// 이벤트를 전달할 GameplayTag.
	UPROPERTY(EditAnywhere, Category = "Player")
	FGameplayTag eventTag;

	// 이벤트를 전달할 GameplayEventData. 단계별 배율 같은 것을 넣을 수 있다.
	UPROPERTY(EditAnywhere, Category = "Player")
	float eventMagnitude = 1.f;
};