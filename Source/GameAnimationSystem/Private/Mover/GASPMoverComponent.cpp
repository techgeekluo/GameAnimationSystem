// Copyright: Jichao Luo


#include "Mover/GASPMoverComponent.h"

UGASPMoverComponent::UGASPMoverComponent()
{
	bSyncInputsForSimProxy = true;	// 让远端也读到输入
}

void UGASPMoverComponent::BeginPlay()
{
	Super::BeginPlay();

	
}