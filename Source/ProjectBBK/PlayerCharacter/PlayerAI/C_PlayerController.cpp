// Fill out your copyright notice in the Description page of Project Settings.

#include "C_PlayerController.h"
#include "../C_PlayerState.h"
#include "../C_BasePlayerCharactor.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

float AC_PlayerController::GetSwitchCooldownRemaining() const
{
	if(!bSwitchOnCooldown)
		return 0.0f;
	float elapsed = GetWorld()->GetTimeSeconds() - switchCooldownStartTime;
	return FMath::Max(0.f, switchCooldownDuration - elapsed);
}

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
	UE_LOG(LogTemp, Warning, TEXT("[Input] OnSwitchChar0Input CALLED"));
	SwitchToCharacter(0);
}

void AC_PlayerController::OnSwitchChar1Input()
{
	SwitchToCharacter(1);
}

void AC_PlayerController::SwitchToCharacter(int32 NextIndex, bool bForce)
{
	UE_LOG(LogTemp, Warning, TEXT("[Switch] Called: NextIndex=%d, Current=%d, bForce=%d"), NextIndex, currentCharacterIndex, bForce);

	if (bIsSwitching)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Switch] BLOCKED: bIsSwitching"));
		return;
	}
	if (NextIndex == currentCharacterIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Switch] BLOCKED: same index"));
		return;
	}
	if (!characterRoster.IsValidIndex(NextIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Switch] BLOCKED: invalid index"));
		return;
	}

	// SkillWheel이 열려 있으면 캐릭터 교체 차단
	if (!bForce)
	{
		if (AC_PlayerState* PS = GetPlayerState<AC_PlayerState>())
		{
			static const FGameplayTag SkillWheelTag = FGameplayTag::RequestGameplayTag(FName("State.SkillWheelOpen"));
			if (PS->GetAbilitySystemComponent()->HasMatchingGameplayTag(SkillWheelTag))
			{
				UE_LOG(LogTemp, Warning, TEXT("[Switch] BLOCKED: SkillWheel open"));
				return;
			}
		}
	}

	AC_BasePlayerCharactor *OldChar = characterRoster[currentCharacterIndex];
	AC_BasePlayerCharactor *NewChar = characterRoster[NextIndex];

	if (!OldChar || !NewChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Switch] BLOCKED: null char"));
		return;
	}

	// 강제 교체가 아닐 때만 살아있는지 확인
	if (!bForce && NewChar->bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Switch] BLOCKED: NewChar is dead"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[Switch] Proceeding with switch"));

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
	UAbilitySystemComponent* ASC = nullptr;

	if (AC_PlayerState* PS = GetPlayerState<AC_PlayerState>())
	{
		ASC = PS->GetAbilitySystemComponent();
	}

	if (ASC)
	{
		OldChar->SaveActiveEffects(ASC);

		//2. State 태그를 부여하는 Duration GE 제거
		FGameplayTagContainer tagsToRemove;
		tagsToRemove.AddTag(FGameplayTag::RequestGameplayTag(FName("State")));
		tagsToRemove.AddTag(FGameplayTag::RequestGameplayTag(FName("Effect.Cooldown")));
		ASC->RemoveActiveEffectsWithGrantedTags(tagsToRemove);
	}

	// 이전 캐릭터의 어빌리티를 ASC에서 제거 (characterAbilitiesGiven = false로 리셋됨)
	OldChar->RemoveCharacterAbilities();

	// Possess → PossessedBy → InitAbilityActorInfo(PS, NewChar) 자동 호출
	Possess(NewChar);
	currentCharacterIndex = NextIndex;

	if (ASC)
	{
		NewChar->RestoreActiveEffects(ASC);

		// 공유 ASC에서 State.Dead 태그 제거(죽은 캐릭터에서 오염됨)
		FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
		ASC->RemoveLooseGameplayTag(DeadTag);
	}

	// 이전 캐릭터 비활성화
	OldChar->SetActorHiddenInGame(true);
	OldChar->SetActorEnableCollision(false);

	// HUD가 여기에 바인딩해서 위젯을 재초기화함 (Possess 이후이므로 새 어빌리티가 ASC에 등록된 상태)
	OnCharacterSwitched.Broadcast(NextIndex);
	bIsSwitching = false;

	// 교체 완료 후 쿨타임 시작
	bSwitchOnCooldown = true;
	switchCooldownStartTime = GetWorld()->GetTimeSeconds();
	OnSwitchCooldownStarted.Broadcast(switchCooldownDuration, NextIndex);

	GetWorldTimerManager().SetTimer(
		switchCooldownTimerHandle,
		[this]() {bSwitchOnCooldown = false; },
		switchCooldownDuration,
		false
	);
}

void AC_PlayerController::HandleCharacterDeath(AC_BasePlayerCharactor* DeadCharacter)
{ 
	int32 NextLivingIndex = -1;
	for (int32 i = 0; i < characterRoster.Num(); i++)
	{
		if (characterRoster[i] == DeadCharacter)
			continue;

		if(characterRoster[i] && characterRoster[i]->IsAlive())
		{
			NextLivingIndex = i;
			break;
		}
	}

	if (NextLivingIndex != -1)
	{
		SwitchToCharacter(NextLivingIndex, true);
		// 모두 사망이면 아무것도 안 함 - FinishDying에서 이미 숨김 처리됨
	}
}

void AC_PlayerController::SwitchToNextCharacter()
{
	if (characterRoster.Num() == 0)
		return;
	const int32 NextIndex = (currentCharacterIndex + 1) % characterRoster.Num();
	SwitchToCharacter(NextIndex);
}
