#include "C_InteractionWidget.h"
#include "Components/TextBlock.h"

void UC_InteractionWidget::SetInteractionText(const FText& InText)
{
	if (InteractionText)
	{
		InteractionText->SetText(InText);
	}
}
