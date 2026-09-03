#include "C_RangeAnimInstance.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

void UC_RangeAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (DeltaSeconds == 0.f) return;
	ACharacter* Character = Cast<ACharacter>(GetOwningActor());
	if (!Character) return;
	USkeletalMeshComponent* Mesh = GetOwningComponent();
	if (!Mesh) return;

	float CapsuleBottomZ = Character->GetActorLocation().Z
		- Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector FootLWorldLoc = Mesh->GetSocketLocation(FName("foot_l"));
	FVector FootRWorldLoc = Mesh->GetSocketLocation(FName("foot_r"));

	float TargetOffsetL = 0.f, TargetOffsetR = 0.f;
	FVector SurfaceNormalL = FVector::UpVector, SurfaceNormalR = FVector::UpVector;

	bool bHitL = PerformFootTrace(FootLWorldLoc, CapsuleBottomZ, TargetOffsetL, SurfaceNormalL);
	bool bHitR = PerformFootTrace(FootRWorldLoc, CapsuleBottomZ, TargetOffsetR, SurfaceNormalR);

	if (!bHitL) TargetOffsetL = 0.f;
	if (!bHitR) TargetOffsetR = 0.f;

	footLCurrentOffset = FMath::FInterpTo(footLCurrentOffset, TargetOffsetL, DeltaSeconds, interpSpeed);
	footRCurrentOffset = FMath::FInterpTo(footRCurrentOffset, TargetOffsetR, DeltaSeconds, interpSpeed);

	footLGoalPosition = FVector(0.f, 0.f, footLCurrentOffset);
	footRGoalPosition = FVector(0.f, 0.f, footRCurrentOffset);

	float PelvisOffset = FMath::Min(footLCurrentOffset, footRCurrentOffset);
	pelvisGoalPosition = FVector(0.f, 0.f, PelvisOffset);

	FVector CharForward = Character->GetActorForwardVector();
	FRotator TargetRotL = bHitL ? CalcFootRotation(SurfaceNormalL, CharForward) : FRotator::ZeroRotator;
	FRotator TargetRotR = bHitR ? CalcFootRotation(SurfaceNormalR, CharForward) : FRotator::ZeroRotator;

	footLCurrentRotation = FMath::RInterpTo(footLCurrentRotation, TargetRotL, DeltaSeconds, interpSpeed);
	footRCurrentRotation = FMath::RInterpTo(footRCurrentRotation, TargetRotR, DeltaSeconds, interpSpeed);

	footLGoalRotation = footLCurrentRotation;
	footRGoalRotation = footRCurrentRotation;
}

FRotator UC_RangeAnimInstance::CalcFootRotation(const FVector& SurfaceNormal, const FVector& CharForward) const
{
	// 경사가 너무 가파르면 (법선이 60° 이상 기울면) 회전 적용 안 함 — 발 뒤집힘 방지
	if (FVector::DotProduct(SurfaceNormal, FVector::UpVector) < 0.5f)
		return FRotator::ZeroRotator;

	FQuat SurfaceAligned = FRotationMatrix::MakeFromZX(SurfaceNormal, CharForward).ToQuat();
	FQuat Flat = FRotationMatrix::MakeFromZX(FVector::UpVector, CharForward).ToQuat();
	FRotator Result = (SurfaceAligned * Flat.Inverse()).Rotator();

	// 최대 각도 클램프 — 극단적인 법선값이 들어와도 뒤집히지 않게
	Result.Pitch = FMath::Clamp(Result.Pitch, -maxRotationAngle, maxRotationAngle);
	Result.Roll  = FMath::Clamp(Result.Roll,  -maxRotationAngle, maxRotationAngle);
	Result.Yaw   = 0.f;
	return Result;
}

bool UC_RangeAnimInstance::PerformFootTrace(
	const FVector& FootWorldLocation, float CapsuleBottomZ,
	float& OutTargetOffsetZ, FVector& OutSurfaceNormal)
{
	UWorld* World = GetWorld();
	if (!World) return false;
	FVector TraceStart(FootWorldLocation.X, FootWorldLocation.Y, CapsuleBottomZ + traceExtentUp);
	FVector TraceEnd  (FootWorldLocation.X, FootWorldLocation.Y, CapsuleBottomZ - traceExtentDown);
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwningActor());
	bool bHit = World->SweepSingleByChannel(HitResult, TraceStart, TraceEnd,
		FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(traceRadius), Params);
	if (bHit)
	{
		// 스피어 스윕은 평면에 법선 방향으로 접하므로, 경사면에서 ImpactPoint가
		// 발밑 실제 지면보다 r·sin²θ/cosθ 만큼 높게 잡힌다. cosθ = ImpactNormal.Z
		// 이므로 추가 트레이스 없이 정확히 상쇄 가능.
		// 평지(n_z = 1)에서는 (1 - 1) = 0 이라 기존 평지 동작에 영향이 없다.
		const float NZ = FMath::Max(HitResult.ImpactNormal.Z, KINDA_SMALL_NUMBER);
		const float SphereBias = traceRadius * (1.f - NZ * NZ) / NZ;

		float RawOffset = HitResult.ImpactPoint.Z - CapsuleBottomZ - SphereBias;
		OutTargetOffsetZ = FMath::Clamp(RawOffset, -maxFootOffset, maxFootOffset);
		OutSurfaceNormal = HitResult.ImpactNormal;
		return true;
	}
	return false;
}
