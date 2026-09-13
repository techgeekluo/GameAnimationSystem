// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GASPMoverComponent.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GAMEANIMATIONSYSTEM_API UGASPMoverComponent : public UCharacterMoverComponent
{
	GENERATED_BODY()
	
public:
	UGASPMoverComponent();
	
protected:
	virtual void BeginPlay() override;

protected:
	UFUNCTION() void OnMoverCompPreSimulateTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd);
	UFUNCTION() void OnMoverCompPostFinalize(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState);
	UFUNCTION() void OnMoverCompBasedMovementApplied(const FTransform& TransformDelta, const FMoverTimeStep& TimeStep);

private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName CachedMovementMode;
};
