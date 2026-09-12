// Copyright: Jichao Luo


#include "GASPFunctionLibrary.h"

float UGASPFunctionLibrary::DamperExactAlpha(float DeltaTime, float HalfLife)
{
	// https://theorangeduck.com/page/spring-roll-call#exactdamper
	return 1.0f - FMath::InvExpApprox(Ln2 / (HalfLife + UE_SMALL_NUMBER) * DeltaTime);
	// HalfLife表示阻尼过程的半衰期，即值在指数衰减过程中减少一半所需的时间
	// Ln2自然对数ln(2)，大约是 0.69314718
	// 指数衰减的半衰期公式：e^(-λ·t) = 0.5，其中λ是衰减常数，λ = ln(2)/t
	// FMath::InvExpApprox(x)提供指数近似函数，约等于e^(-x)
}

FRotator UGASPFunctionLibrary::DamperExactRotation(const FRotator& Current, const FRotator& Target, float DeltaTime, float HalfLife)
{
	return LerpRotation(Current, Target, DamperExactAlpha(DeltaTime, HalfLife));
}

FRotator UGASPFunctionLibrary::LerpRotation(const FRotator& From, const FRotator& To, float Ratio)
{
	FRotator Result = To - From;
	Result.Normalize();

	Result.Pitch = RemapAngleForCounterClockwiseRotation(Result.Pitch);
	Result.Yaw = RemapAngleForCounterClockwiseRotation(Result.Yaw);
	Result.Roll = RemapAngleForCounterClockwiseRotation(Result.Roll);

	Result *= Ratio;
	Result += From;
	Result.Normalize();
	return Result;
}

float UGASPFunctionLibrary::RemapAngleForCounterClockwiseRotation(const float Angle)
{
	// CounterClockwiseRotationAngleThreshold是逆时针旋转阈值，是防抖缓冲区，避免角度插值在180°附近出现跳变
	return Angle > 180.0f - CounterClockwiseRotationAngleThreshold ? Angle - 360.0f : Angle;
}

bool UGASPFunctionLibrary::AngleInRange(float Angle, float MinAngle, float MaxAngle, float Buffer, bool bIncreaseBuffer)
{
	return bIncreaseBuffer ? (Angle >= MinAngle - Buffer && Angle <= MaxAngle + Buffer)
		: (Angle >= MinAngle + Buffer && Angle <= MaxAngle - Buffer);
}
