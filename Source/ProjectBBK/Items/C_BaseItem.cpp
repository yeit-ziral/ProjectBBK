#include "C_BaseItem.h"
#include "C_InteractionWidget.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/WidgetComponent.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../PlayerCharacter/PlayerAI/C_PlayerController.h"
#include "../Inventory/C_InventoryComponent.h"

AC_BaseItem::AC_BaseItem()
{
	PrimaryActorTick.bCanEverTick = false;

	collisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(collisionSphere);
	collisionSphere->SetSphereRadius(150.f);
	collisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	collisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	collisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	collisionSphere->SetGenerateOverlapEvents(true);

	itemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	itemMesh->SetupAttachment(collisionSphere);
	itemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	interactionWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
	interactionWidgetComp->SetupAttachment(collisionSphere);
	interactionWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	interactionWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	interactionWidgetComp->SetDrawAtDesiredSize(true);
	interactionWidgetComp->SetVisibility(false);
}

void AC_BaseItem::BeginPlay()
{
	Super::BeginPlay();

	collisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AC_BaseItem::OnItemBeginOverlap);
	collisionSphere->OnComponentEndOverlap.AddDynamic(this, &AC_BaseItem::OnItemEndOverlap);

	if (interactionWidgetClass)
	{
		interactionWidgetComp->SetWidgetClass(interactionWidgetClass);
	}

	if (itemID != NAME_None)
	{
		InitItem(itemID);
	}

	// 메시를 서브클래스 BeginPlay에서 적용하는 경우가 있어(AC_MoneyItem::ApplyWorldMesh는 Super 호출 뒤)
	// 다음 틱에 붙인다 — 이 시점엔 어느 경로든 메시 적용과 바운드 갱신이 끝나 있다.
	if (bSnapToGroundOnSpawn)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AC_BaseItem::SnapToGround);
	}
}

void AC_BaseItem::SnapToGround()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Origin = GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemGroundSnap), false, this);
	FHitResult Hit;

	const FVector Start = Origin + FVector(0.f, 0.f, groundSnapUpMargin);
	const FVector End   = Origin - FVector(0.f, 0.f, groundSnapTraceDistance);

	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		// 지면을 못 찾으면 건드리지 않는다 — ImpactPoint가 (0,0,0)이라 원점으로 순간이동한다
		// (Debugging Checklist #22와 같은 함정)
		return;
	}

	// 액터 원점은 CollisionSphere 중심이라 메시 바닥과 일치하지 않는다.
	// 지금 위치에서 "원점이 메시 바닥보다 얼마나 위에 있는지"를 재서 그대로 지면 위에 얹는다.
	float BottomOffset = 0.f;
	if (itemMesh && itemMesh->GetStaticMesh())
	{
		const FBoxSphereBounds MeshBounds = itemMesh->Bounds;   // 월드 공간
		BottomOffset = Origin.Z - (MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z);
	}

	FVector NewLocation = Origin;
	NewLocation.Z = Hit.ImpactPoint.Z + BottomOffset + groundSnapOffset;
	SetActorLocation(NewLocation);
}

void AC_BaseItem::InitItem(FName InItemID)
{
	itemID = InItemID;
}

void AC_BaseItem::OnInteract(AC_BasePlayerCharactor* Player)
{
	if (!Player) return;

	AC_PlayerController* PC = Cast<AC_PlayerController>(Player->GetController());
	if (!PC) return;

	UC_InventoryComponent* Inv = PC->GetInventory();
	if (!Inv) return;

	if (Inv->AddItem(itemID) == 0)
	{
		Destroy();
	}
}

void AC_BaseItem::ApplyWorldMesh(UStaticMesh* Mesh)
{
	UStaticMesh* MeshToApply = Mesh ? Mesh : defaultMesh;
	if (MeshToApply)
	{
		itemMesh->SetStaticMesh(MeshToApply);
	}
}

void AC_BaseItem::OnItemBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	AC_BasePlayerCharactor* Player = Cast<AC_BasePlayerCharactor>(OtherActor);
	if (!Player) return;

	if (AC_PlayerController* PC = Cast<AC_PlayerController>(Player->GetController()))
	{
		PC->SetCurrentInteractable(this);
	}

	if (UC_InteractionWidget* Widget = Cast<UC_InteractionWidget>(interactionWidgetComp->GetWidget()))
	{
		Widget->SetInteractionText(cachedItemName);
	}

	interactionWidgetComp->SetVisibility(true);
}

void AC_BaseItem::OnItemEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	AC_BasePlayerCharactor* Player = Cast<AC_BasePlayerCharactor>(OtherActor);
	if (!Player) return;

	if (AC_PlayerController* PC = Cast<AC_PlayerController>(Player->GetController()))
	{
		PC->ClearCurrentInteractable(this);
	}

	interactionWidgetComp->SetVisibility(false);
}
