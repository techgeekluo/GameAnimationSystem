// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "DefaultMovementSet/Modes/WalkingMode.h"
#include "GASPSmoothWalkingMode.generated.h"

/**
 * 平滑行走模式（GASP 版）。
 *
 * 解决的问题：默认的 UWalkingMode 是"瞬间起步 / 瞬间停住 / 按 TurningRate 匀速转身"，
 * 在动画驱动的角色上会显得轻飘。这个模式把「速度」做成一根临界阻尼弹簧：
 *
 *     输入期望速度 ──► 中间速度（按加速度积分，不平滑）
 *                          │
 *                          │ 临界弹簧去追它（带滞后补偿）
 *                          ▼
 *                     输出速度（平滑过的）──► 交给 SimulationTick 推动角色
 *
 * 为什么继承 UWalkingMode 而不是 USimpleWalkingMode：
 *   USimpleWalkingMode 没有 API 宏，**C++ 跨模块继承不了**（会 LNK2019）。
 *   所以只能继承 UWalkingMode，自己实现一套"简单行走"的算法
 *   （GenerateMove_Implementation + GenerateWalkingMove），
 *   但**沿用父类的移动应用逻辑**（SimulationTick：扫掠、沿坡、上台阶、沿墙滑、掉出到空中）。
 *
 * 跨帧状态：弹簧是有记忆的，这些记忆存在 FGASPSmoothWalkingState 里，
 * 并放进同步状态参与网络复制与回滚（见 GASPSmoothWalkingState.h 的说明）。
 */
UCLASS(BlueprintType)
class GAMEANIMATIONSYSTEM_API UGASPSmoothWalkingMode : public UWalkingMode
{
	GENERATED_BODY()

protected:
	// 黑板条目的名字：每次 GenerateWalkingMove 跑完就置 true，存活到下一帧
	static const FName DidGenerateMoveEntry;

public:
	/** 注册黑板条目 DidGenerateMoveEntry（跨帧状态用来判断自己是否"失活"） */
	virtual void OnRegistered(const FName ModeName, const FMoverSimContext& SimContext) override;

	/** 沿用父类的移动应用逻辑，额外把弹簧状态从 StartState 搬到 OutputState */
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	/** 算出这一帧"想要的速度和朝向"（纯计算，不推世界）。细节见 .cpp */
	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;

	// 重写这个来定制"速度/朝向怎么算"。默认实现是直接赋值，也就是不平滑。
	//
	// ⚠️ 命名规则：BlueprintNativeEvent 会被 UHT 拆成两个符号 ——
	//     GenerateWalkingMove(...)               ← 引擎调用的"外壳"，UHT 已经在 .gen.cpp 里写了实现，
	//                                              你**绝对不要**再定义它（否则 C2084 函数已有主体）
	//     GenerateWalkingMove_Implementation(...)← 留给 C++ 的实现槽，**你写的是这个**（在 .cpp 里）
	//   蓝图侧实现的是名为 "GASP Generate Walking Move" 的事件。
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GASP Generate Walking Move"))
	void GenerateWalkingMove(UPARAM(ref) FMoverTickStartData& StartState, float DeltaSeconds, const FMoverSimContext& SimContext, const FVector& DesiredVelocity,
		const FQuat& DesiredFacing, const FQuat& CurrentFacing, UPARAM(ref) FVector& InOutAngularVelocityDegrees, UPARAM(ref) FVector& InOutVelocity);

