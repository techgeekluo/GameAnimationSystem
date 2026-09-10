// Copyright: Jichao Luo


#include "GASPMoverPawn.h"
#include "GASPCamera.h"
#include "MoverComponent.h"

AGASPMoverPawn::AGASPMoverPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	Mesh->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UGASPCamera>(TEXT("Camera"));
	Camera->SetupAttachment(Mesh);

	MoverComponent = CreateDefaultSubobject<UMoverComponent>(TEXT("MoverComponent"));
}

void AGASPMoverPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGASPMoverPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGASPMoverPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& ViewInfo)
{
	if (Camera && Camera->IsActive())
	{
		
	}

	Super::CalcCamera(DeltaTime, ViewInfo);
}
