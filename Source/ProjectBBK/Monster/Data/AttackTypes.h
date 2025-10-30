#pragma once

#include "AttackTypes.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Normal  UMETA(DisplayName = "Normal Attack"),
    Special UMETA(DisplayName = "Special Attack")
};