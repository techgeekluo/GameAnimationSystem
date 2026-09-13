#pragma once

#include "Animation/AnimInstanceProxy.h"
#include "Types/GASPAnimationTypes.h"
#include "GASPChrAnimInstanceProxy.generated.h"

/**
 *
 */
USTRUCT()
struct GAMEANIMATIONSYSTEM_API FGASPChrAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	class AGASPMoverCharacter* Chr;

	UPROPERTY(Transient)
	class UGASPMoverComponent* Mover;

	UPROPERTY(Transient)
	class UGASPChrAnimInstance* AnimInst;

protected:
	virtual void InitializeObjects(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaTime) override;
	virtual void Update(float DeltaTime) override;
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const override;

	void UpdateEssentialStates();
	void UpdateEssentialValues();

private:
	UPROPERTY(Transient)
	FGASPEssentialStates CachedEssentialStates;

	UPROPERTY(Transient)
	FGASPEssentialValues CachedEssentialValues;

	UPROPERTY(Transient)
	FGASPEssentialStates EssentialStates;

	UPROPERTY(Transient)
	FGASPEssentialValues EssentialValues;
};