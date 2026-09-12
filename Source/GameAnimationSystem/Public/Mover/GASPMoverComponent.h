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
	
	virtual void BeginPlay() override;

	


};
