// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/GASPChrAnimInstanceProxy.h"
#include "GASPChrAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAMEANIMATIONSYSTEM_API UGASPChrAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend struct FGASPChrAnimInstanceProxy;

	UPROPERTY(Transient)
	FGASPChrAnimInstanceProxy Proxy;

	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return &Proxy; }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override {}
	
protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Essential", meta = (AllowPrivateAccess = "true"))
	FGASPEssentialStates EssentialStates;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Essential", meta = (AllowPrivateAccess = "true"))
	FGASPEssentialValues EssentialValues;

};
