// Copyright: Jichao Luo


#include "GASPMoverCharacter.h"
#include "GASPCamera.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"

AGASPMoverCharacter::AGASPMoverCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCharacterMoverComponent>(TEXT("MoverComponent")))
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CharacterCapsule"));
	CapsuleComponent->SetCapsuleHalfHeight(88.f);
	CapsuleComponent->SetCapsuleRadius(32.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CapsuleComponent->SetCollisionObjectType(ECC_Pawn);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SetRootComponent(CapsuleComponent);

	Camera->SetupAttachment(CapsuleComponent);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	Mesh->SetupAttachment(CapsuleComponent);
	Mesh->SetCastHiddenShadow(true);

	//MoverComponent->SetPrimaryVisualComponent(Mesh);
	//MoverComponent->SetPrimaryVisualComponent(CapsuleComponent);
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

void AGASPMoverCharacter::OnProduceInput(float DeltaMs, FMoverInputCmdContext& InputCmdResult)
{
	Super::OnProduceInput(DeltaMs, InputCmdResult);

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
		const bool bUseCameraFacing = false;	// TODO：设计角色朝向的逻辑

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

	// 7) 消费「边沿量」：本次输入命令用掉了"刚按下"这个事实，立刻清零。
	//    位置很关键 —— 必须在填完 context 之后、且每个模拟步只消费一次：
	//      · 一个渲染帧跑 2 个模拟步时，第 1 步跳到，第 2 步不会重复跳
	//      · 放在 ReceiveJumpReleased 里清会吞掉"同帧按下+松开"的跳跃
	//      · 放在 ReceiveJump 里清则相反（等于没缓存，一个渲染帧仍可能触发 2 次）
	bIsJumpJustPressed = false;

	// 8) 不要写 SuggestedMovementMode —— 默认后端不消费它
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
