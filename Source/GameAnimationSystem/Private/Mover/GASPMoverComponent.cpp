// Copyright: Jichao Luo


#include "Mover/GASPMoverComponent.h"

UGASPMoverComponent::UGASPMoverComponent()
{
	bSyncInputsForSimProxy = true;	// 让远端也读到输入
}

void UGASPMoverComponent::BeginPlay()
{
	Super::BeginPlay();

	OnPreSimulationTick.AddDynamic(this, &ThisClass::OnMoverCompPreSimulateTick);
	OnPostFinalize.AddDynamic(this, &ThisClass::OnMoverCompPostFinalize);
	OnBasedMovementApplied.AddDynamic(this, &ThisClass::OnMoverCompBasedMovementApplied);
}

void UGASPMoverComponent::OnMoverCompPreSimulateTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd)
{

}

void UGASPMoverComponent::OnMoverCompPostFinalize(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState)
{

}

void UGASPMoverComponent::OnMoverCompBasedMovementApplied(const FTransform& TransformDelta, const FMoverTimeStep& TimeStep)
{

}
