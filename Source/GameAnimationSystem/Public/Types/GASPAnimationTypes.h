#pragma once

#include "CoreMinimal.h"
#include "GASPLocomotionTypes.h"
#include "GASPAnimationTypes.generated.h"

USTRUCT(BlueprintType)
struct GAMEANIMATIONSYSTEM_API FGASPEssentialStates
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EGASPMovementMode MovementMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EGASPGait Gait;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EGASPStance Stance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EGASPRotationMode RotationMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsAiming;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EGASPMoveDirection MoveDirection;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	//EGASPOverlayBase OverlayBase;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	//EGASPOverlayPose OverlayPose;
};

USTRUCT(BlueprintType)
struct GAMEANIMATIONSYSTEM_API FGASPEssentialValues
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Velocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Speed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector InputAcceleration;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FRotator OrientationIntent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FRotator AimingRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector GroundedNormal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector GroundedLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTransform BaseMovementDelta;
};