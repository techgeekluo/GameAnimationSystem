// Copyright: Jichao Luo


#include "GameAnimationSystem.h"
#include "GASPMoverPawn.h"
#include "GASPCamera.h"
#include "GASPFunctionLibrary.h"
#include "MoverComponent.h"
#include "Engine/BlueprintGeneratedClass.h"

AGASPMoverPawn::AGASPMoverPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	SetReplicatingMovement(false);	// disable Actor-level movement replication, Mover component will handle it
	//bUseControllerRotationYaw = false;

	Camera = CreateDefaultSubobject<UGASPCamera>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetVisibility(false);

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
		Camera->GetViewInfo(ViewInfo);
		return;
	}
	Super::CalcCamera(DeltaTime, ViewInfo);
}

void AGASPMoverPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	// 1) C++ 侧填输入（派生类重写 OnProduceInput）
	OnProduceInput((float)SimTimeMs, InputCmdResult);

	// 2) 蓝图侧填输入：能拿到上一步填好的 context 继续改
	if (UGASPFunctionLibrary::IsFuncImplementedInBlueprint(this, TEXT("OnProduceInputInBlueprint")))
	{
		InputCmdResult = OnProduceInputInBlueprint((float)SimTimeMs, InputCmdResult);
	}

	// 3) 统一消费边沿量：一次输入只触发一次（一个渲染帧跑多个模拟步时，跳跃不会重复触发）
	
}

void AGASPMoverPawn::OnProduceInput(float DeltaMs, FMoverInputCmdContext& InputCmdResult)
{

}

void AGASPMoverPawn::ReceiveMoveInput(const FVector2D& MoveInput)
{
	// 收到移动输入（相机空间：X=前后，Y=左右，长度表示力度），松开时收到零向量
	//
	// 坐标系换算：IA_Move 的 X = 屏幕前后、Y = 屏幕左右；
	// 而 FVector 里 X = 右、Y = 前，所以两者要对调。
	// 结果是「相机空间」的方向意图（长度 = 力度），旋到世界空间是后面的事。
	CachedMoveInputIntent = FVector(
		FMath::Clamp(MoveInput.Y, -1.f, 1.f),	// FVector.X = 前（来自 IA_Move 的 Y）
		FMath::Clamp(MoveInput.X, -1.f, 1.f),	// FVector.Y = 右（来自 IA_Move 的 X）
		0);	
}

void AGASPMoverPawn::RequestMoveByVelocity(const FVector& DesiredVelocity)
{
	CachedMoveInputVelocity = DesiredVelocity;	// 世界空间 cm/s
}
