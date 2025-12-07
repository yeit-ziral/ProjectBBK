// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class ProjectBBKAbilityID : uint8
{
	None		UMETA(DisplayName = "None"),
	Confirm		UMETA(DisplayName = "Confirm"),
	Cancel		UMETA(DisplayName = "Cancel"),
	Attack		UMETA(DisplayName = "Attack"), 
	Sprint		UMETA(DisplayName = "Sprint"), 
	Dodge		UMETA(DisplayName = "Dodge"),  
	Skill1		UMETA(DisplayName = "Skill1"), 
	Skill2		UMETA(DisplayName = "Skill2"),
};