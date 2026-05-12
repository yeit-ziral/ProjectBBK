// Fill out your copyright notice in the Description page of Project Settings.

#include "C_PlayerController.h"
#include "../C_PlayerState.h"
#include "../C_BasePlayerCharactor.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

void AC_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 서버(싱글플레이어 포함)에서만 스폰
	if (!HasAuthority())
		return;

	// 스폰 위치: 레벨에 배치된 첫 번째 PlayerStart 기준
	FTransform SpawnTransform = FTransform::Identity;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		SpawnTransform = It->GetActorTransform();
		break;
	}

	if (characterRosterClasses.Num() == 0)
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (TSubclassOf<AC_BasePlayerCharactor> &CharClass : characterRosterClasses)
	{
		if (!CharClass)
			continue;

		AC_BasePlayerCharactor *Char = GetWorld()->SpawnActor<AC_BasePlayerCharactor>(
			CharClass, SpawnTransform, SpawnParams);

		if (Char)
		{
			// 일단 모두 비활성화 (0번만 나중에 활성화)
			Char->SetActorHiddenInGame(true);
			Char->SetActorEnableCollision(false);
			characterRoster.Add(Char);
		}
	}

	// 0번 캐릭터로 시작
	if (characterRoster.IsValidIndex(0))
	{
		characterRoster[0]->SetActorHiddenInGame(false);
		characterRoster[0]->SetActorEnableCollision(true);
		Possess(characterRoster[0]);
		currentCharacterIndex = 0;

		// WBP_HUD는 각 캐릭터의 BeginPlay(Possess 전)에서 생성되므로
		// NativeConstruct 시점엔 어빌리티가 아직 없음.
		// Possess 완료(= AddCharacterAbilities 완료) 후 브로드캐스트해야 HUD 초기화 가능.
		OnCharacterSwitched.Broadcast(0);
	}
}

void AC_PlayerController::OnPossess(APawn *InPawn)
{
	Super::OnPossess(InPawn);

	AC_PlayerState *PS = GetPlayerState<AC_PlayerState>();
	if (PS)
	{
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, InPawn);
	}
}

void AC_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent *EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (IA_SwitchChar0)
			EIC->BindAction(IA_SwitchChar0, ETriggerEvent::Started, this, &AC_PlayerController::OnSwitchChar0Input);

		if (IA_SwitchChar1)
			EIC->BindAction(IA_SwitchChar1, ETriggerEvent::Started, this, &AC_PlayerController::OnSwitchChar1Input);

		if (IA_SwitchCharNext)
			EIC->BindAction(IA_SwitchCharNext, ETriggerEvent::Started, this, &AC_PlayerController::SwitchToNextCharacter);
	}
}

void AC_PlayerController::OnSwitchChar0Input()
{
	SwitchToCharacter(0);
}

void AC_PlayerController::OnSwitchChar1Input()
{
	SwitchToCharacter(1);
}

void AC_PlayerController::SwitchToCharacter(int32 NextIndex)
{
	if (bIsSwitching)
		return;
	if (NextIndex == currentCharacterIndex)
		return;
	if (!characterRoster.IsValidIndex(NextIndex))
		return;

	// SkillWheel이 열려 있으면 캐릭터 교체 차단
	if (AC_PlayerState* PS = GetPlayerState<AC_PlayerState>())
	{
		static const FGameplayTag SkillWheelTag = FGameplayTag::RequestGameplayTag(FName("State.SkillWheelOpen"));
		if (PS->GetAbilitySystemComponent()->HasMatchingGameplayTag(SkillWheelTag))
			return;
	}

	AC_BasePlayerCharactor *OldChar = characterRoster[currentCharacterIndex];
	AC_BasePlayerCharactor *NewChar = characterRoster[NextIndex];

	if (!OldChar || !NewChar)
		return;

	bIsSwitching = true;

	// 새 캐릭터를 현재 위치/회전으로 이동
	NewChar->SetActorLocationAndRotation(
		OldChar->GetActorLocation(),
		OldChar->GetActorRotation());

	// 속도 이어받기 (공중에서 교체 시 자연스러운 낙하)
	NewChar->GetCharacterMovement()->Velocity = OldChar->GetCharacterMovement()->Velocity;

	// 새 캐릭터 활성화
	NewChar->SetActorHiddenInGame(false);
	NewChar->SetActorEnableCollision(true);

	OldChar->SaveCharacterState();

	// 이전 캐릭터의 어빌리티를 ASC에서 제거 (characterAbilitiesGiven = false로 리셋됨)
	OldChar->RemoveCharacterAbilities();

	// Possess → PossessedBy → InitAbilityActorInfo(PS, NewChar) 자동 호출
	Possess(NewChar);
	currentCharacterIndex = NextIndex;

	// 이전 캐릭터 비활성화
	OldChar->SetActorHiddenInGame(true);
	OldChar->SetActorEnableCollision(false);

	// HUD가 여기에 바인딩해서 위젯을 재초기화함 (Possess 이후이므로 새 어빌리티가 ASC에 등록된 상태)
	OnCharacterSwitched.Broadcast(NextIndex);

	bIsSwitching = false;
}

void AC_PlayerController::SwitchToNextCharacter()
{
	if (characterRoster.Num() == 0)
		return;
	const int32 NextIndex = (currentCharacterIndex + 1) % characterRoster.Num();
	SwitchToCharacter(NextIndex);
}
