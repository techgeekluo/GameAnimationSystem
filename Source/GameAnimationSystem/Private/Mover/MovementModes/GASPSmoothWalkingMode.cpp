// Copyright: Jichao Luo


#include "Mover/MovementModes/GASPSmoothWalkingMode.h"
#include "Mover/MovementModes/GASPSmoothWalkingState.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "MoverComponent.h"
#include "MoveLibrary/RollbackBlackboardLibrary.h"
#include "Math/SpringMath.h"
#include "NetworkPredictionWorldManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPSmoothWalkingMode)

// 黑板条目名：标记"上一帧跑过 GenerateWalkingMove"，供跨帧状态（弹簧）判断自己是否失活
const FName UGASPSmoothWalkingMode::DidGenerateMoveEntry = TEXT("DidGenerateMove");

/**
 * 判断"本机是不是真的在跑模拟"。
 *
 * 背景：Mover 的「网络插值模拟代理」（SimulatedProxy + Interpolated LOD）**根本不跑本地模拟**
 * —— liaison 传给 TickInterpolatedSimProxy 的是一个默认构造的空 InputCmd。
 * 所以对它来说"上一帧跑过 GenerateMove 吗"这个判断毫无意义：
 * 它收到的是服务器复制过来的**权威弹簧状态**，必须原样信任。
 *
 * 如果这里判断错了，代理上的弹簧会被反复误判成"失活"而重置 → 远端看到的角色会抖。
 */
static inline bool IsSmoothWalkingMoverSimulatingLocally(const UMoverComponent* MoverComp)
{
	if (MoverComp && MoverComp->GetOwnerRole() == ROLE_SimulatedProxy && UNetworkPredictionWorldManager::ActiveInstance)
	{
		return UNetworkPredictionWorldManager::ActiveInstance->GetSettings().SimulatedProxyNetworkLOD != ENetworkLOD::Interpolated;
	}

	return true;
}

/**
 * 注册黑板条目：一个"存活到下一帧"的 bool，用来回答"上一帧跑过 GenerateWalkingMove 吗"。
 *
 * 为什么需要它：弹簧是跨帧状态，必须能区分"连续工作中"和"被晾了一帧"
 * （比如切到别的模式又切回来、或者网络代理根本不跑模拟）。
 * 如果上一帧没跑过，弹簧里存的速度就对不上了，必须重置，否则会有一次突跳。
 */
void UGASPSmoothWalkingMode::OnRegistered(const FName ModeName, const FMoverSimContext& SimContext)
{
	Super::OnRegistered(ModeName, SimContext);

	URollbackBlackboard::EntrySettings DidGenerateMoveEntrySettings = URollbackBlackboardLibrary::MakeSingleFrameEntrySettings();
	DidGenerateMoveEntrySettings.PersistencePolicy = EBlackboardPersistencePolicy::ThroughNextFrame;

	SimContext.Blackboard.CreateEntry<bool>(DidGenerateMoveEntry, DidGenerateMoveEntrySettings);
}

/**
 * 移动应用阶段：把 GenerateMove 算出的速度真正"推到世界里"。
 *
 * 先调父类（UWalkingMode）做全部脏活：扫掠移动、撞坡沿坡、上台阶、沿墙滑、贴地、掉出到空中。
 * 本类只多补一件事 —— **把 GenerateMove 里更新过的弹簧状态，从 StartState 搬到 OutputState**。
 *
 * ⚠️ 为什么必须每帧搬：FGASPSmoothWalkingState 没有注册进 PersistentSyncStateDataTypes，
 *    它靠"每帧手动搬运"来维持存在。漏一次，弹簧记忆就丢了（表现为速度突然归零/突跳）。
 */
void UGASPSmoothWalkingMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	Super::SimulationTick_Implementation(Params, OutputState);

	// If we've created or updated the spring state during GenerateWalkMove, we need to copy it into the output simulation state.
	if (const FGASPSmoothWalkingState* InSpringState = Params.StartState.SyncState.SyncStateCollection.FindDataByType<FGASPSmoothWalkingState>())
	{
		FGASPSmoothWalkingState& OutputSpringState = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FGASPSmoothWalkingState>();
		OutputSpringState = *InSpringState;
	}
}

