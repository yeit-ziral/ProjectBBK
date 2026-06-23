#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_InteractionWidget.generated.h"

class UTextBlock;

UCLASS()
class PROJECTBBK_API UC_InteractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Item|UI")
	void SetInteractionText(const FText& InText);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* InteractionText;
};
