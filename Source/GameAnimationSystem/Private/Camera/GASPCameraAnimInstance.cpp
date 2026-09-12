// Copyright: Jichao Luo


#include "Camera/GASPCameraAnimInstance.h"
#include "Camera/GASPCameraComponent.h"

void UGASPCameraAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (auto Camera = Cast<UGASPCameraComponent>(GetOwningComponent()))
	{
		Camera->OnViewModeChangedDelegate.AddLambda(
			[this](EGASPViewMode NewViewMode) { ViewMode = NewViewMode; }
		);
		Camera->OnShoulderModeChangedDelegate.AddLambda(
			[this](EGASPShoulderMode NewShoulderMode) { ShoulderMode = NewShoulderMode; }
		);
		Camera->OnActiveCameraAimingDelegate.AddLambda(
			[this](bool bActive)
			{
				bActiveAiming = bActive;
				bActiveRotationMode = true;
				if (bActiveAiming && bIsAiming)
				{
					ShoulderModeBlendTime = 0.35f;
				}
				else
				{
					ShoulderModeBlendTime = 0.5f;
				}
			}
		);
	}

	// TODO: 在Gait/Stance/RotationMode和bAiming切换时修改属性

}
