// Copyright: Jichao Luo


#include "Mover/GASPMoverCharacter.h"
#include "Mover/GASPMoverComponent.h"
#include "Camera/GASPCameraComponent.h"
#include "GASPFunctionLibrary.h"
#include "MoverComponent.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
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
	Mover->SetPrimaryVisualComponent(Capsule);
}

void AGASPMoverCharacter::BeginPlay()
{
	Super::BeginPlay();

	bHasProduceInputInBpFunc = UGASPFunctionLibrary::IsFuncImplementedInBlueprint(this, TEXT("OnProduceInputInBlueprint"));
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

void AGASPMoverCharacter::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	// 1) C++ 侧填输入（派生类重写 OnProduceInput）
	OnProduceInput((float)SimTimeMs, InputCmdResult);

	// 2) 蓝图侧填输入：能拿到上一步填好的 context 继续改
	if (bHasProduceInputInBpFunc)
	{
		InputCmdResult = OnProduceInputInBlueprint((float)SimTimeMs, InputCmdResult);
	}

	// 3) 统一消费边沿量：一次输入只触发一次（一个渲染帧跑多个模拟步时，跳跃不会重复触发）
	
}

void AGASPMoverCharacter::OnProduceInput(float DeltaMs, FMoverInputCmdContext& InputCmdResult)
{
	// 1) 取出（没有就创建）角色输入结构体 —— 返回的是引用，直接改
	FCharacterDefaultInputs& CharacterInputs =
		InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	// 2) 没有玩家控制器（AI 未 possess / 网络模拟代理）→ 给一份"什么都不做"的输入
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		CharacterInputs.ControlRotation = GetActorRotation();
		CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, FVector::ZeroVector);
		CharacterInputs.OrientationIntent = FVector::ZeroVector;
		CharacterInputs.bIsJumpJustPressed = false;
		CharacterInputs.bIsJumpPressed = false;
		return;
	}

	// 3) 控制旋转 = 玩家相机朝向。所有输入都以它为参照系
	CharacterInputs.ControlRotation = PC->GetControlRotation();

	// 4) 移动输入 → 世界空间
	if (CachedMoveInputVelocity.IsNearlyZero())
	{
		// 方向意图：把「相机空间」的输入旋到世界空间（X=前后，Y=左右）
		const FVector WorldMoveIntent = CharacterInputs.ControlRotation.RotateVector(CachedMoveInputIntent);
		CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, WorldMoveIntent);
	}
	else
	{
		// 速度输入（AI / 导航 / 过场）：世界空间 cm/s，优先
		CharacterInputs.SetMoveInput(EMoveInputType::Velocity, CachedMoveInputVelocity);
	}

	// 5) 朝向意图（零向量 = 不改变朝向）
	CharacterInputs.OrientationIntent = FVector::ZeroVector;

	// 只用水平分量判断"有没有移动意图"：速度输入可能带垂直分量（下落速度），不能拿来定朝向
	const FVector MoveDir2D(CharacterInputs.GetMoveInput().X, CharacterInputs.GetMoveInput().Y, 0.f);
	if (MoveDir2D.SizeSquared() > UE_KINDA_SMALL_NUMBER)
	{
		const bool bUseCameraFacing = true;	// TODO：设计角色朝向的逻辑

		CharacterInputs.OrientationIntent = bUseCameraFacing
			? CharacterInputs.ControlRotation.Vector().GetSafeNormal()	// 朝相机（俯仰会被模式丢掉，只留 Yaw）
			: MoveDir2D.GetSafeNormal();								// 朝移动方向

		LastAffirmativeOrientationIntent = CharacterInputs.OrientationIntent;
	}
	else if (bMaintainLastInputOrientation)
	{
		CharacterInputs.OrientationIntent = LastAffirmativeOrientationIntent;
	}

	// 6) 跳跃：只写这两个 bool，其余交给 UCharacterMoverComponent（bHandleJump 默认就是开的）
	CharacterInputs.bIsJumpPressed = bIsJumpPressed;
	CharacterInputs.bIsJumpJustPressed = bIsJumpJustPressed;

	bIsJumpJustPressed = false;
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
