// Copyright: Jichao Luo


#include "GASPMoverCharacter.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"

AGASPMoverCharacter::AGASPMoverCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCharacterMoverComponent>(TEXT("MoverComponent")))
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CharacterCapsule"));
	SetRootComponent(CapsuleComponent);

}