/**
 * 计算阶段：根据「输入 + 上一帧状态」算出这一帧**想要的速度和朝向**（FProposedMove）。
 *
 * 这是本模式替换掉 UWalkingMode 默认算法的地方。整体分三步：
 *   ① 取输入（没有输入就从同步状态里反推，兼容网络模拟代理）
 *   ② 把输入换算成"期望速度"（按 MoveInputType 分两种解法）
 *   ③ 交给 GenerateWalkingMove 得到最终速度与角速度（默认实现是直接赋值，子类/蓝图可重写）
 *
 * 它是 const 的 —— 原则上不该改任何东西。但弹簧状态必须在这里更新，
 * 所以下面用了 const_cast 手工绕过（见函数末尾）。
 */
void UGASPSmoothWalkingMode::GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const FMoverDefaultSyncState* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();

	if (!StartingSyncState)
	{
		return;
	}

	const float DeltaSeconds = TimeStep.StepMs * 0.001f;
	if (DeltaSeconds <= FLT_EPSILON)
	{
		return;
	}

	// 取输入：正常情况下应该有 FCharacterDefaultInputs
	FVector DesiredVelocity;
	EMoveInputType MoveInputType;
	FVector DesiredFacingDir;

	if (const FCharacterDefaultInputs* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>())
	{
		DesiredVelocity = CharacterInputs->GetMoveInput_WorldSpace();
		MoveInputType = CharacterInputs->GetMoveInputType();
		DesiredFacingDir = CharacterInputs->GetOrientationIntentDir_WorldSpace();
	}
	else
	{
		// 没有输入结构体 → 多半是网络模拟代理（它拿不到输入命令）
		// 退而求其次：从同步状态里"反推"一个意图出来，避免远端角色突然停住
		DesiredVelocity = StartingSyncState->GetIntent_WorldSpace();
		MoveInputType = EMoveInputType::DirectionalIntent;
		DesiredFacingDir = StartingSyncState->GetOrientation_WorldSpace().Quaternion().GetForwardVector();
	}

	const bool bHasMaxMoveSpeed = MaxSpeedOverride >= 0.0f || CommonLegacySettings;
	float MaxMoveSpeed = MaxSpeedOverride >= 0.0f ? MaxSpeedOverride : (CommonLegacySettings ? CommonLegacySettings->MaxSpeed : 0.0f);

	const UMoverComponent* MoverComponent = GetMoverComponent();
	const FVector UpVector = MoverComponent ? MoverComponent->GetUpDirection() : FVector::UpVector;

	// 去掉垂直分量，但**保留原来的模长**（否则上坡时输入会被重力方向的分量"吃掉"一截）
	float DesiredVelMag = DesiredVelocity.Length();
	DesiredVelocity -= DesiredVelocity.ProjectOnTo(UpVector);
	float DesiredVel2DSquaredLength = DesiredVelocity.SquaredLength();
	if (DesiredVel2DSquaredLength > 0.0f)
	{
		DesiredVelocity *= DesiredVelMag / FMath::Sqrt(DesiredVel2DSquaredLength);
	}

	// 没有 MaxSpeed 可用时的兜底速度（正常情况不会走到）
	const float DefaultDirectionalIntentSpeed = 100.0f;

	// 按输入类型把"输入"换算成"期望速度"
	switch (MoveInputType)
	{
	case EMoveInputType::DirectionalIntent:
	{
		// 方向意图：输入本身就是"意图空间"的量（长度 1 = 满速意图），可以直接当作方向意图输出；
		// 速度大小 = 意图 × MaxSpeed
		OutProposedMove.DirectionIntent = DesiredVelocity;	// here, DesiredVelocity is already in "intent space" (unit length for "max intent") so we can use it directly
		DesiredVelocity = bHasMaxMoveSpeed ? DesiredVelocity * MaxMoveSpeed : DesiredVelocity * DefaultDirectionalIntentSpeed;
	}
	break;
	case EMoveInputType::Velocity:
	{
		// 速度输入：先按 MaxSpeed 截断，再反算回"意图空间"（除以 MaxSpeed）
		DesiredVelocity = bHasMaxMoveSpeed ? DesiredVelocity.GetClampedToMaxSize(MaxMoveSpeed) : DesiredVelocity;
		OutProposedMove.DirectionIntent = MaxMoveSpeed > UE_KINDA_SMALL_NUMBER ? DesiredVelocity / MaxMoveSpeed : FVector::ZeroVector; // here, DesiredVelocity is converted to "intent space"
	}
	break;
	case EMoveInputType::None:
	case EMoveInputType::Invalid:
	default:
	{
		// 兜底：不该出现的类型，打警告并当作"不动"
		UE_LOGF(LogMover, Warning, "Unhandled MoveInputType %i in USimpleWalkingMode", EnumToUnderlyingType(MoveInputType));
		DesiredVelocity = FVector::ZeroVector;
		OutProposedMove.DirectionIntent = FVector::ZeroVector;
	}
	break;
	}

	OutProposedMove.bHasDirIntent = !OutProposedMove.DirectionIntent.IsNearlyZero();

	// 朝向：同样先去垂直分量，再算出一个"目标朝向"四元数
	DesiredFacingDir -= DesiredFacingDir.ProjectOnTo(UpVector);
	FQuat CurrentFacing = StartingSyncState->GetOrientation_WorldSpace().Quaternion();
	FQuat DesiredFacing = CurrentFacing;

	if (DesiredFacingDir.Normalize())
	{
		DesiredFacing = FQuat::FindBetween(FVector::ForwardVector, DesiredFacingDir);
	}

	// 默认用"上一帧的实际速度和角速度"作为起点（GenerateWalkingMove 可以在此基础上改）
	OutProposedMove.LinearVelocity = StartingSyncState->GetVelocity_WorldSpace();
	OutProposedMove.AngularVelocityDegrees = StartingSyncState->GetAngularVelocityDegrees_WorldSpace();

	// Hack const_cast stuff
	// Why is this needed?
	// Because some modes want to mutate their data inside the generate walk move
	//
	// 中文说明：GenerateMove 是 const 的，但弹簧状态（跨帧记忆）必须在这一步更新。
	// 引擎官方也是这么绕的（SimpleWalkingMode.cpp 里同样的写法），这里照抄。
	// 真正"正统"的做法是把弹簧状态注册进 PersistentSyncStateDataTypes，
	// 但那样就偏离官方实现了 —— 保持和上游一致更好对比调试。
	UGASPSmoothWalkingMode* MutableSmoothWalkMode = const_cast<UGASPSmoothWalkingMode*>(this);
	MutableSmoothWalkMode->GenerateWalkingMove(const_cast<FMoverTickStartData&>(StartState), DeltaSeconds, SimContext, DesiredVelocity, DesiredFacing, CurrentFacing, OutProposedMove.AngularVelocityDegrees, OutProposedMove.LinearVelocity);

	// 打标记：告诉下一帧"我这一帧跑过了"（跨帧状态靠它判断是否失活）
	SimContext.Blackboard.TrySet(DidGenerateMoveEntry, true);
}

