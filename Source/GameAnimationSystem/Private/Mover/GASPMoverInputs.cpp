#include "Mover/GASPMoverInputs.h"
#include "Engine/NetSerialization.h"	// FVector_NetQuantize100

void FGASPMoverInputs::SetMoveInput(EMoveInputType InMoveInputType, const FVector& InMoveInput)
{
	MoveInputType = InMoveInputType;

	// 限制"存起来"的精度，让它和 NetSerialize 发出去的一致（1/100，即 0.01）。
	//
	// ⚠️ 这一步不是可选的优化，而是【正确性要求】：
	//    如果本地存全精度、网络只发量化值，那"自主端本地模拟"和"服务器模拟"用的就是
	//    两个不同的输入，算出来的结果自然对不上 → 触发回滚 → 表现抖动。
	MoveInput.X = FMath::RoundToFloat(InMoveInput.X * 100.0) / 100.0;
	MoveInput.Y = FMath::RoundToFloat(InMoveInput.Y * 100.0) / 100.0;
	MoveInput.Z = FMath::RoundToFloat(InMoveInput.Z * 100.0) / 100.0;
}

bool FGASPMoverInputs::operator==(const FGASPMoverInputs& Other) const
{
	return MoveInputType == Other.MoveInputType
		&& MoveInput == Other.MoveInput
		&& Gait == Other.Gait
		&& Stance == Other.Stance
		&& RotationMode == Other.RotationMode
		&& MoveDirection == Other.MoveDirection
		&& RotationOffset == Other.RotationOffset
		&& ControlRotationRate == Other.ControlRotationRate
		&& bWantsToCrouch == Other.bWantsToCrouch;
}

bool FGASPMoverInputs::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// ★ 必须先调基类：它会设置 bOutSuccess，并处理基类那部分字段。
	//   对照引擎 FCharacterDefaultInputs::NetSerialize 的第一行就是这个。
	//   （漏掉它不一定立刻出错，但基类语义就不对了）
	Super::NetSerialize(Ar, Map, bOutSuccess);

	// ==================================================================
	// ① 移动输入 —— 完全照抄引擎 FCharacterDefaultInputs 的做法
	// ==================================================================

	// MoveInputType 是 uint8 底层的枚举，直接 << 就是发 1 个字节
	Ar << MoveInputType;

	// MoveInput 用 FVector_NetQuantize100 发。
	// 它内部的实现就是 SerializePackedVector<100, 30>（精度 1/100、每分量最多 30 bits），
	// 和引擎对 FCharacterDefaultInputs::MoveInput 的处理完全一致。
	// ⚠️ 精度必须和上面 SetMoveInput() 里的截断保持一致，两者要改一起改。
	{
		FVector_NetQuantize100 QuantizedMoveInput = MoveInput;
		QuantizedMoveInput.NetSerialize(Ar, Map, bOutSuccess);
		MoveInput = QuantizedMoveInput;
	}

	// ==================================================================
	// ② 离散状态（枚举）—— 按"刚好够用的位数"发
	// ==================================================================
	// 这是最省事也最有效的一种量化：枚举只有几个取值，没必要占满 8 bits。
	//
	// 注意：Ar.SerializeBits 对"写"和"读"是【同一个调用】，不需要 IsSaving/IsLoading 分支。
	//      写的时候它从变量里取低 N 位；读的时候它把 N 位写回变量。
	//      ⚠️ 所以【写之前必须先把值放进变量】——下面每处都是先赋值再 SerializeBits。
	//
	// ⚠️ 另外：位数的选择必须和枚举的实际取值个数匹配。
	//      以后往枚举里加值（比如 EGASPGait 再加个 "Crawl"），记得回来改位数，
	//      否则会被截断成错误的值（2 bits 只能表示 0~3）。

	uint8 GaitByte = static_cast<uint8>(Gait);
	Ar.SerializeBits(&GaitByte, 2);				// Walking / Running / Sprinting → 3 个值 → 2 bits（原来是 8 bits）
	Gait = static_cast<EGASPGait>(GaitByte);

	uint8 StanceByte = static_cast<uint8>(Stance);
	Ar.SerializeBits(&StanceByte, 2);			// Standing / Crouching / Crawl → 3 个值 → 2 bits
	Stance = static_cast<EGASPStance>(StanceByte);

	uint8 RotationModeByte = static_cast<uint8>(RotationMode);
	Ar.SerializeBits(&RotationModeByte, 1);		// VelocityDirection / LookingDirection → 2 个值 → 1 bit
	RotationMode = static_cast<EGASPRotationMode>(RotationModeByte);

	uint8 MoveDirectionByte = static_cast<uint8>(MoveDirection);
	Ar.SerializeBits(&MoveDirectionByte, 3);	// F/B/FL/FR/BL/BR → 6 个值 → 3 bits
	MoveDirection = static_cast<EGASPMoveDirection>(MoveDirectionByte);

	// ==================================================================
	// ③ 浮点量 —— 目前全精度（每个 32 bits）
	// ==================================================================
	// 想省带宽可以量化。例如 RotationOffset 只关心 0.1° 时：
	//
	//     int16 QuantizedOffset = (int16)FMath::RoundToInt(RotationOffset * 10.f);
	//     Ar.SerializeInt(QuantizedOffset, 65536);        // 16 bits（原 32 bits）
	//     RotationOffset = QuantizedOffset / 10.f;
	//
	// ⚠️ 但必须【同时在写入端做同样的截断】（理由和 SetMoveInput 那段一样），
	//    否则又会出现"本地全精度 / 网络量化值"的分叉。
	//    在没想清楚到底要多少精度之前，全精度是最安全的选择。
	Ar << RotationOffset;
	Ar << ControlRotationRate;

	// ==================================================================
	// ④ bool —— 1 bit
	// ==================================================================
	// 和引擎对 bIsJumpJustPressed / bIsJumpPressed 的处理一致
	Ar.SerializeBits(&bWantsToCrouch, 1);

	bOutSuccess = true;
	return true;
}
