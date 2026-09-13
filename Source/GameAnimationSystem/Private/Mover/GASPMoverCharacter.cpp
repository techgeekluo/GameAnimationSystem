// Copyright: Jichao Luo


#include "Mover/GASPMoverCharacter.h"
#include "Mover/GASPMoverComponent.h"
#include "Camera/GASPCameraComponent.h"
#include "GASPPlayerController.h"
#include "GASPFunctionLibrary.h"
#include "EnhancedPlayerInput.h"
#include "Components/CapsuleComponent.h"
#include "MoverComponent.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugLibrary.h"
#include "GameAnimationSystem.h"

AGASPMoverCharacter::AGASPMoverCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	SetReplicatingMovement(false);	// disable Actor-level movement replication, Mover component will handle it

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CharacterCapsule"));
	Capsule->SetCapsuleHalfHeight(86.f);
	Capsule->SetCapsuleRadius(30.f);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionObjectType(ECC_Pawn);
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SetRootComponent(Capsule);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	Mesh->SetupAttachment(Capsule);
	Mesh->SetCastHiddenShadow(true);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	Camera = CreateDefaultSubobject<UGASPCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Capsule);
	Camera->SetupAttachment(RootComponent);
	Camera->SetVisibility(false);

	Mover = CreateDefaultSubobject<UGASPMoverComponent>(TEXT("CharacterMoverComponent"));
	Mover->SetPrimaryVisualComponent(Mesh);
}

void AGASPMoverCharacter::BeginPlay()
{
	Super::BeginPlay();

	Mesh->AddTickPrerequisiteComponent(Mover);
	Mesh->AddTickPrerequisiteActor(this);

	if (auto PC = Cast<AGASPPlayerController>(GetController()))
	{
		EnhancedInput = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
		IA_Move = PC->GetMoveInputAction();
		if (!IA_Move) UE_LOG(LogGASPMover, Error, TEXT("Character Get IA_Move Failed..."));
	}
	bHasProduceInputInBpFunc = UGASPFunctionLibrary::IsFuncImplementedInBlueprint(this, TEXT("OnProduceInputInBlueprint"));
}

void AGASPMoverCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Mover || !Mesh) return;

	// 1. Cache Inputs From Mover
	//	  _PreSim 表示的是发送给 Mover 的输入，这些值不会被复制，并且只会在本地控制的 Pawn 上设置
	//	  _PostSim 表示的是从 Mover 中获取出来的输入，这些输入可以安全地用于控制其他系统，例如动画/物理等
	FMoverDataCollection InputCollection = Mover->GetLastInputCmd().InputCollection;
	if (auto FoundedPtr = InputCollection.FindDataByType<FCharacterDefaultInputs>())
	{
		MoverCharacterInputs_PostSim = *FoundedPtr;
	}
	if (auto FoundedPtr = InputCollection.FindDataByType<FGASPMoverInputs>())
	{
		MoverCustomInputs_PostSim = *FoundedPtr;
	}

	// 2. Update Floor Values: 其他各种系统可能需要知道角色所在地板的信息
	bool bDidFindFloor = false;
	FFloorCheckResult CheckFloorResult;
	UFloorQueryUtils::TryFindFloor(Mover, bDidFindFloor, CheckFloorResult);
	FloorNormal = CheckFloorResult.bBlockingHit ? CheckFloorResult.HitResult.ImpactNormal : FVector::ZeroVector;
	FloorLocation = CheckFloorResult.bBlockingHit ? CheckFloorResult.HitResult.ImpactPoint : GetActorLocation();

	// 3. Update Control Rotation Rate: 防止在扫射和瞄准模式下快速旋转相机时旋转不足
	ControlRotationRate = UKismetMathLibrary::NormalizedDeltaRotator(GetControlRotation(), LastControlRotation).Yaw / DeltaTime;
	LastControlRotation = GetControlRotation();

	// 4. Update Slide Audio : （TODO）

	// 5. Update Targeted Actor: 近战动作游戏的锁敌攻击，可根据业务更改逻辑（TODO）
	if (TargetableActors.IsEmpty())
	{
		TargetedActor = nullptr;
	}
	else
	{
		float Distance;
		TargetedActor = UGameplayStatics::FindNearestActor(GetActorLocation(), TargetableActors, Distance);

		auto DebugDrawer = UDrawDebugLibrary::MakeDebugDrawer(this);
		UDrawDebugLibrary::DrawDebugCone(
			DebugDrawer,
			TargetedActor->GetActorLocation() + FVector(0.f, 0.f, 150.f),
			FRotator(0.f, 0.f, 180.f),
			FDrawDebugLineStyle(),
			true,
			20.f,
			10.f,
			4
		);
	}

	// 6. Update Twin Stick Mode: 模拟一种基本的双摇杆控制方案，其中 Pawn 沿右手拇指摇杆的方向旋转（TODO）

	// 7. Update Smoothed Analog Input Amount
	float MoveInputAmount = GetMoveInput2D().Size();
	SmoothedAnalogInputAmount = UKismetMathLibrary::FInterpTo_Constant(
		SmoothedAnalogInputAmount,
		MoveInputAmount,
		DeltaTime,
		MoveInputAmount > SmoothedAnalogInputAmount ? 100.f : 2.f
	);
	
	// 8. Debug Draws

}

void AGASPMoverCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& ViewInfo)
{
	if (Camera && Camera->IsActive())
	{
		Camera->GetViewInfo(ViewInfo);
		return;
	}
	Super::CalcCamera(DeltaTime, ViewInfo);
}

FVector AGASPMoverCharacter::GetFirstPersonCameraLocation_Implementation() const
{
	if (auto CameraSettings = Camera->GetCameraSettings())
	{
		return Mesh->GetSocketLocation(CameraSettings->FP_Settings.CameraSocketName);
	}
	return GetActorLocation();
}

