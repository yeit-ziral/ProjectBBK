#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BaseItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class AC_BasePlayerCharactor;

UCLASS(Abstract)
class PROJECTBBK_API AC_BaseItem : public AActor
{
	GENERATED_BODY()

public:
	AC_BaseItem();

	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void InitItem(FName InItemID);

	virtual void OnInteract(AC_BasePlayerCharactor* Player);

protected:
	virtual void BeginPlay() override;

	void ApplyWorldMesh(UStaticMesh* Mesh);

	UPROPERTY(VisibleAnywhere, Category = "Item")
	USphereComponent* collisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	UStaticMeshComponent* itemMesh;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	UWidgetComponent* interactionWidgetComp;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Mesh")
	UStaticMesh* defaultMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Item|UI")
	TSubclassOf<UUserWidget> interactionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName itemID;

	FText cachedItemName;

private:
	UFUNCTION()
	void OnItemBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnItemEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
};
