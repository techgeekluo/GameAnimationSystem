// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/GASPLocomotionTypes.h"
#include "Types/GASPAnimationTypes.h"
#include "GASPMoverCharacterInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGASPMoverCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GAMEANIMATIONSYSTEM_API IGASPMoverCharacterInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GASP|Properties")
	void GetAnimationProperties(
		FGASPEssentialStates& EssentialStates,
		FGASPEssentialValues& EssentialValues
	) const;


};
