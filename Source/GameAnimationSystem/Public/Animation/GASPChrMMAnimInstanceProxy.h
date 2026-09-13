#pragma once

#include "GASPChrAnimInstanceProxy.h"
#include "Types/GASPMotionMatchingTypes.h"
#include "GASPChrMMAnimInstanceProxy.generated.h"

USTRUCT()
struct GAMEANIMATIONSYSTEM_API FGASPChrMMAnimInstanceProxy : public FGASPChrAnimInstanceProxy
{
	GENERATED_BODY()

protected:
	virtual void InitializeObjects(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaTime) override;
	virtual void Update(float DeltaTime) override;
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const override;
};