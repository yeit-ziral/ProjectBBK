// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
	None				UMETA(DisplayName = "None"),
	Confirm				UMETA(DisplayName = "Confirm"),
	Cancel				UMETA(DisplayName = "Cancel"),
	Sprint				UMETA(DisplayName = "Sprint"),
	Block				UMETA(DisplayName = "Block"),
	Dodge				UMETA(DisplayName = "Dodge"),
	Attack				UMETA(DisplayName = "Attack"),
	Ability1			UMETA(DisplayName = "Ability1"),
	Ability2			UMETA(DisplayName = "Ability2"),
	Ability3			UMETA(DisplayName = "Ability3"),
	Reload				UMETA(DisplayName = "Reload"),
};