#include "Mover/MovementModes/GASPSmoothWalkingState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPSmoothWalkingState)

/**
 * 判定"客户端预测状态"和"服务器权威状态"是否已经分叉到必须回滚的容差。
 *
 * ⚠️ 这几个数字是必须的，不是可选的调优项：
 *   弹簧是连续量，浮点运算在客户端和服务器上不可能逐位一致（编译器优化、平台、SIMD 都可能不同）。
 *   如果不给容差，ShouldReconcile 会每帧都判定"不一致" → 每帧回滚重算 → 角色原地抽搐。
 *
 * 换句话说：这里是在声明"差多少算一样"。
 */
namespace GASPSmoothWalkingStateErrorTolerance
{
	constexpr float VelocityErrorTolerance = 10.f;			// cm/s
	constexpr float AngularVelocityErrorTolerance = 10.f;	// 度/秒
	constexpr float AccelerationErrorTolerance = 50.f;		// cm/s²
	constexpr float FacingDegreeErrorTolerance = 10.0f;		// 度
}

UScriptStruct* FGASPSmoothWalkingState::GetScriptStruct() const
{
	return StaticStruct();
}

FMoverDataStructBase* FGASPSmoothWalkingState::Clone() const
{
	return new FGASPSmoothWalkingState(*this);
}

bool FGASPSmoothWalkingState::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bSuccess = Super::NetSerialize(Ar, Map, bOutSuccess);

	// 这里是最朴素的 Ar << 直通（每个 FVector 3 个 float）。
	// 官方注释原话："Could be quantized to save bandwidth" —— 想要省带宽可以照
	// FMoverDefaultSyncState 的做法用 SerializePackedVector<精度, 位数> 量化，例如：
	//     SerializePackedVector<10, 16>(SpringVelocity, Ar);
	Ar << SpringVelocity;
	Ar << SpringAcceleration;
	Ar << IntermediateVelocity;
	Ar << IntermediateFacing;
	Ar << IntermediateAngularVelocity;

	return bSuccess;
}

void FGASPSmoothWalkingState::ToString(FAnsiStringBuilderBase& Out) const
{
	Super::ToString(Out);

	// 调 LogMover 或 GameplayDebugger 看状态时用的字符串形式
	Out.Appendf("SpringVelocity=%s SpringAcceleration=%s IntVel=%s IntFac=%s IntAng=%s\n",
		*SpringVelocity.ToCompactString(),
		*SpringAcceleration.ToCompactString(),
		*IntermediateVelocity.ToCompactString(),
		*IntermediateFacing.ToString(),
		*IntermediateAngularVelocity.ToString());
}

bool FGASPSmoothWalkingState::ShouldReconcile(const FMoverDataStructBase& AuthorityState) const
{
	const FGASPSmoothWalkingState* AuthoritySpringState = static_cast<const FGASPSmoothWalkingState*>(&AuthorityState);

	// 任意一项超过容差就返回 true → 客户端回滚到权威状态并重算
	// （注意：容器 FMoverInputContainerDataStruct 的 ShouldReconcile 恒为 false，
	//   那是"只传输、不参与纠偏"的用法，和这里刚好相反）
	return (!(SpringVelocity - AuthoritySpringState->SpringVelocity).IsNearlyZero(GASPSmoothWalkingStateErrorTolerance::VelocityErrorTolerance) ||
		!(SpringAcceleration - AuthoritySpringState->SpringAcceleration).IsNearlyZero(GASPSmoothWalkingStateErrorTolerance::AccelerationErrorTolerance) ||
		!(IntermediateVelocity - AuthoritySpringState->IntermediateVelocity).IsNearlyZero(GASPSmoothWalkingStateErrorTolerance::VelocityErrorTolerance) ||
		(FMath::RadiansToDegrees(IntermediateFacing.AngularDistance(AuthoritySpringState->IntermediateFacing)) > GASPSmoothWalkingStateErrorTolerance::FacingDegreeErrorTolerance ||
		!(IntermediateAngularVelocity - AuthoritySpringState->IntermediateAngularVelocity).IsNearlyZero(GASPSmoothWalkingStateErrorTolerance::AngularVelocityErrorTolerance)));
}

/**
 * 网络插值：把两个已知状态之间"补"出一帧。
 *
 * 什么时候会被调用：SimulatedProxy 在 Interpolated LOD 下不跑模拟，
 * 只在收到的两个服务器快照之间做线性插值（见 04 文档 §5.1）。
 *
 * 注意朝向用 Slerp（四元数球面插值），其余用 Lerp —— 朝向不能按分量线性插值，否则会穿模。
 */
void FGASPSmoothWalkingState::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	const FGASPSmoothWalkingState* FromState = static_cast<const FGASPSmoothWalkingState*>(&From);
	const FGASPSmoothWalkingState* ToState = static_cast<const FGASPSmoothWalkingState*>(&To);

	SpringVelocity = FMath::Lerp(FromState->SpringVelocity, ToState->SpringVelocity, Pct);
	SpringAcceleration = FMath::Lerp(FromState->SpringAcceleration, ToState->SpringAcceleration, Pct);
	IntermediateVelocity = FMath::Lerp(FromState->IntermediateVelocity, ToState->IntermediateVelocity, Pct);
	IntermediateFacing = FQuat::Slerp(FromState->IntermediateFacing, ToState->IntermediateFacing, Pct);
	IntermediateAngularVelocity = FMath::Lerp(FromState->IntermediateAngularVelocity, ToState->IntermediateAngularVelocity, Pct);
}
