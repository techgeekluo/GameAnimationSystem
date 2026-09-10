// Copyright: Jichao Luo


#include "GASPCamera.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Engine/OverlapResult.h"

UGASPCamera::UGASPCamera()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bTickInEditor = false;
	bHiddenInGame = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CameraMeshAsset(
		TEXT("/GameAnimationSystem/Blueprints/Camera/SKM_MatineeCamera.SKM_MatineeCamera")
	);
	if (CameraMeshAsset.Succeeded())
	{
		SetSkeletalMesh(CameraMeshAsset.Object);
	}


}

void UGASPCamera::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{

}

void UGASPCamera::PostLoad()
{
}

void UGASPCamera::OnRegister()
{
}

void UGASPCamera::Activate(bool bReset)
{
}

void UGASPCamera::RegisterComponentTickFunctions(bool bRegister)
{
}

void UGASPCamera::BeginPlay()
{
}

void UGASPCamera::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
}

void UGASPCamera::CompleteParallelAnimationEvaluation(bool bDoPostAnimationEvaluation)
{
}

void UGASPCamera::GetViewInfo(FMinimalViewInfo& ViewInfo) const
{

}