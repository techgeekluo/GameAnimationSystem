#include "Animation/GASPChrAnimInstanceProxy.h"
#include "Animation/GASPChrAnimInstance.h"
#include "Mover/GASPMoverCharacter.h"
#include "Mover/GASPMoverComponent.h"

void FGASPChrAnimInstanceProxy::InitializeObjects(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::InitializeObjects(InAnimInstance);

	Chr = Cast<AGASPMoverCharacter>(InAnimInstance->GetOwningActor());
	if (Chr)
	{
		Mover = Chr->GetMoverComponent();
		AnimInst = Cast<UGASPChrAnimInstance>(InAnimInstance);
	}
}

void FGASPChrAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaTime)
{
	// GameThread
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaTime);

	if (!Chr)
	{
		Chr = Cast<AGASPMoverCharacter>(InAnimInstance->GetOwningActor());
		if (!Chr) return;
	}
	if (!Mover)
	{
		Mover = Chr->GetMoverComponent();
		if (!Mover) return;
	}
	if (!AnimInst)
	{
		AnimInst = Cast<UGASPChrAnimInstance>(InAnimInstance);
		if (!AnimInst) return;
	}

	//CachedEssentialStates = AnimInst->EssentialStates;
	//CachedEssentialValues = AnimInst->EssentialValues;

	//UpdateEssentialStates();
	//UpdateEssentialValues();
}

void FGASPChrAnimInstanceProxy::Update(float DeltaTime)
{
	// AnimationThread
	FAnimInstanceProxy::Update(DeltaTime);

}

void FGASPChrAnimInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) const
{
	// GameThread
	FAnimInstanceProxy::PostUpdate(InAnimInstance);
}

void FGASPChrAnimInstanceProxy::UpdateEssentialStates()
{

}

void FGASPChrAnimInstanceProxy::UpdateEssentialValues()
{
	EssentialValues.Velocity = Mover->GetVelocity();
	EssentialValues.Speed = Mover->GetVelocity().Size2D();


}
