#include "C_BaseItem.h"
#include "C_InteractionWidget.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"
#include "GameFramework/PlayerController.h"

AC_BaseItem::AC_BaseItem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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
}

void AC_BaseItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!overlappingPlayer.IsValid()) return;

	APlayerController* PC = Cast<APlayerController>(overlappingPlayer->GetController());
	if (PC && PC->WasInputKeyJustPressed(interactKey))
	{
		OnInteract(overlappingPlayer.Get());
	}
}

void AC_BaseItem::InitItem(FName InItemID)
{
	itemID = InItemID;
}

void AC_BaseItem::OnInteract(AC_BasePlayerCharactor* Player)
{
	// [임시] 서브클래스에서 효과 적용 후 Destroy
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
	if (bConsumed) return;

	AC_BasePlayerCharactor* Player = Cast<AC_BasePlayerCharactor>(OtherActor);
	if (!Player) return;

	overlappingPlayer = Player;

	if (UC_InteractionWidget* Widget = Cast<UC_InteractionWidget>(interactionWidgetComp->GetWidget()))
	{
		Widget->SetInteractionText(cachedItemName);
	}

	interactionWidgetComp->SetVisibility(true);
	SetActorTickEnabled(true);
}

void AC_BaseItem::OnItemEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	AC_BasePlayerCharactor* Player = Cast<AC_BasePlayerCharactor>(OtherActor);
	if (!Player || Player != overlappingPlayer.Get()) return;

	overlappingPlayer.Reset();
	interactionWidgetComp->SetVisibility(false);
	SetActorTickEnabled(false);
}
