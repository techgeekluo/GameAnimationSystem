#pragma once

#include "CoreMinimal.h"
#include "MoverDataModelTypes.h"
#include "Types/GASPLocomotionTypes.h"
#include "GASPMoverInputs.generated.h"

USTRUCT(BlueprintType)
struct FGASPMoverInputs : public FMoverDataStructBase
{
	GENERATED_BODY()

	// Sets the directional move inputs for a simulation frame
	void SetMoveInput(EMoveInputType InMoveInputType, const FVector& InMoveInput);

	const FVector& GetMoveInput() const { return MoveInput; }
	EMoveInputType GetMoveInputType() const { return MoveInputType; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mover)
	EMoveInputType MoveInputType;

	/**
	 * Representing the directional move input for this frame. Must be interpreted according to MoveInputType. Relative to MovementBase if set, world space otherwise. Will be truncated to match network serialization precision.
	 * Note: Use SetDirectionalInput or SetVelocityInput to set MoveInput and MoveInputType
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mover)
	FVector MoveInput;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGASPGait Gait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGASPStance Stance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGASPRotationMode RotationMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGASPMoveDirection MoveDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ControlRotationRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWantsToCrouch;

	FGASPMoverInputs()
		: Gait(EGASPGait::Walking)
		, Stance(EGASPStance::Standing)
		, RotationMode(EGASPRotationMode::VelocityDirection)
		, MoveDirection(EGASPMoveDirection::F)
		, RotationOffset(0.f)
		, ControlRotationRate(0.f)
		, bWantsToCrouch(false)
	{ }

	bool operator==(const FGASPMoverInputs& Other) const;

	virtual FMoverDataStructBase* Clone() const override { return new FGASPMoverInputs(*this); }
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }

	// ---- 网络同步 ----
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

	/**
	 * ShouldReconcile —— 这里有个容易被误导的细节，值得说清楚：
	 *
	 * 【基类的默认实现会 assert！】：如果它被调用而你没重写 → 直接断言崩溃。
	 *
	 * 【但普通输入结构体不会被调用】
	 *     纠偏判定只发生在【同步状态（Sync/Aux）】上（NP 的 QueryRollback 只比这两者）。
	 *     InputCollection 是"每帧重来的输入"，不参与比对。
	 *     → 所以**普通输入结构体完全可以不重写这个函数**（不会走到基类的 checkf）。
	 *
	 * 【那要不要写？】
	 *     - 想稳妥：写一个返回 false（明确声明"我不参与纠偏"）。
	 *       引擎的 FMoverInputContainerDataStruct 就是恒返回 false。
	 *     - 只有【会影响物理模拟的输入】（比如 ChaosMover）才需要认真实现它。
	 */
	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthorityState) const override
	{
		return false;   // 输入不参与纠偏判定
	}

	/**
	 * 调试输出。LogMover 或 GameplayDebugger 打印这个结构体时会调用它。
	 * 空实现 = 调试时什么都看不到，所以这里把关键字段都打出来。
	 */
	virtual void ToString(FAnsiStringBuilderBase& Out) const override
	{
		Super::ToString(Out);

		Out.Appendf("MoveInput: %s (Type %u)\n",
			TCHAR_TO_ANSI(*MoveInput.ToCompactString()), uint32{EnumToUnderlyingType(MoveInputType)});
		Out.Appendf("Gait=%u Stance=%u RotationMode=%u MoveDirection=%u\n",
			uint32{EnumToUnderlyingType(Gait)}, uint32{EnumToUnderlyingType(Stance)},
			uint32{EnumToUnderlyingType(RotationMode)}, uint32{EnumToUnderlyingType(MoveDirection)});
		Out.Appendf("RotationOffset=%.2f ControlRotationRate=%.2f bWantsToCrouch=%i\n",
			RotationOffset, ControlRotationRate, bWantsToCrouch ? 1 : 0);
	}
};