FVector AGASPMoverCharacter::GetThirdPersonPivotLocation_Implementation() const
{
	return (Mesh->GetSocketLocation(FName("root")) + Mesh->GetSocketLocation(FName("head"))) * 0.5f;
}

FQuat AGASPMoverCharacter::GetOwnerMeshQuat_Implementation() const
{
	return Mesh->GetComponentQuat();
}

FVector AGASPMoverCharacter::GetOwnerMeshScale_Implementation() const
{
	return Mesh->GetComponentScale();
}

FVector AGASPMoverCharacter::GetOwnerMeshSocketLocation_Implementation(FName SocketName) const
{
	return Mesh->GetSocketLocation(SocketName);
}

void AGASPMoverCharacter::SetOwnerMeshNoSee_Implementation(bool bNewOwnerNoSee)
{
	Mesh->SetOwnerNoSee(bNewOwnerNoSee);
}

void AGASPMoverCharacter::GetAnimationProperties_Implementation(FGASPEssentialStates& EssentialStates, FGASPEssentialValues& EssentialValues) const
{

}

void AGASPMoverCharacter::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	// 1) C++ 侧填输入（派生类重写 OnProduceInput）
	OnProduceInput((float)SimTimeMs, InputCmdResult);

	// 2) 蓝图侧填输入：能拿到上一步填好的 context 继续改
	if (bHasProduceInputInBpFunc)
	{
		InputCmdResult = OnProduceInputInBlueprint((float)SimTimeMs, InputCmdResult);
	}

	// 3) 统一消费边沿量：一次输入只触发一次（一个渲染帧跑多个模拟步时，不会重复触发）
	bIsJumpJustPressed = false;
}

void AGASPMoverCharacter::OnProduceInput(float DeltaMs, FMoverInputCmdContext& InputCmdResult)
{
	// 1) 取出（没有就创建）角色输入结构体 —— 返回的是引用，直接改
	auto& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	// 2) 使用玩家或AI控制器所需的移动方向设置方向输入
	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, GetMoveInputIntent());

	// 3) 设置瞄准旋转和跳跃输入状态
	CharacterInputs.ControlRotation = GetAimingRotation();
	CharacterInputs.bIsJumpJustPressed = bIsJumpJustPressed;

}

FVector2D AGASPMoverCharacter::GetMoveInput2D() const
{
	if (EnhancedInput && IA_Move)
	{
		return EnhancedInput->GetActionValue(IA_Move).Get<FVector2D>();
	}
	return FVector2D(CachedMoveInputIntent.X, CachedMoveInputIntent.Y);
}

FVector AGASPMoverCharacter::GetMoveInputIntent() const
{
	// 返回AI控制器的所需移动方向，或玩家在相机空间中的当前移动输入方向
	if (auto PC = Cast<APlayerController>(GetController()))
	{
		FVector2D MoveInput2D = GetMoveInput2D();
		FVector MoveInputValue = FVector(
			FMath::Clamp(MoveInput2D.Y, -1.f, 1.f),	// FVector.X = 前（来自 IA_Move 的 Y）
			FMath::Clamp(MoveInput2D.X, -1.f, 1.f),	// FVector.Y = 右（来自 IA_Move 的 X）
			0);

		FRotator ControlRot = GetControlRotation();
		return UKismetMathLibrary::GreaterGreater_VectorRotator(
			UKismetMathLibrary::ClampVectorSize(MoveInputValue, 0.f, 1.f),
			MovementMode == EGASPMovementMode::Flying ? ControlRot : FRotator(0.f, ControlRot.Yaw, 0.f)
		).GetSafeNormal();
	}

	// TODO: 没有玩家控制器（AI 未 possess / 网络模拟代理）→ 走 NavMover
	return FVector();
}

FRotator AGASPMoverCharacter::GetAimingRotation() const
{
	// 返回 AimingRotation，这是 Pawn 在 Strafe 或 Aiming 旋转模式下应该查看的旋转 
	if (TargetedActor)
	{
		return FRotator(
			0.f,
			UKismetMathLibrary::Conv_VectorToRotator(TargetedActor->GetActorLocation() - GetActorLocation()).Yaw,
			0.f
		);
	}
	return bTwinStickMode ? TwinStickAimRotation : GetControlRotation();
}

void AGASPMoverCharacter::ReceiveMoveInput(const FVector2D& MoveInput)
{
	// 收到移动输入（相机空间：X=前后，Y=左右，长度表示力度），松开时收到零向量
	// 坐标系换算：IA_Move 的 X = 屏幕前后、Y = 屏幕左右；
	// 而 FVector 里 X = 右、Y = 前，所以两者要对调。
	// 结果是「相机空间」的方向意图（长度 = 力度），旋到世界空间是后面的事。
	CachedMoveInputIntent = FVector(
		FMath::Clamp(MoveInput.Y, -1.f, 1.f),	// FVector.X = 前（来自 IA_Move 的 Y）
		FMath::Clamp(MoveInput.X, -1.f, 1.f),	// FVector.Y = 右（来自 IA_Move 的 X）
		0);
}

void AGASPMoverCharacter::ReceiveJumpStarted()
{
	bIsJumpJustPressed = true;	// 边沿量：下一次 OnProduceInput 消费后清零
	bIsJumpPressed = true;		// 电平量：松开时清（网络插值重建边沿时要靠它）
}

void AGASPMoverCharacter::ReceiveJumpReleased()
{
	// 只清电平量！边沿量留给 OnProduceInput 去消费 ——
	// 否则"同一帧内按下+松开"（低帧率 / 极快点按 / 手柄抖动）会把这次跳跃整个吞掉
	bIsJumpPressed = false;
}
