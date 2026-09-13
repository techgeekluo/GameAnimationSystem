#include "Animation/GASPChrMMAnimInstanceProxy.h"
#include "Animation/GASPChrMMAnimInstanceProxy.h"
#include "Mover/GASPMoverCharacter.h"
#include "Mover/GASPMoverComponent.h"

void FGASPChrMMAnimInstanceProxy::InitializeObjects(UAnimInstance* InAnimInstance)
{
	FGASPChrAnimInstanceProxy::InitializeObjects(InAnimInstance);
}

void FGASPChrMMAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaTime)
{
	FGASPChrAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaTime);
}

void FGASPChrMMAnimInstanceProxy::Update(float DeltaTime)
{
	FGASPChrAnimInstanceProxy::Update(DeltaTime);
}

void FGASPChrMMAnimInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) const
{
	FGASPChrAnimInstanceProxy::PostUpdate(InAnimInstance);
}