	/** 大于等于 0 时覆盖「共享设置」里的 MaxSpeed（-1 = 不覆盖，用 CommonLegacyMovementSettings 的值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Walking Settings", meta = (ForceUnits = "cm/s"))
	float MaxSpeedOverride = -1.0f;

	// =====================================================================
	// 平滑行走参数
	//
	// ⚠️ 换成这个模式之后，MoverComponent → Shared Settings 里的
	//    Acceleration / Deceleration / TurningRate / TurningBoost / Friction
	//    **全部失效**，手感要调下面这些参数。
	// =====================================================================
protected: // 速度控制
	/** 基础加速度（cm/s²）：期望速度比当前速度**大**时使用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s^2"))
	float Acceleration = 1500.0f;

	/** 基础减速度（cm/s²）：期望速度比当前速度**小**时使用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s^2"))
	float Deceleration = 1500.0f;

	/**
	 * 0~1，控制加速度"往哪加"：
	 *
	 *   = 1（默认）：把加速度加在**期望速度的方向**上，再把结果截断。
	 *                这重现了默认行走模式的手感 —— 速度涨得快，但转弯时会往外甩。
	 *   = 0        ：加速度直接指向**期望速度**（方向和大小一起逼近）。
	 *                转弯更快更规整、加速度恒定，但**转弯过程中速度大小会掉**。
	 *
	 * 中间值就是两者的混合，是调手感的主要旋钮之一。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Advanced Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ClampMax = "1", UIMax = "1"))
	float DirectionalAccelerationFactor = 1.0f;

	/**
	 * 额外的"转向力"：把当前速度的方向**旋转**向期望方向。
	 *
	 * 好处是转弯**不掉速**（只转方向、不改模长），代价是给系统注入额外加速度。
	 * 有效范围大约 0~100：10 已经算相当强的转向力，100 接近"瞬间转向"（在平滑之前）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0"))
	float TurningStrength = 10.0f;

	/** 加速时对速度变化做多少平滑（秒）。设 0 = 不平滑 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "s"))
	float AccelerationSmoothingTime = 0.1f;

	/** 减速时对速度变化做多少平滑（秒）。设 0 = 不平滑 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "s"))
	float DecelerationSmoothingTime = 0.1f;

	/**
	 * 平滑天然会"滞后"于目标。这个系数用来补偿滞后：通过追踪一个"提前若干时间"的未来目标。
	 *
	 *   = 0（默认）：速度曲线更 S 形、更顺，但响应感偏弱；
	 *   = 1        ：响应更跟手，但起步那一下不够平滑、"预加速"感变弱。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Advanced Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ClampMax = "1", UIMax = "1"))
	float AccelerationSmoothingCompensation = 0.0f;

	/** 同上，但作用在**减速**阶段 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Advanced Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ClampMax = "1", UIMax = "1"))
	float DecelerationSmoothingCompensation = 0.0f;

	/** 速度"吸附"阈值（cm/s）：离期望速度足够近就直接设成期望值，避免无限逼近产生抖动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Advanced Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float VelocityDeadzoneThreshold = 0.01f;

	/** 加速度"归零"阈值（cm/s²）：追上目标后把加速度也吸到 0，彻底稳定下来 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Advanced Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s^2"))
	float AccelerationDeadzoneThreshold = 0.001f;

	/**
	 * 角色被**外部因素**（碰撞、被推、站在移动平台上）影响后，
	 * 内部速度重新对齐实际速度的快慢（秒）。
	 *
	 *   小值 → 碰撞后内部速度立刻"重置"，角色显得被卡住；
	 *   大值 → 角色保留更多动量，擦碰后能顺势继续跑。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Advanced Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "s"))
	float OutsideInfluenceSmoothingTime = 0.05f;

protected: // 朝向控制

	/** 对转身做多少平滑（秒）。设 0 = 不平滑（瞬间转到目标朝向） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "s"))
	float FacingSmoothingTime = 0.25f;

	/**
	 * 用"双弹簧"而不是单弹簧来平滑朝向。
	 *
	 *   单弹簧：进入快、**收尾拖沓**；
	 *   双弹簧（默认）：更 S 形的曲线、**收尾更干脆**（两级各用一半的 FacingSmoothingTime）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Advanced Smooth Walking Settings")
	bool bSmoothFacingWithDoubleSpring = true;

	/** 朝向"吸附"阈值（度）：离目标朝向足够近就直接设成目标，避免无限逼近 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Advanced Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "deg"))
	float FacingDeadzoneThreshold = 0.1f;

	/** 角速度"归零"阈值（度/秒）：转到位后把角速度也吸到 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Advanced Smooth Walking Settings", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "deg/s"))
	float AngularVelocityDeadzoneThreshold = 0.01f;
};
