// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Types/GASPLocomotionTypes.h"
#include "GASPCameraSettings.h"
#include "GASPCameraAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAMEANIMATIONSYSTEM_API UGASPCameraAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
protected:
	void NativeInitializeAnimation() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Camera", Meta = (AllowPrivateAccess = "true"))
	EGASPViewMode ViewMode = EGASPViewMode::ThirdPerson;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Camera", Meta = (AllowPrivateAccess = "true"))
	EGASPShoulderMode ShoulderMode = EGASPShoulderMode::Middle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Locomotion", Meta = (AllowPrivateAccess = "true"))
	EGASPGait Gait = EGASPGait::Walking;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Locomotion", Meta = (AllowPrivateAccess = "true"))
	EGASPStance Stance = EGASPStance::Standing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Locomotion", Meta = (AllowPrivateAccess = "true"))
	EGASPRotationMode RotationMode = EGASPRotationMode::VelocityDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Locomotion", Meta = (AllowPrivateAccess = "true"))
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|BlendTime", Meta = (AllowPrivateAccess = "true"))
	float ShoulderModeBlendTime = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|ActiveStates", Meta = (AllowPrivateAccess = "true"))
	bool bActiveRotationMode = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|ActiveStates", Meta = (AllowPrivateAccess = "true"))
	bool bActiveAiming = false;

public:
	FORCEINLINE bool GetActiveAiming() const { return bActiveAiming; }
};
