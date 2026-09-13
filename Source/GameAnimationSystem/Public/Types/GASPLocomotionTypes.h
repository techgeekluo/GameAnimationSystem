#pragma once

#include "CoreMinimal.h"
#include "GASPLocomotionTypes.generated.h"

UENUM(BlueprintType)
enum class EGASPMovementMode : uint8
{
	InAir,
	OnGround,
	OnWater,		// TODO
	UnderWater,		// TODO
};

UENUM(BlueprintType)
enum class EGASPGait : uint8
{
	Walking,
	Running,
	Sprinting
};

UENUM(BlueprintType)
enum class EGASPStance : uint8
{
	Standing,
	Crouching,
	Crawl,		// TODO
};

UENUM(BlueprintType)
enum class EGASPRotationMode : uint8
{
	VelocityDirection,
	LookingDirection
};

UENUM(BlueprintType)
enum class EGASPMoveDirection : uint8
{
	F,
	B,
	FL,
	FR,
	BL,
	BR
};

UENUM(BlueprintType)
enum class EGASPMoveStickMode : uint8
{
	FixedSpeed_SingleGait,		// 固定速度，单一步态：Stick输入值不影响速度和步态
	FixedSpeed_WalkRun,			// 固定速度，走跑切换：Stick输入值不影响速度，拉满会切位跑步
	VariableSpeed_SingleGait,	// 可变速度，单一步态：Stick输入值影响速度，但不影响步态
	VariableSpeed_WalkRun		// 可变速度，走跑切换：Stick输入值影响速度，拉满会切位跑步
};


