// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "Animation/GASPChrAnimInstance.h"
#include "Animation/GASPChrMMAnimInstanceProxy.h"
#include "GASPChrMMAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAMEANIMATIONSYSTEM_API UGASPChrMMAnimInstance : public UGASPChrAnimInstance
{
	GENERATED_BODY()

	friend struct FGASPChrMMAnimInstanceProxy;

	UPROPERTY(Transient)
	FGASPChrMMAnimInstanceProxy MotionMatchingProxy;

	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return &MotionMatchingProxy; }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override {}
	
protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

private:

};
