#include "ANC_PlayerGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"

void UANC_PlayerGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if(!MeshComp || !eventTag.IsValid())
		return;

	// 에디터의 애니메이션 미리보기 창에서도 노티파이는 실행된다. 거기엔 진짜 게임 액터가 없으므로 게임 월드일 때만 보낸다.
	const UWorld* World = MeshComp->GetWorld();
	if(!World || !World->IsGameWorld())
		return;

	AActor* Owner = MeshComp->GetOwner();
	if(!Owner)
		return;

	FGameplayEventData EventData;
	EventData.EventTag = eventTag;
	EventData.Instigator = Owner;
	EventData.Target = Owner;
	EventData.EventMagnitude = eventMagnitude;

	// 캐릭터가 IAbilitySystemInterface를 구현하므로, 이벤트는 PlayerState의 ASC로 전달된다.
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, eventTag, EventData);
}

FString UANC_PlayerGameplayEvent::GetNotifyName_Implementation() const
{
	// 타임라인에 태그 이름을 그대로 표시한다. 태그가 없으면 클래스 기본 이름.
	return eventTag.IsValid() ? eventTag.ToString() : Super::GetNotifyName_Implementation();
}