#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "MoverTypes.h"
#include "GASPSmoothWalkingState.generated.h"

/**
 * 平滑行走模式（UGASPSmoothWalkingMode）专用的「跨帧状态」。
 *
 * ⚠️ 为什么它必须是同步状态（SyncState）里的一员，而不是模式的成员变量：
 *
 *   - 放进 SyncState  → 参与网络复制、参与回滚重算（resim）。服务器纠偏时，
 *                       客户端会拿着权威状态重新跑一遍模拟，弹簧的"记忆"跟着一起恢复；
 *   - 放进成员变量    → 弹簧的中间量不参与回滚。服务器一纠偏，客户端的速度就会
 *                       突然跳一下（弹簧累积的动量凭空消失），表现为角色抽搐。
 *
 * 生命周期：由 UGASPSmoothWalkingMode::GenerateWalkingMove 创建/更新，
 * 再在 SimulationTick 里从 StartState 搬到 OutputState（见 .cpp）。
 */
USTRUCT()
struct FGASPSmoothWalkingState : public FMoverDataStructBase
{
	GENERATED_BODY()

	// ---- FMoverDataStructBase 要求实现的接口（具体都在 .cpp 里）----
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FMoverDataStructBase* Clone() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual void ToString(FAnsiStringBuilderBase& Out) const override;
	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthorityState) const override;
	virtual void Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct) override;

	// ------------------------------------------------------------------
	// 下面 5 个字段就是"弹簧"的全部记忆。读懂它们，就读懂了整个平滑模型：
	//
	//   IntermediateVelocity（中间速度）：按加速度积分出来的"理想速度"，不做平滑
	//            ↑ 用临界弹簧去追它
	//   SpringVelocity（弹簧速度）：最终输出给移动模式的速度（平滑过的）
	//
	// 另外 SpringAcceleration 是 SpringVelocity 的一阶量（弹簧积分需要），
	// IntermediateFacing / IntermediateAngularVelocity 是"双弹簧转身"用的中间量。
	// ------------------------------------------------------------------

	/** 内部速度弹簧的"当前位置" —— 也就是最终输出给移动模式的速度 */
	UPROPERTY(BlueprintReadOnly, Category = "Mover|Experimental")
	FVector SpringVelocity = FVector::ZeroVector;

	/** 内部速度弹簧的"当前速度"（弹簧的一阶量，临界阻尼积分要用） */
	UPROPERTY(BlueprintReadOnly, Category = "Mover|Experimental")
	FVector SpringAcceleration = FVector::ZeroVector;

	/** 中间速度：不做平滑、纯粹按加速度积分出来的目标速度，弹簧负责去追它 */
	UPROPERTY(BlueprintReadOnly, Category = "Mover|Experimental")
	FVector IntermediateVelocity = FVector::ZeroVector;

	/** 双弹簧转身时的中间朝向（只有 bSmoothFacingWithDoubleSpring = true 才用得到） */
	UPROPERTY(BlueprintReadOnly, Category = "Mover|Experimental")
	FQuat IntermediateFacing = FQuat::Identity;

	/** 双弹簧转身时，中间朝向的角速度 */
	UPROPERTY(BlueprintReadOnly, Category = "Mover|Experimental")
	FVector IntermediateAngularVelocity = FVector::ZeroVector;
};
