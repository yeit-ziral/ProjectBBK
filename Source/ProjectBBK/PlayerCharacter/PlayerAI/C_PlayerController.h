// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "NiagaraSystem.h"
#include "C_PlayerController.generated.h"

class AC_BaseItem;
class AC_BasePlayerCharactor;
class AC_MerchantNPC;
class UInputAction;
class UC_EndingScreenWidget;
class UC_GameOverWidget;
class UC_InventoryComponent;
class UC_InventoryWidget;
class UC_EquipmentWidget;
class UC_MerchantDialogueWidget;
class UC_ShopWidget;
class UC_StatusWidget;
class UDataTable;
class UUserWidget;

// 마우스 커서를 켜는 UI 소스. 비트플래그 — 여러 UI가 동시에 커서를 요청해도
// 모두 닫혔을 때만 커서를 끄기 위해 사용. 신규 UI 추가 시 비트 하나씩 부여.
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EMouseUISource : uint8
{
	None       = 0			UMETA(Hidden),
	Inventory  = 1 << 0,
	SkillWheel = 1 << 1,
	Equipment  = 1 << 2,
	Merchant   = 1 << 3,
	Status     = 1 << 4,
};

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

	// 장비창 열기/닫기/토글 — 별도 키(IA_Equipment)로 작동. 현재 캐릭터의 장비창을 생성/표시.
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Equipment")
	void OpenEquipment();

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Equipment")
	void CloseEquipment();

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Equipment")
	void ToggleEquipment();

	UFUNCTION(BlueprintPure, Category = "ProjectBBK|Equipment")
	bool IsEquipmentOpen() const { return bEquipmentOpen; }

	// 스탯 창 열기/닫기/토글 — IA_Status 키로 작동. 어빌리티 발동은 차단하지 않음.
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|UI")
	void OpenStatus();

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|UI")
	void CloseStatus();

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|UI")
	void ToggleStatus();

	UFUNCTION(BlueprintPure, Category = "ProjectBBK|UI")
	bool IsStatusOpen() const { return bStatusOpen; }

	// 마우스 커서를 켜는 UI를 등록한다. 비어 있다가 처음 켜질 때만 커서/입력모드를 전환.
	// widgetToFocus가 있으면 해당 위젯에 포커스를 준다(드래그 등 UMG 입력 필요 시).
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|UI")
	void PushMouseUI(EMouseUISource source, UUserWidget* widgetToFocus = nullptr);

	// 등록했던 UI를 해제한다. 모든 UI가 닫혔을 때만 커서를 끄고 게임 입력으로 복귀.
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|UI")
	void PopMouseUI(EMouseUISource source);

	void SetCurrentInteractable(AC_BaseItem* Item);
	void ClearCurrentInteractable(AC_BaseItem* Item);

	// ===== 상인 NPC / 상점 =====

	// 대화창 열기 (G키 → 시선이 향한 상인) — 대사 표시 후 구매/그만두기 선택.
	void OpenMerchantDialogue(AC_MerchantNPC* NPC);

	// 대화창 닫기 (그만두기 / 취소)
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Merchant")
	void CloseMerchantDialogue();

	// 대화창 "구매" 선택 → 대화창을 닫고 상점창 + 인벤토리를 연다.
	void OnDialogueBuyChosen(AC_MerchantNPC* NPC);

	// 상점창 + 인벤토리 동시 열기
	void OpenShop(AC_MerchantNPC* NPC);

	// 상점창 + 인벤토리 닫기
	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|Merchant")
	void CloseShop();

	// 현재 열려 있는 상점창 (없으면 nullptr) — 인벤 슬롯 더블클릭 판매 경로에서 조회
	UC_ShopWidget* GetActiveShopWidget() const { return bShopOpen ? shopWidget : nullptr; }

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	// 시선(카메라 전방) 트레이스로 상인 포커스 갱신
	virtual void PlayerTick(float DeltaTime) override;

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

	// 장비창 토글 키 (BP에서 IA_Equipment 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_Equipment;

	// 스탯창 토글 키 (BP에서 IA_Status 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_Status;

	// E키로 상호작용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_Interact;

	// G키로 상인 NPC 대화
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectBBK|Input")
	UInputAction* IA_TalkToNPC;

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

	// BP_PlayerController에서 WBP_MerchantDialogue 할당
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UC_MerchantDialogueWidget> dialogueWidgetClass;

	// BP_PlayerController에서 WBP_Shop 할당
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UC_ShopWidget> shopWidgetClass;

	// 상인 시선 감지 최대 거리 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectBBK|Merchant", meta = (ClampMin = "0"))
	float merchantGazeDistance = 800.f;

	// BP_PlayerController에서 WBP_Status 할당
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UC_StatusWidget> statusWidgetClass;

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
	void OnTalkToNPCInput();
	void ExecuteCharacterSwitch(int32 NextIndex);

	// 시선 트레이스 → 포커스 상인 갱신 (매 틱)
	void UpdateMerchantGaze();

	// 현재 시선이 향한 상인 (외곽선 표시 + G키 대상)
	TWeakObjectPtr<AC_MerchantNPC> focusedMerchant;

	// 상인 UI 위젯 인스턴스
	UPROPERTY()
	UC_MerchantDialogueWidget* dialogueWidget = nullptr;

	UPROPERTY()
	UC_ShopWidget* shopWidget = nullptr;

	bool bShopOpen = false;

	// 인벤토리 위젯 인스턴스 (최초 열 때 생성, 닫아도 유지 — 드래그 위치 보존)
	UPROPERTY()
	UC_InventoryWidget* inventoryWidget = nullptr;

	bool bInventoryOpen = false;

	// 장비창 위젯 인스턴스 (열 때마다 현재 캐릭터 클래스로 생성, 닫으면 파기)
	UPROPERTY()
	UC_EquipmentWidget* equipmentWidget = nullptr;

	bool bEquipmentOpen = false;

	// 스탯창 위젯 인스턴스 (최초 1회 생성 후 유지 — 드래그 위치 보존)
	UPROPERTY()
	UC_StatusWidget* statusWidget = nullptr;

	bool bStatusOpen = false;

	// 현재 커서를 요청 중인 UI들의 비트마스크 (EMouseUISource OR 누적)
	uint8 activeMouseUISources = 0;

	bool bIsSwitching = false;

	FTimerHandle switchDelayTimerHandle;

	// <교체 쿨타임 관리>
	bool bSwitchOnCooldown = false;
	FTimerHandle switchCooldownTimerHandle;
	float switchCooldownStartTime = 0.0f;

	FRotator savedControlRotation; // 캐릭터 교체 시 컨트롤 회전 저장

	TArray<TWeakObjectPtr<AC_BaseItem>> overlappingItems;
};
