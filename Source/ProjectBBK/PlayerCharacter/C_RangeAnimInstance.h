#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "C_RangeAnimInstance.generated.h"

UCLASS()
class PROJECTBBK_API UC_RangeAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// IK Rig Goal position pins
	UPROPERTY(BlueprintReadWrite, Category = "FootIK")
	FVector footLGoalPosition = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "FootIK")
	FVector footRGoalPosition = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "FootIK")
	FVector pelvisGoalPosition = FVector::ZeroVector;

	// Transform (Modify) Bone rotation pins
	UPROPERTY(BlueprintReadWrite, Category = "FootIK")
	FRotator footLGoalRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadWrite, Category = "FootIK")
	FRotator footRGoalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FootIK|Config")
	float interpSpeed = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FootIK|Config")
	float animatedFootOffset = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FootIK|Config")
	float traceExtentUp = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FootIK|Config")
	float traceExtentDown = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FootIK|Config")
	float traceRadius = 3.f;
	// 발이 올라가거나 내려갈 수 있는 최대 범위 — 모서리에서 극단값 방지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FootIK|Config")
	float maxFootOffset = 50.f;
	// 발목 회전 최대 각도 — 이 이상이면 발이 뒤집힘
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FootIK|Config")
	float maxRotationAngle = 30.f;

private:
	float footLCurrentOffset = 0.f;
	float footRCurrentOffset = 0.f;
	FRotator footLCurrentRotation = FRotator::ZeroRotator;
	FRotator footRCurrentRotation = FRotator::ZeroRotator;

	bool PerformFootTrace(const FVector& FootWorldLocation, float CapsuleBottomZ,
	                      float& OutTargetOffsetZ, FVector& OutSurfaceNormal);
	FRotator CalcFootRotation(const FVector& SurfaceNormal, const FVector& CharForward) const;
};
