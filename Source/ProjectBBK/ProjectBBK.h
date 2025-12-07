// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class ProjectBBKAbilityID : uint8
{
	None			UMETA(DisplayName = "None"),
	Confirm			UMETA(DisplayName = "Confirm"),
	Cancel			UMETA(DisplayName = "Cancel"),
	Attack			UMETA(DisplayName = "Attack"), 
	Sprint			UMETA(DisplayName = "Sprint"), 
	Dodge			UMETA(DisplayName = "Dodge"),  
	CommonSkill		UMETA(DisplayName = "CommonSkill"),
	UniqueSkill		UMETA(DisplayName = "UniqueSkill"),
	Ultimate		UMETA(DisplayName = "Ultimate"),
};