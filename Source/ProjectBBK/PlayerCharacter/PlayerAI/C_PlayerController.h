// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "NiagaraSystem.h"
#include "C_PlayerController.generated.h"

class AC_BaseItem;
class AC_BasePlayerCharactor;
class UInputAction;
class UC_EndingScreenWidget;
class UC_GameOverWidget;
class UC_InventoryComponent;
class UC_InventoryWidget;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSwitched, int32, NewCharacterIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSwitchCooldownStarted, float, Duration, int32, CharacterIndex);
UCLASS()
class PROJECTBBK_API AC_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AC_PlayerController();

	// 캐릭터 교체 완료 시 브로드캐스트 — WBP_HUD에서 바인딩해 위젯 재초기화에 사용
	UPROPERTY(BlueprintAssignable, Category = "ProjectBBK|CharacterRoster")
	FOnCharacterSwitched OnCharacterSwitched;

	// 캐릭터 교체 (0-based index). Blueprint에서도 호출 가능.
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|CharacterRoster")
	void SwitchToCharacter(int32 NextIndex, bool bForce = false);

	void HandleCharacterDeath(AC_BasePlayerCharactor* DeadCharacter);

	// TravelToNextLevel 직전에 GameInstance에 현재 상태를 저장 — UC_BBKGameInstance에서 호출
	void SaveStateForLevelTransition();

	// Tab키: 다음 캐릭터로 순환
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|CharacterRoster")
	void SwitchToNextCharacter();

	UFUNCTION(BlueprintPure, Category = "ProjectBBK|CharacterRoster")
	int32 GetCurrentCharacterIndex() const { return currentCharacterIndex; }

	///// 캐릭터 교체 쿨타임 시작 시 브로드캐스트 (Duration: 쿨타임 지속 시간, CharacterIndex: 교체된 캐릭터 인덱스)
	UPROPERTY(BlueprintAssignable, Category = "ProjectBBK|CharacterRoster")
	FOnSwitchCooldownStarted OnSwitchCooldownStarted;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectBBK|CharacterRoster")
	float switchCooldownDuration = 2.0f; // 캐릭터 교체 쿨타임 (초)

	UFUNCTION(BlueprintPure, Category = "ProjectBBK|CharacterRoster")
	bool IsSwitchOnCooldown() const { return bSwitchOnCooldown; }

	UFUNCTION(BlueprintPure, Category = "ProjectBBK|CharacterRoster")
	float GetSwitchCooldownRemaining() const;

	// 인벤토리 컴포넌트 접근 (아이템 픽업 등에서 AddItem 호출용)
	UFUNCTION(BlueprintPure, Category = "ProjectBBK|Inventory")
	UC_InventoryComponent* GetInventory() const { return inventory; }

	// 인벤토리 열기/닫기/토글 — I키 입력 또는 다른 시스템에서 호출 가능
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Inventory")
	void OpenInventory();

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Inventory")
	void CloseInventory();

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintPure, Category = "ProjectBBK|Inventory")
	bool IsInventoryOpen() const { return bInventoryOpen; }

	void SetCurrentInteractable(AC_BaseItem* Item);
	void ClearCurrentInteractable(AC_BaseItem* Item);

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

protected:
	// BP_PlayerController에서 교체할 캐릭터 클래스를 순서대로 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|CharacterRoster")
	TArray<TSubclassOf<AC_BasePlayerCharactor>> characterRosterClasses;

	// 런타임에 스폰된 캐릭터 인스턴스 목록
	UPROPERTY(BlueprintReadOnly, Category = "ProjectBBK|CharacterRoster")
	TArray<AC_BasePlayerCharactor*> characterRoster;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectBBK|CharacterRoster")
	int32 currentCharacterIndex = 0;

	// 숫자 키 1, 2로 직접 교체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_SwitchChar0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_SwitchChar1;

	// Tab키로 순환 교체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_SwitchCharNext;

	// I키로 인벤토리 토글
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_Inventory;

	// E키로 상호작용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_Interact;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* playerMappingContext;

	// BP_PlayerController에서 WBP_EndingScreen 할당
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UC_EndingScreenWidget> EndingScreenClass;

	// BP_PlayerController에서 WBP_GameOverScreen 할당
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UC_GameOverWidget> GameOverScreenClass;

	// BP_PlayerController에서 WBP_Inventory 할당
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UC_InventoryWidget> inventoryWidgetClass;

	// 캐릭터 교체와 무관하게 유지되는 인벤토리 보관소
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectBBK|Inventory")
	UC_InventoryComponent* inventory;

	// 인벤토리 컴포넌트에 주입할 아이템 조회용 DataTable (BP에서 DT_ConsumableItem / DT_EquipmentItem 지정)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectBBK|Inventory")
	UDataTable* consumableTable;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectBBK|Inventory")
	UDataTable* equipmentTable;

	// 시작 시 인벤토리에 미리 넣을 아이템 (테스트/디버그용) — Key: itemID(Row Name), Value: 개수
	UPROPERTY(EditDefaultsOnly, Category = "ProjectBBK|Inventory")
	TMap<FName, int32> startingItems;

	// 시작 보유 금액 (테스트/디버그용)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectBBK|Inventory", meta = (ClampMin = "0"))
	int32 startingMoney = 0;

	UFUNCTION()
	void ShowEndingScreen();

	UFUNCTION()
	void ShowGameOverScreen();

	UPROPERTY(EditDefaultsOnly, Category = "ProjectBBK|CharacterRoster")
	UNiagaraSystem* switchEffect; // 캐릭터 교체 시 재생할 이펙트

private:
	void OnSwitchChar0Input();
	void OnSwitchChar1Input();
	void OnInteractInput();
	void ExecuteCharacterSwitch(int32 NextIndex);

	// 인벤토리 위젯 인스턴스 (최초 열 때 생성, 닫아도 유지 — 드래그 위치 보존)
	UPROPERTY()
	UC_InventoryWidget* inventoryWidget = nullptr;

	bool bInventoryOpen = false;

	bool bIsSwitching = false;

	FTimerHandle switchDelayTimerHandle;

	// <교체 쿨타임 관리>
	bool bSwitchOnCooldown = false;
	FTimerHandle switchCooldownTimerHandle;
	float switchCooldownStartTime = 0.0f;

	FRotator savedControlRotation; // 캐릭터 교체 시 컨트롤 회전 저장

	TArray<TWeakObjectPtr<AC_BaseItem>> overlappingItems;
};
