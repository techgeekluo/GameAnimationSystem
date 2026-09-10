// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "GASPMoverPawn.h"
#include "GASPMoverCharacter.generated.h"

/**
 * 
 */
UCLASS()
class GAMEANIMATIONSYSTEM_API AGASPMoverCharacter : public AGASPMoverPawn
{
	GENERATED_BODY()

public:
	AGASPMoverCharacter(const FObjectInitializer& ObjectInitializer);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* CapsuleComponent;

};