/**
 * 默认的"速度 / 朝向"计算 —— 也就是本模式的弹簧核心。
 *
 * 双层速度模型（理解本函数的钥匙）：
 *
 *     DesiredVelocity（期望速度，来自输入）
 *            │
 *            │  ① 按加速度积分、限制不超速、转向时保持模长
 *            ▼
 *     IntermediateVelocity（中间速度："物理正确"但不平滑）
 *            │
 *            │  ② 临界弹簧去追它（带滞后补偿）
 *            ▼
 *     SpringVelocity（输出速度：平滑过的）──► InOutVelocity
 *
 * 为什么要拆两层：弹簧的参数是"时间"，不是"cm/s²"，没法直接按需求精确调加速曲线。
 * 拆开之后 —— **中间速度管物理，弹簧管观感**。
 *
 * 蓝图可以重写名叫 "GASP Generate Walking Move" 的事件来替换本函数。
 */
void UGASPSmoothWalkingMode::GenerateWalkingMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds, const FMoverSimContext& SimContext, const FVector& DesiredVelocity, const FQuat& DesiredFacing, const FQuat& CurrentFacing, FVector& InOutAngularVelocityDegrees, FVector& InOutVelocity)
{
	// TODO: 


	if (DeltaSeconds <= FLT_EPSILON) return;

	const FMoverDefaultSyncState* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	if (!ensure(StartingSyncState)) return;

	// 取出（没有就创建）弹簧状态。它在同步状态里，所以参与网络复制和回滚
	bool bIsSmoothWalkingStateNew = false;
	FGASPSmoothWalkingState& SpringState = StartState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FGASPSmoothWalkingState>(bIsSmoothWalkingStateNew);

	bool bDidGenerateMoveSinceLastFrame = false;
	SimContext.Blackboard.TryGet(DidGenerateMoveEntry, bDidGenerateMoveSinceLastFrame);

	// 状态是新建的、或者上一帧没跑过（弹簧失活）→ 必须初始化，否则会有一次突跳。
	//
	// 注意注释里说的问题：角速度和加速度**没法**从 FMoverDefaultSyncState 里继承过来，
	// 因为它们在模式切换时不会跨模式传递 —— 所以这里只能保守地清零。
	if (bIsSmoothWalkingStateNew || (!bDidGenerateMoveSinceLastFrame && IsSmoothWalkingMoverSimulatingLocally(GetMoverComponent())))
	{
		SpringState.SpringVelocity = InOutVelocity;
		SpringState.SpringAcceleration = FVector::ZeroVector;
		SpringState.IntermediateVelocity = InOutVelocity;
		SpringState.IntermediateFacing = CurrentFacing;
		SpringState.IntermediateAngularVelocity = FVector::ZeroVector;
	}

	// VelocityMatch：把"内部弹簧速度"投影到"上一帧实际移动的速度"上，
	// 得到一个 0~1 的数，表示"我们的预期和实际差多远"。
	//   1 = 完全一致（没人打扰我们）
	//   0 = 完全对不上（比如刚撞墙、刚被推）
	const float VelocityMatch = FMath::Clamp(SpringState.SpringVelocity.Dot(InOutVelocity) /
		FMath::Max(InOutVelocity.Length() * SpringState.SpringVelocity.Length(), UE_SMALL_NUMBER), 0.0f, 1.0f);

	// 外部影响重同步：偏离越大，用的平滑时间越短（VelocityMatch 越小 → 分母越小 → 时间越短），
	// 于是内部速度**快速**对齐实际速度。效果是"碰撞后能丢掉不该保留的动量，但不会把动量全丢光"。
	// （调 OutsideInfluenceSmoothingTime 就是在调这个"丢多少"）
	FMath::ExponentialSmoothingApprox(SpringState.IntermediateVelocity, InOutVelocity, DeltaSeconds,
		(OutsideInfluenceSmoothingTime + UE_KINDA_SMALL_NUMBER) / (1.0f - VelocityMatch));

	// 每帧用**真实速度**校正弹簧位置，避免预测误差慢慢把弹簧带跑偏
	SpringState.SpringVelocity = InOutVelocity;

	// 转向力：把中间速度的**方向**朝期望方向旋转（只转方向、不改模长 → 急转弯不掉速）。
	// StrengthToSmoothingTime 把 0~100 的"力度"换算成弹簧的平滑时间。
	if (TurningStrength > 0.0f)
	{
		if (!DesiredVelocity.IsNearlyZero())
		{
			FMath::ExponentialSmoothingApprox(
				SpringState.IntermediateVelocity,
				DesiredVelocity.GetSafeNormal() * SpringState.IntermediateVelocity.Length(),
				DeltaSeconds,
				SpringMath::StrengthToSmoothingTime(TurningStrength));
		}
	}

	// 判断当前是"加速"还是"减速"，并据此分配两种加速度。
	// 注意：**减速永远走"侧向"**（哪怕 DirectionalAccelerationFactor 拉满），
	// 这是为了模拟默认行走模式的行为（刹车是直接对着目标速度刹，不是沿途慢慢磨）。
	const bool bIsAccelerating = (1.01f * DesiredVelocity.SquaredLength()) > SpringState.SpringVelocity.SquaredLength();
	const float LateralAccelerationMagnitude = bIsAccelerating ? (1.0f - DirectionalAccelerationFactor) * Acceleration : Deceleration;
	const float DirectionalAccelerationMagnitude = bIsAccelerating ? DirectionalAccelerationFactor * Acceleration : 0.0f;

	// 记下加速前的速度模长（后面要用它做上限，防止越跑越快）
	const float PreviousVelocityLength = SpringState.IntermediateVelocity.Length();

	// 期望速度与当前中间速度的差
	const FVector VelocityDifference = DesiredVelocity - SpringState.IntermediateVelocity;

	// 侧向加速度：直接指向期望速度（并保证一帧内不会冲过头）
	const FVector LateralAccelerationVector = VelocityDifference.GetSafeNormal() * FMath::Min(LateralAccelerationMagnitude, VelocityDifference.Length() / FMath::Max(DeltaSeconds, UE_SMALL_NUMBER));

	// 纵向加速度：沿期望速度方向（这一步才是"模拟默认行走模式"的那部分）
	const FVector DirectionalAccelerationVector = DesiredVelocity.GetSafeNormal() * DirectionalAccelerationMagnitude;

	// 两个加速度合成
	const FVector DesiredAcceleration = LateralAccelerationVector + DirectionalAccelerationVector;

	// 积分出"下一帧的中间速度"。
	// 两个保护：
	//   ① 快要到目标时直接吸附到目标，避免冲过头；
	//   ② 结果不允许超过"上一帧的速度"或"期望速度"—— 否则纵向加速度会不断给系统注入速度，
	//      角色会无限加速下去。
	FVector NextVelocity = VelocityDifference.Dot(DesiredAcceleration * DeltaSeconds) < VelocityDifference.SquaredLength() ?
		SpringState.IntermediateVelocity + DesiredAcceleration * DeltaSeconds : DesiredVelocity;

	NextVelocity = NextVelocity.GetClampedToMaxSize(FMath::Max(PreviousVelocityLength, DesiredVelocity.Length()));

	// 按"加速/减速"选不同的平滑时间
	const float VelocitySmoothingTime = bIsAccelerating ? AccelerationSmoothingTime : DecelerationSmoothingTime;

	// 同理选不同的滞后补偿系数
	const float VelocitySmoothingCompensation = bIsAccelerating ? AccelerationSmoothingCompensation : DecelerationSmoothingCompensation;

	// 滞后补偿：平滑天然会让输出慢半拍，所以我们不追"当前"的中间速度，
	// 而是追一个"提前 LagSeconds 之后"的未来速度，把滞后补回来。
	const float LagSeconds = DeltaSeconds + (VelocitySmoothingCompensation * VelocitySmoothingTime);

	FVector TrackVelocity = VelocityDifference.Dot(DesiredAcceleration * LagSeconds) < VelocityDifference.SquaredLength() ?
		SpringState.IntermediateVelocity + DesiredAcceleration * LagSeconds : DesiredVelocity;

	TrackVelocity = TrackVelocity.GetClampedToMaxSize(FMath::Max(PreviousVelocityLength, DesiredVelocity.Length()));

	// ★ 核心一步：临界阻尼弹簧。让 SpringVelocity 平滑地追 TrackVelocity。
	// （用"临界"弹簧而不是普通弹簧，是为了不过冲 —— 速度过冲会看到角色抖一下）
	SpringMath::CriticalSpringDamper(SpringState.SpringVelocity, SpringState.SpringAcceleration, TrackVelocity, VelocitySmoothingTime, DeltaSeconds);

	// 死区吸附：已经足够接近目标了就直接设成目标值。
	// 不做的话会无限逼近，浮点上永远差一点点，表现为极高频的微小抖动。
	if ((DesiredVelocity - SpringState.SpringVelocity).SquaredLength() < FMath::Square(VelocityDeadzoneThreshold))
	{
		// 到目标了 → 速度咬死
		SpringState.SpringVelocity = DesiredVelocity;

		// 加速度也足够小了 → 归零，让系统彻底静下来（否则加速度会一直微微跳动）
		if (SpringState.SpringAcceleration.SquaredLength() < FMath::Square(AccelerationDeadzoneThreshold))
		{
			SpringState.SpringAcceleration = FVector::ZeroVector;
		}
	}

	// 输出：平滑后的速度交给移动模式去推世界
	InOutVelocity = SpringState.SpringVelocity;

	// 中间速度推进到下一帧（本帧算出的 NextVelocity）
	SpringState.IntermediateVelocity = NextVelocity;

	// ------------------------------------------------------------------
	// 朝向：和速度一样用弹簧，但有"单弹簧 / 双弹簧"两种选择
	// ------------------------------------------------------------------
	FVector CurrentAngularVelocityRadians = FMath::DegreesToRadians(InOutAngularVelocityDegrees);
	FQuat UpdatedFacing = CurrentFacing;

	if (bSmoothFacingWithDoubleSpring)
	{
		// 双弹簧：两级串联，每级用一半的 FacingSmoothingTime。
		// 总时长和单弹簧一样，但曲线更 S 形、收尾更干脆（单弹簧收尾会拖）。
		SpringMath::CriticalSpringDamperQuat(SpringState.IntermediateFacing, SpringState.IntermediateAngularVelocity, DesiredFacing, FacingSmoothingTime / 2.0f, DeltaSeconds);
		SpringMath::CriticalSpringDamperQuat(UpdatedFacing, CurrentAngularVelocityRadians, SpringState.IntermediateFacing, FacingSmoothingTime / 2.0f, DeltaSeconds);
	}
	else
	{
		// 单弹簧：直接追目标朝向
		SpringState.IntermediateFacing = DesiredFacing;
		SpringState.IntermediateAngularVelocity = CurrentAngularVelocityRadians;
		SpringMath::CriticalSpringDamperQuat(UpdatedFacing, CurrentAngularVelocityRadians, DesiredFacing, FacingSmoothingTime, DeltaSeconds);
	}

	// 朝向死区吸附：转到位了就咬死，并且**精确反算**这一刻的角速度。
	//
	// 为什么这里要特殊处理角速度（而不是像平时那样直接用弹簧算出来的值）：
	// 平时输出的角速度会保持连续（更稳），但如果每帧都用"更新后的朝向"反算角速度，
	// 低帧率下会因为弹簧内部近似求逆的误差而算出错误的值 —— 所以只在"到目标"这一刻精确反算一次。
	if (DesiredFacing.AngularDistance(UpdatedFacing) < FMath::DegreesToRadians(FacingDeadzoneThreshold))
	{
		// We reached our target
		// Ensure the output angular velocity will snap perfectly to the target
		// Note we don't do this normally because its better to have a consistent angular velocity
		// If we output the angular velocity based on the updated facing every frame it can cause errors at low dt due to inaccuracy in the inverse
		// exponential approximation inside the spring damper
		CurrentAngularVelocityRadians = DeltaSeconds > 0.0f ? ((CurrentFacing.Inverse() * UpdatedFacing).GetShortestArcWith(FQuat::Identity)).ToRotationVector() / DeltaSeconds : FVector::ZeroVector;
		SpringState.IntermediateFacing = DesiredFacing;

		// 角速度也足够小了 → 归零
		if (CurrentAngularVelocityRadians.SquaredLength() < FMath::Square(FMath::DegreesToRadians(AngularVelocityDeadzoneThreshold)))
		{
			SpringState.IntermediateAngularVelocity = FVector::ZeroVector;
		}
	}

	// 输出：转成"度/秒"交给移动模式
	InOutAngularVelocityDegrees = FMath::RadiansToDegrees(CurrentAngularVelocityRadians);
}
