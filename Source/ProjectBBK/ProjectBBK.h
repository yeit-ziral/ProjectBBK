// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class ProjectBBKAbilityID : uint8
{
	None			UMETA(DisplayName = "None"),
	Confirm			UMETA(DisplayName = "Confirm"),
	Cancel			UMETA(DisplayName = "Cancel"),
	CommonAbility	UMETA(DisplayName = "CommonAbility"),
	UniqueAbility	UMETA(DisplayName = "UniqueAbility"),
	Ultimate		UMETA(DisplayName = "Ultimate"),
};